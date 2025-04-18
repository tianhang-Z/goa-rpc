#pragma once

#include <goa-ev/src/Buffer.hpp>
#include <goa-ev/src/Callbacks.hpp>
#include <goa-ev/src/CountDownLatch.hpp>
#include <goa-ev/src/EventLoop.hpp>
#include <goa-ev/src/InetAddress.hpp>
#include <goa-ev/src/Logger.hpp>
#include <goa-ev/src/TcpClient.hpp>
#include <goa-ev/src/TcpConnection.hpp>
#include <goa-ev/src/TcpServer.hpp>
#include <goa-ev/src/ThreadPool.hpp>
#include <goa-ev/src/Timestamp.hpp>
#include <goa-json/include/Value.hpp>

namespace goa {

namespace rpc {

using namespace std::literals::string_view_literals;
using namespace std::literals::chrono_literals;

using ev::Buffer;
using ev::ConnectionCallback;
using ev::CountDownLatch;
using ev::EventLoop;
using ev::InetAddress;
using ev::noncopyable;
using ev::TcpClient;
using ev::TcpConnection;
using ev::TcpConnectionPtr;
using ev::TcpServer;
using ev::ThreadPool;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

using RpcDoneCallback = std::function<void(json::Value response)>;

class UserDoneCallback {
 public:
  UserDoneCallback(json::Value &request, const RpcDoneCallback &callback)
      : request_(request), callback_(callback) {}

  void operator()(json::Value &&result) const {
    json::Value response(json::ValueType::TYPE_OBJECT);
    response.addMember("jsonrpc", "2.0");
    response.addMember("id", request_["id"]);
    response.addMember("result", result);
    callback_(response);
  }

 private:
  mutable json::Value request_;
  RpcDoneCallback callback_;
};

}  // namespace rpc
}  // namespace goa