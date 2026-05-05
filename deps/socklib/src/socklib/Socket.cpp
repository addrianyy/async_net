#include "Socket.hpp"
#include "private/AddressConversion.hpp"
#include "private/Error.hpp"
#include "private/SocketHelpers.hpp"
#include "private/System.hpp"

namespace sock {

static Status set_socket_option_timeout_ms(
  detail::RawSocket socket, int level, int option, uint64_t timeout_ms
) {
#if defined(SOCKLIB_WINDOWS)
  if (timeout_ms > std::numeric_limits<uint32_t>::max()) {
    return Status::TimeoutTooLarge;
  }
  return detail::set_socket_option<uint32_t>(socket, level, option, timeout_ms);
#else
  timeval timeout_timeval{};
  timeout_timeval.tv_sec = static_cast<long>(timeout_ms / 1000);
  timeout_timeval.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
  return detail::set_socket_option<timeval>(socket, level, option, timeout_timeval);
#endif
}

Socket::Socket(detail::RawSocket socket_)
    : socket_(socket_) {}

Socket::Socket(Socket&& other) noexcept {
  socket_ = other.socket_;
  other.socket_ = detail::invalid_socket;
}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    detail::close_socket(detail::invalid_socket);

    socket_ = other.socket_;
    other.socket_ = detail::invalid_socket;
  }
  return *this;
}

Socket::~Socket() {
  detail::close_socket(socket_);
}

Status Socket::set_non_blocking(bool non_blocking) {
  return detail::set_socket_non_blocking(socket_, non_blocking);
}

Status Socket::last_error() const {
  int error{};
  socklen_t error_length = sizeof(error);
  if (::getsockopt(socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &error_length) !=
      0) {
    return Status::Unknown;
  }

  if (error == 0) {
    return Status::Ok;
  }

  return detail::system_error_to_status(error);
}

Status Socket::local_address(SocketAddress& address) const {
  detail::SockaddrBuffer socket_address;
  socklen_t sockaddr_size = sizeof(socket_address);

  if (detail::is_error(
        ::getsockname(socket_, reinterpret_cast<sockaddr*>(socket_address.data), &sockaddr_size)
      )) {
    return detail::system_last_error_to_status();
  }

  if (!detail::AddressConverter::from_raw(socket_address, sockaddr_size, address)) {
    return Status::AddressConversionFailed;
  }

  return Status::Ok;
}

Status ReadWriteSocket::set_receive_timeout_ms(uint64_t timeout_ms) {
  return set_socket_option_timeout_ms(socket_, SOL_SOCKET, SO_RCVTIMEO, timeout_ms);
}

Status ReadWriteSocket::set_send_timeout_ms(uint64_t timeout_ms) {
  return set_socket_option_timeout_ms(socket_, SOL_SOCKET, SO_SNDTIMEO, timeout_ms);
}

Status ReadWriteSocket::set_receive_buffer_size(size_t size) {
  if (size > std::numeric_limits<int>::max()) {
    return Status::SizeTooLarge;
  }
  return detail::set_socket_option<int>(socket_, SOL_SOCKET, SO_RCVBUF, int(size));
}

Status ReadWriteSocket::set_send_buffer_size(size_t size) {
  if (size > std::numeric_limits<int>::max()) {
    return Status::SizeTooLarge;
  }
  return detail::set_socket_option<int>(socket_, SOL_SOCKET, SO_SNDBUF, int(size));
}

}  // namespace sock
