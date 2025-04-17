#include "server/RpcServer.hpp"
#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string_view>
#include "goa-json/include/Document.hpp"
#include "goa-json/include/Exception.hpp"
#include "goa-json/include/Value.hpp"
#include "utils/Exception.hpp"
#include "utils/RpcError.hpp"
#include "utils/utils.hpp"

namespace goa {

namespace rpc {


namespace  {

// 检测type是否和模板参数之一匹配
template <json::ValueType dst, json::ValueType... rest>
void checkValueType(json::ValueType type) {
  if (dst == type) return;
  if constexpr (sizeof...(rest) > 0) {
    checkValueType<rest...>(type);
  } else {
    throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),"bad type , must in field");
  }
}

// 加入id
template <json::ValueType... types>
void checkValueType(json::ValueType type, json::Value& id) {
  try {
    checkValueType<types...>(type);
  } catch (RequestException& e) {
    throw RequestException(e.err(),id,e.detail());
  }
}


template <json::ValueType... types>
json::Value& findValue(json::Value& request, const char* key) {
  static_assert(sizeof...(types) > 0, "type field must not be empty");

  auto it = request.findMember(key);
  if (it == request.endMember()) {
    throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                           "missing at least one field");
  }
  checkValueType<types...>(it->value.getType());
  return it->value;
}


// 加入 id
template <json::ValueType... types>
json::Value& findValue(json::Value& request, json::Value& id, const char* key) {
  try {
    return findValue<types...>(request, key);
  } catch (RequestException& e) {
    throw RequestException(e.err(), id, e.detail());
  }
}

bool hasParams(const json::Value& request) {
  return request.findMember("params") !=request.endMember();
}

// 判断是否为一个notify请求，notify没有id，json-rpc 2.0协议
bool isNotify(const json::Value& request) {
  return request.findMember("id") == request.endMember();
}

// 用于handleBatchRequest 将response线程安全的放在一起
class ThreadSafeBatchResponse {
 public:
  explicit ThreadSafeBatchResponse(const RpcDoneCallback& done)
      : data_(std::make_shared<ThreadSafeDate>(done)) {}

  void addResponse(const json::Value& response) {
    std::lock_guard lock(data_->mutex_);
    data_->response_.addValue(response);
  }


private:
 struct ThreadSafeDate {
   explicit ThreadSafeDate(const RpcDoneCallback& done)
       : response_(json::ValueType::TYPE_ARRAY), done_(done) {}

   ~ThreadSafeDate() { done_(response_); }
   
    std::mutex mutex_;
   json::Value response_;
   RpcDoneCallback done_;
 };

 using DataPtr = std::shared_ptr<ThreadSafeDate>;
 DataPtr data_;
};

}  // anonymous namespace

void RpcServer::addService(std::string_view serviceName, RpcService* service) {
  assert(services_.find(serviceName) == services_.end());
  services_.insert({serviceName,std::unique_ptr<RpcService>(service)});
}

// 通过BaseServer 将其加入onMessage 并设置为server的回调
// 最终设置为ev::channel的回调 在有可读信号时被调用
void RpcServer::handleRequest(
    const std::string& json,
    const RpcDoneCallback& done) {
json::Document request;
json::ParseError err = request.parse(json);
if (err != json::ParseError::PARSE_OK) {
    throw RequestException(
        RpcError(ERROR::RPC_PARSE_ERROR),
        json::parseErrorString(err));
}
switch (request.getType()) {
    case json::ValueType::TYPE_OBJECT:
    if (isNotify(request)) {
        handleSingleNotify(request);
    } else {
      handleSingleRequest(request, done);
    }
    break;
    case json::ValueType::TYPE_ARRAY:
      handleBatchRequests(request, done);
      break;
    default:
    throw RequestException(
        RpcError(ERROR::RPC_INVALID_REQUEST),
        "request should be json object or array");
}
}

void RpcServer::handleSingleRequest(json::Value& request,
                                    const RpcDoneCallback& done) {
  validateRequest(request);

  auto& id = request["id"];
  auto methodName = request["method"].getStringView();
  auto pos = methodName.find('.');
  // 格式为"method":"serviceName.methodName"
  if (pos == std::string_view::npos || pos == 0) {
    throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id,
                           "missing service name in method");
  }

  auto serviceName = methodName.substr(0, pos);
  auto it = services_.find(serviceName);
  if (it == services_.end()) {
    throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id,
                           "service not found");
  }

  methodName.remove_prefix(pos + 1);  // 移除service name和'.'
  if (methodName.length() == 0) {
    throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id,
                           "missing method name in method field");
  }

  auto& service = it->second;
  service->callProcedureReturn(methodName, request, done);
}


void RpcServer::handleBatchRequests(json::Value& requests,
                                    const RpcDoneCallback& done) {
  size_t num = requests.getSize();
  if (num == 0) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "batch request is empty");
  }

  // 可能存在竞态的点，因此用线程安全的自定义数据类型来存储结果responses的集合
  ThreadSafeBatchResponse responses(
      done);
  try {
    size_t n = requests.getSize();
    for (size_t i = 0; i < n; ++i) {
      auto& request = requests[i];

      if (!request.isObject()) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                               "request should be json object");
      }
      if (isNotify(request)) {
        handleSingleNotify(request);
      } else {
        // done为lambda函数，执行handleSingleRequest调用完method之后，通过done将结果response添加进结果集当中
        // 当所有的request都执行完后，在responses析构时，再调用handleBatchRequests函数的done来处理结果集
       // 线程安全，由于method调用时存在静态，结果集responses为临界区
        handleSingleRequest(request, [&](json::Value response) {
          responses.addResponse(response);
        });
      }
    }
  }
  catch (RequestException& e) {
    // 失败信息也加入结果集
  auto response = wrapException(e);
  responses.addResponse(response);
  }
  catch (NotifyException& e) {
    // notify失败是无需给用户返回信息的，因此notify成功与否，用户都应该能接受其结果，用户逻辑不应依赖于notify的成功
    WARN("notify error, code:{}, message:{}, data:{}", e.err().asCode(),
         e.err().asString(), e.detail());
    }
}

    
void RpcServer::handleSingleNotify(json::Value& request) {
  validateNotify(request);

  // 找到匹配的service.method
  auto methodName = request["method"].getStringView();
  auto pos = methodName.find(".");
  if (pos == std::string_view::npos || pos == 0) {
    throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST),
                          "missing service name in method field");
  }

  auto serviceName = methodName.substr(0, pos);
  auto it = services_.find(serviceName);
  if (it == services_.end()) {
    throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND),
                          "service not found");
  }

  methodName.remove_prefix(pos + 1);
  if (methodName.size() == 0) {
    throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST),
                          "missing method name in method field");
    auto& service = it->second;
    service->callProcedureNotify(methodName, request);
  }
    }

    // 确认request合法

void RpcServer::validateRequest(json::Value& request) {
  auto& id =
      findValue<json::ValueType::TYPE_STRING, json::ValueType::TYPE_INT32,
                json::ValueType::TYPE_INT64>(request, "id");

  auto& version =
      findValue<json::ValueType::TYPE_STRING>(request, id, "jsonrpc");
  if (version.getStringView() != "2.0") {
    throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), id,
                           "jsonrpc version must be 2.0");
  }

  auto& method =
      findValue<json::ValueType::TYPE_STRING>(request, id, "method");
  if (method.getStringView() == "rpc.") {
    throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id,
                           "method name is internal use");
  }
  
  size_t nMembers = 3u + hasParams(request);

  if (request.getSize() != nMembers) {
    throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), id,
                           "unexpected field");
  }
}
void RpcServer::validateNotify(json::Value& request) {
  auto& version =
      findValue<json::ValueType::TYPE_STRING>(request, "jsonrpc");
  if (version.getStringView() != "2.0") {
    throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST),
                          "jsonrpc version must be 2.0");
  }

  auto& method =
      findValue<json::ValueType::TYPE_STRING>(request, "method");
  if (method.getStringView() == "rpc.") {
    throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND),
                          "method name is internal use");
  }

  size_t nMembers = 2u + hasParams(request);

  if (request.getSize() != nMembers) {
    throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST),
                          "unexpected field");
  }
    }
}
}