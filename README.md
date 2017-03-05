# micro-nf-datapath
DPDK-based datapath for stitching microservices to create network services    

### CONFIGURATION    
export RTE_SDK=/home/nfuser/dpdk_study/dpdk-17.02   
 
### BUILD & RUN     
make    
sudo ./micronf_agent_daemon -c 0x01 -n 4     
./micronf_orchest    

### DONE:     
- skeleton class for agent    
- rpc channel created and tested    
- implementing ring creation  

### TODO:    
- configuration files handling    
- microservice deployment       
