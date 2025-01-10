#pragma once
#include "Packet.hpp"

#include <base/containers/BinaryBuffer.hpp>

namespace websocket {

class Serializer {
 public:
  static void serialize(const Packet& packet,
                        std::span<const uint8_t> payload,
                        base::BinaryBuffer& buffer);

  static size_t serialized_packet_size(size_t payload_size, bool masked);
};

class Deserializer {
 public:
  enum class Error {
    Ok,
    NeedMoreData,
    ReservedFieldsSet,
    InvalidOpcode,
    PayloadTooLarge,
    ControlPacketHasPayload,
  };

  struct Result {
    Error error{};
    Packet packet{};
    size_t header_size{};
  };

  static Result deserialize(std::span<const uint8_t> data);
};

void mask_packet_payload(MaskingKey key, std::span<uint8_t> payload);

}  // namespace websocket