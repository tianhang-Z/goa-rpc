#pragma once
#include <memory>
#include <unordered_map>

#include "goa-json/include/Value.hpp"
#include "server/BaseServer.hpp"
#include "server/RpcService.hpp"
#include "utils/utils.hpp"

namespace goa {

namespace rpc {

// RpcServer管理RpcService，RpcService管理Procedure

class RpcServer : public BaseServer<RpcServer> {
 public:
  RpcServer(EventLoop* loop, const InetAddress& local)
      : BaseServer(loop, local) {}

  ~RpcServer() = default;

  void addService(std::string_view serviceName, RpcService* service);

  // 通过BaseServer 将其加入onMessage 并设置为server的回调
  // 最终设置为ev::channel的回调 在有可读信号时被调用
  void handleRequest(const std::string& json, const RpcDoneCallback& done);

 private:
  void handleSingleRequest(json::Value& request, const RpcDoneCallback& done);
  void handleBatchRequests(json::Value& request, const RpcDoneCallback& done);
  void handleSingleNotify(json::Value& request);

  void validateRequest(json::Value& request);
  void validateNotify(json::Value& request);

  using RpcServicePtr = std::unique_ptr<RpcService>;
  using ServiceList = std::unordered_map<std::string_view, RpcServicePtr>;
  ServiceList services_;
};

}  // namespace rpc
}  // namespace goa
