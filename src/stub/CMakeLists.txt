add_executable(goa-rpc-stub
        StubGenerator.hpp StubGenerator.cc
        ServiceStubGenerator.hpp ServiceStubGenerator.cc
        ClientStubGenerator.hpp ClientStubGenerator.cc
        main.cc)

target_link_libraries(goa-rpc-stub goa-json)
install(TARGETS goa-rpc-stub DESTINATION ${CMAKE_BINARY_DIR}/bin)