# micro-nf-datapath
DPDK-based datapath for stitching microservices to create network services    

### CONFIGURATION    
export RTE_SDK=/home/nfuser/dpdk_study/dpdk-17.02   
 
### BUILD & RUN     
make    

sudo ./micronf_agent_daemon --proc-type primary -c 0x1c

### Moving packet processor executables   
copy file to micro-nf-datapath/exec/$PACKET_PROCESSOR_CLASS   
e.g.     
cp ../../micro-nf/build/micronf ../exec/MacSwapper    


### DONE:     
- skeleton class for agent    
- rpc channel created and tested    
- implementing ring creation  
- configuration files handling (orchest)   
- statistic shared between ms
- microservice deployment       

### TODO:    
- scale out testing and impact    
- evaluation    

### Run pktgen
$ cd ~/pktgen-dpdk
$ sudo ./app/app/x86_64-native-linuxapp-gcc/pktgen -c 0x00000FC0 -n 8 --master-lcore 6 -- -P -m "[7:7].0"
