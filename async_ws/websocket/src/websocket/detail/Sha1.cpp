// based on https://github.com/mohaps/TinySHA1/

#include "Sha1.hpp"

#include <cstring>

namespace websocket::handshake {

static uint32_t left_rotate(uint32_t value, size_t count) {
  return (value << count) ^ (value >> (32 - count));
}

void Sha1::process_block() {
  uint32_t w[80];
  for (size_t i = 0; i < 16; i++) {
    w[i] = (block_[i * 4 + 0] << 24);
    w[i] |= (block_[i * 4 + 1] << 16);
    w[i] |= (block_[i * 4 + 2] << 8);
    w[i] |= (block_[i * 4 + 3]);
  }
  for (size_t i = 16; i < 80; i++) {
    w[i] = left_rotate((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
  }

  uint32_t a = digest_[0];
  uint32_t b = digest_[1];
  uint32_t c = digest_[2];
  uint32_t d = digest_[3];
  uint32_t e = digest_[4];

  for (std::size_t i = 0; i < 80; ++i) {
    uint32_t f = 0;
    uint32_t k = 0;

    if (i < 20) {
      f = (b & c) | (~b & d);
      k = 0x5A827999;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDC;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }
    uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = left_rotate(b, 30);
    b = a;
    a = temp;
  }

  digest_[0] += a;
  digest_[1] += b;
  digest_[2] += c;
  digest_[3] += d;
  digest_[4] += e;
}

void Sha1::process_byte(uint8_t byte) {
  block_[block_byte_index_++] = byte;
  ++byte_count_;
  if (block_byte_index_ == 64) {
    block_byte_index_ = 0;
    process_block();
  }
}

Sha1::Sha1() {
  reset();
}

void Sha1::reset() {
  digest_[0] = 0x67452301;
  digest_[1] = 0xEFCDAB89;
  digest_[2] = 0x98BADCFE;
  digest_[3] = 0x10325476;
  digest_[4] = 0xC3D2E1F0;
  block_byte_index_ = 0;
  byte_count_ = 0;
}

void Sha1::feed(const void* data, size_t size) {
  const auto block = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; ++i) {
    process_byte(block[i]);
  }
}

std::array<uint32_t, 5> Sha1::digest_u32() {
  size_t bit_count = byte_count_ * 8;
  process_byte(0x80);

  if (block_byte_index_ > 56) {
    while (block_byte_index_ != 0) {
      process_byte(0);
    }
    while (block_byte_index_ < 56) {
      process_byte(0);
    }
  } else {
    while (block_byte_index_ < 56) {
      process_byte(0);
    }
  }
  process_byte(0);
  process_byte(0);
  process_byte(0);
  process_byte(0);
  process_byte(static_cast<unsigned char>((bit_count >> 24) & 0xFF));
  process_byte(static_cast<unsigned char>((bit_count >> 16) & 0xFF));
  process_byte(static_cast<unsigned char>((bit_count >> 8) & 0xFF));
  process_byte(static_cast<unsigned char>((bit_count) & 0xFF));

  return digest_;
}

std::array<uint8_t, 20> Sha1::digest() {
  std::array<uint8_t, 20> result{};

  const auto d = digest_u32();
  size_t di = 0;

  result[di++] = ((d[0] >> 24) & 0xFF);
  result[di++] = ((d[0] >> 16) & 0xFF);
  result[di++] = ((d[0] >> 8) & 0xFF);
  result[di++] = ((d[0]) & 0xFF);

  result[di++] = ((d[1] >> 24) & 0xFF);
  result[di++] = ((d[1] >> 16) & 0xFF);
  result[di++] = ((d[1] >> 8) & 0xFF);
  result[di++] = ((d[1]) & 0xFF);

  result[di++] = ((d[2] >> 24) & 0xFF);
  result[di++] = ((d[2] >> 16) & 0xFF);
  result[di++] = ((d[2] >> 8) & 0xFF);
  result[di++] = ((d[2]) & 0xFF);

  result[di++] = ((d[3] >> 24) & 0xFF);
  result[di++] = ((d[3] >> 16) & 0xFF);
  result[di++] = ((d[3] >> 8) & 0xFF);
  result[di++] = ((d[3]) & 0xFF);

  result[di++] = ((d[4] >> 24) & 0xFF);
  result[di++] = ((d[4] >> 16) & 0xFF);
  result[di++] = ((d[4] >> 8) & 0xFF);
  result[di++] = ((d[4]) & 0xFF);

  return result;
}

}  // namespace websocket::handshake