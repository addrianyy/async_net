#pragma once
#include "StreamSocket.hpp"

#include <memory>

namespace sock {

namespace detail {

template <typename TConnectingSocket>
struct ConnectionSocketPair {
  TConnectingSocket connecting;
  StreamSocket connected;
};

}  // namespace detail

class ConnectingStreamSocket : public Socket {
  std::unique_ptr<uint64_t[]> socket_address{};
  size_t socket_address_size{};

  ConnectingStreamSocket(
    detail::RawSocket socket_,
    std::unique_ptr<uint64_t[]> socket_address,
    size_t socket_address_size
  );

 public:
  using SocketPair = detail::ConnectionSocketPair<ConnectingStreamSocket>;

  ConnectingStreamSocket() = default;

  ConnectingStreamSocket(const ConnectingStreamSocket& other) = delete;
  ConnectingStreamSocket& operator=(const ConnectingStreamSocket& other) = delete;

  ConnectingStreamSocket(ConnectingStreamSocket&& other) noexcept;
  ConnectingStreamSocket& operator=(ConnectingStreamSocket&& other) noexcept;

  struct ConnectParameters {
    constexpr static ConnectParameters default_parameters() { return ConnectParameters{}; }
  };

  static Result<SocketPair> initiate(
    const SocketAddress& address,
    const ConnectParameters& connect_parameters = ConnectParameters::default_parameters()
  );

  Result<StreamSocket> connect();
};

}  // namespace sock
