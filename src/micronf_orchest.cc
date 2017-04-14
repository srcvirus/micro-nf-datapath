#include<iostream>
#include<string>
#include<stdlib.h>
#include <grpc++/grpc++.h>
#include "micronf_agent.grpc.pb.h"
#include "micronf_config.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;


using namespace std;
using namespace rpc_agent;
using namespace micronf_config;


class RpcAgent{
	public:
		RpcAgent(std::shared_ptr<Channel> channel)
			: stub_(RPC::NewStub(channel)) {}

		int RPCCreateRing(string ring_name){
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

		int RPCDeployConfig(DeployConfig *dc){
			ClientContext context;
			Errno result;

			Status status = stub_->rpc_deploy_microservices(&context, *dc, &result);
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

void AddConfigFile(Microservice* ms, string msType, string r1, string r2){
	ms->set_id(to_string(rand()%1000 + 1));

	if(msType == "classifier")
		ms->set_type(Microservice::CLASSIFIER);
	if(msType == "counter")
		ms->set_type(Microservice::COUNTER);
	if(msType == "modifier")
		ms->set_type(Microservice::MODIFIER);
	
	ms->add_in_port_types(Microservice::NORMAL_INGRESS);
	ms->add_eg_port_types(Microservice::NORMAL_EGRESS);
	ms->add_in_port_names(r1);
	ms->add_eg_port_names(r2);
	
//	inPortType->set
}

int main(int argc, char* argv[]){
	cout<<"this is orchestrator"<<endl;

	RpcAgent* rpcAgent = new RpcAgent(grpc::CreateChannel(
												"0.0.0.0:50051", grpc::InsecureChannelCredentials()));
	
	//rpcAgent->RPCCreateRing("ring_u1_u2");
	//rpcAgent->RPCCreateRing("ring_u2_u3");
	cout<<"creating ring r1"<<endl;
	rpcAgent->RPCCreateRing("rx_ring_1");
	rpcAgent->RPCCreateRing("rx_ring_2");
	
/*
	string str_deployConf;
	DeployConfig deployConf;
	MicronfConfig micronfConf;
	
	AddConfigFile(micronfConf.add_list(), "classifier", "ring_u1_u2", "ring_u2_u3");
	//AddConfigFile(micronfConf.add_list(), "counter", "ring_u2_u3", "ring_u3_u4");
	if(!micronfConf.SerializeToString(&str_deployConf)){
		cerr<<"Failed to serialize"<<endl;
	}
	
	deployConf.set_config(str_deployConf);
		
	rpcAgent->RPCDeployConfig(&deployConf);

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
*/

return 0;	
}
