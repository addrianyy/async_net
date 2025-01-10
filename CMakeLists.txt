cmake_minimum_required(VERSION 3.20)
project(async_networking)

add_subdirectory(deps/baselib)
add_subdirectory(deps/socklib)
add_subdirectory(async_net)
add_subdirectory(async_ws)

add_subdirectory(examples)