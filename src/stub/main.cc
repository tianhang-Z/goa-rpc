
#include <iostream>

#include "goa-json/include/Document.hpp"
#include "goa-json/include/Exception.hpp"
#include "goa-json/include/FileReadStream.hpp"
#include "stub/ClientStubGenerator.hpp"
#include "stub/ServiceStubGenerator.hpp"
#include "stub/StubGenerator.hpp"
#include "utils/Exception.hpp"

using namespace goa::rpc;

static void usage() {
  std::cerr << "usage: stub_generator <-c/s> [-o] [-i input]\n";
  exit(1);
}

static void writeToFile(StubGenerator& generator, bool outputToFile) {
  FILE* output;
  if (!outputToFile) {
    output = stdout;
  } else {
    std::string outputFileName = generator.genStubClassName() + ".hpp";
    output = fopen(outputFileName.c_str(), "w+");
    if (output == nullptr) {
      std::cerr << "Failed to open file " << outputFileName << "\n";
      exit(1);
    }
  }
  auto stubString = generator.genStub();
  fputs(stubString.c_str(), output);
}

static std::unique_ptr<StubGenerator> makeGenerator(bool serverSide,
                                                    goa::json::Value& proto) {
  if (serverSide) {
    return std::make_unique<ServiceStubGenerator>(proto);
  } else {
    return std::make_unique<ClientStubGenerator>(proto);
  }
}

static void genStub(FILE* input, bool serverSide, bool outputToFile) {
  goa::json::Document proto;
  goa::json::FileReadStream is(input);

  auto err = proto.parseStream(is);
  if (err != goa::json::ParseError::PARSE_OK) {
    std::cerr << goa::json::parseErrorString(err) << std::endl;
    exit(1);
  }
  try {
    auto generator = makeGenerator(serverSide, proto);
    writeToFile(*generator, outputToFile);
  } catch (StubException& e) {
    std::cerr << "input error: " << e.what() << std::endl;
    exit(1);
  }
}

int main(int argc, char** argv) {
  bool serverSide = false;
  bool clientSide = false;
  bool outputToFile = false;
  const char* inputFileName = nullptr;

  int opt;
  // 使用getopt来处理传入参数的解析，i:表示 -i
  // 后面必须要有一个额外参数传入，"csi:o"表示可匹配的参数列表 -c -s -i filename
  // -o
  while ((opt = getopt(argc, argv, "csi:o")) != -1) {
    switch (opt) {
      case 'c':
        clientSide = true;
        break;
      case 's':
        serverSide = true;
        break;
      case 'o':
        outputToFile = true;
        break;
      case 'i':
        inputFileName = optarg;  // optarg 为-i filename 中的filename
        break;
      default:
        std::cerr << "unknown flag " << '0' + opt << std::endl;
        usage();
    }
  }

  // 如果两个选项都未标记，则认为同时打开
  if (!serverSide && !clientSide) {
    serverSide = clientSide = true;
  }

  FILE* input = stdin;
  if (inputFileName != nullptr) {
    input = fopen(inputFileName, "r");
    if (input == nullptr) {
      std::cerr << "open inputFile failed" << std::endl;
      exit(1);
    }
  }

  try {
    if (serverSide) {
      genStub(input, true, outputToFile);
      rewind(input);  // 重置文件指针，以便下次读取
    }
    if (clientSide) {
      genStub(input, false, outputToFile);
    }
  }

  catch (StubException& e) {
    std::cerr << "input error: " << e.what() << std::endl;
    exit(1);
  }
  return 0;
}
