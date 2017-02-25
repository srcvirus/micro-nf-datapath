#include<iostream>
#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;

using namespace std;
using namespace rpc_agent;


class RpcAgent{
	public:
		RpcAgent(std::shared_ptr<Channel> channel)
			: stub_(RPC::NewStub(channel)) {}

		int rpc_create_ring(string ring_name){
			ClientContext context;
			CreateRingRequest req;
			Errno result;
			req.set_name(ring_name);
			
			Status status = stub_->rpc_create_ring(&context, req, &result);
			if(result.err() != 0){
					cout << "errno: " << result.err() <<endl;
					return -result.err();
			}
			if(!status.ok()){
					cout << "errno: " << result.err() <<endl;
					return -1;
			}

			return 0;
		}

	
	private:
		std::unique_ptr<RPC::Stub> stub_;

};

int main(int argc, char* argv[]){
	cout<<"this is orchestrator"<<endl;

	RpcAgent* rpcAgent = new RpcAgent(grpc::CreateChannel(
												"0.0.0.0:50051", grpc::InsecureChannelCredentials()));
	
	//TODO test rpc_create_ring
	rpcAgent->rpc_create_ring("ring_u1_u2");

}
