#pragma once

#include <cassert>
#include <cstdint>
namespace goa {

namespace rpc {

#define ERROR_MAP(XX)                              \
  XX(PARSE_ERROR, -32700, "Parse error")           \
  XX(INVALID_REQUEST, -32600, "Invalid request")   \
  XX(METHOD_NOT_FOUND, -32601, "Method not found") \
  XX(INVALID_PARAMS, -32602, "Invalid params")     \
  XX(INTERNAL_ERROR, -32603, "Internal error")

enum class ERROR {
#define GEN_ERROR(e, c, s) PRC_##e,
  ERROR_MAP(GEN_ERROR)
#undef GEN_ERROR
};

class RpcError {
 public:
  explicit RpcError(ERROR err) : err_(err) {}
  explicit RpcError(int32_t code) : err_(fromErrorCode(code)) {}
  const char* asString() const {
    return errorString[static_cast<unsigned int>(err_)];
  }

  int32_t asCode() const { return errorCode[static_cast<unsigned int>(err_)]; }

 private:
  const ERROR err_;
  static ERROR fromErrorCode(int32_t code) {
    switch (code) {
      case -32700:
        return ERROR::RPC_PARSE_ERROR;
      case -32600:
        return ERROR::RPC_INVALID_REQUEST;
      case -32601:
        return ERROR::RPC_METHOD_NOT_FOUND;
      case -32602:
        return ERROR::RPC_INVALID_PARAMS;
      case -32603:
        return ERROR::RPC_INTERNAL_ERROR;
      default:
        assert(false && "bad error code");
    }
  }
  static const int32_t errorCode[];
  static const char* errorString[];
};

inline const int32_t RpcError::errorCode[] = {
#define GEN_ERROR_CODE(e, c, s) c,
    ERROR_MAP(GEN_ERROR_CODE)
#undef GEN_ERROR_CODE
};

inline const char* RpcError::errorString[] = {
#define GEN_ERROR_STRING(e, c, s) s,
    ERROR_MAP(GEN_ERROR_STRING)
#undef GEN_ERROR_STRING
};
}  // namespace rpc

}  // namespace goa