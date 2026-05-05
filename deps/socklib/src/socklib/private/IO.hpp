#pragma once
#include "../Socket.hpp"
#include "System.hpp"

#include <span>

namespace sock::detail {

#if defined(SOCKLIB_WINDOWS)

struct ScatterGatherBase {
  using Entry = WSABUF;

  __forceinline static void* get_base(const Entry& entry) { return entry.buf; }
  __forceinline static void set_base(Entry& entry, void* p) { entry.buf = static_cast<char*>(p); }

  __forceinline static size_t get_size(const Entry& entry) { return entry.len; }
  __forceinline static void set_size(Entry& entry, size_t size) {
    entry.len = static_cast<ULONG>(size);
  }
};

#else

struct ScatterGatherBase {
  using Entry = iovec;

  [[gnu::always_inline]] static void* get_base(const Entry& entry) { return entry.iov_base; }
  [[gnu::always_inline]] static void set_base(Entry& entry, void* p) { entry.iov_base = p; }

  [[gnu::always_inline]] static size_t get_size(const Entry& entry) { return entry.iov_len; }
  [[gnu::always_inline]] static void set_size(Entry& entry, size_t size) { entry.iov_len = size; }
};

#endif

struct ScatterGather : ScatterGatherBase {
  [[gnu::always_inline]] static void set_span(Entry& entry, std::span<const uint8_t> data) {
    set_base(entry, const_cast<uint8_t*>(data.data()));
    set_size(entry, data.size());
  }

  [[gnu::always_inline]] static void offset_by(Entry& entry, size_t amount) {
    set_base(entry, reinterpret_cast<uint8_t*>(get_base(entry)) + amount);
    set_size(entry, get_size(entry) - amount);
  }
};

Result<size_t> socket_receive_sg(
  RawSocket socket,
  std::span<const ScatterGather::Entry> sg,
  void* source_address,
  size_t source_address_size
);
Result<size_t> socket_send_sg(
  RawSocket socket,
  std::span<const ScatterGather::Entry> sg,
  const void* dest_address,
  size_t dest_address_size
);

Result<size_t> socket_receive(
  RawSocket socket, std::span<uint8_t> data, void* source_address, size_t source_address_size
);
Result<size_t> socket_send(
  RawSocket socket,
  std::span<const uint8_t> data,
  const void* dest_address,
  size_t dest_address_size
);

constexpr static size_t max_scatter_gather_entries = 32;

}  // namespace sock::detail
