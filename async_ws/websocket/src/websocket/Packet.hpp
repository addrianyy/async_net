#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace websocket {

enum class PacketType {
  ContinuationFrame,
  TextFrame,
  BinaryFrame,
  ConnectionClose,
  Ping,
  Pong,
};

using MaskingKey = std::array<uint8_t, 4>;

struct Packet {
  PacketType packet_type{};
  bool final{};
  size_t payload_size{};
  std::optional<MaskingKey> masking_key{};
};

}  // namespace websocket