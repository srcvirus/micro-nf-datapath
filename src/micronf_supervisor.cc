#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <fstream>
#include <fcntl.h>

#include <sched.h>
#include <assert.h>
#include <unistd.h>
#include <rte_timer.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <queue>
#include <map>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_cycles.h>
#include <rte_launch.h>

#include "micronf_config.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include "./sched/sched.h"

#define RTE_LOGTYPE_UNIS RTE_LOGTYPE_USER1

#define TIMER_RESOLUTION_CYCLES 3000ULL /* around 1us at 3 Ghz */ // 1s -> 3x10^9;  1 ms -> 3x10^6; 1 us -> 3x10^3

#define RING_SIZE 2048 
#define BATCH_SIZE 64
#define CORE_ID 0
#define PIN_CORE_ID 3
#define UPPERBOUND RING_SIZE * 8 / 10
#define LOWERBOUND BATCH_SIZE 

float complx_1 = 39.2;   // per packet processing cost in nsec 
float complx_2 = 79.3;   // per packet processing cost in nsec 
float ideal_occ = 0.75;
int SHARE_1 = ideal_occ * RING_SIZE * complx_1 / 1000;   // share is in usec
int SHARE_2 = ideal_occ * RING_SIZE * complx_2 / 1000;   // share is in usec

// Contains info for all rte_rings
std::vector< struct ring_stat* > rings_info;
// Mapping between coreid and list of processes' share (usec)
std::map< unsigned, std::vector< unsigned > > share_map;
// list of per core data structure
std::vector< struct sched_core* > core_sched;

// Mapping between pid->next_ring_stat*
std::map< unsigned, struct ring_stat* > next_ring_map;
// Mapping between pid->prev_ring_stat*
std::map< unsigned, struct ring_stat* > prev_ring_map;
// Given a pid, what is the idx among pids in the core
std::map< unsigned, unsigned > pid_to_idx;
// core_id -> list of pids
std::map< unsigned, std::vector< unsigned > > core_pids;


unsigned hz = 0;
unsigned lcore_id = CORE_ID;

struct sched_core {
   unsigned core_id;
   struct rte_timer timer;
   bool expired = false;
   std::queue< unsigned > wait_queue;
};


struct ring_stat {
   struct rte_ring* ring;
   std::string name;
   unsigned occupancy;
   unsigned size;
   // FIXME 
   // Do we need to handle branch-in and out?
   int pid_pull = 0; 
   int pid_push = 0;
}; 

std::unique_ptr< std::map < std::string, std::string > > 
ParseArgs( int argc, char* argv[] ) {
   auto ret_map = std::unique_ptr< std::map < std::string, std::string > > (
         new std::map< std::string, std::string >() );
   const std::string kDel = "=";
   for (int i = 0; i < argc; ++i) {
      char *key = strtok(argv[i] + 2, kDel.c_str());
      char *val = strtok(NULL, kDel.c_str());
      ret_map->insert(std::make_pair(key, val));
   }
   return std::move(ret_map);
}

std::vector< std::string > 
ParseChainConf( std::string file_path ) {
   std::ifstream in_file( file_path.c_str() );
   std::string line;
   std::vector< std::string > conf_vector;
   while ( in_file ) {
      getline( in_file, line );
      if( !line.empty() )
         conf_vector.push_back( line );
   }

   return conf_vector;
}

void 
DeduceOutRing( std::vector< std::string >& chain_conf ) {
   for ( int i = 0; i < chain_conf.size(); i++ ) {
      PacketProcessorConfig pp_config;
      std::string config_file_path = chain_conf[ i ];
      int fd = open(config_file_path.c_str(), O_RDONLY);
      if (fd < 0)
         rte_exit(EXIT_FAILURE, "Cannot open configuration file %s\n",
                  config_file_path.c_str());
      google::protobuf::io::FileInputStream config_file_handle(fd);
      config_file_handle.SetCloseOnDelete(true);
      google::protobuf::TextFormat::Parse(&config_file_handle, &pp_config);
      
      unsigned pid = 0;
      // Iterating over the available ports
      for ( int port_id = 0; port_id < pp_config.port_configs_size(); port_id++ ) {
         const PortConfig& p_config = pp_config.port_configs( port_id );
         if ( p_config.port_type() == PortConfig::INGRESS_PORT ) {
            const auto conf_ring_it = p_config.port_parameters().find( "ring_id" );
            if ( conf_ring_it == p_config.port_parameters().end() ) 
               continue;
            // Find the pull_pid for this ingress ring name.
            // Keep the pid; This pid is the pid_push for the egress ring in the conf file.
            // ring_info: global variable.
            for ( auto it = rings_info.begin(); it != rings_info.end(); it++ ) {
               if ( (*it)->name == conf_ring_it->second ) {
                  pid = (*it)->pid_pull;
               }
            }
         }
         if ( p_config.port_type() == PortConfig::EGRESS_PORT ) {
            const auto conf_ring_it = p_config.port_parameters().find( "ring_id" );
            if ( conf_ring_it == p_config.port_parameters().end() )
               continue;
            // Find the entry in rings_info data structure
            // update the pid_push to the pid we get from the ingress ring (above code). 
            // ring_info: global variable.
            for ( auto it = rings_info.begin(); it != rings_info.end(); it++ ) {
               if ( (*it)->name == conf_ring_it->second ) {
                  (*it)->pid_push = pid;
               }
            }
         }
      }

   }
}

void
PinThisProcess( int cpuid ){
   pthread_t current_thread = pthread_self();
   cpu_set_t cpuset;
   CPU_ZERO( &cpuset );
   CPU_SET( cpuid, &cpuset );
   int s = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
   if ( s != 0 ) { 
      fprintf( stderr, "FAILED: pthread_setaffinity_np\n" );
   } 
}


void 
PopulateRingInfo( std::vector< struct ring_stat* > & rings_info ) {
  
  std::ifstream file("../log/ring_info");
  std::string line;
  while( std::getline( file, line ) ) {
     struct ring_stat *rs = new struct ring_stat;
     char *cstr = new char[ line.length() + 1 ];
     std::strcpy( cstr, line.c_str() );
     char * p = std::strtok( cstr, "," );
     while ( p != 0 ) {
        rs->name = std::string( p );
        p = std::strtok( NULL, "," );
        rs->pid_pull = atoi( p );
        p = std::strtok( NULL, "," );
        core_pids[ atoi( p ) ].push_back( rs->pid_pull );
        p = std::strtok( NULL, "," );
     }
     delete cstr;  
     
     struct rte_ring* r = rte_ring_lookup( rs->name.c_str() );
     if ( r == NULL )
        std::cerr << "Ring not found."; 

     rs->ring = r;
     rs->occupancy = rte_ring_count( r );
     rs->size = rte_ring_get_size( r );
     rings_info.push_back( rs );
  }
  file.close();
}

void 
RefreshRingInfo( std::vector< struct ring_stat* > & rings_info ) {
   for ( int i = 0; i < rings_info.size(); i++ ) {
      rings_info[i]->occupancy = rte_ring_count( rings_info[i]->ring );
   }
}

void
PopulateNextRingMap() {
   for ( int i = 0; i < rings_info.size(); i++ ) {
      next_ring_map[ rings_info[ i ]->pid_push ] = rings_info[ i ];
   }
}

void
PopulatePrevRingMap() {
   for ( int i = 0; i < rings_info.size(); i++ ) {
      prev_ring_map[ rings_info[ i ]->pid_pull ] = rings_info[ i ];
   }
}

void
PrintRingInfo() {
   std::cout << "####### Ring Information #######" <<  std::endl;
   std::cout << "<name>\t\t<occupancy>\t<ring-size>\t<pid_pull>\t<pid_push>" <<  std::endl;
   for ( int i = 0; i < rings_info.size(); i++ ) {
      std::cout << rings_info[ i ]->name  << "\t";
      std::cout << rings_info[ i ]->occupancy  << "\t\t";
      std::cout << rings_info[ i ]->size  << "\t\t";
      std::cout << rings_info[ i ]->pid_pull << "\t\t";
      std::cout << rings_info[ i ]->pid_push  << std::endl;
   }
   std::cout <<  std::endl;
}

// arg is the expire flag in a sched_core 
static void
ExpiredCallback(__attribute__((unused)) struct rte_timer *tim,
                void *arg)
{
//   RTE_LOG( INFO, UNIS, "Expired Callback Fired");
   bool* core_exp = (bool*) arg;
   *core_exp = true;
}

static void
print_ring_cb( __attribute__((unused)) struct rte_timer *tim, void *arg ) {
   auto ri = ( std::vector< struct ring_stat* > * ) arg;
   RefreshRingInfo( *ri );
   PrintRingInfo();
}

void PrintRingInfoPeriodically( int ms ) {
   static struct rte_timer timer0;
   rte_timer_init(&timer0);
   uint64_t hz = rte_get_timer_hz();
   uint64_t timeout = hz / 1000 * ms;
   rte_timer_reset( &timer0, timeout, PERIODICAL, lcore_id, print_ring_cb, &rings_info );
}

// Initialize per core data structure 'sched_core'
static void
InitCoreSched() {
   for ( auto it = core_pids.begin(); it != core_pids.end(); it++ ) {
      struct sched_core *sc = new struct sched_core;
      sc->core_id = it->first;
      for ( int i = 0; i < it->second.size(); i++ ) {
         sc->wait_queue.push( it->second[ i ] );
         pid_to_idx[ it->second[ i ] ] = i;
      } 
      core_sched.push_back( sc );
   }
}

// Run(kick) a process in each core to run
static void
KickScheduler() {
   unsigned lcore_id = CORE_ID;
   printf( "Kicking scheduler on core %u\n", lcore_id );
   rte_timer_subsystem_init();
   uint64_t timeout = 0;

   for ( volatile int i = 0; i < core_sched.size() - 1; i++ ) {
      unsigned pid = core_sched[ i ]->wait_queue.front();
      unsigned coreid = core_sched[ i ]->core_id; 
      rte_timer_init( &core_sched[ i ]->timer  );
      timeout = ( uint64_t ) ( hz *  (float) share_map[ coreid ][ pid_to_idx[ pid ] ] / 1000000 );

      Switch( 0, pid );
     
      rte_timer_reset( &core_sched[ i ]->timer, timeout, SINGLE, lcore_id, ExpiredCallback, &core_sched[ i ]->expired );
   }
}

static int  __attribute__((noreturn)) 
empty_mainloop(__attribute__((unused)) void *arg) 
{
   uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
   uint64_t timeout = 0;
  
   while (1) {
      // Manage timer
      cur_tsc = rte_rdtsc();
      diff_tsc = cur_tsc - prev_tsc;
      
      if ( diff_tsc > TIMER_RESOLUTION_CYCLES ) {
         rte_timer_manage();
         prev_tsc = cur_tsc;
      }
   }
}
static int  __attribute__((noreturn)) 
unis_mainloop(__attribute__((unused)) void *arg) 
{
   uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
   uint64_t timeout = 0;
   unsigned pid, coreid, npid;

   // run process on each core.
   KickScheduler();

   unsigned lcore_id = CORE_ID;
   printf( "Unis mainloop on core %u\n", lcore_id );

   while (1) {
      // Manage timer
      cur_tsc = rte_rdtsc();
      diff_tsc = cur_tsc - prev_tsc;
      
      if ( diff_tsc > TIMER_RESOLUTION_CYCLES ) {
         rte_timer_manage();
         prev_tsc = cur_tsc;
         
         RefreshRingInfo( rings_info );

         // Check and manage expiry in each core
         for ( int i = 0; i < core_sched.size() - 1; i++ ) {
            pid = core_sched[ i ]->wait_queue.front();
            coreid = core_sched[ i ]->core_id;
            npid;

            // Reschedule a core if a running process is expired, or 
            // the input ring is almost empty, or the output ring almost full.
            if ( core_sched[ i ]->expired        \
//                 || prev_ring_map[ pid ]->occupancy <= LOWERBOUND     \
//                 || next_ring_map[ pid ]->occupancy >= UPPERBOUND  ) {
               ) {
               core_sched[ i ]->wait_queue.pop();
               core_sched[ i ]->wait_queue.push( pid );       
               npid =  core_sched[ i ]->wait_queue.front();
/*
               // Make sure the next one has meaningful work to do before being scheduled.
               // If not, check the next one in the queue repeatedly until all are checked.
               while( npid != pid && ( prev_ring_map[ npid ]->occupancy <= LOWERBOUND || \
                                    next_ring_map[ npid ]->occupancy >= UPPERBOUND ) ) {
                  core_sched[ i ]->wait_queue.pop();
                  core_sched[ i ]->wait_queue.push( npid );
                  npid =  core_sched[ i ]->wait_queue.front();
               }
*/               
               // New pid ready to be run.
//               if ( npid != pid ) {
                  rte_timer_stop_sync( &core_sched[ i ]->timer );
                  core_sched[ i ]->expired = false;

                  // Put pid to waiting state and npid to runing state
                  Switch( pid, npid );

                  // Timer responsible for the new running npid is started.
                  timeout = ( uint64_t ) ( hz *  (float) \
                                           share_map[ coreid ][ pid_to_idx[ npid ] ] / 1000000 );
                  rte_timer_reset( &core_sched[ i ]->timer, timeout, SINGLE, lcore_id, \
                                   ExpiredCallback, &core_sched[ i ]->expired );
//               }
            }
         }
      }
   }
   
}

void handler( int sig ){
   Handler();
   exit( 1 );
}

int 
main( int argc, char* argv[] ) {
   // Initialize DPDK args
   int args_processed = rte_eal_init( argc, argv );
   argc -= args_processed;
   argv  += args_processed;

   // Register signals 
   signal(SIGINT, handler);   

   // Process our own args ( if any )
   auto arg_map = ParseArgs( argc - 1, argv + 1 );

   // Populate the ring info
   PopulateRingInfo( rings_info );
   
   // Pin to CPU core
   PinThisProcess( PIN_CORE_ID );      

   hz = rte_get_timer_hz();   

   // Parsing config file to deduce out_ring occupancy of a given pid.
   std::vector<std::string> chain_conf = ParseChainConf( "../confs/Chain.conf" );
   DeduceOutRing( chain_conf );
   PrintRingInfo();
   
   // For quick access of next_ring_map[pid] prev_ring_map[pid] occupancy.
   // Used in addition to time quantum.
   PopulateNextRingMap();
   PopulatePrevRingMap();

   // Manually assigned shares
   unsigned share = SHARE_1; 
   unsigned share_2 = SHARE_2;
   RTE_LOG( INFO, UNIS, "Share: %u %u\n", share, share_2 );
   // 1st process in core 1 gets usec share
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   share_map[ 1 ].push_back( share );
   
   // share_map[ 1 ].push_back( share );
   // share_map[ 2 ].push_back( 3 );
   // share_map[ 2 ].push_back( 6 );

   // Initialize per core data structure 'sched_core'
   InitCoreSched();

   // Populate pid array
   unsigned int pid_array[ 100 ];
   int i = 0;
   for ( auto it = pid_to_idx.begin(); it != pid_to_idx.end(); it++ ) {
      pid_array[ i++ ] = it->first;
   }
   pid_array[ i - 1 ] = 0;
   
   // Stop all
   Init_Sched( pid_array );
   
   if ( arg_map->find("print-only") != arg_map->end() ) {
      PrintRingInfoPeriodically( stoi( (*arg_map)[ "print-only" ] ) );
      RTE_LCORE_FOREACH_SLAVE(lcore_id) {
         rte_eal_remote_launch(empty_mainloop, &rings_info, lcore_id);
      }
      // call it on master lcore too
      (void) empty_mainloop(NULL);
   }
   else {
      if ( arg_map->find("print") != arg_map->end() )
         PrintRingInfoPeriodically( stoi( (*arg_map)[ "print" ] ) );

      // call unis_mainloop() on every slave lcore 
      RTE_LCORE_FOREACH_SLAVE(lcore_id) {
         rte_eal_remote_launch(unis_mainloop, &rings_info, lcore_id);
      }  
      // call it on master lcore too
      (void) unis_mainloop(NULL);
   }
}


