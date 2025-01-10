add_subdirectory(detail)

target_sources(async_net PUBLIC
    IoContext.cpp
    IoContext.hpp
    IpAddress.cpp
    IpAddress.hpp
    TcpListener.cpp
    TcpListener.hpp
    TcpConnection.hpp
    TcpConnection.cpp
    IpResolver.cpp
    IpResolver.hpp
    Timer.cpp
    Timer.hpp
    Status.hpp
    Status.cpp
)