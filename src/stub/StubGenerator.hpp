#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "goa-json/include/Value.hpp"

namespace goa {

namespace rpc {

class StubGenerator {
 public:
  explicit StubGenerator(json::Value& proto) {
    parseProto(proto);  // 解析json 将rpc信息放入serviceInfo_
  }
  virtual ~StubGenerator() = default;

  virtual std::string genStub() = 0;
  virtual std::string genStubClassName() = 0;

 protected:
  struct RpcReturn {
    RpcReturn(const std::string& name, json::Value& params,
              json::Value& returns)
        : name_(name), params_(params), returns_(returns) {}
    std::string name_;
    mutable json::Value params_;
    mutable json::Value returns_;
  };

  struct RpcNotify {
    RpcNotify(const std::string& name, json::Value& params)
        : name_(name), params_(params) {}

    std::string name_;
    mutable json::Value params_;
  };

  struct ServiceInfo {
    std::string name_;
    std::vector<RpcReturn> rpcReturn_;
    std::vector<RpcNotify> rpcNotify_;
  };

  ServiceInfo serviceInfo_;

 private:
  void parseProto(json::Value& proto);
  void parseRpc(json::Value& rpc);
  void validateParams(json::Value& params);
  void validateReturns(json::Value& returns);
};

// 将str中所有的from字符串替换为to字符串
inline void replaceAll(std::string& str, const std::string& from,
                       const std::string& to) {
  while (true) {
    size_t i = str.find(from);
    if (i != std::string::npos)
      str.replace(i, from.size(), to);
    else
      return;
  }
}
}  // namespace rpc
}  // namespace goa