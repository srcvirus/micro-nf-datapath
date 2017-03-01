#include "micronf_agent.h"

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
using namespace std;

#define MAX_NUM_USERV 100
#define MBUFS_PER_USERV 1536
#define MBUFS_PER_PORT 1536
#define MBUF_CACHE_SIZE 512
#define RTE_MP_RX_DESC_DEFAULT 512
#define RTE_MP_TX_DESC_DEFAULT 512
#define USERV_QUEUE_RINGSIZE 128

#define PKTMBUF_POOL_NAME "MICRONF_MBUF_POOL"

MicronfAgent::MicronfAgent(void){
	num_microservices_ = 0;
	num_shared_rings_ = 0;
	num_ports_ = 0;

}

MicronfAgent::~MicronfAgent(void){}

int MicronfAgent::Init(int argc, char* argv[]){

	int retval = rte_eal_init(argc, argv);
	if(retval < 0){
		cerr<<"rte_eal_init() fails "<<strerror(errno)<<endl;
		return -1;
	}
	argc -= retval;
	argv += retval;

	num_ports_ = rte_eth_dev_count();	

	//FIXME do we need to store & share port info

	retval = InitMbufPool();
	if (retval != 0)
		rte_exit(EXIT_FAILURE, "Cannot create needed mbuf pools\n");
	
	
};

int MicronfAgent::CreateRing(){};
/*
int MicronfAgent::DeployMicroServices(){

}

int  MicronfAgent::StartMicroService(){

} 

int  MicronfAgent::StopMicroService(){

}

int  MicronfAgent::DestroyMicroService(){

}

int  MicronfAgent::DeployOneMicroService(){

}
*/

int MicronfAgent::InitMbufPool(void){
	const unsigned num_mbufs = (MAX_NUM_USERV * MBUFS_PER_USERV) \
			+ (num_ports_ * MBUFS_PER_PORT);

	printf("Creating mbuf pool '%s' [%u mbufs] ...\n",
			PKTMBUF_POOL_NAME, num_mbufs);

	pktmbuf_pool = rte_pktmbuf_pool_create(PKTMBUF_POOL_NAME, num_mbufs,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	return (pktmbuf_pool == NULL); /* 0  on success */
}


