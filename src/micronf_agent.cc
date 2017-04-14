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

// Port configuration
#define RTE_MP_RX_DESC_DEFAULT 2048 // 512
#define RTE_MP_TX_DESC_DEFAULT 2048 // 512
#define USERV_QUEUE_RINGSIZE 2048 // 128
#define NUM_TX_QUEUE_PERPORT 1
#define NUM_RX_QUEUE_PERPORT 1

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

	//FIXME hardcoded number of port to 1
	// no portmask parsing 
	int port_id = 0;
	retval = InitPort(port_id);
	if(retval < 0){
			rte_exit(EXIT_FAILURE, "Cannot initialise port %u\n",
					(unsigned)port_id);
	}

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
	if(ring == NULL)
		CreateRing(ring_it->second.c_str());
}

void MicronfAgent::MaintainLocalDS(PacketProcessorConfig& pp_conf){
	// retrieve the config form this DS when scale out
	ppConfigList[pp_conf.instance_id()] = pp_conf;
	printf("\npp_conf instance_id: %d\n", pp_conf.instance_id());
	printf("pp_conf class: %s\n", pp_conf.packet_processor_class().c_str());
	printf("pp_con num_ingress: %d\n\n", pp_conf.num_ingress_ports());

	
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
		/*
		//For debugging purpose only.
		printf("############################### %d #################################### \n", i);
		std::string str = "";
		google::protobuf::TextFormat::PrintToString(pp_config, &str);
		printf("%s\n\n\n", str.c_str());
		*/

		MaintainLocalDS(pp_config);
		DeployOneMicroService(pp_config, config_file_path);
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
    char * const argv[] = {"/home/nfuser/dpdk_study/micro-nf-datapath/exec/MacSwapper", "-c", 
										strdup(core_mask.c_str()), "-n", "2", "--proc-type", "secondary", "--",
    								strdup(config_para.c_str()), NULL};

    execv("/home/nfuser/dpdk_study/micro-nf-datapath/exec/MacSwapper", argv);
    return pid;
  }
	else {
		printf("parent id: %d\n", pid);
		return pid;
	}
}

std::string MicronfAgent::getScaleRingName(){
	return ring_prefix_ + std::to_string(highest_ring_num_++);
}

int MicronfAgent::getNewInstanceId(){
	return ++highest_instance_id;
}

/*
int  MicronfAgent::StartMicroService(){

} 

int  MicronfAgent::StopMicroService(){

}

int  MicronfAgent::DestroyMicroService(){

}
*/


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

int MicronfAgent::InitPort(int port_id)
{
	/* for port configuration all features are off by default
		 The rte_eth_conf structure includes:
			the hardware offload features such as IP checksum or VLAN tag stripping.
			the Receive Side Scaling (RSS) configuration when using multiple RX queues per port.
	 */
	printf("InitPort %d\n", port_id);
	struct rte_eth_conf *port_conf;
	struct rte_eth_rxmode *t_rxmode;
	t_rxmode = (struct rte_eth_rxmode*) calloc(1, sizeof(*t_rxmode));
	t_rxmode->mq_mode = ETH_MQ_RX_RSS;
	//t_rxmode->mq_mode = ETH_MQ_RX_NONE;
	port_conf = (struct rte_eth_conf*) calloc(1, sizeof(*port_conf));
	port_conf->rxmode = *t_rxmode;
	

	const uint16_t rx_rings = NUM_RX_QUEUE_PERPORT, tx_rings = NUM_TX_QUEUE_PERPORT;
	const uint16_t rx_ring_size = RTE_MP_RX_DESC_DEFAULT;
	const uint16_t tx_ring_size = RTE_MP_TX_DESC_DEFAULT;
	
  int retval;
	if((retval = rte_eth_dev_configure((uint8_t)port_id, rx_rings, tx_rings,
				port_conf)) != 0)
		return retval;
		
	for(int q=0; q < rx_rings; q++){
		retval = rte_eth_rx_queue_setup(port_id, q, rx_ring_size,
				rte_eth_dev_socket_id(port_id),	NULL, pktmbuf_pool);
		if (retval < 0) return retval;
	}
	
	for(int q=0; q < rx_rings; q++){
		retval = rte_eth_tx_queue_setup(port_id, q, tx_ring_size,
				rte_eth_dev_socket_id(port_id), NULL);
		if (retval < 0) return retval;
	}
	
	//rte_eth_promiscuous_enable(port_id);

	retval = rte_eth_dev_start(port_id);
	if(retval < 0) return retval;

	return 0;
}

