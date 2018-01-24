#! /bin/bash

#
# $1 : ../confs/CONF_NAME

sudo chrt --rr 99 ../exec/micronf -b 0000:05:00.1 -b 0000:05:00.0 -n 2 --proc-type secondary -- --config-file=$1
