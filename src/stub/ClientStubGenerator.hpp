#pragma once

#include "goa-json/include/Value.hpp"
#include "stub/StubGenerator.hpp"

namespace goa {
namespace rpc {

class ClientStubGenerator : public StubGenerator {
 public:
  explicit ClientStubGenerator(json::Value& proto) : StubGenerator(proto) {}

  std::string genStub() override;
  std::string genStubClassName() override;

 private:
  std::string genMacroName();
  std::string genProcedureDefinitions();
  std::string genNotifyDefinitions();

  template <typename Rpc>
  std::string genGenericArgs(const Rpc& r, bool appendCommand);
  template <typename Rpc>
  std::string genGenericParamMembers(const Rpc& r);
};
}  // namespace rpc
}  // namespace goa