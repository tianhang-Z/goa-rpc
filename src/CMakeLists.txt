add_library(goa-rpc STATIC_LIBRARY_FLAGS
            util/RpcERROR.hpp)
target_link_libraries(goa-rpc goa-json goa-ev)
install(TARGETS goa-rpc DESTINATION lib)

