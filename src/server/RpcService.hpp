#pragma once

#include <cassert>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "goa-json/include/Value.hpp"
#include "server/Procedure.hpp"
#include "utils/Exception.hpp"
#include "utils/RpcError.hpp"
#include "utils/utils.hpp"

namespace goa {
namespace rpc {

class RpcService : noncopyable {
 public:
  void addProcedureReturn(std::string_view methodName, ProcedureReturn* p) {
    assert(procedureReturnList_.find(methodName) == procedureReturnList_.end());
    procedureReturnList_.insert(
        {methodName, std::unique_ptr<ProcedureReturn>(p)});
  }

  void addProcedureNotify(std::string_view methodName, ProcedureNotify* p) {
    assert(procedureNotifyList_.find(methodName) == procedureNotifyList_.end());
    procedureNotifyList_.insert(
        {methodName, std::unique_ptr<ProcedureNotify>(p)});
  }

  void callProcedureReturn(std::string_view methodName, json::Value& request,
                           const RpcDoneCallback& done) {
    auto it = procedureReturnList_.find(methodName);
    if (it == procedureReturnList_.end()) {
      throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND),
                             request["id"], "method not found");
    }
    it->second->invoke(request, done);
  }

  // notify无需callback，无返回
  void callProcedureNotify(std::string_view methodName, json::Value& request) {
    auto it = procedureNotifyList_.find(methodName);
    if (it == procedureNotifyList_.end()) {
      throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND),
                            "method not found");  // 同上
    }
    it->second->invoke(request);
  }

 private:
  using ProcedureReturnPtr = std::unique_ptr<ProcedureReturn>;
  using ProcedureNotifyPtr = std::unique_ptr<ProcedureNotify>;
  using ProcedureReturnList =
      std::unordered_map<std::string_view, ProcedureReturnPtr>;
  using ProcedureNotifyList =
      std::unordered_map<std::string_view, ProcedureNotifyPtr>;

  ProcedureReturnList procedureReturnList_;
  ProcedureNotifyList procedureNotifyList_;
};

}  // namespace rpc
}  // namespace goa
