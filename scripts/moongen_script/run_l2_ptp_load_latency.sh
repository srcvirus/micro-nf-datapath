#!/bin/bash

# Usage: libmoon ./examples/timestamper-latency.lua [-r <rate>] [-s <size>] [-d <duration>] [-f <file>] [-l <load>] [-h]
#       <dev0> <dev1>
# generate traffic and measure the latency.
 
# Arguments:
#   dev0                                 Device to recv from
#   dev1                                 Device to transmit from

# Options:
#   -r <rate>, --rate <rate>             Transmit rate in Mbit/s
#   -s <size>, --size <size>             Size of the packets in bytes.
#   -d <duration>, --duration <duration> Duration of the expriment in seconds.
#   -f <file>, --file <file>             Filename of the latency histogram. (default: histogram.csv)
#   -l <load>, --load <load>             Rate for additional load run together with TS task in Mbps.
#   -h, --help                           Show this help message and exit.

# sudo ./build/MoonGen ./examples/timestamper-latency.lua -s 64 -r 10000 0 1 -d 15 -l 0
