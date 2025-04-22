add_library(goa-rpc STATIC
            utils/RpcError.hpp
            utils/Exception.hpp
            utils/utils.hpp
            server/RpcService.hpp 
            server/BaseServer.hpp server/BaseServer.cc
            server/RpcServer.hpp server/RpcServer.cc
            server/Procedure.hpp server/Procedure.cc
            client/BaseClient.hpp client/BaseClient.cc
            )


            
target_link_libraries(goa-rpc goa-json goa-ev)
install(TARGETS goa-rpc DESTINATION lib)

set(HEADERS
        utils/RpcError.hpp
        utils/Exception.hpp
        utils/utils.hpp
        server/BaseServer.hpp
        server/RpcServer.hpp
        server/RpcService.hpp
        server/Procedure.hpp
        client/BaseClient.hpp)
install(FILES ${HEADERS} DESTINATION include)

add_subdirectory(stub)
