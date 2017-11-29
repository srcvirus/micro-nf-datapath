#include <iostream>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "micronf_agent.h"
#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "grpc_service_impl.h"
#include "nic_classifier.h"
#include "micronf_monitor.h"

#include <sched.h>
#define MY_RT_PRIORITY 99

#include <semaphore.h>

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

int main(int argc, char* argv[]){
/*   // Change scheduler to RT Round Robin
   int rc, old_sched_policy;
   struct sched_param my_params;
   my_params.sched_priority = MY_RT_PRIORITY;
   old_sched_policy = sched_getscheduler(0);
   rc = sched_setscheduler(0, SCHED_RR, &my_params);
   if (rc == -1) {
      printf("Agent sched_setscheduler call is failed\n");
   } 
   printf( "SCHED_RR: %d\n", SCHED_RR );
   printf( "SCHED_FIFO: %d\n", SCHED_FIFO );
   printf( "SCHED_OTHER: %d\n", SCHED_OTHER );
   printf("Old Scheduler: %d\n", old_sched_policy);
   printf("Current Scheduler: %d\n", sched_getscheduler( 0 ));
*/
   // Setting up semaphores
   std::string sem_names [] = { "SEM_CORE_2", "SEM_CORE_3", "SEM_CORE_1" };
   std::map <std::string, sem_t*> semaphores;
   for ( std::string & sname : sem_names ) {
      sem_t* sm = sem_open( sname.c_str(), O_CREAT, 0644, 1 );
      if (sm == SEM_FAILED ) { 
         std::cerr << "sem_open failed!\n";
         return -1;
      }
      int val;
      sem_getvalue(sm, &val);
      std::cout << sname << "Agent Val: " << val <<std::endl;
      
      semaphores.insert( std::pair< std::string, sem_t* > ( sname, sm ) );
   }
   
   // Initializing Agent
   cout<<"Agent is running"<<endl;
   MicronfAgent micronfAgent;
   micronfAgent.Init(argc, argv);
	
   // std::string conf_folder_path = "/home/nfuser/dpdk_study/micro-nf-datapath/confs/";    
   std::string conf_folder_path = "../confs/";	
   std::vector<std::string> chain_conf = {
      conf_folder_path + "MacSwap_ShareCore_1.conf",
      conf_folder_path + "MacSwap_ShareCore_2.conf",
      conf_folder_path + "MacSwap_ShareCore_3.conf"
      //conf_folder_path + "mac_swapper_test.conf"
      //conf_folder_path + "mac_swapper_2.conf",
      //conf_folder_path + "mac_sapper_3.conf",
      //conf_folder_path + "mac_swapper_4.conf"
   };

   // Fake cores
   micronfAgent.addAvailCore( "0x04" );	   
   micronfAgent.addAvailCore( "0x08" );
   micronfAgent.addAvailCore( "0x10" );	


   micronfAgent.addRealCore("3");
   micronfAgent.addRealCore("3");
   micronfAgent.addRealCore("5");
	
   micronfAgent.DeployMicroservices(chain_conf);

   //int monitor_lcore_id = rte_get_next_lcore(rte_lcore_id(), 1, 1);
   int monitor_lcore_id = 1;
   int nic_classifier_lcore_id = 2;
   //int nic_classifier_lcore_id = rte_get_next_lcore(monitor_lcore_id, 1, 1);

   printf("master lcore: %d, monitor lcore: %d, nic_classifier lcore: %d\n", rte_lcore_id(), monitor_lcore_id, nic_classifier_lcore_id);

   //rte_eal_remote_launch(RunMonitor, reinterpret_cast<void*> (&micronfAgent), monitor_lcore_id);
   rte_eal_remote_launch(RunNICClassifier, reinterpret_cast<void*>(&micronfAgent), nic_classifier_lcore_id);

   RunGRPCService(&micronfAgent); 
   rte_eal_mp_wait_lcore();

   for ( auto it = semaphores.begin(); it != semaphores.end(); ++it ) {
      int res = sem_close( it->second );
      if ( res != 0 ) 
         std::cerr << it->first << " sem_close failed." << std::endl; 
   }

   return 0;
}
