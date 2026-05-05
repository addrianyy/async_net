#pragma once
#include <cstddef>
#include <cstdint>

#include "Address.hpp"
#include "Status.hpp"

namespace sock {

namespace detail {

#if defined(_WIN32)
using RawSocket = uintptr_t;
constexpr RawSocket invalid_socket = static_cast<RawSocket>(~static_cast<uintptr_t>(0));
#else
using RawSocket = int;
constexpr RawSocket invalid_socket = -1;
#endif

struct RawSocketAccessor;

}  // namespace detail

class Socket {
  friend class detail::RawSocketAccessor;

 protected:
  detail::RawSocket socket_{detail::invalid_socket};

  explicit Socket(detail::RawSocket socket_);

 public:
  Socket() = default;

  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  Socket(const Socket& other) = delete;
  Socket& operator=(const Socket& other) = delete;

  ~Socket();

  bool valid() const { return socket_ != detail::invalid_socket; }
  operator bool() const { return valid(); }

  Status set_non_blocking(bool non_blocking);

  Status last_error() const;
  Status local_address(SocketAddress& address) const;

  template <typename T>
  Result<T> local_address() const {
    T address{};
    const auto status = local_address(address);
    return status == Status::Ok ? Result<T>{address} : std::unexpected{status};
  }
};

class ReadWriteSocket : public Socket {
 protected:
  using Socket::Socket;

 public:
  Status set_receive_timeout_ms(uint64_t timeout_ms);
  Status set_send_timeout_ms(uint64_t timeout_ms);
  Status set_receive_buffer_size(size_t size);
  Status set_send_buffer_size(size_t size);
};

}  // namespace sock
