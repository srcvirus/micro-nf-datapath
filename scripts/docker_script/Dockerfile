FROM ubuntu:trusty

RUN apt-get update
RUN apt-get -y install git vim emacs curl autoconf automake libtool curl make g++ unzip
RUN apt-get -y install build-essential libz-dev libibverbs-dev libssl-dev pkg-config wget


RUN git clone -b v1.0.x https://github.com/grpc/grpc && cd grpc \
    && git submodule update --init && cd third_party/protobuf \
    && ./autogen.sh && ./configure && make -j && make install && ldconfig\
    && cd ../../ && make -j && make install && cd ..

CMD ["/bin/bash"]

WORKDIR /anthony
VOLUME ["/anthony"]

ENV RTE_SDK=/anthony/dpdk-stable-17.05.2
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
ENV LD_LIBRARY_PATH=/usr/local/lib

