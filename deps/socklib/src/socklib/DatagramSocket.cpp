#include "DatagramSocket.hpp"
#include "private/AddressConversion.hpp"
#include "private/IO.hpp"
#include "private/Initialization.hpp"
#include "private/ResolveAndRun.hpp"
#include "private/SocketHelpers.hpp"
#include "private/System.hpp"

namespace sock {

Result<size_t> DatagramSocket::send_to_internal(
  const SocketAddress* to, std::span<std::span<const uint8_t> const> data
) {
  const auto send_to_with_address =
    [&](const void* socket_address, socklen_t sockaddr_size) -> Result<size_t> {
    if (data.size() > detail::max_scatter_gather_entries) {
      return std::unexpected{Status::SizeTooLarge};
    }

    detail::ScatterGather::Entry sg[detail::max_scatter_gather_entries];
    for (size_t i = 0; i < data.size(); ++i) {
      detail::ScatterGather::set_span(sg[i], data[i]);
    }

    return detail::socket_send_sg(
      socket_, std::span{sg}.subspan(0, data.size()), socket_address, sockaddr_size
    );
  };

  if (to) {
    Result<size_t> result{std::unexpected{Status::AddressConversionFailed}};
    detail::AddressConverter::to_raw(
      *to, [&](const sockaddr* socket_address, socklen_t sockaddr_size) {
        result = send_to_with_address(socket_address, sockaddr_size);
      }
    );
    return result;
  }

  return send_to_with_address(nullptr, 0);
}

Result<size_t> DatagramSocket::receive_from_internal(
  SocketAddress* from, std::span<const std::span<uint8_t>> data
) {
  const auto receive_from_with_address =
    [&](void* socket_address, socklen_t sockaddr_size) -> Result<size_t> {
    if (data.size() > detail::max_scatter_gather_entries) {
      return std::unexpected{Status::SizeTooLarge};
    }

    detail::ScatterGather::Entry sg[detail::max_scatter_gather_entries];
    for (size_t i = 0; i < data.size(); ++i) {
      detail::ScatterGather::set_span(sg[i], data[i]);
    }

    return detail::socket_receive_sg(
      socket_, std::span{sg}.subspan(0, data.size()), socket_address, sockaddr_size
    );
  };

  if (from) {
    detail::SockaddrBuffer socket_address;
    socklen_t sockaddr_size = sizeof(socket_address);

    const auto result = receive_from_with_address(socket_address.data, sockaddr_size);
    if (!result) {
      return result;
    }

    if (!detail::AddressConverter::from_raw(socket_address, sockaddr_size, *from)) {
      return std::unexpected{Status::AddressConversionFailed};
    }

    return result;
  }

  return receive_from_with_address(nullptr, 0);
}

Result<DatagramSocket> DatagramSocket::bind(
  const SocketAddress& address, const BindParameters& bind_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto datagram_socket =
    ::socket(detail::address_type_to_protocol(address.type()), SOCK_DGRAM, 0);
  if (!detail::is_socket_valid(datagram_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  const auto setup_status = detail::setup_socket(
    datagram_socket,
    bind_parameters.reuse_address,
    bind_parameters.reuse_port,
    bind_parameters.non_blocking
  );
  if (setup_status != Status::Ok) {
    detail::close_socket(datagram_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  int bind_result{detail::default_error_value};
  detail::AddressConverter::to_raw(
    address, [&](const sockaddr* socket_address, socklen_t sockaddr_size) {
      bind_result = ::bind(datagram_socket, socket_address, sockaddr_size);
    }
  );
  if (detail::is_error(bind_result)) {
    return std::unexpected{detail::get_last_error_and_close_socket(datagram_socket)};
  }

  return DatagramSocket{datagram_socket};
}

Result<DatagramSocket> DatagramSocket::bind(
  IpVersion ip_version,
  std::string_view hostname,
  uint16_t port,
  const BindParameters& bind_parameters
) {
  return detail::resolve_and_run<Result<DatagramSocket>>(
    ip_version, hostname, port, [&](const SocketAddress& resolved_address) {
      return bind(resolved_address, bind_parameters);
    }
  );
}

Result<DatagramSocket> DatagramSocket::create(
  SocketAddress::Type type, const CreateParameters& create_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto datagram_socket = ::socket(detail::address_type_to_protocol(type), SOCK_DGRAM, 0);
  if (!detail::is_socket_valid(datagram_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  const auto setup_status =
    detail::setup_socket(datagram_socket, false, false, create_parameters.non_blocking);
  if (setup_status != Status::Ok) {
    detail::close_socket(datagram_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  return DatagramSocket{datagram_socket};
}

Result<DatagramSocket> DatagramSocket::connect(
  const SocketAddress& address, const ConnectParameters& connect_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto datagram_socket =
    ::socket(detail::address_type_to_protocol(address.type()), SOCK_DGRAM, 0);
  if (!detail::is_socket_valid(datagram_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  // Set non-blocking state later after we estabilish the connection.
  const auto setup_status = detail::setup_socket(datagram_socket, false, false, false);
  if (setup_status != Status::Ok) {
    detail::close_socket(datagram_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  int connect_status{detail::default_error_value};
  detail::AddressConverter::to_raw(address, [&](const sockaddr* sockaddr, socklen_t sockaddr_size) {
    connect_status =
      detail::retry_on_eintr([&] { return ::connect(datagram_socket, sockaddr, sockaddr_size); });
  });
  if (detail::is_error(connect_status)) {
    return std::unexpected{detail::get_last_error_and_close_socket(datagram_socket)};
  }

  if (connect_parameters.non_blocking) {
    const auto non_blocking_status = detail::set_socket_non_blocking(datagram_socket, true);
    if (non_blocking_status != Status::Ok) {
      detail::close_socket(datagram_socket);
      return std::unexpected{non_blocking_status};
    }
  }

  return DatagramSocket{datagram_socket};
}

Result<DatagramSocket> DatagramSocket::connect(
  IpVersion ip_version,
  std::string_view hostname,
  uint16_t port,
  const ConnectParameters& connect_parameters
) {
  return detail::resolve_and_run<Result<DatagramSocket>>(
    ip_version, hostname, port, [&](const SocketAddress& resolved_address) {
      return connect(resolved_address, connect_parameters);
    }
  );
}

Status DatagramSocket::set_broadcast_enabled(bool broadcast_enabled) {
  return detail::set_socket_option<int>(
    socket_, SOL_SOCKET, SO_BROADCAST, broadcast_enabled ? 1 : 0
  );
}

}  // namespace sock
