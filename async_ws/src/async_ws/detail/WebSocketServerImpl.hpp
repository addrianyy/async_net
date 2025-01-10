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

  std::function<void()> on_listening;
  std::function<void(Status)> on_error;
  std::function<void(std::string_view, WebSocketClient)> on_client_connected;

  void cleanup_deferred(std::shared_ptr<WebSocketServerImpl> self);

 public:
  WebSocketServerImpl(async_net::IoContext& context, std::string hostname, uint16_t port);
  WebSocketServerImpl(async_net::IoContext& context, const async_net::SocketAddress& address);

  WebSocketServer::State state() const { return state_; }

  async_net::IoContext& io_context();
  const async_net::IoContext& io_context() const;

  void startup(std::shared_ptr<WebSocketServerImpl> self);
  void shutdown(std::shared_ptr<WebSocketServerImpl> self);

  void set_on_listening(const std::shared_ptr<WebSocketServerImpl>& self,
                        std::function<void()> callback,
                        bool instant);
  void set_on_error(const std::shared_ptr<WebSocketServerImpl>& self,
                    std::function<void(Status)> callback,
                    bool instant);
  void set_on_client_connected(const std::shared_ptr<WebSocketServerImpl>& self,
                               std::function<void(std::string_view, WebSocketClient)> callback,
                               bool instant);
};

}  // namespace detail

}  // namespace async_ws