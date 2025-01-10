add_executable(proxy_server "")
target_link_libraries(proxy_server PUBLIC baselib async_ws)
target_compile_features(proxy_server PUBLIC cxx_std_20)

target_sources(proxy_server PUBLIC
    main.cpp
)