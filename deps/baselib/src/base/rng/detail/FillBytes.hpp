#pragma once
#include <cstddef>
#include <cstdint>

namespace base::detail {

template <typename Rng>
void fill_bytes_from_rng(Rng& rng, void* data, size_t size) {
  const auto full_values_count = size / sizeof(typename Rng::result_type);
  for (size_t i = 0; i < full_values_count; ++i) {
    reinterpret_cast<typename Rng::result_type*>(data)[i] = rng.gen();
  }

  const auto full_values_size = full_values_count * sizeof(typename Rng::result_type);
  const auto remaining_size = size - full_values_size;
  if (remaining_size > 0) {
    const auto v = rng.gen();
    for (size_t i = 0; i < remaining_size; ++i) {
      reinterpret_cast<uint8_t*>(data)[full_values_size + i] =
        reinterpret_cast<const uint8_t*>(&v)[i];
    }
  }
}

}  // namespace base::detail
