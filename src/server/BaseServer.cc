#include "server/BaseServer.hpp"

#include <cstdint>
#include <functional>
#include <goa-json/include/Document.hpp>
#include <goa-json/include/StringWriteStream.hpp>
#include <goa-json/include/Writer.hpp>

#include "goa-json/include/Exception.hpp"
#include "goa-json/include/Value.hpp"
#include "server/RpcServer.hpp"
#include "utils/Exception.hpp"
#include "utils/RpcError.hpp"
#include "utils/utils.hpp"
namespace goa {
namespace rpc {

namespace {

const size_t kHighWaterMark = 65536;
const size_t kMaxMessageLen = 100 * 1024 * 1024;

}  // anonymous namespace

using std::placeholders::_1;
using std::placeholders::_2;

template class BaseServer<RpcServer>;

template <typename ProtocolServer>
BaseServer<ProtocolServer>::BaseServer(EventLoop* loop,
                                       const InetAddress& local)
    : server_(loop, local) {
  server_.setConnectionCallback(std::bind(&BaseServer::onConnection, this, _1));
  server_.setMessageCallback(std::bind(&BaseServer::onMessage, this, _1, _2));
  server_.setWriteCompleteCallback(
      std::bind(&BaseServer::onWriteComplete, this, _1));
}

template <typename ProtocolServer>
void BaseServer<ProtocolServer>::onConnection(const TcpConnectionPtr& conn) {
  if (conn->connected()) {
    DEBUG("connection {} success", conn->peer().toIpPort());
    conn->setHighWaterMarkCallback(
        std::bind(&BaseServer::onHighWaterMark, this, _1, _2), kHighWaterMark);
  } else {
    DEBUG("connection {} fail", conn->peer().toIpPort());
  }
}

template <typename ProtocolServer>
void BaseServer<ProtocolServer>::onMessage(const TcpConnectionPtr& conn,
                                           Buffer& buf) {
  try {
    handleMessage(conn, buf);
  }
  // 失败则返回错误信息 以json发送给客户端
  catch (RequestException& e) {
    json::Value response = wrapException(e);
    sendResponse(conn, response);
    conn->shutdown();

    WARN("BaseServer::onMessage() {} request error: {}",
         conn->peer().toIpPort(), e.what());
  }
}

template <typename ProtocolServer>
void BaseServer<ProtocolServer>::onHighWaterMark(const TcpConnectionPtr& conn,
                                                 size_t mark) {
  /*
   *如果没有发生write错误，并且还有数据没有写完，说明sockfd_的内核缓冲区已满，只写了一部分进去，
   *此刻将会触发高水位回调，并且注册sockfd_(channel_) 的EPOLLOUT事件，
   *当channel_中有数据出去后，内核缓冲区将有空余空间，
   *此时将之前没写完，暂存在outputBuffer_中的数据继续向sockfd_的内核缓冲区写入
   * 高水位回调, fd会stopRea, 当完成一次write之后再重新startRead
   */

  DEBUG("connection {} high watermark {},stop read", conn->peer().toIpPort(),
        mark);
  conn->stopRead();
}

template <typename ProtocolServer>
void BaseServer<ProtocolServer>::onWriteComplete(const TcpConnectionPtr& conn) {
  DEBUG("connection {} write complete", conn->peer().toIpPort());
  conn->startRead();
}

/* message有header和body两部分组成
header: body的长度
body: response + crlf分隔符

内存分布：header + "\r\n" + body + "\r\n"
服务端和客户端都按这个格式收发信息
*/
// 从buf中获取消息string
template <typename ProtocolServer>
void BaseServer<ProtocolServer>::handleMessage(const TcpConnectionPtr& conn,
                                               Buffer& buf) {
  // 消息体格式为header+body 都以\r\n结尾  具体参考sendRequest()函数
  while (true) {
    const char* crlf = buf.findCRLF();
    if (crlf == nullptr) break;
    if (crlf == buf.peek()) {  //空消息
      buf.retrieve(2);
      break;
    }

    size_t headerLen =
        static_cast<size_t>(crlf - buf.peek() + 2);  // crlf长度为2

    json::Document header;
    auto err = header.parse(buf.peek(), headerLen);

    if (err != json::ParseError::PARSE_OK || !header.isInt32() ||
        header.getInt32() <= 32) {
      throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                             "invalid message header");
    }

    auto jsonLen = static_cast<uint32_t>(header.getInt32());
    if (jsonLen > kMaxMessageLen) {
      throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                             "meaasge is too long");
    }

    if (buf.readableBytes() < headerLen + jsonLen) {
      throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                             "message is incomplete");
    }

    buf.retrieve(headerLen);
    auto json_str = buf.retrieveAsString(jsonLen);
    // 调用子类类型对象中的handleRequest
    convert().handleRequest(json_str, [conn,
                                       this](const json::Value& response) {
      if (!response.isNull()) {
        sendResponse(conn, response);
        TRACE("BaseServer::handleMessage() {} request&&response success",
              conn->peer().toIpPort())
      } else {
        TRACE(
            "BaseServer::handleMessage() {} notify sucess",
            conn->peer()
                .toIpPort());  // notify是没有response的，按协议无需发送应答给客户端
      }
    });
  }
}

/*
错误信息
exception消息体结构
{
    "jsonrpc":"2.0",
    "error": {
        "code":...,
        "message":...,
        "data":...
    },
    "id":...
}
 */
template <typename ProtocolServer>
json::Value BaseServer<ProtocolServer>::wrapException(RequestException& e) {
  json::Value response(json::ValueType::TYPE_OBJECT);
  response.addMember("jsonrpc", "2.0");
  auto& value = response.addMember("error", json::ValueType::TYPE_OBJECT);
  value.addMember("code", e.err().asCode());
  value.addMember("message", e.err().asString());
  value.addMember("data", e.detail());
  response.addMember("id", e.id());
  return response;
}

template <typename ProtocolServer>
void BaseServer<ProtocolServer>::sendResponse(const TcpConnectionPtr& conn,
                                              const json::Value& response) {
  json::StringWriteStream os;
  json::Writer writer(os);

  response.writeTo(writer);  // writer实现了递归解析和处理
                             // message即回复消息, 格式为header+body
  // header为消息长度, body为json格式的回复内容, header和body以\r\n结尾
  auto message = std::to_string(os.getStringView().length() + 2)
                     .append("\r\n")
                     .append(os.getStringView())
                     .append("\r\n");
  conn->send(message);
}

template <typename ProtocolServer>
ProtocolServer& BaseServer<ProtocolServer>::convert() {
  return static_cast<ProtocolServer&>(*this);
}

template <typename ProtocolServer>
const ProtocolServer& BaseServer<ProtocolServer>::convert() const {
  return static_cast<const ProtocolServer&>(*this);
}

}  // namespace rpc
}  // namespace goa