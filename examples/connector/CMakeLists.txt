add_executable(connector "")
target_link_libraries(connector PUBLIC baselib async_net)
target_compile_features(connector PUBLIC cxx_std_20)

target_sources(connector PUBLIC
    main.cpp
)