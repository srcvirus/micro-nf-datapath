ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

INCS = -I$(RTE_SDK)/$(RTE_TARGET)/include
LIBDIR = -L$(RTE_SDK)/$(RTE_TARGET)/lib

CXX = g++
CPPFLAGS += -I/usr/local/include -g -pthread 
CXXFLAGS += -std=c++11 -mssse3 -O3 $(INCS) $(LIBDIR)
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldpdk -ldl  
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ./
SRCDIR = src
OBJDIR = obj
BINDIR = build
STUBDIR = ./ 

vpath %.proto $(PROTOS_PATH)

all: micronf_orchest micronf_agent_daemon

micronf_agent: 
	$(CXX) $^ $(INCS) $(LDFLAGS)  $(LIBDIR) -o $@

micronf_orchest: micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_orchest.o
	$(CXX) $^ $(INCS) $(LDFLAGS) $(LIBDIR) -o $@

micronf_agent_daemon: micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_agent.o micronf_agent_daemon.o
	$(CXX) $^ $(INCS) $(LDFLAGS) $(LIBDIR) -o $@


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

