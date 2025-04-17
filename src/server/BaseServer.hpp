#pragma once
#include <cstddef>

#include "goa-json/include/Value.hpp"
#include "utils/Exception.hpp"
#include "utils/utils.hpp"
namespace goa {
namespace rpc {

// CRTP设计模式 此为基类  派生类声明为基类的模板参数 实现静态多态
template <typename ProtocolServer>
class BaseServer {
 public:
  void setNumThreads(int numThreads) { server_.setNumThread(numThreads); }
  void start() { server_.start(); }

 protected:
  // CRTP常用权限控制  基类不能实例化 因为其依赖于派生类来实现
  BaseServer(EventLoop* loop, const InetAddress& local);
  ~BaseServer() = default;
  json::Value wrapException(RequestException& e);

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn, Buffer& buf);
  void onWriteComplete(const TcpConnectionPtr& conn);
  void onHighWaterMark(const TcpConnectionPtr& conn, size_t mark);

  void handleMessage(const TcpConnectionPtr& conn, Buffer& buf);
  void sendResponse(const TcpConnectionPtr& conn, const json::Value& response);

  ProtocolServer& convert();  // 基类转换为子类
  const ProtocolServer& convert() const;

  TcpServer server_;
};
}  // namespace rpc
}  // namespace goa