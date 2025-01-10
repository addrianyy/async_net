#pragma once
#include "Status.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <async_net/IoContext.hpp>
#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>

#include <base/macro/ClassTraits.hpp>

namespace async_ws {

namespace detail {
class WebSocketServerImpl;
}  // namespace detail

class WebSocketClient;

class WebSocketServer {
  std::shared_ptr<detail::WebSocketServerImpl> impl_;

 public:
  enum class State {
    Waiting,
    Listening,
    Error,
    Shutdown,
  };

  CLASS_NON_COPYABLE(WebSocketServer)

  WebSocketServer() = default;

  WebSocketServer(async_net::IoContext& context, std::string hostname, uint16_t port);
  WebSocketServer(async_net::IoContext& context, const async_net::SocketAddress& address);
  WebSocketServer(async_net::IoContext& context,
                  const async_net::IpAddress& address,
                  uint16_t port);
  WebSocketServer(async_net::IoContext& context, uint16_t port);
  ~WebSocketServer();

  WebSocketServer(WebSocketServer&& other) noexcept;
  WebSocketServer& operator=(WebSocketServer&& other) noexcept;

  async_net::IoContext* io_context();
  const async_net::IoContext* io_context() const;

  bool valid() const { return impl_ != nullptr; }
  operator bool() const { return valid(); }

  State state() const;

  bool is_listening() const { return state() == State::Listening; }

  void shutdown();

  void set_on_listening(std::function<void()> callback, bool instant = false);
  void set_on_error(std::function<void(Status)> callback, bool instant = false);
  void set_on_client_connected(std::function<void(std::string_view, WebSocketClient)> callback,
                               bool instant = false);
};

}  // namespace async_ws