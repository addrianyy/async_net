#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <async_ws/Status.hpp>
#include <async_ws/WebSocketServer.hpp>

#include <async_net/TcpListener.hpp>

namespace async_ws {

class WebSocketClient;

namespace detail {

class WebSocketAcceptingClientImpl;

class WebSocketServerImpl {
  friend WebSocketAcceptingClientImpl;

  async_net::IoContext& context;
  async_net::TcpListener listener;

  WebSocketServer::State state_{WebSocketServer::State::Waiting};

  std::move_only_function<void()> on_listening;
  std::move_only_function<void(Status)> on_error;
  std::move_only_function<bool(std::string_view, async_net::SocketAddress)> on_connection_request;
  std::move_only_function<void(std::string_view, WebSocketClient)> on_client_connected;

  void cleanup_deferred(std::shared_ptr<WebSocketServerImpl> self);

 public:
  WebSocketServerImpl(async_net::IoContext& context, std::string hostname, uint16_t port);
  WebSocketServerImpl(async_net::IoContext& context, const async_net::SocketAddress& address);

  WebSocketServer::State state() const { return state_; }

  async_net::IoContext& io_context();
  const async_net::IoContext& io_context() const;

  void startup(std::shared_ptr<WebSocketServerImpl> self);
  void shutdown(std::shared_ptr<WebSocketServerImpl> self);

  void set_on_listening(std::move_only_function<void()> callback);
  void set_on_error(std::move_only_function<void(Status)> callback);
  void set_on_connection_request(
    std::move_only_function<bool(std::string_view, async_net::SocketAddress)> callback);
  void set_on_client_connected(
    std::move_only_function<void(std::string_view, WebSocketClient)> callback);
};

}  // namespace detail

}  // namespace async_ws
