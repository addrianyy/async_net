#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include <base/macro/ClassTraits.hpp>

namespace base {

class Fnv1a {
 public:
  using HashType = uint64_t;

 private:
  constexpr static HashType offset_basis = 0xcbf29ce484222325;
  HashType hash_ = offset_basis;

 public:
  CLASS_NON_COPYABLE_NON_MOVABLE(Fnv1a)

  Fnv1a() = default;

  template <typename T>
  void feed(const T& data) {
    static_assert(std::is_trivial_v<T>, "feed works only with trivial types");
    feed(&data, sizeof(T));
  }

  void feed(std::span<const uint8_t> data) { feed(data.data(), data.size()); }

  void feed(const void* data, size_t size) {
    const auto bytes = reinterpret_cast<const uint8_t*>(data);

    for (size_t i = 0; i < size; ++i) {
      hash_ = (hash_ ^ uint64_t(bytes[i])) * uint64_t(0x100000001b3);
    }
  }

  HashType finalize() const { return hash_; }

  void reset() { hash_ = offset_basis; }
};

}  // namespace base
