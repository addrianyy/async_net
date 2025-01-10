add_subdirectory(detail)

target_sources(websocket PUBLIC
    Http.cpp
    Http.hpp
    Handshake.cpp
    Handshake.hpp
    Packet.hpp
    Packet.cpp
    PacketSerialization.cpp
    PacketSerialization.hpp
)