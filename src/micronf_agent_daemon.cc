#include<iostream>
#include "micronf_agent.h"


#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using namespace std;
using namespace rpc_agent;

class AgentServiceImpl final : public RPC::Service {
	Status rpc_create_ring(ServerContext* context, const CreateRingRequest* s, 
														Errno* reply) override {
		//TODO create dpdk ring
		cout<<"RPCCreateRing is called"<<endl;
		//
		return Status::OK;
	}
};

void RunAgent(){
	//FIXME specify non-dpdk interface
	std::string server_address("0.0.0.0:50051");
  AgentServiceImpl service;

  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Agent(Server) listening on " << server_address << std::endl;

  server->Wait();

}

int main(int argc, char* argv[]){
	cout<<"Agent is running"<<endl;
	RunAgent();	
	cout<<"Agent finished blocking"<<endl;
	MicronfAgent micronfAgent;
	micronfAgent.Init(argc, argv);
}
