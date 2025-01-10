#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace websocket::handshake {

class Sha1 {
  std::array<uint32_t, 5> digest_{};
  std::array<uint8_t, 64> block_{};
  size_t block_byte_index_{};
  size_t byte_count_{};

  void process_block();
  void process_byte(uint8_t byte);

 public:
  Sha1();

  void reset();
  void feed(const void* data, size_t size);

  std::array<uint32_t, 5> digest_u32();
  std::array<uint8_t, 20> digest();
};

}  // namespace websocket::handshake