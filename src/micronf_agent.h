#ifndef _MICRONF_AGENT_H_
#define _MICRONF_AGENT_H_

#include <string>
#include <vector>
#include <sys/types.h>
#include <iostream>
#include <fcntl.h>
#include <utility>

#include "common.h"

#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "micronf_config.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>


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
    int DeployMicroservices(std::vector<std::string> chain_conf);
    //int StartMicroService();
    //int StopMicroService();
		std::string getScaleRingName();
		int getNewInstanceId();
		const struct rte_memzone *stat_mz;
		MSStats *micronf_stats;
		const struct rte_memzone *scale_bits_mz;
		ScaleBitVector *scale_bits;
		// store the next microservice id of the current uS and port	id
		std::pair<int,int> neighborGraph [MAX_NUM_MS][MAX_NUM_PORT];			
		// store the pp_config of microservice by id
		PacketProcessorConfig ppConfigList[MAX_NUM_MS];
    int DeployOneMicroService(const PacketProcessorConfig& pp_conf,
																const std::string config_path);

  private:
		int InitMbufPool();
		int InitPort(int);	
		int InitStatMz(int);
		int InitScaleBits(int);
	
		void UpdateNeighborGraph(PacketProcessorConfig& pp_config, const PortConfig& pconfig);
		void MaintainLocalDS(PacketProcessorConfig& pp_conf);
		

		struct rte_mempool *pktmbuf_pool;
		int num_ports_;
		std::string ring_prefix_ = "scale_ring_";
		unsigned int highest_ring_num_ = MAX_NUM_MS;
		unsigned int highest_instance_id = MAX_NUM_MS;	
		// intermediate info for building neighborGraph. Store ring_id
		std::string pp_ingress_name[MAX_NUM_MS][MAX_NUM_PORT];
		std::string pp_egress_name[MAX_NUM_MS][MAX_NUM_PORT];
};
#endif
