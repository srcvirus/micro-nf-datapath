// Generated by the gRPC protobuf plugin.
// If you make any local change, they will be lost.
// source: micronf_agent.proto

#include "micronf_agent.pb.h"
#include "micronf_agent.grpc.pb.h"

#include <grpc++/impl/codegen/async_stream.h>
#include <grpc++/impl/codegen/async_unary_call.h>
#include <grpc++/impl/codegen/channel_interface.h>
#include <grpc++/impl/codegen/client_unary_call.h>
#include <grpc++/impl/codegen/method_handler_impl.h>
#include <grpc++/impl/codegen/rpc_service_method.h>
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/impl/codegen/sync_stream.h>
namespace rpc_agent {

static const char* RPC_method_names[] = {
  "/rpc_agent.RPC/rpc_create_ring",
  "/rpc_agent.RPC/rpc_deploy_microservices",
};

std::unique_ptr< RPC::Stub> RPC::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< RPC::Stub> stub(new RPC::Stub(channel));
  return stub;
}

RPC::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_rpc_create_ring_(RPC_method_names[0], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_rpc_deploy_microservices_(RPC_method_names[1], ::grpc::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status RPC::Stub::rpc_create_ring(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::rpc_agent::Errno* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_rpc_create_ring_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>* RPC::Stub::Asyncrpc_create_ringRaw(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>(channel_.get(), cq, rpcmethod_rpc_create_ring_, context, request);
}

::grpc::Status RPC::Stub::rpc_deploy_microservices(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::rpc_agent::Errno* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_rpc_deploy_microservices_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>* RPC::Stub::Asyncrpc_deploy_microservicesRaw(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>(channel_.get(), cq, rpcmethod_rpc_deploy_microservices_, context, request);
}

RPC::Service::Service() {
  (void)RPC_method_names;
  AddMethod(new ::grpc::RpcServiceMethod(
      RPC_method_names[0],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< RPC::Service, ::rpc_agent::CreateRingRequest, ::rpc_agent::Errno>(
          std::mem_fn(&RPC::Service::rpc_create_ring), this)));
  AddMethod(new ::grpc::RpcServiceMethod(
      RPC_method_names[1],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< RPC::Service, ::rpc_agent::DeployConfig, ::rpc_agent::Errno>(
          std::mem_fn(&RPC::Service::rpc_deploy_microservices), this)));
}

RPC::Service::~Service() {
}

::grpc::Status RPC::Service::rpc_create_ring(::grpc::ServerContext* context, const ::rpc_agent::CreateRingRequest* request, ::rpc_agent::Errno* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status RPC::Service::rpc_deploy_microservices(::grpc::ServerContext* context, const ::rpc_agent::DeployConfig* request, ::rpc_agent::Errno* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace rpc_agent

