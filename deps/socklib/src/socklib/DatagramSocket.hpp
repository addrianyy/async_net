#pragma once
#include "Socket.hpp"

#include <span>

namespace sock {

class DatagramSocket : public ReadWriteSocket {
  using ReadWriteSocket::ReadWriteSocket;

  Result<size_t> send_to_internal(
    const SocketAddress* to, std::span<const std::span<const uint8_t>> data
  );
  Result<size_t> receive_from_internal(
    SocketAddress* from, std::span<const std::span<uint8_t>> data
  );

 public:
  struct BindParameters {
    bool non_blocking = false;
    bool reuse_address = false;
    bool reuse_port = false;

    constexpr static BindParameters default_parameters() { return BindParameters{}; }
  };
  static Result<DatagramSocket> bind(
    const SocketAddress& address,
    const BindParameters& bind_parameters = BindParameters::default_parameters()
  );
  static Result<DatagramSocket> bind(
    IpVersion ip_version,
    std::string_view hostname,
    uint16_t port,
    const BindParameters& bind_parameters = BindParameters::default_parameters()
  );

  struct CreateParameters {
    bool non_blocking = false;

    constexpr static CreateParameters default_parameters() { return CreateParameters{}; }
  };
  static Result<DatagramSocket> create(
    SocketAddress::Type type,
    const CreateParameters& create_parameters = CreateParameters::default_parameters()
  );

  struct ConnectParameters {
    bool non_blocking = false;

    constexpr static ConnectParameters default_parameters() { return ConnectParameters{}; }
  };
  static Result<DatagramSocket> connect(
    const SocketAddress& address,
    const ConnectParameters& connect_parameters = ConnectParameters::default_parameters()
  );
  static Result<DatagramSocket> connect(
    IpVersion ip_version,
    std::string_view hostname,
    uint16_t port,
    const ConnectParameters& connect_parameters = ConnectParameters::default_parameters()
  );

  Status set_broadcast_enabled(bool broadcast_enabled);

  Result<size_t> send_to(const SocketAddress& to, std::span<const uint8_t> data) {
    return send_to_internal(&to, std::span{&data, 1});
  }
  Result<size_t> receive_from(SocketAddress& from, std::span<uint8_t> data) {
    return receive_from_internal(&from, std::span{&data, 1});
  }

  Result<size_t> send(std::span<const uint8_t> data) {
    return send_to_internal(nullptr, std::span{&data, 1});
  }
  Result<size_t> receive(std::span<uint8_t> data) {
    return receive_from_internal(nullptr, std::span{&data, 1});
  }

  Result<size_t> send_to(const SocketAddress& to, std::span<const std::span<const uint8_t>> data) {
    return send_to_internal(&to, data);
  }
  Result<size_t> receive_from(SocketAddress& from, std::span<const std::span<uint8_t>> data) {
    return receive_from_internal(&from, data);
  }

  Result<size_t> send(std::span<const std::span<const uint8_t>> data) {
    return send_to_internal(nullptr, data);
  }
  Result<size_t> receive(std::span<const std::span<uint8_t>> data) {
    return receive_from_internal(nullptr, data);
  }
};

}  // namespace sock
