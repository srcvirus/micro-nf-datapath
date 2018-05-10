#!/bin/bash

# -r --rate", "Transmit rate in Mbit/s"):default(1000)
# -s --size", "Size of the packets in bytes."):default(60)
# -d --duration", "Duration of the expriment in secs."):default(10)
# -f --file", "Filename of the latency histogram."):default("histogram.csv")

sudo ./build/MoonGen ./examples/timestamper-latency.lua -s 60 -r 1000 0 1 -d 12 
