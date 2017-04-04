# micro-nf-datapath
DPDK-based datapath for stitching microservices to create network services    

### CONFIGURATION    
export RTE_SDK=/home/nfuser/dpdk_study/dpdk-17.02   
 
### BUILD & RUN     
make    

sudo ./micronf_agent_daemon --proc-type primary -c 0x01

./micronf_orchest    

### DONE:     
- skeleton class for agent    
- rpc channel created and tested    
- implementing ring creation  
- configuration files handling (orchest)   
- statistic shared between ms

### TODO:    
- configuration files handling (agent)   
- microservice deployment       
