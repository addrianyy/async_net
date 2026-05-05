#include "ConnectingStreamSocket.hpp"
#include "private/AddressConversion.hpp"
#include "private/Initialization.hpp"
#include "private/SocketHelpers.hpp"

namespace sock {

ConnectingStreamSocket::ConnectingStreamSocket(
  detail::RawSocket socket_, std::unique_ptr<uint64_t[]> socket_address, size_t socket_address_size
)
    : Socket(socket_),
      socket_address(std::move(socket_address)),
      socket_address_size(socket_address_size) {}

ConnectingStreamSocket::ConnectingStreamSocket(ConnectingStreamSocket&& other) noexcept {
  socket_ = other.socket_;
  socket_address = std::move(other.socket_address);
  socket_address_size = other.socket_address_size;

  other.socket_ = detail::invalid_socket;
  other.socket_address = {};
  other.socket_address_size = 0;
}

ConnectingStreamSocket& ConnectingStreamSocket::operator=(ConnectingStreamSocket&& other) noexcept {
  if (this != &other) {
    detail::close_socket(socket_);

    socket_ = other.socket_;
    socket_address = std::move(other.socket_address);
    socket_address_size = other.socket_address_size;

    other.socket_ = detail::invalid_socket;
    other.socket_address = {};
    other.socket_address_size = 0;
  }

  return *this;
}

Result<ConnectingStreamSocket::SocketPair> ConnectingStreamSocket::initiate(
  const SocketAddress& address, const ConnectParameters& connect_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

  const auto connection_socket =
    ::socket(detail::address_type_to_protocol(address.type()), SOCK_STREAM, 0);
  if (!detail::is_socket_valid(connection_socket)) {
    return std::unexpected{detail::system_last_error_to_status()};
  }

  const auto setup_status = detail::setup_socket(connection_socket, false, false, true);
  if (setup_status != Status::Ok) {
    detail::close_socket(connection_socket);
    return std::unexpected{Status::SocketSetupFailed};
  }

  std::unique_ptr<uint64_t[]> socket_address_buffer;
  size_t socket_address_size{};

  detail::AddressConverter::to_raw(address, [&](const sockaddr* sockaddr, socklen_t sockaddr_size) {
    socket_address_buffer =
      std::make_unique<uint64_t[]>((sockaddr_size + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    std::memcpy(socket_address_buffer.get(), sockaddr, sockaddr_size);
    socket_address_size = size_t(sockaddr_size);
  });

  const int connect_status = detail::retry_on_eintr([&] {
    return ::connect(
      connection_socket,
      reinterpret_cast<sockaddr*>(socket_address_buffer.get()),
      int(socket_address_size)
    );
  });

  if (detail::is_error(connect_status)) {
    const auto status = detail::system_last_error_to_status();
    if (status != Status::WouldBlock && status != Status::AlreadyInProgress &&
        status != Status::NowInProgress) {
      detail::close_socket(connect_status);
      return std::unexpected{status};
    }

    return SocketPair{
      ConnectingStreamSocket{
        connection_socket, std::move(socket_address_buffer), socket_address_size
      },
      StreamSocket{}
    };
  }

  return SocketPair{ConnectingStreamSocket{}, StreamSocket{connection_socket}};
}

Result<StreamSocket> ConnectingStreamSocket::connect() {
  if (socket_ == detail::invalid_socket) {
    return std::unexpected{Status::NotConnected};
  }

  const int connect_status = detail::retry_on_eintr([&] {
    return ::connect(
      socket_, reinterpret_cast<sockaddr*>(socket_address.get()), int(socket_address_size)
    );
  });

  if (detail::is_error(connect_status)) {
    auto status = detail::system_last_error_to_status();
    if (status != Status::AlreadyConnected) {
      auto is_expected_error = status == Status::WouldBlock ||
                               status == Status::AlreadyInProgress ||
                               status == Status::NowInProgress;
#if defined(SOCKLIB_WINDOWS)
      // In order to preserve backward compatibility, this error is reported as WSAEINVAL to
      // Windows Sockets 1.1.
      is_expected_error |= status == Status::InvalidValue;
#endif

      if (is_expected_error) {
        status = Status::WouldBlock;
      }

      return std::unexpected{status};
    }
  }

  const auto connected_socket = socket_;

  socket_ = detail::invalid_socket;
  socket_address = {};
  socket_address_size = 0;

  return StreamSocket{connected_socket};
}

}  // namespace sock
