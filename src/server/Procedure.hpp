#pragma once

#include <string_view>

#include "goa-json/include/Value.hpp"
#include "utils/utils.hpp"
namespace goa {
namespace rpc {

using ProcedureReturnCallback =
    std::function<void(goa::json::Value&, const RpcDoneCallback&)>;
using ProcedureNotifyCallback = std::function<void(goa::json::Value&)>;

// procedure有两个特化的实现 ProcedureReturn 和 ProcedureNotify
template <typename Func>
class Procedure : noncopyable {
 public:
  template <typename... ParamNameAndTypes>
  explicit Procedure(Func&& callback, ParamNameAndTypes&&... nameAndTypes)
      : callback_(std::forward<Func>(callback)) {
    constexpr int n = sizeof...(nameAndTypes);
    static_assert(n % 2 == 0, "Procedure must have param name and type pairs");

    if constexpr (n > 0) {
      initProcedure(nameAndTypes...);
    }
  }

  // 使用时只需要调用invoke  函数内部校验了参数并执行了回调
  void invoke(goa::json::Value& request, const RpcDoneCallback& done);

  void invoke(goa::json::Value& request);

 private:
  template <typename Name, typename... ParamNameAndType>
  void initProcedure(Name paramName, goa::json::ValueType paramType,
                     ParamNameAndType&&... nameAndType) {
    static_assert(std::is_same_v<Name, const char(&)[]> ||
                      std::is_same_v<Name, const char*> ||
                      std::is_same_v<Name, std::string> ||
                      std::is_same_v<Name, std::string_view>,
                  "wrong type with name");
    params_.emplace_back(paramName, paramType);
    if constexpr (sizeof...(nameAndType) > 0)
      initProcedure(nameAndType...);  // 递归解析参数列表
  }

  void validateRequest(goa::json::Value& request) const;
  bool validateGeneric(goa::json::Value& request) const;

  struct Param {
    Param(std::string_view paramName_, goa::json::ValueType paramTypem_)
        : paramName(paramName_), paramType(paramTypem_) {}

    std::string_view paramName;
    goa::json::ValueType paramType;
  };

  Func callback_;
  std::vector<Param> params_;
};

using ProcedureReturn = Procedure<ProcedureReturnCallback>;
using ProcedureNotify = Procedure<ProcedureNotifyCallback>;

}  // namespace rpc
}  // namespace goa
