cmake_minimum_required(VERSION 3.20)

add_library(websocket STATIC "")
add_subdirectory(src)

target_include_directories(websocket PUBLIC src)
target_link_libraries(websocket PUBLIC baselib)
target_compile_features(websocket PUBLIC cxx_std_20)