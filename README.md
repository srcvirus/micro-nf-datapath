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

### Run pktgen
$ cd ~/pktgen-dpdk
$ sudo ./app/app/x86_64-native-linuxapp-gcc/pktgen -c 0x00000FC0 -n 8 --master-lcore 6 -- -P -m "[7:7].0"

### Docker Deployment     
First timers, build the docker image from Dockerfile.     
$ ./build_image.sh ubuntu:dpdk.X    

Then, instantiate docker container.    
$ ./build_container.sh micro-con-X ubuntu:dpdk.X    

To detach from docker     
$ <Ctrl> + p + <Ctrl> + q    

To quit the docker    
$ <Ctrl> + d      

To start & reattach    
$ ./start_attach_docker.sh micro-con-X (or container ID)   