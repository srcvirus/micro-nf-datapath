#ifndef _GRPC_SERVICE_IMPL_
#define _GRPC_SERVICE_IMPL_

#include <string>
#include <iostream>

class GrpcServiceImpl final : public RPC::Service {
  private:
    MicronfAgent *mAgent;

    Status rpc_create_ring(ServerContext* context, const CreateRingRequest* s,  
                              Errno* reply) override {
      std::cout<<"RPCCreateRing is called"<<std::endl;
      std::string ring_name = s->name();
      int ret = mAgent->CreateRing(ring_name);  
    
      return Status::OK;
    }   

  
  public:
    int set_mAgent(MicronfAgent* agent){
      mAgent = agent;
    }   
};


#endif

