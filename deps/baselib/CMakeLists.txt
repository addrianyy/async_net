cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(fmt)

add_library(baselib STATIC "")
add_subdirectory(src)

target_include_directories(baselib PUBLIC src)
target_compile_features(baselib PUBLIC cxx_std_23)
target_link_libraries(baselib PUBLIC fmt)
