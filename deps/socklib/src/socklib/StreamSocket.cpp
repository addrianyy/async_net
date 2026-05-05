#include "StreamSocket.hpp"
#include "private/AddressConversion.hpp"
#include "private/IO.hpp"
#include "private/Initialization.hpp"
#include "private/ResolveAndRun.hpp"
#include "private/SocketHelpers.hpp"
#include "private/System.hpp"

namespace sock {

template <typename Data, typename Fn>
void split_scatter_gather_data(std::span<const Data> data, Fn&& callback) {
  while (!data.empty()) {
    const auto group_size = std::min(detail::max_scatter_gather_entries, data.size());
    if (!callback(data.subspan(0, group_size))) {
      return;
    }
    data = data.subspan(group_size);
  }
}

template <typename Data, typename Fn>
Result<size_t> send_or_receive_sg(std::span<const std::span<Data>> data, Fn&& callback) {
  size_t total_bytes_processed{0};
  Result<size_t> result{0};

  split_scatter_gather_data(data, [&](std::span<const std::span<Data>> subdata) {
    size_t expected_size = 0;

    detail::ScatterGather::Entry sg_entries[detail::max_scatter_gather_entries];
    for (size_t i = 0; i < subdata.size(); ++i) {
      detail::ScatterGather::set_span(sg_entries[i], subdata[i]);
      expected_size += subdata[i].size();
    }

    if (expected_size == 0) {
      return true;
    }

    const auto r = callback(std::span{sg_entries}.subspan(0, subdata.size()));
    if (!r) {
      result = r;
      return false;
    }

    total_bytes_processed += *r;
    return true;
  });

  if (total_bytes_processed > 0) {
    return total_bytes_processed;
  }
  return result;
}

template <typename Data, typename Fn>
Result<size_t> send_or_receive_sg_exact(std::span<const std::span<Data>> data, Fn&& callback) {
  Result<size_t> result{0};

  split_scatter_gather_data(data, [&](std::span<const std::span<Data>> subdata) {
    size_t expected_size = 0;

    detail::ScatterGather::Entry sg_entries[detail::max_scatter_gather_entries];
    for (size_t i = 0; i < subdata.size(); ++i) {
      detail::ScatterGather::set_span(sg_entries[i], subdata[i]);
      expected_size += subdata[i].size();
    }

    if (expected_size == 0) {
      return true;
    }

    size_t current_sg_index = 0;
    size_t total_bytes_processed = 0;

    while (total_bytes_processed < expected_size) {
      const auto r = callback(
        std::span{sg_entries}.subspan(current_sg_index, subdata.size() - current_sg_index)
      );
      if (!r) {
        result = r;
        return false;
      }

      const auto processed_now = *r;

      *result += processed_now;
      total_bytes_processed += processed_now;

      if (total_bytes_processed != expected_size) {
        auto offset_remaining = processed_now;
        while (offset_remaining > 0) {
          const auto sg_size = detail::ScatterGather::get_size(sg_entries[current_sg_index]);
          const auto sg_offset_amount = std::min(sg_size, offset_remaining);

          offset_remaining -= sg_offset_amount;

          if (sg_size == sg_offset_amount) {
            current_sg_index++;
          } else {
            detail::ScatterGather::offset_by(sg_entries[current_sg_index], sg_offset_amount);
          }
        }
      }
    }

    return true;
  });

  return result;
}

Result<StreamSocket> StreamSocket::connect(
  const SocketAddress& address, const ConnectParameters& connect_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto connection_socket =
    ::socket(detail::address_type_to_protocol(address.type()), SOCK_STREAM, 0);
  if (!detail::is_socket_valid(connection_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  // Set non-blocking state later after we estabilish the connection.
  const auto setup_status = detail::setup_socket(connection_socket, false, false, false);
  if (setup_status != Status::Ok) {
    detail::close_socket(connection_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  int connect_status{detail::default_error_value};
  detail::AddressConverter::to_raw(address, [&](const sockaddr* sockaddr, socklen_t sockaddr_size) {
    connect_status =
      detail::retry_on_eintr([&] { return ::connect(connection_socket, sockaddr, sockaddr_size); });
  });
  if (detail::is_error(connect_status)) {
    return std::unexpected{detail::get_last_error_and_close_socket(connection_socket)};
  }

  if (connect_parameters.non_blocking) {
    const auto non_blocking_status = detail::set_socket_non_blocking(connection_socket, true);
    if (non_blocking_status != Status::Ok) {
      detail::close_socket(connection_socket);
      return std::unexpected{non_blocking_status};
    }
  }

  return StreamSocket{connection_socket};
}

Result<StreamSocket> StreamSocket::connect(
  IpVersion ip_version,
  std::string_view hostname,
  uint16_t port,
  const ConnectParameters& connect_parameters
) {
  return detail::resolve_and_run<Result<StreamSocket>>(
    ip_version, hostname, port, [&](const SocketAddress& resolved_address) {
      return connect(resolved_address, connect_parameters);
    }
  );
}

Status StreamSocket::peer_address(SocketAddress& address) const {
  detail::SockaddrBuffer socket_address;
  socklen_t sockaddr_size = sizeof(socket_address);

  if (detail::is_error(
        ::getpeername(socket_, reinterpret_cast<sockaddr*>(socket_address.data), &sockaddr_size)
      )) {
    return detail::system_last_error_to_status();
  }

  if (!detail::AddressConverter::from_raw(socket_address, sockaddr_size, address)) {
    return Status::AddressConversionFailed;
  }

  return Status::Ok;
}

Status StreamSocket::set_keep_alive(bool keep_alive_enabled) {
  return detail::set_socket_option<int>(
    socket_, SOL_SOCKET, SO_KEEPALIVE, keep_alive_enabled ? 1 : 0
  );
}

Status StreamSocket::set_no_delay(bool no_delay_enabled) {
  return detail::set_socket_option<int>(
    socket_, IPPROTO_TCP, TCP_NODELAY, no_delay_enabled ? 1 : 0
  );
}

Result<size_t> StreamSocket::send(std::span<const std::span<const uint8_t>> data) {
  if (data.size() == 1) {
    return detail::socket_send(socket_, data[0], nullptr, 0);
  }

  return send_or_receive_sg(data, [&](std::span<const detail::ScatterGather::Entry> sg) {
    return detail::socket_send_sg(socket_, sg, nullptr, 0);
  });
}

Result<size_t> StreamSocket::send_all(std::span<const std::span<const uint8_t>> data) {
  if (data.size() == 1) {
    const auto data_size = data[0].size();
    auto current = data[0].data();
    size_t bytes_sent = 0;

    while (bytes_sent < data_size) {
      const auto result =
        detail::socket_send(socket_, std::span{current, data_size - bytes_sent}, nullptr, 0);
      if (!result) {
        if (bytes_sent == 0) {
          return std::unexpected{result.error()};
        }
        return bytes_sent;
      }
      current += *result;
      bytes_sent += *result;
    }

    return bytes_sent;
  }

  return send_or_receive_sg_exact(data, [&](std::span<const detail::ScatterGather::Entry> sg) {
    return detail::socket_send_sg(socket_, sg, nullptr, 0);
  });
}

Result<size_t> StreamSocket::receive(std::span<const std::span<uint8_t>> data) {
  if (data.size() == 1) {
    return detail::socket_receive(socket_, data[0], nullptr, 0);
  }

  return send_or_receive_sg(data, [&](std::span<const detail::ScatterGather::Entry> sg) {
    return detail::socket_receive_sg(socket_, sg, nullptr, 0);
  });
}

Result<size_t> StreamSocket::receive_exact(std::span<const std::span<uint8_t>> data) {
  if (data.size() == 1) {
    const auto data_size = data[0].size();
    auto current = data[0].data();
    size_t bytes_received = 0;

    while (bytes_received < data_size) {
      const auto result =
        detail::socket_receive(socket_, std::span{current, data_size - bytes_received}, nullptr, 0);
      if (!result) {
        if (bytes_received == 0) {
          return std::unexpected{result.error()};
        }
        return bytes_received;
      }
      current += *result;
      bytes_received += *result;
    }

    return bytes_received;
  }

  return send_or_receive_sg_exact(data, [&](std::span<const detail::ScatterGather::Entry> sg) {
    return detail::socket_receive_sg(socket_, sg, nullptr, 0);
  });
}

}  // namespace sock
