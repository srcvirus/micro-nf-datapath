#include <iostream>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <rte_log.h>

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

struct nic_class_args {
   struct MicronfAgent* ma;
   int port_id;
};

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

int RunNICClassifier(void* args_ptr) {
   struct nic_class_args* args = (struct nic_class_args*) args_ptr;
   MicronfAgent* micronfAgent = reinterpret_cast<MicronfAgent*>(args->ma);
   // FIXME
   int n_ports = 2;
   RTE_LOG(INFO, USER1, "in RunNICClassifier. port_id: %d num_ports: %d \n", args->port_id, n_ports);
   NICClassifier nicClassifier;
   nicClassifier.Init(micronfAgent, args->port_id, n_ports);
	
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

   // if 2nd arg (dry_run) is true, monitor won't deploy scale-out instances
   micronfMonitor.Init( micronfAgent, true );  

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
   // All configurations should be stored in ../confs/Chain.conf
   std::vector<std::string> chain_conf = parse_chain_conf( "../confs/Chain.conf" );
   
   // DPDK EAL inititaion done in MicronfAgent
   // It also changes the sched policy to SCHED_RR, thus child process inherits.
   MicronfAgent micronfAgent;
   micronfAgent.Init(argc, argv);        

   micronfAgent.DeployMicroservices( chain_conf );

//   int monitor_lcore_id = 1;
   int nic_classifier_lcore_id_1 = 2;
   int nic_classifier_lcore_id_2 = 3;
   
   // Fixme: 
   // Monitor will spawn new process with obolete config.
   //rte_eal_remote_launch(RunMonitor, reinterpret_cast<void*> (&micronfAgent), monitor_lcore_id);

   nic_class_args nicClassArgs1, nicClassArgs2;
   nicClassArgs1.ma = &micronfAgent;
   nicClassArgs1.port_id = 0;

   nicClassArgs2.ma = &micronfAgent;
   nicClassArgs2.port_id = 1;
      
   rte_eal_remote_launch(RunNICClassifier, reinterpret_cast<void*>(&nicClassArgs1), nic_classifier_lcore_id_1);

   rte_eal_remote_launch(RunNICClassifier, reinterpret_cast<void*>(&nicClassArgs2), nic_classifier_lcore_id_2);

   RunGRPCService(&micronfAgent); 

   rte_eal_mp_wait_lcore();

   return 0;
}
