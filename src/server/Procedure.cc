#include <cstddef>
#include <server/Procedure.hpp>

#include "goa-json/include/Value.hpp"
#include "utils/Exception.hpp"
#include "utils/RpcError.hpp"
#include "utils/utils.hpp"

namespace goa {
namespace rpc {

template class Procedure<ProcedureReturnCallback>;
template class Procedure<ProcedureNotifyCallback>;

// 分别实现两个特化的模板
template <>
void Procedure<ProcedureReturnCallback>::validateRequest(
    goa::json::Value& request) const {
  switch (request.getType()) {
    case goa::json::ValueType::TYPE_OBJECT:
    case goa::json::ValueType::TYPE_ARRAY:
      if (!validateGeneric(request)) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_PARAMS),
                               request["id"], "pramas name or type mismatch");
      }
      break;
    default:
      throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST),
                             request["id"], "params must be object or array");
  }
}

template <>
void Procedure<ProcedureNotifyCallback>::validateRequest(
    json::Value& request) const {
  switch (request.getType()) {
    case goa::json::ValueType::TYPE_OBJECT:
    case goa::json::ValueType::TYPE_ARRAY:
      if (!validateGeneric(request))
        throw NotifyException(RpcError(ERROR::RPC_INVALID_PARAMS),
                              "params name or type mismatch");
      break;
    default:
      throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST),
                            "params type must be object or array");
  }
}

// 比较json::Value和procedure的params_是否一致
template <typename Func>
bool Procedure<Func>::validateGeneric(goa::json::Value& request) const {
  auto mIter = request.findMember("params");
  if (mIter == request.endMember()) {
    return params_.empty();
  }
  auto& params = mIter->value;
  // 有"params"这个key时，不能值为空
  if (params.getSize() == 0 || params.getSize() != params_.size()) {
    return false;
  }

  switch (params.getType()) {
    case json::ValueType::TYPE_ARRAY:
      for (size_t i = 0; i < params_.size(); i++) {
        if (params[i].getType() != params_[i].paramType) {
          return false;
        }
      }
      break;

    case json::ValueType::TYPE_OBJECT:
      for (auto& p : params_) {
        auto it = params.findMember(p.paramName);
        if (it == params.endMember()) return false;
        if (it->value.getType() != p.paramType) return false;
      }
      break;
    default:
      return false;
  }
  return true;
}

template <>
void Procedure<ProcedureReturnCallback>::invoke(json::Value& request,
                                                const RpcDoneCallback& done) {
  validateRequest(request);
  callback_(request, done);
}

template <>
void Procedure<ProcedureNotifyCallback>::invoke(json::Value& request) {
  validateRequest(request);
  callback_(request);
}

}  // namespace rpc
}  // namespace goa
