# GOA-RPC : A JSON-RPC 2.0 framework implemented in C++20

## 项目简介

goa-rpc是一款基于C++20开发的使用于Linux的RPC（Remote Procedure Call）框架，使用JSON数据格式作为序列化/反序列化方案，实现了JSON-RPC 2.0协议。客户端支持异步RPC调用，也可以通过线程同步达到同步RPC调用的效果。服务端支持基于线程池的多线程RPC，以提高IO线程的响应速度和处理能力上限。服务采用service.method的命名方式，一个TCP端口可以对外提供多个service，每个service中可以含有多个method。

## 开发环境

| Tool | Version |
| :---- | :------: |
| Ubuntu(WSL2) | 20.04 |
| GCC |  13.1.0 |
| CMake | 3.23.0 |
| Git | 2.25.1 |
| clang-format | 10.0.0 |

## 项目架构

<img src=".\image_uml\architecture.png" style="zoom: 15%;" />

本项目编译生成的文件中包含有一个stub generator，可以根据spec.json中对一组rpc request和response的完整描述，自动生成client端和server端的stub代码。使用时client端（RPC调用方）只需包含生成的ClientStub头文件即可获取到相应的RPC服务，server端（RPC被调用方）除了要包含生成的ServiceStub头文件外，还需实现所提供的RPC服务的具体函数逻辑。

goa-RPC中的JSON序列化与反序列化模块基于[GOA-JSON](https://github.com/tianhang-Z/goa-json)实现，网络模块基于[GOA-EV](https://github.com/tianhang-Z/goa-ev)实现。goa-JSON的测试依赖于googletest和benchmark，除此之外无其他依赖。

## 使用示例

每个rpc服务都由一个spec.json文件来描述，arithmetic服务的spec.json文件如下，其中定义了method name、params list和返回值类型：

```json
{
  "name": "Arithmetic",
  "rpc": [
    {
      "name": "Add",
      "params": {"lhs": 1.0, "rhs": 1.0},
      "returns": 2.0
    },
    {
      "name": "Sub",
      "params": {"lhs": 1.0, "rhs": 1.0},
      "returns": 0.0
    },
    {
      "name": "Mul",
      "params": {"lhs": 2.0, "rhs": 3.0},
      "returns": 6.0
    },
    {
      "name": "Div",
      "params": {"lhs": 6.0, "rhs": 2.0},
      "returns": 3.0
    }
  ]
}
```

使用`goa-rpc-stub`，输入spec.json，将生成client stub和service stub头文件。使用示例如下：

```shell
goa-rpc-stub -i spec.json -o -c -s
```

`-i`参数表示输入json文件路径，`-o`表示以文件格式输出，`-c`和`-s`分别表示生成客户端和服务端的stub头文件，二者都缺省时表示二者都生成。

对生成的代码format一下，方便阅读：

```
clang-format -i ArithmeticClient.hpp ArithmeticService.hpp
```

项目代码仓库中的**examples**文件夹下，有**arithmetic**示例，给出了客户端和服务端的代码。若添加CMake选项`-DCMAKE_BUILD_EXAMPLES=1`，将会编译该示例。由于CMakeLists文件中以写明了若编译examples则自动执行stub generator程序生成客户端和服务端的stub，并将其作为编译依赖，直接可以得到客户端和服务端的可执行程序。

首先启动arithmetic_server程序，服务端打印日志：

```shell
# server log
zth@DESKTOP-79OKEST:/mnt/d/cs_git_rep/goa-rpc/build/bin$ ./arithmetic_server 
20250422 12:09:41.278 [ 2697] [  INFO] log level :1, [ DEBUG] - mnt/d/cs_git_rep/goa-rpc/examples/arithmetic/ArithmeticService.cc:40
20250422 12:09:41.280 [ 2697] [  INFO] create TcpServer 0.0.0.0:9877 - mnt/d/cs_git_rep/goa-rpc/third_party/goa-ev/src/TcpServer.cc:16
20250422 12:09:41.280 [ 2697] [  INFO] TcpServer::start() 0.0.0.0:9877 with 1 eventLoop thread(s) - mnt/d/cs_git_rep/goa-rpc/third_party/goa-ev/src/TcpServer.cc:62
```

随后启动arithmetic_client程序，客户端连接成功后，将随机生成两个double值，并调用RPC的Add、Sub、Mul、Div服务进行加减乘除：

```shell
# client log
./arithmetic_client 
20250422 12:11:35.518 [ 3176] [  INFO] log level :1, [ DEBUG] - mnt/d/cs_git_rep/goa-rpc/examples/arithmetic/ArithmeticClient.cc:82
20250422 12:11:35.518 [ 3176] [  INFO] connected - mnt/d/cs_git_rep/goa-rpc/build/examples/arithmetic/ArithmeticClientStub.hpp:23
93+70=163
93-70=23
93*70=6510
93/70=1.32857
8+10=18
8-10=-2
8/10=0.8

# server log
20250422 12:11:35.518 [ 3152] [  INFO] connection 127.0.0.1:58098 success - mnt/d/cs_git_rep/goa-rpc/src/server/BaseServer.cc:44
```

随后关闭客户端，服务端打印日志：

```shell
# server log
20250422 12:12:08.957 [ 3152] [  INFO] connection 127.0.0.1:58098 fail - mnt/d/cs_git_rep/goa-rpc/src/server/BaseServer.cc:48
```

RPC调用完毕，返回成功。

## 编译&&安装

```shell
$ git clone https://github.com/moonlightleaf/goa-rpc.git
$ cd goa-rpc
$ git submodule update --init --recursive
$ make all
```

可以通过在makefile选择是否添加`-DCMAKE_BUILD_EXAMPLES=1`选项，来决定是否要对`examples`目录下的文件进行编译。

## 参考

- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)

- [json-rpc-cxx](https://github.com/jsonrpcx/json-rpc-cxx)

  
