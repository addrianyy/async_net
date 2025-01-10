add_executable(chat_server "")
target_link_libraries(chat_server PUBLIC baselib async_ws)
target_compile_features(chat_server PUBLIC cxx_std_20)

target_sources(chat_server PUBLIC
    ChatClient.cpp
    ChatClient.hpp
    ChatServer.cpp
    ChatServer.hpp
    main.cpp
)