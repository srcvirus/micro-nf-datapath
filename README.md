# micro-nf-datapath
DPDK-based datapath for stitching microservices to create network services

## prerequired  
1. hugepage enabled and mounted  
2. mellanox ofed_driver and mellanox dpdk is built  

## Building  
`export RTE_SDK=/path-to-dpdk/MLNX_DPDK_2.2_4.2`  
`export RTE_TARGET=x86_64-native-linuxapp-gcc`  

make  

## Running   
### Dpdk daemon   
Responsible for creating rte_ring for uservs to communicate
`sudo ./dpdk_daemon/build/dpdk_daemon -c 1 -n 4 -- -p 1 -n 2`  

### Userv1   
First microservice that receive packets from rx queue and put it into rte_ring1 for next userv use   
`sudo ./userv1/build/userv1 -c 2 -n 4 --procype=auto -- -n 0`  


### Userv2   
Second microservice that receive packets from rte_ring1, do some processing (TODO) and put it into rte_ring2 for next userv use   
`sudo ./userv2/build/userv2 -c 4 -n 4 --procype=auto -- -n 1`   


