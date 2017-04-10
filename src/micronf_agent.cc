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

using namespace std;

// mbuf pools configuration
#define MAX_NUM_USERV 100
#define MBUFS_PER_USERV 1536
#define MBUFS_PER_PORT 1536
#define MBUF_CACHE_SIZE 512
#define PKTMBUF_POOL_NAME "MICRONF_MBUF_POOL"

// Port configuration
#define RTE_MP_RX_DESC_DEFAULT 1024 // 512
#define RTE_MP_TX_DESC_DEFAULT 1024 // 512
#define USERV_QUEUE_RINGSIZE 256 // 128
#define NUM_TX_QUEUE_PERPORT 1
#define NUM_RX_QUEUE_PERPORT 1

MicronfAgent::MicronfAgent(){
	num_microservices_ = 0;
	num_shared_rings_ = 0;
	num_ports_ = 0;
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

	//FIXME do we need to store & share port info
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
	int num_nfs = 1;
	InitStatMz(num_nfs);
		
}

int MicronfAgent::CreateRing(string ring_name){
	unsigned socket_id = rte_socket_id();
	const unsigned ring_size = USERV_QUEUE_RINGSIZE;
	struct rte_ring* rx_q = rte_ring_create(ring_name.c_str(), ring_size, socket_id,
														RING_F_SP_ENQ | RING_F_SC_DEQ ); /* single prod, single cons */
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


int MicronfAgent::DeployMicroservices(string config_str){
	micronf_config::MicronfConfig micronfConfig;

	if(!micronfConfig.ParseFromString(config_str)){
		cerr<<"Failed to parse the config"<<endl;
		return -1;
	}	

	for(int i=0; i<micronfConfig.list_size(); i++){
		const micronf_config::Microservice& mserv = micronfConfig.list(i);
		//TODO deploy accordingly(factory may be) 
		DeployOneMicroService(mserv);
	}

}

int  MicronfAgent::DeployOneMicroService(const micronf_config::Microservice& mserv){
	string mserv_id = mserv.id();
	vector<int> in_port_types; 
	vector<int> eg_port_types; 
	vector<string> in_port_names; 
	vector<string> eg_port_names;
 
	for(int j=0; j < mserv.in_port_types_size(); j++){
		//in_port_types.push_back(mserv.in_port_types(j));
		//in_port_names.push_back(mserv.in_port_names(j));
		CreateRing(mserv.in_port_names(j));
		switch(mserv.in_port_types(j)){
			case micronf_config::Microservice_PortType::Microservice_PortType_NORMAL_EGRESS:
				
				break;
			
			case micronf_config::Microservice_PortType::Microservice_PortType_SYNC_INGRESS:
				break;
			case micronf_config::Microservice_PortType::Microservice_PortType_NIC_INGRESS:
				break;
			
		}
	}

	for(int j=0; j < mserv.eg_port_types_size(); j++){
		//eg_port_types.push_back(mserv.eg_port_types(j));
		//eg_port_names.push_back(mserv.eg_port_names(j));
		CreateRing(mserv.eg_port_names(j));
		switch(mserv.eg_port_types(j)){
			case micronf_config::Microservice_PortType::Microservice_PortType_NORMAL_EGRESS:
				//TODO instantiate the micronf !
			 
			break;

			case micronf_config::Microservice_PortType::Microservice_PortType_BRANCH_EGRESS:
			break;
		}

	}
	
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

	for(int i = 0; i < micronf_stats->num_nf; i++){
		micronf_stats->packet_drop[i] = 0;
	}
	
	micronf_stats->num_nf = num_nfs;

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

