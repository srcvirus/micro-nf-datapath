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

class GrpcServiceImpl final : public RPC::Service {
	private:
		MicronfAgent *mAgent;
		Status rpc_create_ring(ServerContext* context, const CreateRingRequest* s, 
															Errno* reply) override {
			cout<<"RPCCreateRing is called"<<endl;
			string ring_name = s->name();
			int ret = mAgent->CreateRing(ring_name);	
			
			return Status::OK;
		}
	
	public:
		int set_mAgent(MicronfAgent* agent){
			mAgent = agent;
		}
};

void RunAgent(MicronfAgent* agent){
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

}

int main(int argc, char* argv[]){
	cout<<"Agent is running"<<endl;
	MicronfAgent micronfAgent;
	micronfAgent.Init(argc, argv);

	RunAgent(&micronfAgent);	
	cout<<"Agent finished blocking"<<endl;
}
