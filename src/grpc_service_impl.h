#ifndef _GRPC_SERVICE_IMPL_H_
#define _GRPC_SERVICE_IMPL_H_

#include <string>
#include <iostream>
#include <vector>

class GrpcServiceImpl final : public RPC::Service {
  private:
    MicronfAgent *mAgent;

    Status rpc_create_ring(ServerContext* context, const CreateRingRequest* s,  
                              Errno* reply) override {
      std::cout<<"rpc_create_ring is called"<<std::endl;
      std::string ring_name = s->name();
      int ret = mAgent->CreateRing(ring_name);  
			if(ret != 0){
					std::cerr<<"CreateRing fails"<<std::endl;
      		return Status::CANCELLED;
			} 	
    
      return Status::OK;
    }   
		
		Status rpc_deploy_microservices(ServerContext* context, const DeployConfig* dc,
															Errno* reply) override {
      std::cout<<"rpc_deploy_microservices is called"<<std::endl;
			//std::string chain_config = dc->config();	
		 	std::vector<std::string> chain_config;
			int ret = mAgent->DeployMicroservices(chain_config);
			if(ret != 0){
					std::cerr<<"DeployMicroservices fails"<<std::endl;
      		return Status::CANCELLED;
			} 	
      
			return Status::OK;
		}
  
  public:
    int set_mAgent(MicronfAgent* agent){
      mAgent = agent;
    }   
};


#endif

