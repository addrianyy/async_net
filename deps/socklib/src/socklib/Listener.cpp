#include "Listener.hpp"
#include "private/AddressConversion.hpp"
#include "private/Initialization.hpp"
#include "private/ResolveAndRun.hpp"
#include "private/SocketHelpers.hpp"
#include "private/System.hpp"

namespace sock {

Result<Listener> Listener::bind(
  const SocketAddress& address, const BindParameters& bind_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto listener_socket =
    ::socket(detail::address_type_to_protocol(address.type()), SOCK_STREAM, 0);
  if (!detail::is_socket_valid(listener_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  const auto setup_status = detail::setup_socket(
    listener_socket,
    bind_parameters.reuse_address,
    bind_parameters.reuse_port,
    bind_parameters.non_blocking
  );
  if (setup_status != Status::Ok) {
    detail::close_socket(listener_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  int bind_result{detail::default_error_value};
  detail::AddressConverter::to_raw(
    address, [&](const sockaddr* socket_address, socklen_t sockaddr_size) {
      bind_result = ::bind(listener_socket, socket_address, sockaddr_size);
    }
  );
  if (detail::is_error(bind_result)) {
    return std::unexpected{detail::get_last_error_and_close_socket(listener_socket)};
  }

  const auto backlog_size = std::min(int(bind_parameters.max_pending_connections), SOMAXCONN);
  if (detail::is_error(::listen(listener_socket, backlog_size))) {
    return std::unexpected{detail::get_last_error_and_close_socket(listener_socket)};
  }

  return Listener{listener_socket};
}

Result<Listener> Listener::bind(
  IpVersion ip_version,
  std::string_view hostname,
  uint16_t port,
  const BindParameters& bind_parameters
) {
  return detail::resolve_and_run<Result<Listener>>(
    ip_version, hostname, port, [&](const SocketAddress& resolved_address) {
      return bind(resolved_address, bind_parameters);
    }
  );
}

Result<StreamSocket> Listener::accept(SocketAddress* peer_address) {
  detail::SockaddrBuffer socket_address;
  socklen_t sockaddr_size = sizeof(socket_address);

  const detail::RawSocket accepted_socket = detail::retry_on_eintr([&]() -> detail::RawSocket {
    if (peer_address) {
      return ::accept(socket_, reinterpret_cast<sockaddr*>(socket_address.data), &sockaddr_size);
    } else {
      return ::accept(socket_, nullptr, nullptr);
    }
  });

  if (!detail::is_socket_valid(accepted_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  if (peer_address) {
    if (!detail::AddressConverter::from_raw(socket_address, sockaddr_size, *peer_address)) {
      detail::close_socket(accepted_socket);
      return std::unexpected{Status::AddressConversionFailed};
    }
  }

  return StreamSocket{accepted_socket};
}

}  // namespace sock
