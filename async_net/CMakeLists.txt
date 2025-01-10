cmake_minimum_required(VERSION 3.20)

add_library(async_net STATIC "")
add_subdirectory(src)

target_include_directories(async_net PUBLIC src)
target_link_libraries(async_net PUBLIC baselib socklib)
target_compile_features(async_net PUBLIC cxx_std_20)