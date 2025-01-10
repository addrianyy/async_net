add_executable(echo_server "")
target_link_libraries(echo_server PUBLIC baselib async_ws)
target_compile_features(echo_server PUBLIC cxx_std_20)

target_sources(echo_server PUBLIC
    main.cpp
)