#include <iostream>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fstream>

#include "micronf_agent.h"
#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "grpc_service_impl.h"
#include "nic_classifier.h"
#include "micronf_monitor.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using namespace std;
using namespace rpc_agent;

int RunGRPCService(void* arg) {
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
   CIDRAddress src_addr_1("10.0.0.10/24");
   CIDRAddress dst_addr_1("10.0.0.7/24");
   FwdRule rule_1(src_addr_1, dst_addr_1, 1234, 5678, "rx_ring_0");
   nicClassifier.AddRule(rule_1); 
   nicClassifier.Run();
   return 0;
}

int RunMonitor(void* arg) {
   MicronfAgent* micronfAgent = reinterpret_cast<MicronfAgent*>(arg);
   MicronfMonitor micronfMonitor;
   micronfMonitor.Init(micronfAgent);
   printf("in RunMonitor\n");

   micronfMonitor.Run();
   return 0;
}

std::vector< std::string > parse_chain_conf( std::string file_path ){
   std::ifstream in_file( file_path.c_str() );
   std::string line;
   std::vector< std::string > conf_vector;
   while ( in_file ) {
      getline( in_file, line );
      if( !line.empty() )
      conf_vector.push_back( line );
   }

   return conf_vector;
}

int main(int argc, char* argv[]){

   cout << "Agent is running" << endl;

   // Parse Chain Configurations
   std::vector<std::string> chain_conf = parse_chain_conf( "../confs/Chain.conf" );
   
   // DPDK EAL inititaion done in MicronfAgent
   // It also changes the sched policy to SCHED_RR, thus child process inherits.
   MicronfAgent micronfAgent;
   micronfAgent.Init(argc, argv);        

   micronfAgent.DeployMicroservices( chain_conf );

   int monitor_lcore_id = 1;
   int nic_classifier_lcore_id = 2;

   printf("master lcore: %d, monitor lcore: %d, nic_classifier lcore: %d\n", rte_lcore_id(), monitor_lcore_id, nic_classifier_lcore_id);

   //rte_eal_remote_launch(RunMonitor, reinterpret_cast<void*> (&micronfAgent), monitor_lcore_id);
   rte_eal_remote_launch(RunNICClassifier, reinterpret_cast<void*>(&micronfAgent), nic_classifier_lcore_id);

   RunGRPCService(&micronfAgent); 

   rte_eal_mp_wait_lcore();

   return 0;
}
