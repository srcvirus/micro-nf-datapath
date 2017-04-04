#ifndef _MICRONF_AGENT_H_
#define _MICRONF_AGENT_H_

#include <string>
#include <sys/types.h>
#include <iostream>
#include "common.h"

#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "micronf_config.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using namespace rpc_agent;

class MicronfAgent final : public RPC::Service {
  public:
    MicronfAgent();
    ~MicronfAgent();
    int Init(int argc, char* argv[]);
    int CreateRing(std::string ring_name);
		int DeleteRing(std::string ring_name); 
    int DeployMicroservices(std::string config_str);
    int StartMicroService();
    int StopMicroService();

		const struct rte_memzone *stat_mz;
		MSStats* micronf_stats;

  private:
    int DeployOneMicroService(const micronf_config::Microservice& mserv);
		int InitMbufPool();
		int InitPort(int);	
		int InitStatMz(int num_nf);

		struct rte_mempool *pktmbuf_pool;

    int num_microservices_;
    int num_shared_rings_;
		int num_ports_;
	
};
#endif
