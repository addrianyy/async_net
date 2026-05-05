add_subdirectory(private)

target_sources(socklib PRIVATE
    Address.cpp
    Address.hpp
    Resolver.cpp
    Resolver.hpp
    Socket.cpp
    Socket.hpp
    Listener.cpp
    Listener.hpp
    DatagramSocket.cpp
    DatagramSocket.hpp
    StreamSocket.cpp
    StreamSocket.hpp
    ConnectingStreamSocket.cpp
    ConnectingStreamSocket.hpp
    ConnectedPair.cpp
    ConnectedPair.hpp
    Poller.cpp
    Poller.hpp
    Status.cpp
    Status.hpp
    Status.inc
)