#pragma once
#include "SeedFromSystemRng.hpp"
#include "detail/FillBytes.hpp"

#include <cstdint>
#include <limits>
#include <span>

namespace base {

class Xorshift {
  uint64_t value{};

 public:
  explicit Xorshift(SeedFromSystemRng seed) { reseed(seed); }
  explicit Xorshift(uint64_t seed) { reseed(seed); }

  void reseed(SeedFromSystemRng seed);
  void reseed(uint64_t seed) {
    value = seed;

    for (int i = 0; i < 2; ++i) {
      gen();
    }
  }

  uint64_t gen() {
    uint64_t x = value;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    value = x;

    return x;
  }

  using result_type = uint64_t;

  result_type operator()() { return gen(); }

  constexpr static result_type min() { return std::numeric_limits<result_type>::min(); }
  constexpr static result_type max() { return std::numeric_limits<result_type>::max(); }

  void fill_bytes(void* data, size_t size) { detail::fill_bytes_from_rng(*this, data, size); }
  void fill_bytes(std::span<uint8_t> data) { fill_bytes(data.data(), data.size()); }
};

}  // namespace base