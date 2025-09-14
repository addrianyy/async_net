#pragma once
#include "detail/FillBytes.hpp"

#include <cstdint>
#include <limits>
#include <random>
#include <span>

namespace base {

class SystemRng32 {
  std::random_device device;

 public:
  SystemRng32() = default;

  uint32_t gen() { return device(); }

  using result_type = uint32_t;

  result_type operator()() { return gen(); }

  constexpr static result_type min() { return std::numeric_limits<result_type>::min(); }
  constexpr static result_type max() { return std::numeric_limits<result_type>::max(); }

  void fill_bytes(void* data, size_t size) { detail::fill_bytes_from_rng(*this, data, size); }
  void fill_bytes(std::span<uint8_t> data) { fill_bytes(data.data(), data.size()); }
};

static_assert(std::random_device::min() == SystemRng32::min());
static_assert(std::random_device::max() == SystemRng32::max());

class SystemRng64 {
  base::SystemRng32 rng32;

 public:
  SystemRng64() = default;

  uint64_t gen() { return uint64_t(rng32()) | (uint64_t(rng32()) << 32); }

  using result_type = uint64_t;

  result_type operator()() { return gen(); }

  constexpr static result_type min() { return std::numeric_limits<result_type>::min(); }
  constexpr static result_type max() { return std::numeric_limits<result_type>::max(); }

  void fill_bytes(void* data, size_t size) { detail::fill_bytes_from_rng(*this, data, size); }
  void fill_bytes(std::span<uint8_t> data) { fill_bytes(data.data(), data.size()); }
};

}  // namespace base
