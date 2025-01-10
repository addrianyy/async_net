#pragma once
#include <cstddef>
#include <memory>

#include <async_net/TcpConnection.hpp>
#include <async_net/Timer.hpp>

#include <websocket/Http.hpp>

namespace async_ws::detail {

class WebSocketServerImpl;

class WebSocketAcceptingClientImpl {
  async_net::IoContext& io_context;

  std::weak_ptr<WebSocketServerImpl> server;
  async_net::TcpConnection connection;

  async_net::Timer timeout;

  bool handle_handshake_request(const websocket::http::Request& request);

  size_t on_data_received(std::span<const uint8_t> data);

 public:
  explicit WebSocketAcceptingClientImpl(std::weak_ptr<WebSocketServerImpl> server,
                                        async_net::TcpConnection connection);

  void startup(std::shared_ptr<WebSocketAcceptingClientImpl> self);
};

}  // namespace async_ws::detail
