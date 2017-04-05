#include <iostream>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <thread>
#include <unistd.h>

#include "micronf_agent.h"
#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "grpc_service_impl.h"
#include "nic_classifier.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using namespace std;
using namespace rpc_agent;
using namespace micronf_config;

int RunAgent(void* arg) {
  MicronfAgent* agent = reinterpret_cast<MicronfAgent*>(arg);
	//FIXME specify non-dpdk interface
	std::string server_address("0.0.0.0:50051");
  GrpcServiceImpl service;
	service.set_mAgent(agent);
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Agent(Server) listening on " << server_address << std::endl;
  server->Wait();
  return 0;
}

int RunNICClassifier(void* arg) {
  MicronfAgent* micronfAgent = reinterpret_cast<MicronfAgent*>(arg);
  printf("in RunNICClassifier\n");
	NICClassifier nicClassifier;
	nicClassifier.Init(micronfAgent);
	
	//create rule and add rule
	vector<FwdRule> sampleRules;
	CIDRAddress src_addr_1("10.10.0.7/24");
	CIDRAddress dst_addr_1("10.10.0.10/24");
	FwdRule rule_1(src_addr_1, dst_addr_1, 1234, 5678, "rx_ring_0");
 	nicClassifier.AddRule(rule_1); 
	nicClassifier.Run();
  return 0;
}


int main(int argc, char* argv[]){
	cout<<"Agent is running"<<endl;
	MicronfAgent micronfAgent;
	micronfAgent.Init(argc, argv);
  int nic_classifier_lcore_id = rte_get_next_lcore(rte_lcore_id(), 1, 1);
  rte_eal_remote_launch(RunNICClassifier, 
                        reinterpret_cast<void*>(&micronfAgent), 
                        nic_classifier_lcore_id);
  RunAgent(&micronfAgent);
  rte_eal_wait_lcore(nic_classifier_lcore_id);
	cout<<"Agent finished blocking"<<endl;
}
