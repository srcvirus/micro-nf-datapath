#! /bin/bash

# $1 : container name e.g micro-con-1
# $2 : docker image name eg. ubuntu:dpdk.0

docker run --privileged \
   -v /mnt/huge:/mnt/huge \
   -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
   -v /dev:/dev \
   -v /var/run:/var/run \
   -v /home/z3bai/tem64/dpdk-stable-17.05.2:/anthony/dpdk-stable-17.05.2 \
   --name $1 -it $2

# If you need to share folder
#   -v /home/z3bai/tem64/container_study:/anthony/container_study \

# If we need to bind NIC, we might need these:
#   -v /sys/devices/system/node:/sys/devices/system/node \
#   -v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
