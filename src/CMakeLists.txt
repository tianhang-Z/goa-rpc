add_library(goa-rpc STATIC
            utils/RpcError.hpp
            utils/Exception.hpp
            )


            
target_link_libraries(goa-rpc goa-json goa-ev)
install(TARGETS goa-rpc DESTINATION lib)

