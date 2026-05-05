#include "SocketHelpers.hpp"

namespace sock::detail {

Status set_socket_non_blocking(RawSocket socket, bool non_blocking) {
#if defined(SOCKLIB_WINDOWS)
  auto non_blocking_value = static_cast<u_long>(non_blocking);
  if (is_error(::ioctlsocket(socket, FIONBIO, &non_blocking_value))) {
    return system_last_error_to_status();
  }
  return Status::Ok;
#else
  if (is_error(
        ::fcntl(
          socket,
          F_SETFL,
          (non_blocking ? O_NONBLOCK : 0) | (::fcntl(socket, F_GETFL) & ~O_NONBLOCK)
        )
      )) {
    return system_last_error_to_status();
  }
  return Status::Ok;
#endif
}

Status setup_socket(RawSocket socket, bool reuse_address, bool reuse_port, bool non_blocking) {
  Status status{};

#if !defined(SOCKLIB_WINDOWS)
  if (reuse_address) {
    status = set_socket_option<int>(socket, SOL_SOCKET, SO_REUSEADDR, 1);
    if (status != Status::Ok) {
      return status;
    }
  }

  if (reuse_port) {
    status = set_socket_option<int>(socket, SOL_SOCKET, SO_REUSEPORT, 1);
    if (status != Status::Ok) {
      return status;
    }
  }
#else
  if (reuse_port) {
    status = set_socket_option<int>(socket, SOL_SOCKET, SO_REUSEADDR, 1);
    if (status != Status::Ok) {
      return status;
    }
  }
#endif

  // Can fail.
  (void)set_socket_option<int>(socket, IPPROTO_IPV6, IPV6_V6ONLY, 0);

  if (non_blocking) {
    status = set_socket_non_blocking(socket, true);
    if (status != Status::Ok) {
      return status;
    }
  }

  return Status::Ok;
}

}  // namespace sock::detail
