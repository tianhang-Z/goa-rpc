#include <iostream>
#include <random>

#include "examples/arithmetic/ArithmeticClientStub.hpp"
#include "goa-ev/src/Logger.hpp"

using namespace goa::rpc;

void run(ArithmeticClientStub& client) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution dis(1, 100);

  double lhs = static_cast<double>(dis(gen));
  double rhs = static_cast<double>(dis(gen));

  client.Add(
      lhs, rhs, [=](goa::json::Value response, bool isError, bool timeout) {
        if (!isError) {
          std::cout << lhs << "+" << rhs << "=" << response.getDouble()
                    << std::endl;
        } else if (timeout) {
          std::cout << "timeout" << std::endl;
        } else {  // isError && !timeout
          std::cout << "response: " << response["message"].getStringView()
                    << ": " << response["data"].getStringView() << std::endl;
        }
      });

  client.Sub(lhs, rhs,
             [=](const goa::json::Value& response, bool isError, bool timeout) {
               if (!isError) {
                 if (response.getType() != goa::json::ValueType::TYPE_DOUBLE)
                   std::cout << static_cast<unsigned int>(response.getType())
                             << response.getInt32() << std::endl;
                 std::cout << lhs << "-" << rhs << "=" << response.getDouble()
                           << std::endl;
               } else if (timeout) {
                 std::cout << "timeout" << std::endl;
               } else {  // isError && !timeout
                 std::cout << "response: "
                           << response["message"].getStringView() << ": "
                           << response["data"].getStringView() << std::endl;
               }
             });

  client.Mul(lhs, rhs,
             [=](const goa::json::Value& response, bool isError, bool timeout) {
               if (!isError) {
                 if (response.getType() != goa::json::ValueType::TYPE_DOUBLE)
                   std::cout << static_cast<unsigned int>(response.getType())
                             << std::endl;
                 std::cout << lhs << "*" << rhs << "=" << response.getDouble()
                           << std::endl;
               } else if (timeout) {
                 std::cout << "timeout" << std::endl;
               } else {  // isError && !timeout
                 std::cout << "response: "
                           << response["message"].getStringView() << ": "
                           << response["data"].getStringView() << std::endl;
               }
             });

  client.Div(lhs, rhs,
             [=](const goa::json::Value& response, bool isError, bool timeout) {
               if (!isError) {
                 std::cout << lhs << "/" << rhs << "=" << response.getDouble()
                           << std::endl;
               } else if (timeout) {
                 std::cout << "timeout" << std::endl;
               } else {  // isError && !timeout
                 std::cout << "response: "
                           << response["message"].getStringView() << ": "
                           << response["data"].getStringView() << std::endl;
               }
             });
}

int main() {
  goa::ev::setLogLevel(goa::ev::LOG_LEVEL::LOG_LEVEL_DEBUG);
  std::string log_level = std::string(goa::ev::logLevelStr[static_cast<unsigned>(goa::ev::logLevel)]);
  INFO("log level :{}, {}", static_cast<unsigned>(goa::ev::logLevel),
       log_level.c_str())

  
  EventLoop loop;
  InetAddress addr(9877);
  ArithmeticClientStub client(&loop, addr);

  client.setConnectionCallback([&](const TcpConnectionPtr& conn) {
    if (conn->disconnected()) {
      loop.quit();
    } else {
      loop.runEvery(1s, [&] { run(client); });
    }
  });
  client.start();
  loop.loop();
}