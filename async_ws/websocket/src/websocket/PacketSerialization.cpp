#include "PacketSerialization.hpp"

#include <base/Panic.hpp>

#include <limits>
#include <optional>

namespace websocket {

static uint8_t packet_type_to_opcode(PacketType type) {
  // clang-format off
  switch (type) {
    case PacketType::ContinuationFrame: return 0;
    case PacketType::TextFrame: return 1;
    case PacketType::BinaryFrame: return 2;
    case PacketType::ConnectionClose: return 8;
    case PacketType::Ping: return 9;
    case PacketType::Pong: return 10;
    default:
      panic_unreachable();
  }
  // clang-format on
}

static std::optional<PacketType> opcode_to_packet_type(uint8_t opcode) {
  // clang-format off
  switch (opcode) {
    case 0: return PacketType::ContinuationFrame;
    case 1: return PacketType::TextFrame;
    case 2: return PacketType::BinaryFrame;
    case 8: return PacketType::ConnectionClose;
    case 9: return PacketType::Ping;
    case 10: return PacketType::Pong;
    default: return std::nullopt;
  }
  // clang-format on
}

struct WebsocketPacketReader {
  std::span<const uint8_t> data;

  bool fin{};
  bool rsv1{};
  bool rsv2{};
  bool rsv3{};
  uint8_t opcode{};
  bool mask{};
  uint64_t payload_size{};
  std::array<uint8_t, 4> masking_key{};

  template <size_t N>
  bool read_bytes(std::array<uint8_t, N>& bytes) {
    if (data.size() >= N) {
      std::memcpy(bytes.data(), data.data(), N);
      data = data.subspan(N);
      return true;
    }

    return false;
  }

  bool read_u8(uint8_t& value) {
    std::array<uint8_t, 1> buffer{};
    if (!read_bytes(buffer)) {
      return false;
    }
    value = buffer[0];
    return true;
  }

  bool read_u16(uint16_t& value) {
    std::array<uint8_t, 2> buffer{};
    if (!read_bytes(buffer)) {
      return false;
    }
    value = (uint16_t(buffer[0]) << 8) | (uint16_t(buffer[1]) << 0);
    return true;
  }

  bool read_u64(uint64_t& value) {
    std::array<uint8_t, 8> buffer{};
    if (!read_bytes(buffer)) {
      return false;
    }
    value = (uint64_t(buffer[0]) << 56) | (uint64_t(buffer[1]) << 48) |
            (uint64_t(buffer[2]) << 40) | (uint64_t(buffer[3]) << 32) |
            (uint64_t(buffer[4]) << 24) | (uint64_t(buffer[5]) << 16) | (uint64_t(buffer[6]) << 8) |
            (uint64_t(buffer[7]) << 0);
    return true;
  }

  bool run() {
    uint8_t field1{};
    if (!read_u8(field1)) {
      return false;
    }

    fin = field1 & (1 << 7);
    rsv1 = field1 & (1 << 6);
    rsv2 = field1 & (1 << 5);
    rsv3 = field1 & (1 << 4);
    opcode = field1 & 0b1111;

    uint8_t field2{};
    if (!read_u8(field2)) {
      return false;
    }

    mask = field2 & (1 << 7);

    const uint8_t size0 = field2 & ~(1 << 7);
    if (size0 <= 125) {
      payload_size = size0;
    } else {
      if (size0 == 126) {
        uint16_t size1{};
        if (!read_u16(size1)) {
          return false;
        }
        payload_size = size1;
      } else if (size0 == 127) {
        uint64_t size1{};
        if (!read_u64(size1)) {
          return false;
        }
        payload_size = size1;
      }
    }

    if (mask) {
      if (!read_bytes(masking_key)) {
        return false;
      }
    }

    return true;
  }
};

struct WebsocketPacketWriter {
  base::BinaryBuffer& buffer;

  template <size_t N>
  void write_bytes(const std::array<uint8_t, N>& bytes) {
    buffer.append(bytes);
  }

  void write_u8(uint8_t value) {
    const std::array<uint8_t, 1> buffer{
      value,
    };
    write_bytes(buffer);
  }

  void write_u16(uint16_t value) {
    const std::array<uint8_t, 2> buffer{
      uint8_t(value >> 8),
      uint8_t(value >> 0),
    };
    write_bytes(buffer);
  }

  void write_u64(uint64_t value) {
    const std::array<uint8_t, 8> buffer{
      uint8_t(value >> 56), uint8_t(value >> 48), uint8_t(value >> 40), uint8_t(value >> 32),
      uint8_t(value >> 24), uint8_t(value >> 16), uint8_t(value >> 8),  uint8_t(value >> 0),
    };
    write_bytes(buffer);
  }

  void run(const Packet& packet, std::span<const uint8_t> payload) {
    uint8_t field1{};
    if (packet.final) {
      field1 |= (1 << 7);
    }
    field1 |= packet_type_to_opcode(packet.packet_type);

    write_u8(field1);

    uint8_t field2{};
    if (packet.masking_key) {
      field2 |= (1 << 7);
    }

    const auto payload_size = payload.size();
    if (payload_size <= 125) {
      field2 |= uint8_t(payload_size);
      write_u8(field2);
    } else {
      const bool fits_16bit = payload_size <= size_t(std::numeric_limits<uint16_t>::max());
      if (fits_16bit) {
        field2 |= 126;
        write_u8(field2);
        write_u16(uint16_t(payload_size));
      } else {
        field2 |= 127;
        write_u8(field2);
        write_u64(uint64_t(payload_size));
      }
    }

    if (packet.masking_key) {
      write_bytes(*packet.masking_key);
    }

    if (!payload.empty()) {
      const auto size_before = buffer.size();
      buffer.append(payload);

      if (packet.masking_key) {
        mask_packet_payload(*packet.masking_key,
                            buffer.span().subspan(size_before, payload.size()));
      }
    }
  }
};

void Serializer::serialize(const Packet& packet,
                           std::span<const uint8_t> payload,
                           base::BinaryBuffer& buffer) {
  WebsocketPacketWriter writer{
    .buffer = buffer,
  };
  writer.run(packet, payload);
}

size_t Serializer::serialized_packet_size(size_t payload_size, bool masked) {
  size_t packet_size = 2;
  if (payload_size > 125) {
    if (payload_size > size_t(std::numeric_limits<uint16_t>::max())) {
      packet_size += 8;
    } else {
      packet_size += 2;
    }
  }
  if (masked) {
    packet_size += 4;
  }
  return packet_size + payload_size;
}

Deserializer::Result Deserializer::deserialize(std::span<const uint8_t> data) {
  WebsocketPacketReader reader{
    .data = data,
  };
  if (!reader.run()) {
    return {Error::NeedMoreData};
  }

  if (reader.payload_size > uint64_t(std::numeric_limits<size_t>::max())) {
    return {Error::PayloadTooLarge};
  }

  if (reader.data.size() < reader.payload_size) {
    return {Error::NeedMoreData};
  }

  if (reader.rsv1 || reader.rsv2 || reader.rsv3) {
    return {Error::ReservedFieldsSet};
  }

  const auto packet_type = opcode_to_packet_type(reader.opcode);
  if (!packet_type) {
    return {Error::InvalidOpcode};
  }

  const auto header_size = data.size() - reader.data.size();

  return {
    .error = Error::Ok,
    .packet =
      {
        .packet_type = *packet_type,
        .final = reader.fin,
        .payload_size = size_t(reader.payload_size),
        .masking_key = reader.mask ? std::optional{reader.masking_key} : std::nullopt,
      },
    .header_size = header_size,
  };
}

void mask_packet_payload(MaskingKey key, std::span<uint8_t> payload) {
  const uint32_t masking_key_u32 = (uint32_t(key[0]) << 0) | (uint32_t(key[1]) << 8) |
                                   (uint32_t(key[2]) << 16) | (uint32_t(key[3]) << 24);
  const uint64_t masking_key_u64 = uint64_t(masking_key_u32) | (uint64_t(masking_key_u32) << 32);

  const auto u64s = reinterpret_cast<uint64_t*>(payload.data());
  const auto u64_count = payload.size() / sizeof(uint64_t);

  for (size_t i = 0; i < u64_count; ++i) {
    u64s[i] ^= masking_key_u64;
  }

  for (size_t i = u64_count * sizeof(uint64_t); i < payload.size(); i++) {
    payload[i] ^= key[i % 4];
  }
}

}  // namespace websocket
