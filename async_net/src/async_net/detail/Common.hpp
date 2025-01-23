#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace async_net::detail {

constexpr size_t invalid_context_index = std::numeric_limits<size_t>::max();
constexpr size_t default_send_buffer_max_size = 8 * 1024 * 1024;
constexpr size_t max_datagram_size = std::numeric_limits<uint16_t>::max();

}  // namespace async_net::detail