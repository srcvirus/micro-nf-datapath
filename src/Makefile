ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

INCS = -I$(RTE_SDK)/$(RTE_TARGET)/include
LDFLAGS = -L$(RTE_SDK)/$(RTE_TARGET)/lib -L/usr/local/lib -Xlinker -Map=a.map

CXX = g++
#CPPFLAGS += -I/usr/local/include -g -pthread 
CXXFLAGS += -std=c++11 -mssse3 -g -O3 $(INCS) $(LIBDIR)
LDLIBS +=  -Wl,--whole-archive -lrte_pmd_mlx4 -ldpdk -libverbs -Wl,--no-whole-archive\
					 `pkg-config --libs grpc++ grpc --libs protobuf`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf  -lpthread -ldl
           #-lprotobuf -lpthread -ldpdk -ldl   
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ./
SRCDIR = src
OBJDIR = obj
BINDIR = build
STUBDIR = ./ 

vpath %.proto $(PROTOS_PATH)

all: micronf_config micronf_agent_daemon micronf_orchest 

micronf_agent: 
	$(CXX) -c $^ $(INCS) -o $@

cidr_address.o: util/cidr_address.cc
	$(CXX) -c $(CXXFLAGS) $^ $(INCS) -o $@

nic_classifier.o: nic_classifier.cc 
	$(CXX) -c $(CXXFLAGS) $^ $(INCS) -o $@

micronf_orchest: micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_orchest.o micronf_config.pb.o
	$(CXX) $^ $(INCS) $(LDLIBS) $(LDFLAGS) -o $@

micronf_agent_daemon: micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_agent_daemon.o  micronf_config.pb.o micronf_agent.o cidr_address.o nic_classifier.o
	$(CXX) $^ $(INCS) $(LDLIBS) $(LDFLAGS) -o $@


micronf_config: micronf_config.proto micronf_config.pb.cc
	protoc --cpp_out=. micronf_config.proto

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(STUBDIR) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(STUBDIR) $<

clean:
	rm -f ./*.o
	rm -f ./*.pb.cc
	rm -f ./*.pb.h
	rm -f micronf_agent_daemon
	rm -f micronf_orchest

