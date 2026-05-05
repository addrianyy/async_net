#pragma once
#include "StreamSocket.hpp"

namespace sock {

class Listener : public Socket {
  using Socket::Socket;

 public:
  struct BindParameters {
    bool non_blocking = false;
    bool reuse_address = false;
    bool reuse_port = false;
    uint32_t max_pending_connections = 16;

    constexpr static BindParameters default_parameters() { return BindParameters{}; }
  };
  static Result<Listener> bind(
    const SocketAddress& address,
    const BindParameters& bind_parameters = BindParameters::default_parameters()
  );
  static Result<Listener> bind(
    IpVersion ip_version,
    std::string_view hostname,
    uint16_t port,
    const BindParameters& bind_parameters = BindParameters::default_parameters()
  );

  Result<StreamSocket> accept(SocketAddress* peer_address = nullptr);
};

}  // namespace sock
