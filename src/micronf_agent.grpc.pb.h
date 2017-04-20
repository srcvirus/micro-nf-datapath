// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: micronf_agent.proto
#ifndef GRPC_micronf_5fagent_2eproto__INCLUDED
#define GRPC_micronf_5fagent_2eproto__INCLUDED

#include "micronf_agent.pb.h"

#include <grpc++/impl/codegen/async_stream.h>
#include <grpc++/impl/codegen/async_unary_call.h>
#include <grpc++/impl/codegen/method_handler_impl.h>
#include <grpc++/impl/codegen/proto_utils.h>
#include <grpc++/impl/codegen/rpc_method.h>
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/impl/codegen/status.h>
#include <grpc++/impl/codegen/stub_options.h>
#include <grpc++/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class RpcService;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace rpc_agent {

class RPC final {
 public:
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status rpc_create_ring(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::rpc_agent::Errno* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>> Asyncrpc_create_ring(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>>(Asyncrpc_create_ringRaw(context, request, cq));
    }
    virtual ::grpc::Status rpc_deploy_microservices(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::rpc_agent::Errno* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>> Asyncrpc_deploy_microservices(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>>(Asyncrpc_deploy_microservicesRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>* Asyncrpc_create_ringRaw(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rpc_agent::Errno>* Asyncrpc_deploy_microservicesRaw(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status rpc_create_ring(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::rpc_agent::Errno* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>> Asyncrpc_create_ring(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>>(Asyncrpc_create_ringRaw(context, request, cq));
    }
    ::grpc::Status rpc_deploy_microservices(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::rpc_agent::Errno* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>> Asyncrpc_deploy_microservices(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>>(Asyncrpc_deploy_microservicesRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>* Asyncrpc_create_ringRaw(::grpc::ClientContext* context, const ::rpc_agent::CreateRingRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::rpc_agent::Errno>* Asyncrpc_deploy_microservicesRaw(::grpc::ClientContext* context, const ::rpc_agent::DeployConfig& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::RpcMethod rpcmethod_rpc_create_ring_;
    const ::grpc::RpcMethod rpcmethod_rpc_deploy_microservices_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status rpc_create_ring(::grpc::ServerContext* context, const ::rpc_agent::CreateRingRequest* request, ::rpc_agent::Errno* response);
    virtual ::grpc::Status rpc_deploy_microservices(::grpc::ServerContext* context, const ::rpc_agent::DeployConfig* request, ::rpc_agent::Errno* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_rpc_create_ring : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_rpc_create_ring() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_rpc_create_ring() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status rpc_create_ring(::grpc::ServerContext* context, const ::rpc_agent::CreateRingRequest* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void Requestrpc_create_ring(::grpc::ServerContext* context, ::rpc_agent::CreateRingRequest* request, ::grpc::ServerAsyncResponseWriter< ::rpc_agent::Errno>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_rpc_deploy_microservices : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_rpc_deploy_microservices() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_rpc_deploy_microservices() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status rpc_deploy_microservices(::grpc::ServerContext* context, const ::rpc_agent::DeployConfig* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void Requestrpc_deploy_microservices(::grpc::ServerContext* context, ::rpc_agent::DeployConfig* request, ::grpc::ServerAsyncResponseWriter< ::rpc_agent::Errno>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_rpc_create_ring<WithAsyncMethod_rpc_deploy_microservices<Service > > AsyncService;
  template <class BaseClass>
  class WithGenericMethod_rpc_create_ring : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_rpc_create_ring() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_rpc_create_ring() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status rpc_create_ring(::grpc::ServerContext* context, const ::rpc_agent::CreateRingRequest* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_rpc_deploy_microservices : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_rpc_deploy_microservices() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_rpc_deploy_microservices() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status rpc_deploy_microservices(::grpc::ServerContext* context, const ::rpc_agent::DeployConfig* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_rpc_create_ring : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_rpc_create_ring() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::StreamedUnaryHandler< ::rpc_agent::CreateRingRequest, ::rpc_agent::Errno>(std::bind(&WithStreamedUnaryMethod_rpc_create_ring<BaseClass>::Streamedrpc_create_ring, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_rpc_create_ring() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status rpc_create_ring(::grpc::ServerContext* context, const ::rpc_agent::CreateRingRequest* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status Streamedrpc_create_ring(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::rpc_agent::CreateRingRequest,::rpc_agent::Errno>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_rpc_deploy_microservices : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_rpc_deploy_microservices() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::StreamedUnaryHandler< ::rpc_agent::DeployConfig, ::rpc_agent::Errno>(std::bind(&WithStreamedUnaryMethod_rpc_deploy_microservices<BaseClass>::Streamedrpc_deploy_microservices, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_rpc_deploy_microservices() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status rpc_deploy_microservices(::grpc::ServerContext* context, const ::rpc_agent::DeployConfig* request, ::rpc_agent::Errno* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status Streamedrpc_deploy_microservices(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::rpc_agent::DeployConfig,::rpc_agent::Errno>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_rpc_create_ring<WithStreamedUnaryMethod_rpc_deploy_microservices<Service > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_rpc_create_ring<WithStreamedUnaryMethod_rpc_deploy_microservices<Service > > StreamedService;
};

}  // namespace rpc_agent


#endif  // GRPC_micronf_5fagent_2eproto__INCLUDED
