#include "WebSocketServer.hpp"
#include "detail/WebSocketServerImpl.hpp"

namespace async_ws {

WebSocketServer::WebSocketServer(async_net::IoContext& context, std::string hostname, uint16_t port)
    : impl_(std::make_shared<detail::WebSocketServerImpl>(context, std::move(hostname), port)) {
  impl_->startup(impl_);
}

WebSocketServer::WebSocketServer(async_net::IoContext& context,
                                 const async_net::SocketAddress& address)
    : impl_(std::make_shared<detail::WebSocketServerImpl>(context, address)) {
  impl_->startup(impl_);
}

WebSocketServer::WebSocketServer(async_net::IoContext& context,
                                 const async_net::IpAddress& address,
                                 uint16_t port)
    : WebSocketServer(context, async_net::SocketAddress{address, port}) {}

WebSocketServer::WebSocketServer(async_net::IoContext& context, uint16_t port)
    : WebSocketServer(context, async_net::IpAddress::unspecified(), port) {}

WebSocketServer::~WebSocketServer() {
  if (impl_) {
    impl_->shutdown(impl_);
  }
}

WebSocketServer::WebSocketServer(WebSocketServer&& other) noexcept {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
}

WebSocketServer& WebSocketServer::operator=(WebSocketServer&& other) noexcept {
  if (this != &other) {
    shutdown();

    impl_ = std::move(other.impl_);
    other.impl_ = nullptr;
  }
  return *this;
}

async_net::IoContext* WebSocketServer::io_context() {
  return impl_ ? &impl_->io_context() : nullptr;
}
const async_net::IoContext* WebSocketServer::io_context() const {
  return impl_ ? &impl_->io_context() : nullptr;
}

WebSocketServer::State WebSocketServer::state() const {
  return impl_ ? impl_->state() : State::Shutdown;
}

void WebSocketServer::shutdown() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

void WebSocketServer::set_on_listening(std::function<void()> callback) {
  if (impl_) {
    impl_->set_on_listening(std::move(callback));
  }
}

void WebSocketServer::set_on_error(std::function<void(Status)> callback) {
  if (impl_) {
    impl_->set_on_error(std::move(callback));
  }
}

void WebSocketServer::set_on_connection_request(
  std::function<bool(std::string_view, async_net::SocketAddress)> callback) {
  if (impl_) {
    impl_->set_on_connection_request(std::move(callback));
  }
}

void WebSocketServer::set_on_client_connected(
  std::function<void(std::string_view, WebSocketClient)> callback) {
  if (impl_) {
    impl_->set_on_client_connected(std::move(callback));
  }
}

}  // namespace async_ws
