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
#include "sched.h"

#define TIMER_RESOLUTION_CYCLES 3000ULL /* around 10ms at 3 Ghz */ // 1s = 3x10^9;  1 ms = 3x10^6; 1 us = 3x10^3
#define NUM_CORES 4 
// FIFO queue for wait PIDs
// std::queue< unsigned > wait_queue;

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
ParseChainConf( std::string file_path ){
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
DeduceOutRing( std::vector< std::string > chain_conf ) {
   for ( int i = 0; i < chain_conf.size(); i++ ) {
      //printf( "Chain_conf size: %lu i: %d. name: %s\n", chain_conf.size(), i, chain_conf[ i ].c_str() );
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

void inline 
RunPid( unsigned  pid ) {
   int rc;
   struct sched_param sp;
   sp.sched_priority = 2;
   rc = sched_setscheduler(pid, SCHED_RR, &sp);
   assert( rc != -1 );
}

void inline 
StopPid( unsigned  pid ) {
   int rc;
   struct sched_param sp;
   sp.sched_priority = 1;
   rc = sched_setscheduler(pid, SCHED_RR, &sp);
   assert( rc != -1 );
}

// arg is the expire flag in a sched_core 
static void
ExpiredCallback(__attribute__((unused)) struct rte_timer *tim,
                void *arg)
{
   bool* core_exp = (bool*) arg;
   *core_exp = true;
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
   for ( int i = 0; i < core_sched.size() - 1; i++ ) {
      int pid  = core_sched[ i ]->wait_queue.front();
      //printf( "InitCoreSched: i:%d, pid:%d, coreid:%d\n", i, pid, core_sched[ i ]->core_id );
      //printf( "prev_ring_map occ: %d\n", prev_ring_map[ pid ]->occupancy );
      //printf( "next_ring_map occ: %d\n", next_ring_map[ pid ]->occupancy );
   }
}

// Run(kick) a process in each core to run
static void
KickScheduler() {
   unsigned lcore_id;
   lcore_id = rte_lcore_id();
   printf( "Initializing scheduler on core %u\n", lcore_id );
   rte_timer_subsystem_init();
   uint64_t timeout = 0;

   for ( volatile int i = 0; i < core_sched.size() - 1; i++ ) {
      unsigned pid = core_sched[ i ]->wait_queue.front();
      unsigned coreid = core_sched[ i ]->core_id; 
      rte_timer_init( &core_sched[ i ]->timer  );
      timeout = ( uint64_t ) ( hz *  (float) share_map[ coreid ][ pid_to_idx[ pid ] ] / 1000000 );
      RunPid( pid );
      // std::cout << " i: " << i << " pid: " << pid << " timeout: " << timeout << std::endl;
      rte_timer_reset( &core_sched[ i ]->timer, timeout, SINGLE, lcore_id, ExpiredCallback, &core_sched[ i ]->expired );
   }
}

static __attribute__((noreturn)) int
lcore_mainloop(__attribute__((unused)) void *arg)
{
   uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
   unsigned lcore_id;
   lcore_id = rte_lcore_id();
   uint64_t timeout = 0;
   
   // run process on each core.
   KickScheduler();

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
            unsigned pid = core_sched[ i ]->wait_queue.front();
            unsigned coreid = core_sched[ i ]->core_id;

            // Reschedule a core if a running process is expired, has empty input ring, has full output ring.
            if ( core_sched[ i ]->expired ||        \
                 prev_ring_map[ pid ]->occupancy == 0 || \
                 next_ring_map[ pid ]->occupancy == 2048 ) {
               StopPid( pid );
               core_sched[ i ]->wait_queue.pop();
               core_sched[ i ]->wait_queue.push( pid );
               rte_timer_stop_sync( &core_sched[ i ]->timer );
               core_sched[ i ]->expired = false;
               pid =  core_sched[ i ]->wait_queue.front();
               RunPid( pid );
               timeout = ( uint64_t ) ( hz *  (float) share_map[ coreid ][ pid_to_idx[ pid ] ] / 1000000 );
               rte_timer_reset( &core_sched[ i ]->timer, timeout, SINGLE, lcore_id, ExpiredCallback, &core_sched[ i ]->expired );
            }
         }
      }
   }
   
}

int 
main( int argc, char* argv[] ) {
   // Initialize DPDK args
   int args_processed = rte_eal_init( argc, argv );
   argc -= args_processed;
   argv  += args_processed;

   // Process our own args ( if any )
   auto arg_map = ParseArgs( argc - 1, argv + 1 );

   // Populate the ring info
   PopulateRingInfo( rings_info );
   
   // Pin to CPU core
   PinThisProcess( 3 );      

   unsigned lcore_id = rte_lcore_id();
   hz = rte_get_timer_hz();   

   // Parsing config file to deduce out_ring occupancy of a given pid.
   std::vector<std::string> chain_conf = ParseChainConf( "../confs/Chain.conf" );
   DeduceOutRing( chain_conf );
   PrintRingInfo();
   
   // For quick access of next_ring_map[pid] prev_ring_map[pid] occupancy.
   // Used in addition to time quantum.
   PopulateNextRingMap();
   PopulatePrevRingMap();

   // FIXME
   // Manually assigned shares
   share_map[ 1 ].push_back( 2 ); // 1st process in core 1 gets 2 usec share
   share_map[ 1 ].push_back( 2 ); // 2nd process in core 1 gets 2 usec share
   share_map[ 2 ].push_back( 2 );
   share_map[ 2 ].push_back( 2 );


   InitCoreSched();

   // call lcore_mainloop() on every slave lcore 
   RTE_LCORE_FOREACH_SLAVE(lcore_id) {
      rte_eal_remote_launch(lcore_mainloop, &rings_info, lcore_id);
   }
   // call it on master lcore too
   (void) lcore_mainloop(NULL);
}


