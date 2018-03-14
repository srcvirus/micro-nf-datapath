#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <fstream>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_ring.h>

struct ring_stat {
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

void populateRingInfo( std::vector< struct ring_stat > & rings_info ) {
  std::ifstream file("../log/ring_info");
  std::string line;
  while( std::getline( file, line ) ) {
     struct ring_stat *rs = new struct ring_stat;
     std::cout << line << std::endl;
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

     rs->occupancy = rte_ring_count( r );
     rs->size = rte_ring_get_size( r );
     
     rings_info.push_back( *rs );
  }

  file.close();
}

int 
main( int argc, char* argv[] ) {
   std::vector< struct ring_stat > rings_info;

   // Initialize DPDK args
   int args_processed = rte_eal_init( argc, argv );
   argc -= args_processed;
   argv  += args_processed;

   // Process our own args ( if any )
   auto arg_map = ParseArgs( argc - 1, argv + 1 );

   // Populate the ring info
   populateRingInfo( rings_info );

   for ( int i = 0; i < rings_info.size(); i++ ) {
      std::cout << rings_info[ i ].name  << std::endl;
      std::cout << rings_info[ i ].occupancy  << std::endl;
      std::cout << rings_info[ i ].size  << std::endl;
      std::cout << rings_info[ i ].pid_pull  << std::endl << std::endl;
   }

   // Continuously monitor the rings
   
   
}

