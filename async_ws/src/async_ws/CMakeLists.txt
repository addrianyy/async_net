add_subdirectory(detail)

target_sources(async_ws PUBLIC
    WebSocketServer.cpp
    WebSocketServer.hpp
    WebSocketClient.cpp
    WebSocketClient.hpp
    Status.hpp
    Status.cpp
)