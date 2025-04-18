#pragma once

#include <functional>
#include <goa-json/include/Value.hpp>
#include <unordered_map>

#include "goa-ev/src/Buffer.hpp"
#include "goa-ev/src/Callbacks.hpp"
#include "utils/utils.hpp"

namespace goa {

namespace rpc {

using ResponseCallback =
    std::function<void(const json::Value& json, bool isError, bool isTimeout)>;

class BaseClient : noncopyable {
 public:
  BaseClient(EventLoop* loop, const InetAddress& serverAddr);

  void start();

  void setConnectionCallback(const ConnectionCallback& callback);

  void sendCall(const TcpConnectionPtr& conn, json::Value& call,
                const ResponseCallback& callback);

  void sendNotify(const TcpConnectionPtr& conn, json::Value& notify);

 private:
  void onMessage(const TcpConnectionPtr& conn, Buffer& buf);
  void handleMessage(Buffer& buf);
  void handleResponse(std::string& json);
  void handleSingleResponse(json::Value& response);
  void validateResponse(json::Value& response);
  void sendRequest(const TcpConnectionPtr& conn, json::Value& request);

 private:
  using Callbacks = std::unordered_map<int64_t, ResponseCallback>;
  int64_t id_;
  Callbacks callbacks_;  // request得到response后，执行id对应的callback
  TcpClient client_;
};

}  // namespace rpc
}  // namespace goa