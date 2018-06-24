#!/bin/bash

# How to run:
# ./cycles_estimator.sh <IN_RING_NAME> <OUT_RING_NAME> <SAMPLE_SIZE> <CPU_ID>
# e.g
#   ./cycles_estimator.sh rx_ring_0 rx_ring_1 200 3
# 
# After the agent is ready, Press Enter to start the measurement.
# 

SAMPLE_SIZE="200"
CPU_ID="3"
IN_RING="rx_ring_0"
OUT_RING="rx_ring_1"

if [ "$1" != "" ]; then
    IN_RING="$1"
else
    echo "Input ring is $IN_RING"
fi
if [ "$1" != "" ]; then
    OUT_RING="$2"
else
    echo "Out ring is $OUT_RING"
fi

if [ "$3" != "" ]; then
    SAMPLE_SIZE="$3"
    echo "Sample size is changed to $3"
else
    echo "Default sample size is 100"
fi
if [ "$4" != "" ]; then
    CPU_ID="$4"
    echo "This process is pinned to $4"
else
    echo "This process is pinned to $CPU_ID"
fi

sudo ./micronf_agent_daemon -n 4 --proc-type primary -c 0xfe -- &

read cont

echo "sudo ./micronf_cycles_estimator -n 2 --proc-type secondary -- --in_ring=$IN_RING --out_ring=$OUT_RING --sample_size=$SAMPLE_SIZE --cpu_id=$CPU_ID"

sudo ./micronf_cycles_estimator -n 2 --proc-type secondary -- --in_ring=$IN_RING --out_ring=$OUT_RING --sample_size=$SAMPLE_SIZE --cpu_id=$CPU_ID --pkt_len=$5

sudo pkill -P $$
sudo pkill micronf
