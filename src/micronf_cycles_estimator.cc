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

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define SAMPLE_SIZE 50 
#define DISCARD_FIRST_N_SAMPLE 10

#define BATCH_SIZE 32
#define PKTMBUF_POOL_NAME "MICRONF_MBUF_POOL"

// Finding the start of metadata field
#define MDATA_PTR(mbuf)                                                 \
   reinterpret_cast<char *>(reinterpret_cast<unsigned long>((mbuf)) +   \
                            sizeof(struct rte_mbuf))

// Function Prototypes
inline uint64_t
RetrievePackets( struct rte_ring* r, struct timespec &ts )__attribute__((optimize("O0"))); 
inline uint64_t
InjectPackets( struct rte_ring* r, struct timespec& ts ) __attribute__((optimize("O0")));

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

/* 
Input   : pointer to rte_ring
Output  : timespec (passed by reference) 
Note    :
   The timestamp is taken after packets are crafted but before they are enqueued \
   to the ring. Our assumption is waiting time in the ring is negligible \
   since the ring is empty when we run this cycles_estimators program. 
*/          
inline uint64_t
InjectPackets( struct rte_ring* r, struct timespec& ts ) {
   struct rte_mbuf* buf[ BATCH_SIZE ] = { nullptr };
   
   struct rte_mempool* mp = rte_mempool_lookup( PKTMBUF_POOL_NAME );
   int res = rte_pktmbuf_alloc_bulk( mp, buf, BATCH_SIZE );
   assert( res == 0 );

//   clock_gettime( CLOCK_MONOTONIC, &ts );
   clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts );

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
RetrievePackets( struct rte_ring* r, struct timespec &ts ) {
   struct rte_mbuf* buf[ BATCH_SIZE ] = { nullptr };
   int rx; 
   while ( ( rx = rte_ring_dequeue_burst( r, ( void** ) buf, BATCH_SIZE, NULL ) ) == 0 ); 
   
//   clock_gettime( CLOCK_MONOTONIC, &ts );
   clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts );

   for ( int i = 0; i < BATCH_SIZE; i++ ) {
      rte_pktmbuf_free( buf[i] );
   }

   return rx;
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
pinThisProcess( int cpuid ){
   pthread_t current_thread = pthread_self();
   cpu_set_t cpuset;
   CPU_ZERO( &cpuset );
   CPU_SET( cpuid, &cpuset );
   int s = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
   if ( s != 0 ) { 
      fprintf( stderr, "FAILED: pthread_setaffinity_np\n" );
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
   
   pinThisProcess( atoi( (*arg_map)[ "cpu_id" ].c_str() ) );

   struct rte_ring* in_r = rte_ring_lookup( (*arg_map)["in_ring"].c_str() );
   struct rte_ring* out_r = rte_ring_lookup( (*arg_map)["out_ring"].c_str() );
  
   assert( in_r != NULL && out_r != NULL );
   
   std::vector< uint64_t > samples;
   struct timespec tx_ts, rx_ts;
   int num_sample = SAMPLE_SIZE;
   if ( arg_map->find("sample_size") != arg_map->end() )
      num_sample = atoi( (*arg_map)["sample_size"].c_str() );

   for ( int i = 0; i < num_sample; i++ ) {
      memset( &tx_ts, 0, sizeof( struct timespec ) );
      memset( &rx_ts, 0, sizeof( struct timespec ) );

      int tx = InjectPackets( in_r, tx_ts );
      int rx = RetrievePackets( out_r, rx_ts );

      
/*      std::cout << "tx: " << tx << " rx: " 
                << rx ;
      std::cout << " diff tv_sec: " <<  rx_ts.tv_sec - tx_ts.tv_sec 
                << " diff tv_nsec: " << rx_ts.tv_nsec - tx_ts.tv_nsec << std::endl;
*/      
      if ( i > DISCARD_FIRST_N_SAMPLE )
         samples.push_back( 1000000000 * ( rx_ts.tv_sec - tx_ts.tv_sec ) 
                            + rx_ts.tv_nsec - tx_ts.tv_nsec );
    
      rte_delay_ms( 150 );
   }

   std::cout << "StdDev: " << GetStdDev( samples ) << " Mean: " << GetMean( samples ) << std::endl;

}


// DEBUG CODE SNIPPETS

/* // Checking the sizes
   uint16_t headroom = rte_pktmbuf_headroom( buf[ 0 ] );
   uint16_t dataroom = rte_pktmbuf_data_room_size( mp );
   std::cout << "Dataroom: " << dataroom << std::endl;   
   std::cout << "Headroom: " << headroom << std::endl;
*/
/*
   std::cout << "tx tv_sec: " << tx_ts.tv_sec 
             << " tx tv_nsec: " << tx_ts.tv_nsec << std::endl;
   std::cout << "rx tv_sec: " << rx_ts.tv_sec 
             << " rx tv_nsec: " << rx_ts.tv_nsec << std::endl;

*/
   //std::cout << (*arg_map)["in_ring"] << std::endl;
   //std::cout << (*arg_map)["out_ring"] << std::endl;

/*
   for ( int i = 0; i < BATCH_SIZE; i++ ) {
      char* meta_data = MDATA_PTR( buf[i] );
      memset( meta_data, 0, sizeof( long ) );
   }

*/

/*std::cout << "tx: " << tx << " rx: " 
                << rx ;
      std::cout << " diff tv_sec: " <<  rx_ts.tv_sec - tx_ts.tv_sec 
                << " diff tv_nsec: " << rx_ts.tv_nsec - tx_ts.tv_nsec << std::endl;
*/
      
