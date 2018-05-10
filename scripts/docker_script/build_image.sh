#! /bin/bash

# $1 : image_name:tag

docker build -t $1 . 
