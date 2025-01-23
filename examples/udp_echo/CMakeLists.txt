add_executable(udp_echo_server "")
target_link_libraries(udp_echo_server PUBLIC baselib async_net)
target_compile_features(udp_echo_server PUBLIC cxx_std_20)

target_sources(udp_echo_server PUBLIC
    main.cpp
)