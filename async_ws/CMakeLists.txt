cmake_minimum_required(VERSION 3.20)

add_subdirectory(websocket)

add_library(async_ws STATIC "")
add_subdirectory(src)

target_include_directories(async_ws PUBLIC src)
target_link_libraries(async_ws PUBLIC baselib websocket async_net)
target_compile_features(async_ws PUBLIC cxx_std_23)
