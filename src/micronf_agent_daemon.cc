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

// #include <semaphore.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#define PATH_TO_SEM "/home/h4bian/aqua10/micro-nf-datapath/src/VSEM"
#define SEM_PROJECT_ID 100

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

void set_scheduler_RR(){
   // Change scheduler to RT Round Robin
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

}

// This function will create a systemV set of 'num_sems' semaphores. 
// The first semaphore in the set is set to 1 whereas the others are 0.
// The idea is to have a baton moving in the set of the semaphores.
int init_systemv_semaphore( int num_sems ){
   key_t sem_key = ftok( PATH_TO_SEM, SEM_PROJECT_ID );
   int sem_id = semget( sem_key, num_sems, IPC_CREAT | 0666 );
   if ( sem_id < 0 ) {
      perror( "ERROR: semget(). Errno:" );
      return sem_id;
   } else {
      // Set the first sem in the set to 1.
      int res = semctl( sem_id, 0, SETVAL, 1 );
      for ( int i=1; i < num_sems; i++ ) {  
         // set value of other sems to 0
         res = semctl( sem_id, i, SETVAL, 0 );
      }
      if ( res < 0 ) { 
         perror( "ERROR: semctl SETVAL." );
         return -1;
      }
      
      return 0;
   }
}

int main(int argc, char* argv[]){

   // Initializing Agent
   cout << "Agent is running" << endl;

   // Configurations
   std::string conf_folder_path = "../confs/";	
   std::vector<std::string> chain_conf = {
      conf_folder_path + "MacSwap_ShareCore_1.conf",
      conf_folder_path + "MacSwap_ShareCore_2.conf",
      conf_folder_path + "MacSwap_ShareCore_3.conf"
   };

   // Setup scheduler
   // set_scheduler_RR();
   
   // Setup systemV semaphore
   int n_share_core_x = 2;   // fixme pass it as arg to agent and micronf
   init_systemv_semaphore( n_share_core_x );

   // DPDK EAL inititaion done in MicronfAgent
   MicronfAgent micronfAgent;
   micronfAgent.Init(argc, argv);        

   // Fake cores
   micronfAgent.addAvailCore( "0x10" );	   
   micronfAgent.addAvailCore( "0x10" );
   micronfAgent.addAvailCore( "0x20" );	
	
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

   return 0;
}
