#ifndef _MICRONF_AGENT_H_
#define _MICRONF_AGENT_H_

class MicronfAgent {
  public:
    MicronfAgent(){}
    ~MicronfAgent(){}
    int Init(int argc, char* argv[]);
    int CreateRing(); 
    int DeployMicroServices();
    int StartMicroService();
    int StopMicroService();
    int DestroyMicroService();

  private:
    int DeployOneMicroService();
    int num_microservices_;
    int num_shared_rings_;
};
#endif
