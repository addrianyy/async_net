#pragma once
#include <cstddef>
#include <cstdint>

namespace base {

template <size_t Size, size_t Alignment>
struct AlignedStorage {
  alignas(Alignment) uint8_t buffer[Size];
};

}  // namespace base
