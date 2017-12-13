#include "micronf_agent.h"
#include "micronf_config.pb.h"
#include "common.h"

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_ring.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_cycles.h>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;

// mbuf pools configuration
#define MAX_NUM_USERV 150
#define MBUFS_PER_USERV 1536
#define MBUFS_PER_PORT 8192 // 1536
#define MBUF_CACHE_SIZE 512
#define PKTMBUF_POOL_NAME "MICRONF_MBUF_POOL"

#define USERV_QUEUE_RINGSIZE 2048 // 128

// Port configuration
#define NUM_TX_QUEUE_PERPORT 1
#define NUM_RX_QUEUE_PERPORT 1
#define RX_QUEUE_SZ 2048 // 512
#define TX_QUEUE_SZ 2048 // 512

MicronfAgent::MicronfAgent(){
   num_ports_ = 0;
   for(int i=0; i < MAX_NUM_MS; i++)
      for(int j=0; j < MAX_NUM_PORT; j++){
         neighborGraph[i][j] = std::make_pair(0,0);
         pp_ingress_name[i][j] = "";
         pp_egress_name[i][j] = "";
      }
}

MicronfAgent::~MicronfAgent(){}

// Function prototype
void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask);

// Initialize the ports for microservices and check both ports status
int MicronfAgent::Init(int argc, char* argv[]){
   int retval = rte_eal_init(argc, argv);
   if(retval < 0){
      cerr<<"rte_eal_init() fails "<<strerror(errno)<<endl;
      return -1;
   }
	
   argc -= retval;
   argv += retval;

   num_ports_ = rte_eth_dev_count();
   if(num_ports_ == 0)
      rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");	

   retval = InitMbufPool();
   if (retval != 0)
      rte_exit(EXIT_FAILURE, "Cannot create needed mbuf pools\n");

   for( int port_id = num_ports_-1; port_id >= 0; --port_id ) {
      retval = InitPort( port_id );
      if( retval < 0 ){
         rte_exit( EXIT_FAILURE, "Cannot initialise port %u\n",
                   (unsigned)port_id );
      }
   }

   
   check_all_ports_link_status( num_ports_, 0x03 );

   // Create memzone to store statistic
   // FIXME initialize num_nfs from config file
   int num_nfs = 1;
   InitStatMz(num_nfs);
   InitScaleBits(num_nfs);	
}

int MicronfAgent::CreateRing(string ring_name){
   unsigned socket_id = rte_socket_id();
   const unsigned ring_size = USERV_QUEUE_RINGSIZE;
   struct rte_ring* rx_q = rte_ring_create(ring_name.c_str(), ring_size, socket_id,
                                           //RING_F_SP_ENQ | RING_F_SC_DEQ ); /* single prod, single cons */
                                           // RING_F_SC_DEQ ); /* multi prod, single cons */
                                           0 ); /* multi prod cons */
   if (rx_q == NULL)
      rte_exit(EXIT_FAILURE, "Cannot create rx ring queue for userv %s\n", ring_name.c_str());
	
   return 0;
}

int MicronfAgent::DeleteRing(string ring_name){
   struct rte_ring* rx_q = rte_ring_lookup(ring_name.c_str()); /* single prod, single cons */
   if (rx_q == NULL)
      rte_exit(EXIT_FAILURE, "Cannot lookup rx ring %s\n", ring_name.c_str());
   rte_ring_free(rx_q);
   return 0;
}

void MicronfAgent::UpdateNeighborGraph(PacketProcessorConfig& pp_config, 
                                       const PortConfig& pconfig){
   const auto ring_it = pconfig.port_parameters().find("ring_id");
   if(ring_it == pconfig.port_parameters().end())
      return;
	
   // FIXME hardcoded for the next of rx thread
   // Can be determine by looking at config
   std::get<0>(neighborGraph[0][0]) = 1;
   std::get<1>(neighborGraph[0][0]) = 0;

   for(int i=0; i < MAX_NUM_MS; i++){
      for(int j=0; j < MAX_NUM_PORT; j++){
         if(pconfig.port_type() == PortConfig::INGRESS_PORT){
            if(pp_egress_name[i][j] == ring_it->second){
               std::get<0>(neighborGraph[i][j]) = pp_config.instance_id();
               std::get<1>(neighborGraph[i][j]) = pconfig.port_index();
            }
         }
         else if(pconfig.port_type() == PortConfig::EGRESS_PORT){
            if(pp_ingress_name[i][j] == ring_it->second){
               std::get<0>(neighborGraph[pp_config.instance_id()][pconfig.port_index()]) = i;	
               std::get<1>(neighborGraph[pp_config.instance_id()][pconfig.port_index()]) = j;	
            }
         }
      }
   }

}

void MicronfAgent::MaintainRingCreation(const PortConfig& pconfig){
   const auto ring_it = pconfig.port_parameters().find("ring_id");
   if(ring_it == pconfig.port_parameters().end())
      return;
   struct rte_ring *ring;
   ring = rte_ring_lookup(ring_it->second.c_str());
   if(ring == NULL){
      printf("Creating Ring: %s\n", ring_it->second.c_str());
      CreateRing(ring_it->second.c_str());
   }
}

void MicronfAgent::MaintainLocalDS(PacketProcessorConfig& pp_conf){
   // retrieve the config form this DS when scale out
   ppConfigList[pp_conf.instance_id()] = pp_conf;
   printf("pp_conf instance_id: %d\n", pp_conf.instance_id());
   printf("pp_conf class: %s\n", pp_conf.packet_processor_class().c_str());
   printf("pp_con num_ingress: %d\n", pp_conf.num_ingress_ports());

	
   // Check the existing edge and update graph if there is a link
   for(int pid = 0; pid < pp_conf.port_configs_size(); pid++){
      const PortConfig& pconfig = pp_conf.port_configs(pid);
      // Maintain rings created for this config
      MaintainRingCreation(pconfig);		

      UpdateNeighborGraph(pp_conf, pconfig);	
   }
	
   // Update the edges information
   for(int pid = 0; pid < pp_conf.port_configs_size(); pid++){
      const PortConfig& pconfig = pp_conf.port_configs(pid);
      // assuming port_parameter.at(1) countains ring_id/nic_queue_id
      const auto ring_it = pconfig.port_parameters().find("ring_id");
      if(ring_it == pconfig.port_parameters().end()){
         continue;
      }
      if(pconfig.port_type() == PortConfig::INGRESS_PORT){
         pp_ingress_name[pp_conf.instance_id()][pconfig.port_index()] = ring_it->second; 
      } 
      else if(pconfig.port_type() == PortConfig::EGRESS_PORT){
         pp_egress_name[pp_conf.instance_id()][pconfig.port_index()] = ring_it->second; 
      }
   }
	
}

void set_scheduler( int pid ) {   
   // Change scheduler to RT Round Robin
   int rc, old_sched_policy;
   struct sched_param my_params;
   my_params.sched_priority = 99;
   old_sched_policy = sched_getscheduler( pid );
   rc = sched_setscheduler( pid, SCHED_RR, &my_params ); 
   if (rc == -1) {
      printf( "sched_setscheduler call is failed\n" );
   }
   else {
      printf( "set_scheduler(). pid: %d. old_sched_policy:  %d. new_sched_policy: %d \n", pid, old_sched_policy, sched_getscheduler( pid ) );
   }
}

int MicronfAgent::DeployMicroservices(std::vector<std::string> chain_conf){
   for(int i=0; i < chain_conf.size(); i++){
      printf("Chain_conf size: %lu i: %d\n", chain_conf.size(), i);
      PacketProcessorConfig pp_config;
      std::string config_file_path = chain_conf[i];
	
      int fd = open(config_file_path.c_str(), O_RDONLY);
      if (fd < 0)
         rte_exit(EXIT_FAILURE, "Cannot open configuration file %s\n",
                  config_file_path.c_str());
      google::protobuf::io::FileInputStream config_file_handle(fd);
      config_file_handle.SetCloseOnDelete(true);
      google::protobuf::TextFormat::Parse(&config_file_handle, &pp_config);

      // Goes through the local DS, create ring if needed. 
      MaintainLocalDS( pp_config );

      // Deploy one microservice using fork and execv
      int ms_pid = DeployOneMicroService(pp_config, config_file_path);      
      
      // Set the scheduler to RR
      set_scheduler( ms_pid );
      
   }
	
   //For debugging purpose only.
   for(int t = 0; t < MAX_NUM_MS; t++){
      for(int u = 0; u < MAX_NUM_PORT; u++){
         if(std::get<0>(neighborGraph[t][u]) != 0){
            printf("%d %d -> %d\n", t, u, std::get<0>(neighborGraph[t][u]));
         }
      }
   }	

   return 0;
}

int MicronfAgent::DeployOneMicroService(const PacketProcessorConfig& pp_conf, 
                                        const std::string config_path){
   printf("Deploying One Micro Service . . .\n");
   std::string core_mask = getAvailCore();
   std::string config_para = "--config-file="+config_path;
   
   int pid = fork();
   if(pid == 0){
      printf("child started. id: %d\n", pid);
      char * const argv[] = {"../exec/micronf", "-n", "2", "-c", strdup( core_mask.c_str() ),
                             "-b", "0000:04:00.0", "-b", "0000:05:00.0", "-b", "0000:05:00.1",  
                             "--proc-type", "secondary", "--", strdup(config_para.c_str()), NULL };

      /* printf( "execv ../exec/micronf ");
      for (int i=0; i < 5; i++) { 
         fprintf( stdout, " %s ", argv[i] );
      } 
      */
      execv( "../exec/micronf", argv );
      std::exit(0);
   }
   else {
      printf("In parent, child id: %d\n\n", pid);
      return pid;
   }
}

std::string MicronfAgent::getScaleRingName(){
   return ring_prefix_ + std::to_string(highest_ring_num_++);
}

int MicronfAgent::getNewInstanceId(){
   return ++highest_instance_id;
}

int MicronfAgent::InitMbufPool(){
   const unsigned num_mbufs = (MAX_NUM_USERV * MBUFS_PER_USERV) \
      + (num_ports_ * MBUFS_PER_PORT);
   // const unsigned num_mbufs = num_ports_ * MBUFS_PER_PORT;
   printf("Creating mbuf pool '%s' [%u mbufs] ...\n",
          PKTMBUF_POOL_NAME, num_mbufs);

   pktmbuf_pool = rte_pktmbuf_pool_create(PKTMBUF_POOL_NAME, num_mbufs,
                                          MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

   return (pktmbuf_pool == NULL); /* 0  on success */
}

int MicronfAgent::InitStatMz(int num_nfs){
   stat_mz = rte_memzone_reserve(MZ_STAT, sizeof(*micronf_stats),
                                 rte_socket_id(), 0);
	
   if (stat_mz == NULL)
      rte_exit(EXIT_FAILURE, "Cannot reserve memory zone for port information\n");
   memset(stat_mz->addr, 0, sizeof(*micronf_stats));
	
   micronf_stats = (MSStats*) stat_mz->addr;

   for(int i = 0; i < MAX_NUM_MS; i++){
      for(int j = 0; j < MAX_NUM_PORT; j++){
         micronf_stats->packet_drop[i][j] = 0;
      }
   }
	
   micronf_stats->num_nf = num_nfs;

   return 0;
}

int MicronfAgent::InitScaleBits(int num_nfs){
   scale_bits_mz = rte_memzone_reserve(MZ_SCALE, sizeof(*scale_bits),
                                       rte_socket_id(), 0);

   if (scale_bits_mz == NULL)
      rte_exit(EXIT_FAILURE, "Cannot reserve memory zone for port information\n");

   scale_bits = (ScaleBitVector*) scale_bits_mz->addr;

   for(int i=0; i < MAX_NUM_MS; i++){
      for(int j = 0; j < MAX_NUM_PORT; j++){
         scale_bits->bits[i][j] = 0;
      }
   }
   scale_bits->num_nf = num_nfs;

   return 0;
}


/* Check the link status of all ports in up to 9s, and print them finally */
void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 150 /*15s (150 * 100ms) in total */
   uint8_t portid, count, all_ports_up, print_flag = 0;
   struct rte_eth_link link;

   rte_delay_ms(CHECK_INTERVAL);
   printf("\nChecking link status");
   fflush(stdout);
   for (count = 0; count <= MAX_CHECK_TIME; count++) {
      all_ports_up = 1;
      for (portid = 0; portid < port_num; portid++) {
         if ((port_mask & (1 << portid)) == 0)
            continue;
         memset(&link, 0, sizeof(link));
         rte_eth_link_get_nowait(portid, &link);
         /* print link status if flag set */
         if (print_flag == 1) {
            if (link.link_status)
               printf("Port %d Link Up - speed %u "
                      "Mbps - %s\n", (uint8_t)portid,
                      (unsigned)link.link_speed,
                      (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                      ("full-duplex")  : ("half-duplex\n"));
            else
               printf("Port %d Link Down\n",
                      (uint8_t)portid);
            continue;
         }
         /* clear all_ports_up flag if any link down */
         if (link.link_status == ETH_LINK_DOWN) {
            all_ports_up = 0;
            break;
         }
      }
      /* after finally printing all link status, get out */
      if (print_flag == 1)
         break;

      if (all_ports_up == 0) {
         printf(".");
         fflush(stdout);
         rte_delay_ms(CHECK_INTERVAL);
      }

      /* set the print_flag if all ports up or timeout */
      if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
         print_flag = 1;
         printf("done\n");
      }
   }
}


int MicronfAgent::InitPort( int port_id )
{
   /* for port configuration all features are off by default
      The rte_eth_conf structure includes:
      the hardware offload features such as IP checksum or VLAN tag stripping.
      the Receive Side Scaling (RSS) configuration when using multiple RX queues per port.
   */
   printf("Initializing port %u . . . \n", port_id);
   fflush(stdout);

   struct rte_eth_conf *port_conf;
   struct rte_eth_rxmode *rx_mode;
   struct rte_eth_txmode *tx_mode;

   rx_mode = (struct rte_eth_rxmode*) calloc(1, sizeof(*rx_mode));
   rx_mode->mq_mode = ETH_MQ_RX_RSS;
   rx_mode->split_hdr_size = 0;
   rx_mode->header_split   = 0; // Header Split disabled 
   rx_mode->hw_ip_checksum = 1; // IP checksum offload enabled 
   rx_mode->hw_vlan_filter = 0; // VLAN filtering disabled 
   rx_mode->jumbo_frame    = 0; // Jumbo Frame Support disabled 
   rx_mode->hw_strip_crc   = 1; // CRC stripped by hardware 

   tx_mode = (struct rte_eth_txmode*) calloc(1, sizeof(*tx_mode));
   tx_mode->mq_mode = ETH_MQ_TX_NONE;

   port_conf = (struct rte_eth_conf*) calloc(1, sizeof(*port_conf));
   port_conf->rxmode = *rx_mode;   
   port_conf->txmode = *tx_mode;

   struct rte_eth_dev_info dev_info;
   rte_eth_dev_info_get( port_id, &dev_info );
   dev_info.default_rxconf.rx_drop_en = 1;

   int retval;
   uint16_t q;
   retval = rte_eth_dev_configure( port_id, NUM_RX_QUEUE_PERPORT, 
                                   NUM_TX_QUEUE_PERPORT, port_conf );
   printf( "After rte_eth_dev_configure. retval: %d \n", retval );
   if ( retval< 0 )
      return retval;
 
   for ( q = 0; q < NUM_RX_QUEUE_PERPORT; q++ ) {
      retval = rte_eth_rx_queue_setup( port_id, q, RX_QUEUE_SZ,
                                       rte_eth_dev_socket_id( port_id ), 
                                       &dev_info.default_rxconf, 
                                       pktmbuf_pool );
      if ( retval < 0 )
         return retval; 
   }

   for ( q = 0; q < NUM_TX_QUEUE_PERPORT; q++ ) {
      retval = rte_eth_tx_queue_setup( port_id, q, TX_QUEUE_SZ,
                                       rte_eth_dev_socket_id( port_id ), 
                                       NULL );
      if ( retval < 0 )
         return retval; 
   }

   retval = rte_eth_dev_start( port_id );
   if ( retval < 0 ) 
      return retval;
   
   printf( "After rte_eth_dev_start. retval:%d \n", retval );

   return 0;
}

