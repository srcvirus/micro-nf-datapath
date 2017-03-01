#ifndef _MICRONF_AGENT_H_
#define _MICRONF_AGENT_H_

#include <string>
#include <sys/types.h>

class MicronfAgent {
  public:
    MicronfAgent();
    ~MicronfAgent();
    int Init(int argc, char* argv[]);
    int CreateRing(std::string ring_name); 
    int DeployMicroServices();
    int StartMicroService();
    int StopMicroService();
    int DestroyMicroService();
		

  private:
    //int DeployOneMicroService();
		int InitMbufPool();
		int InitPort(uint8_t);	
		/* The mbuf pool for packet rx */
		struct rte_mempool *pktmbuf_pool;

	
    int num_microservices_;
    int num_shared_rings_;
		int num_ports_;
};
#endif
