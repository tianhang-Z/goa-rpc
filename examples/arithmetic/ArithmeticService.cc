#include "examples/arithmetic/ArithmeticServiceStub.hpp"
#include "goa-ev/src/Logger.hpp"

using namespace goa::rpc;

class ArithmeticService : public ArithmeticServiceStub<ArithmeticService> {
 public:
  explicit ArithmeticService(RpcServer& server)
      : ArithmeticServiceStub(server), pool_(4) {}

  void Add(double lhs, double rhs, const UserDoneCallback& callback) {
    pool_.runTask(
        [=]() { callback(goa::json::Value(static_cast<double>(lhs + rhs))); });
  }

  void Sub(double lhs, double rhs, const UserDoneCallback& callback) {
    pool_.runTask(
        [=]() { callback(goa::json::Value(static_cast<double>(lhs - rhs))); });
  }

  void Mul(double lhs, double rhs, const UserDoneCallback& callback) {
    pool_.runTask(
        [=]() { callback(goa::json::Value(static_cast<double>(lhs * rhs))); });
  }

  void Div(double lhs, double rhs, const UserDoneCallback& callback) {
    pool_.runTask(
        [=]() { callback(goa::json::Value(static_cast<double>(lhs / rhs))); });
  }

 private:
  ThreadPool pool_;
};  // ArithmeticServer

int main() {
  goa::ev::setLogLevel(goa::ev::LOG_LEVEL::LOG_LEVEL_DEBUG);
  std::string log_level = std::string(
      goa::ev::logLevelStr[static_cast<unsigned>(goa::ev::logLevel)]);
  // 使用c20格式化方法
  INFO("log level :{}, {}", static_cast<unsigned>(goa::ev::logLevel),log_level.c_str())

  EventLoop loop;
  InetAddress addr(9877);

  RpcServer rpcServer(&loop, addr);
  ArithmeticService service(rpcServer);

  rpcServer.start();
  loop.loop();
}