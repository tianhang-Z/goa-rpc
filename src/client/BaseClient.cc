#include "client/BaseClient.hpp"

#include <cstddef>
#include <functional>
#include <string>

#include "goa-json/include/Document.hpp"
#include "goa-json/include/Exception.hpp"
#include "goa-json/include/StringWriteStream.hpp"
#include "goa-json/include/Writer.hpp"
#include "utils/Exception.hpp"
#include "utils/RpcError.hpp"

namespace goa {

namespace rpc {

namespace {

const size_t kMaxMessageLen = 65536;

json::Value& findValue(json::Value& value, const char* key,
                       json::ValueType type) {
  auto it = value.findMember(key);
  if (it == value.endMember()) {
    throw ResponseException("missing field");
  }
  if (it->value.getType() != type) {
    throw ResponseException("bad type");
  }
  return it->value;
}

// 给responseException包装上id
json::Value& findValue(json::Value& value, const char* key,
                       json::ValueType type, int32_t id) {
  try {
    return findValue(value, key, type);
  } catch (ResponseException& e) {
    throw ResponseException(e.what(), id);
  }
}
}  // anonymous namespace

BaseClient::BaseClient(EventLoop* loop, const InetAddress& serverAddr)
    : id_(0), client_(loop, serverAddr) {
  client_.setMessageCallback(std::bind(&BaseClient::onMessage, this, _1, _2));
}

void BaseClient::start() { client_.start(); }

void BaseClient::setConnectionCallback(const ConnectionCallback& callback) {
  client_.setConnectionCallback(callback);
}

//  带回调处理函数的request发送
void BaseClient::sendCall(const TcpConnectionPtr& conn, json::Value& call,
                          const ResponseCallback& callback) {
  call.addMember("id", id_);
  callbacks_[id_] = callback;
  id_++;
  sendRequest(conn, call);
}

void BaseClient::sendNotify(const TcpConnectionPtr& conn, json::Value& notify) {
  sendRequest(conn, notify);
}

void BaseClient::sendRequest(const TcpConnectionPtr& conn,
                             json::Value& request) {
  json::StringWriteStream os;
  json::Writer writer(os);
  request.writeTo(writer);  // json格式的request 序列化为string

  /* message有header和body两部分组成
header: body的长度
body: response + crlf分隔符

内存分布：header + "\r\n" + body + "\r\n"
*/
  auto message = std::to_string(os.getStringView().length() + 2)
                     .append("\r\n")
                     .append(os.getStringView())
                     .append("\r\n");
  conn->send(message);
}

// 处理收到的response
void BaseClient::onMessage(const TcpConnectionPtr& conn, Buffer& buf) {
  try {
    handleMessage(buf);
  } catch (ResponseException& e) {
    // 如果handleMessage发生异常，那么就抛弃本次请求
    // 并清理对callback的存档，退化为notify
    std::string errMsg;
    errMsg.append("response error: ")
        .append(e.what())
        .append(", and this request will be abandoned.");

    if (e.hasId()) {
      errMsg += " id:" + std::to_string(e.id());
      callbacks_.erase(e.id());
    }
    ERROR(errMsg);
  }
}

void BaseClient::handleMessage(Buffer& buf) {
  while (true) {
    auto crlf = buf.findCRLF();
    if (crlf == nullptr) break;

    size_t headerLen =
        static_cast<size_t>(crlf - buf.peek() + 2);  // bodyLength + "\r\n"
    json::Document header;
    auto err = header.parse(buf.peek(), headerLen);  // 反序列化header
    if (err != json::ParseError::PARSE_OK || !header.isInt32() ||
        header.getInt32() <= 0) {
      buf.retrieve(headerLen);
      throw ResponseException("invalid message length in header");
    }

    auto bodyLen = static_cast<uint32_t>(header.getInt32());
    if (bodyLen >= kMaxMessageLen) {
      throw ResponseException("message is too long");
    }

    if (buf.readableBytes() < headerLen + bodyLen)
      break;  // message实际长度小于header中记录的长度，直接丢弃，做报废处理
    buf.retrieve(headerLen);
    auto json = buf.retrieveAsString(bodyLen);
    handleResponse(json);  // body交由下层继续处理，这里只负责拆包逻辑
  }
}

void BaseClient::handleResponse(std::string& json) {
  json::Document response;  //反序列化body
  auto err = response.parse(json);
  if (err != json::ParseError::PARSE_OK) {
    throw ResponseException(json::parseErrorString(err));
  }

  switch (response.getType()) {
      // single response
    case json::ValueType::TYPE_OBJECT:
      handleSingleResponse(response);
      break;
    // batch response
    case json::ValueType::TYPE_ARRAY: {
      size_t n = response.getSize();
      if (n == 0) {
        throw ResponseException("batch response is empty");
      }
      for (size_t i = 0; i < n; ++i) {
        handleSingleResponse(response[i]);
      }
      break;
    }
    default:
      throw ResponseException("response should be json object or array");
  }
}

void BaseClient::handleSingleResponse(json::Value& response) {
  validateResponse(response);
  auto id = response["id"].getInt32();

  auto it = callbacks_.find(id);
  if (it == callbacks_.end()) {
    WARN("response {} not found in stub", id);
    return;
  }

  // response中应该有result字段或者error字段
  auto result = response.findMember("result");
  if (result != response.endMember()) {
    it->second(result->value, false,
               false);  // 后两个bool标志位 isError, isTimeout
  }  // 每个request正确被response后 执行每个id对应的回调
  else {
    auto error = response.findMember("error");
    assert(error != response.endMember());
    if (error != response.endMember()) {
      it->second(
          error->value, true,
          false);  // 对于release版本，实在是错误，那么就抛弃此response，request退化为notify
    } else {
      ERROR("response error, this response will be abandoned, id: {}", id);
    }
  }

  callbacks_.erase(it);
}

// 检查response的字段是否都合法且符合预期
// jsonrpc, error/result, id
void BaseClient::validateResponse(json::Value& response) {
  if (response.getSize() != 3) {
    throw ResponseException(
        "response should have exactly 3 fields: (jsonrpc, error/result, id)");
  }

  auto id = findValue(response, "id", json::ValueType::TYPE_INT32).getInt32();

  auto version =
      findValue(response, "jsonrpc", json::ValueType::TYPE_STRING, id)
          .getStringView();

  if (version != "2.0") {
    throw ResponseException("unknown json rpc version", id);
  }

  if (response.findMember("result") != response.endMember()) return;

  findValue(response, "error", json::ValueType::TYPE_OBJECT, id);
}

}  // namespace rpc
}  // namespace goa
