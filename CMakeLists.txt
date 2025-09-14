cmake_minimum_required(VERSION 3.20)
project(async_networking)

option(ASYNC_NET_BUILD_EXAMPLES "" OFF)

add_subdirectory(deps/baselib)
add_subdirectory(deps/socklib)
add_subdirectory(async_net)
add_subdirectory(async_ws)

if (ASYNC_NET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
