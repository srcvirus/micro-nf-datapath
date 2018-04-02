#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <fstream>

#include <sched.h>
#include <assert.h>
#include <event.h>
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

#define TIMER_RESOLUTION_CYCLES 3000ULL /* around 10ms at 3 Ghz */ // 1s = 3x10^9;  1 ms = 3x10^6; 1 us = 3x10^3

std::vector< struct ring_stat* > rings_info;

unsigned hz = 0;
uint64_t timeout = 0;
bool expired = false;
// FIFO queue for wait PIDs
std::queue< unsigned > wait_queue;
// Mapping between pid and its share (usec)
std::map< unsigned, unsigned > share_map;

struct ring_stat {
   struct rte_ring* ring;
   std::string name;
   unsigned occupancy;
   unsigned size;
   int pid_pull;
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

static void
ExpiredCallback(__attribute__((unused)) struct rte_timer *tim,
          __attribute__((unused)) void *arg)
{
   expired = true;
}

static __attribute__((noreturn)) int
lcore_mainloop(__attribute__((unused)) void *arg)
{
   uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
   unsigned lcore_id;
   lcore_id = rte_lcore_id();
   printf("Starting mainloop on core %u\n", lcore_id);

   unsigned pid = wait_queue.front();
   static struct rte_timer timer0;
   rte_timer_subsystem_init();
   rte_timer_init( &timer0 );
   timeout = ( uint64_t ) ( hz *  (float) share_map[ pid ] / 1000000 );

   RunPid( pid );
   rte_timer_reset( &timer0, timeout, SINGLE, lcore_id, ExpiredCallback, NULL );
   
   while (1) {
      /*
       * Call the timer handler on each core: as we don't
       * need a very precise timer, so only call
       * rte_timer_manage() every ~10ms (at 2Ghz). In a real
       * application, this will enhance performances as
       * reading the HPET timer is not efficient.
       */
      cur_tsc = rte_rdtsc();
      diff_tsc = cur_tsc - prev_tsc;
      if ( diff_tsc > TIMER_RESOLUTION_CYCLES ) {
         rte_timer_manage();
         prev_tsc = cur_tsc;
         RefreshRingInfo( rings_info );

         if ( expired ) {   // TODO or in_ring.empty() or out_ring.full()
            StopPid( pid );
            wait_queue.pop();
            wait_queue.push( pid );
            // In case timer hasn't expired but ring is empty/full
            rte_timer_stop_sync( &timer0 );
            expired = false;
            pid = wait_queue.front();
            RunPid( pid );
            timeout = ( uint64_t ) ( hz *  (float) share_map[ pid ] / 1000000 );
            rte_timer_reset( &timer0, timeout, SINGLE, lcore_id, ExpiredCallback, NULL );
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
   
   //FIXME hardcode
   std::vector< unsigned > pids = { 433, 434 };
   std::vector< unsigned > shares = { 2, 2 }; //usec

   for ( int i = 0; i < pids.size(); i++ ) {
      wait_queue.push( pids[ i ] );
      share_map[ pids[ i ] ] = shares[ i ];
      StopPid( pids[ i ] );
   }

   // call lcore_mainloop() on every slave lcore 
   RTE_LCORE_FOREACH_SLAVE(lcore_id) {
      rte_eal_remote_launch(lcore_mainloop, &rings_info, lcore_id);
   }
   // call it on master lcore too
   (void) lcore_mainloop(NULL);
}


