#pragma once

#include <goa-ev/src/EventLoop.hpp>
#include <goa-json/include/Value.hpp>

#include "noncopyable.hpp"

namespace goa {

namespace rpc {

using ev::EventLoop;
using ev::noncopyable;
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