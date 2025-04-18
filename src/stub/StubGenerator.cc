

#include "stub/StubGenerator.hpp"

#include <unordered_set>

#include "utils/Exception.hpp"
namespace goa {

namespace rpc {
namespace {

void expect(bool result, const char* errMsg) {
  if (!result) throw StubException(errMsg);
}

}  // anonymous namespace

void StubGenerator::parseProto(json::Value& proto) {
  expect(proto.isObject(), "expect object");
  expect(proto.getSize() == 2, "expect 'name' and 'rpc' fields in object");

  // 获取serviceName
  auto nameIter = proto.findMember("name");
  expect(nameIter != proto.endMember(), "missing service name");
  expect(nameIter->value.isString(), "type of service name must be string");
  serviceInfo_.name_ = nameIter->value.getStringView();

  // 获取rpc
  auto rpcIter = proto.findMember("rpc");
  expect(rpcIter != proto.endMember(), "missing service rpc definition");
  expect(rpcIter->value.isArray(), "rpc field must be array");

  size_t n = rpcIter->value.getSize();
  for (size_t i = 0; i < n; ++i) {
    parseRpc(rpcIter->value[i]);
  }
}

// 一个rpc调用 包括name params [returns]
void StubGenerator::parseRpc(json::Value& rpc) {
  expect(rpc.isObject(), "rpc definition must be object");

  // 获取rpc name
  auto nameIter = rpc.findMember("name");
  expect(nameIter != rpc.endMember(), "missing name in rpc definition");
  expect(nameIter->value.isString(), "rpc name must be string");

  auto paramsIter = rpc.findMember("params");
  bool hasParams = paramsIter != rpc.endMember();
  if (hasParams) {
    validateParams(paramsIter->value);
  }

  auto returnsIter = rpc.findMember("returns");
  bool hasReturns = returnsIter != rpc.endMember();
  if (hasReturns) {
    validateReturns(returnsIter->value);
  }

  // 如果没有参数传入那就构造一个Object类型的空Value
  auto paramsValue =
      hasParams ? paramsIter->value : json::Value(json::ValueType::TYPE_OBJECT);

  if (hasReturns) {
    RpcReturn rr(nameIter->value.getString(), paramsValue, returnsIter->value);
    serviceInfo_.rpcReturn_.push_back(rr);
  } else {
    // motify没有return
    RpcNotify rn(nameIter->value.getString(), paramsValue);
    serviceInfo_.rpcNotify_.push_back(rn);
  }
}

void StubGenerator::validateParams(json::Value& params) {
  std::unordered_set<std::string_view> ust;  // 用于判断参数名是否重复

  for (auto& p : params.getObject()) {
    auto key = p.key.getStringView();
    // insert第一个返回值为插入后的迭代器，第二个返回值为bool类型
    // 如果元素已经存在则返回false，成功插入就返回true
    auto unique = ust.insert(key).second;
    expect(unique, "duplicate param name");

    switch (p.value.getType()) {
      case json::ValueType::TYPE_NULL:
        expect(false, "bad param type");
        break;
      default:
        break;
    }
  }
}

// returns不能为null和array
void StubGenerator::validateReturns(json::Value& returns) {
  switch (returns.getType()) {
    case json::ValueType::TYPE_NULL:
    case json::ValueType::TYPE_ARRAY:
      expect(false, "bad returns type");
      break;
    case json::ValueType::TYPE_OBJECT:
      validateReturns(returns);
      break;
    default:
      break;
  }
}

}  // namespace rpc
}  // namespace goa