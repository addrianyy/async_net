#pragma once
#include "Socket.hpp"

#include <span>

namespace sock {

class StreamSocket : public ReadWriteSocket {
  friend class Listener;
  friend class ConnectingStreamSocket;
  friend class ConnectedPair;

  using ReadWriteSocket::ReadWriteSocket;

 public:
  struct ConnectParameters {
    bool non_blocking = false;

    constexpr static ConnectParameters default_parameters() { return ConnectParameters{}; }
  };
  static Result<StreamSocket> connect(
    const SocketAddress& address,
    const ConnectParameters& connect_parameters = ConnectParameters::default_parameters()
  );
  static Result<StreamSocket> connect(
    IpVersion ip_version,
    std::string_view hostname,
    uint16_t port,
    const ConnectParameters& connect_parameters = ConnectParameters::default_parameters()
  );

  Status peer_address(SocketAddress& address) const;

  template <typename T>
  Result<T> peer_address() const {
    T address{};
    const auto status = peer_address(address);
    return status == Status::Ok ? Result<T>{address} : std::unexpected{status};
  }

  Status set_keep_alive(bool keep_alive_enabled);
  Status set_no_delay(bool no_delay_enabled);

  Result<size_t> send(std::span<const uint8_t> data) { return send(std::span{&data, 1}); }
  Result<size_t> send_all(std::span<const uint8_t> data) { return send_all(std::span{&data, 1}); }

  Result<size_t> receive(std::span<uint8_t> data) { return receive(std::span{&data, 1}); }
  Result<size_t> receive_exact(std::span<uint8_t> data) {
    return receive_exact(std::span{&data, 1});
  }

  Result<size_t> send(std::span<const std::span<const uint8_t>> data);
  Result<size_t> send_all(std::span<const std::span<const uint8_t>> data);

  Result<size_t> receive(std::span<const std::span<uint8_t>> data);
  Result<size_t> receive_exact(std::span<const std::span<uint8_t>> data);
};

}  // namespace sock
