add_library(goa-rpc STATIC
            utils/RpcError.hpp
            utils/Exception.hpp
            server/RpcService.hpp 
            server/BaseServer.hpp server/BaseServer.cc
            server/RpcServer.hpp server/RpcServer.cc
            server/Procedure.hpp server/Procedure.cc
            )


            
target_link_libraries(goa-rpc goa-json goa-ev)
install(TARGETS goa-rpc DESTINATION lib)

