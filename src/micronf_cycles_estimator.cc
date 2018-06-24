// Author: Anthony Anthony.

/**********************

How to run:
   sudo ./micronf_cycles_estimator <DPDK_EAL_PARAMS> -- --in_ring=<INPUT_RING_NAME> --out_ring=<OUTPUT_RING_NAME> [--sample_size=<NUM_OF_SAMPLES>]
   
e.g. 
   sudo ./micronf_cycles_estimator -n 2 --proc-type secondary -- --in_ring=rx_ring_0 --out_ring=rx_ring_1 --sample_size=150

 *********************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <fstream>
#include <assert.h>
#include <time.h>
#include <cmath>
#include <pthread.h>
#include <iomanip>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define SAMPLE_SIZE 200 
#define DISCARD_FIRST_N_SAMPLE SAMPLE_SIZE * 0.2

#define BATCH_SIZE 64
#define PKTMBUF_POOL_NAME "MICRONF_MBUF_POOL"

// Finding the start of metadata field
#define MDATA_PTR(mbuf)                                                 \
   reinterpret_cast<char *>(reinterpret_cast<unsigned long>((mbuf)) +   \
                            sizeof(struct rte_mbuf))

// Function Prototypes
inline uint64_t
RetrievePackets( struct rte_ring* r, struct timespec &ts, volatile uint64_t& rdtsc )__attribute__((optimize("O0"))); 
inline uint64_t
InjectPackets( struct rte_ring* r, struct timespec& ts, volatile uint64_t& rdtsc ) __attribute__((optimize("O0")));
void __inline__ 
imitate_processing( int load ) __attribute__((optimize("O0")));   

__inline__ uint64_t start_rdtsc() {
   unsigned int lo,hi;
   //preempt_disable();
   //raw_local_irq_save(_flags);

   __asm__ __volatile__ ("CPUID\n\t"
                         "RDTSC\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t": "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx");
   return ((uint64_t)hi << 32) | lo;
}

__inline__ uint64_t end_rdtsc() {
   unsigned int lo, hi;

   __asm__ __volatile__ ("RDTSCP\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         "CPUID\n\t": "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx");
   //raw_local_irq_save(_flags);
   //preempt_enable();
   return ((uint64_t)hi << 32) | lo;
}

// Parsing execution arguments after those dpdk args
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

double
GetMean( std::vector<uint64_t>& vec ) {
   uint64_t sum = 0;
   for ( auto it = vec.begin(); it != vec.end(); it++) {
      sum += *it;
   }
   return sum / vec.size();
}

double
GetStdDev( std::vector<uint64_t>& vec ) {
   double dev = 0.0;
   double mean = GetMean( vec );
 
   for (auto it = vec.begin(); it != vec.end(); it++) {
      dev += std::pow( *it - mean, 2 );
   }
   return std::sqrt( dev / vec.size() );;
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

// Imitating work load.
// void imitate_processing( int load ) __attribute__((optimize("O0"))); 
void __inline__ imitate_processing( int load ) {   
   // Imitate extra processing
   int n = 1000 * load;
   for ( int i = 0; i < n; i++ ) {
      int volatile r = 0;
      int volatile s = 999;
      r =  s * s;
   }
}

void inline SetScheduler(int pid) {
  // Change scheduler to RT Round Robin
  int rc, old_sched_policy;
  struct sched_param my_params;
  my_params.sched_priority = 99;
  old_sched_policy = sched_getscheduler(pid);
  rc = sched_setscheduler(pid, SCHED_RR, &my_params);
  if (rc == -1) {
    printf("sched_setscheduler call is failed\n");
    exit(-1);
  } else {
    printf(
        "set_scheduler(). pid: %d. old_sched_policy:  %d. new_sched_policy: %d "
        "\n",
        pid, old_sched_policy, sched_getscheduler(pid));
  }
}

void DrainPacketsFromRing( rte_ring* r ) {
   struct rte_mbuf* buf[ BATCH_SIZE ] = { nullptr };
   int rx = 0;
   while ( ( rx = rte_ring_dequeue_burst( r, ( void** ) buf, BATCH_SIZE, NULL ) ) > 0 ) {
      for ( int i = 0; i < rx; i++ ) {
         rte_pktmbuf_free( buf[ i ] );
      }  
   }    
}

/* 
Input   : pointer to rte_ring
Output  : timespec (passed by reference) 
Note    :
   The timestamp is taken after packets are crafted but before they are enqueued \
   to the ring. Our assumption is waiting time in the ring is negligible \
   since the ring is empty when we run this cycles_estimators program. 
*/          
inline uint64_t
   InjectPackets( struct rte_ring* r, struct timespec& ts, volatile uint64_t& rdtsc, struct rte_mbuf** buf ) {
   DrainPacketsFromRing( r );

   clock_gettime( CLOCK_MONOTONIC, &ts );
   rdtsc = start_rdtsc();

   unsigned tx = rte_ring_enqueue_burst( r, ( void** ) buf, BATCH_SIZE, NULL );

   return tx;
}

/* 
Input   : pointer to rte_ring
Output  : timespec (passed by reference) 
Note    :
   The timestamp is taken after a non-empty read from the ring.
*/          
inline uint64_t
RetrievePackets( struct rte_ring* r, struct timespec &ts, volatile uint64_t& rdtsc ) {
   struct rte_mbuf* buf[ BATCH_SIZE ] = { nullptr };
   int rx = 0; 
   while ( ( rx = rte_ring_dequeue_burst( r, ( void** ) buf, BATCH_SIZE, NULL ) ) < BATCH_SIZE ); 
   
   clock_gettime( CLOCK_MONOTONIC, &ts );
   rdtsc = end_rdtsc();

   for ( int i = 0; i < rx; i++ ) 
      rte_pktmbuf_free( buf[i] );
   
   DrainPacketsFromRing( r );
   
   return rx;
}

int 
main( int argc, char* argv[] ) {
   // Initialize DPDK args
   int args_processed = rte_eal_init( argc, argv );
   argc -= args_processed;
   argv  += args_processed;
   
   // Process our own args ( if any )
   auto arg_map = ParseArgs( argc - 1, argv + 1 );
   int num_sample = SAMPLE_SIZE;
   if ( arg_map->find("sample_size") != arg_map->end() )
      num_sample = atoi( (*arg_map)["sample_size"].c_str() );

   int PKT_LEN = 0;
   if ( arg_map->find( "pkt_len" ) != arg_map->end() ) 
      PKT_LEN = atoi( (*arg_map)["pkt_len"].c_str() );

   std::cout << "################## START ##############\n";
   // For precise measurement, I pin this process to a core and use the RT scheduler
   PinThisProcess( atoi( (*arg_map)[ "cpu_id" ].c_str() ) );
   SetScheduler( 0 );

   struct rte_ring* in_r = rte_ring_lookup( (*arg_map)["in_ring"].c_str() );
   struct rte_ring* out_r = rte_ring_lookup( (*arg_map)["out_ring"].c_str() );
   assert( in_r != NULL && out_r != NULL );
   
   std::vector< uint64_t > samples, cycles;

   struct rte_mbuf* buf[ BATCH_SIZE ] = { nullptr };
   struct rte_mempool* mp = rte_mempool_lookup( PKTMBUF_POOL_NAME );
   int res = rte_pktmbuf_alloc_bulk( mp, buf, BATCH_SIZE );
   assert( res == 0 );

   // faking the packet length
   if ( PKT_LEN != 0 ) {
      for ( int i = 0; i < BATCH_SIZE; i++ )
         buf[ i ]->pkt_len = PKT_LEN;
   }
  
   // Start measurement
   for ( int i = 0; i < num_sample; i++ ) {
      volatile uint64_t st_rdtsc,  en_rdtsc;
      struct timespec tx_ts, rx_ts;

      int tx = InjectPackets( in_r, tx_ts, st_rdtsc, buf );
      int rx = RetrievePackets( out_r, rx_ts, en_rdtsc );
      
      if ( i > DISCARD_FIRST_N_SAMPLE ) {
         samples.push_back( 1000000000 * ( rx_ts.tv_sec - tx_ts.tv_sec ) 
                            + rx_ts.tv_nsec - tx_ts.tv_nsec );
         cycles.push_back( en_rdtsc - st_rdtsc );
      }
   }

   std::cout << std::fixed;
   std::cout << std::setprecision(2);
   std::cout << "################## RESULT ##############\n\n";
   std::cout << "###\t SAMPLE SIZE " << num_sample << "\t ###\n";
/*   std::cout << "###\t Per Batch \t ###\n";
   std::cout << "SD \t: " << GetStdDev( samples ) << " \tMean: " << GetMean( samples ) << std::endl;
   std::cout << "RDTSC cycles StdDev \t: " << GetStdDev( cycles ) << " \tMean: " << GetMean( cycles ) << std::endl;
*/
   std::cout << "\n###\t Average Per Packet\t ###\n";
   std::cout << "SD: " << GetStdDev( samples ) / BATCH_SIZE << "  Mean: " << GetMean( samples ) / BATCH_SIZE << std::endl;
//   std::cout << "RDTSC cycles StdDev : " << GetStdDev( cycles ) / BATCH_SIZE << " \tMean: " << GetMean( cycles ) / BATCH_SIZE << std::endl;

}


// DEBUG CODE SNIPPETS

/* // METADATA & HEADROOM
   uint16_t headroom = rte_pktmbuf_headroom( buf[ 0 ] );
   uint16_t dataroom = rte_pktmbuf_data_room_size( mp );
   std::cout << "Dataroom: " << dataroom << std::endl;   
   std::cout << "Headroom: " << headroom << std::endl;

   for ( int i = 0; i < BATCH_SIZE; i++ ) {
      char* meta_data = MDATA_PTR( buf[i] );
      memset( meta_data, 0, sizeof( long ) );
   }

*/
