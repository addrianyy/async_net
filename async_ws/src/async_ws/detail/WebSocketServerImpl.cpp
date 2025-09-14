#include "WebSocketServerImpl.hpp"
#include "UpdateCallback.hpp"
#include "WebSocketAcceptingClientImpl.hpp"

#include <async_net/TcpConnection.hpp>

#include <base/Log.hpp>

namespace async_ws::detail {

void WebSocketServerImpl::cleanup_deferred(std::shared_ptr<WebSocketServerImpl> self) {
  // We don't want to clean up immediately because we may free ourselves if we are only referenced
  // by the callbacks.
  context.post([self = std::move(self)] {
    self->listener.shutdown();

    self->on_listening = nullptr;
    self->on_error = nullptr;
    self->on_connection_request = nullptr;
    self->on_client_connected = nullptr;
  });
}

WebSocketServerImpl::WebSocketServerImpl(async_net::IoContext& context,
                                         std::string hostname,
                                         uint16_t port)
    : context(context), listener(context, std::move(hostname), port) {}

WebSocketServerImpl::WebSocketServerImpl(async_net::IoContext& context,
                                         const async_net::SocketAddress& address)
    : context(context), listener(context, address) {}

async_net::IoContext& WebSocketServerImpl::io_context() {
  return context;
}

const async_net::IoContext& WebSocketServerImpl::io_context() const {
  return context;
}

void WebSocketServerImpl::startup(std::shared_ptr<WebSocketServerImpl> self) {
  listener.set_on_listening([self] {
    self->state_ = WebSocketServer::State::Listening;

    if (self->on_listening) {
      self->on_listening();
    }
  });

  listener.set_on_error([self](async_net::Status status) {
    if (self->state_ == WebSocketServer::State::Waiting ||
        self->state_ == WebSocketServer::State::Listening) {
      self->state_ = WebSocketServer::State::Error;

      const auto ws_status = Status{.error = Error::NetworkError, .net_status = status};

      if (self->on_error) {
        self->on_error(ws_status);
      } else {
        log_error("listening on WebSocket server: error {}", ws_status.stringify());
      }
    }

    self->cleanup_deferred(self);
  });

  listener.set_on_accept([self](async_net::Status status, async_net::TcpConnection connection) {
    if (status && self->on_client_connected) {
      auto client = std::make_shared<WebSocketAcceptingClientImpl>(self, std::move(connection));
      client->startup(client);
    }
  });
}

void WebSocketServerImpl::shutdown(std::shared_ptr<WebSocketServerImpl> self) {
  if (state_ != WebSocketServer::State::Shutdown) {
    const auto should_cleanup = state_ != WebSocketServer::State::Error;

    state_ = WebSocketServer::State::Shutdown;

    if (should_cleanup) {
      self->cleanup_deferred(std::move(self));
    }
  }
}

void WebSocketServerImpl::set_on_listening(std::move_only_function<void()> callback) {
  detail::update_callback(context, on_listening, std::move(callback));
}

void WebSocketServerImpl::set_on_error(std::move_only_function<void(Status)> callback) {
  detail::update_callback(context, on_error, std::move(callback));
}

void WebSocketServerImpl::set_on_connection_request(
  std::move_only_function<bool(std::string_view, async_net::SocketAddress)> callback) {
  detail::update_callback(context, on_connection_request, std::move(callback));
}

void WebSocketServerImpl::set_on_client_connected(
  std::move_only_function<void(std::string_view, WebSocketClient)> callback) {
  detail::update_callback(context, on_client_connected, std::move(callback));
}

}  // namespace async_ws::detail
