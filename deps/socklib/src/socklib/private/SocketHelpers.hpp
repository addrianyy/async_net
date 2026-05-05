#pragma once
#include "../Socket.hpp"
#include "Error.hpp"
#include "SocketHelpers.hpp"
#include "System.hpp"

namespace sock::detail {

inline bool is_socket_valid(RawSocket socket) {
  return socket != invalid_socket;
}

inline void close_socket(RawSocket socket) {
  if (is_socket_valid(socket)) {
#if defined(SOCKLIB_WINDOWS)
    ::shutdown(socket, SD_BOTH);
    ::closesocket(socket);
#else
    ::shutdown(socket, SHUT_RDWR);
    ::close(socket);
#endif
  }
}

inline Status get_last_error_and_close_socket(RawSocket socket) {
  const auto status = system_last_error_to_status();
  close_socket(socket);
  return status;
}

template <typename T>
Status set_socket_option(RawSocket socket, int level, int option, const T& value) {
  if (is_error(
        ::setsockopt(socket, level, option, reinterpret_cast<const char*>(&value), sizeof(value))
      )) {
    return system_last_error_to_status();
  }
  return Status::Ok;
}

template <typename Fn>
auto retry_on_eintr(Fn&& callback) {
  while (true) {
    const auto result = callback();
#if defined(SOCKLIB_WINDOWS)
    if (is_error(result) && WSAGetLastError() == WSAEINTR) {
      continue;
    }
#else
    if (is_error(result) && errno == EINTR) {
      continue;
    }
#endif
    return result;
  }
}

Status set_socket_non_blocking(RawSocket socket, bool non_blocking);
Status setup_socket(RawSocket socket, bool reuse_address, bool reuse_port, bool non_blocking);

}  // namespace sock::detail
