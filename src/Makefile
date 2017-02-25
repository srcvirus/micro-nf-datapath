CXX = g++
CPPFLAGS += -I/usr/local/include -g -pthread
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ./
SRCDIR = src
OBJDIR = obj
BINDIR = build
STUBDIR = ./ 

vpath %.proto $(PROTOS_PATH)

all: micronf_agent micronf_orchest main

micronf_agent: 
	$(CXX) $^ $(LDFLAGS) -o $@

micronf_orchest: micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_orchest.o
	$(CXX) $^ $(LDFLAGS) -o $@

main: main.cc micronf_agent.pb.o micronf_agent.grpc.pb.o micronf_agent.o 
	$(CXX) $^ $(LDFLAGS) -o $@


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
