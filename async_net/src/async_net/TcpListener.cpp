#include "TcpListener.hpp"
#include "IoContext.hpp"
#include "detail/SetCallback.hpp"
#include "detail/TcpListenerImpl.hpp"

namespace async_net {

TcpListener::TcpListener(IoContext& context, std::string hostname, uint16_t port)
    : impl_(std::make_shared<detail::TcpListenerImpl>(context)) {
  impl_->startup(impl_, std::move(hostname), port);
}

TcpListener::TcpListener(IoContext& context, const IpAddress& address, uint16_t port)
    : TcpListener(context, SocketAddress{address, port}) {}

TcpListener::TcpListener(IoContext& context, const SocketAddress& address)
    : impl_(std::make_shared<detail::TcpListenerImpl>(context)) {
  impl_->startup(impl_, address);
}

TcpListener::TcpListener(IoContext& context, uint16_t port)
    : TcpListener(context, SocketAddress{IpAddress::unspecified(), port}) {}

TcpListener::~TcpListener() {
  if (impl_) {
    impl_->shutdown(impl_);
  }
}

TcpListener::TcpListener(TcpListener&& other) noexcept {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
}

TcpListener& TcpListener::operator=(TcpListener&& other) noexcept {
  if (this != &other) {
    shutdown();

    impl_ = std::move(other.impl_);
    other.impl_ = nullptr;
  }
  return *this;
}

IoContext* TcpListener::io_context() {
  return impl_ ? &impl_->context : nullptr;
}
const IoContext* TcpListener::io_context() const {
  return impl_ ? &impl_->context : nullptr;
}

TcpListener::State TcpListener::state() const {
  return impl_ ? impl_->state : State::Shutdown;
}

bool TcpListener::accept_connections() const {
  return impl_ ? impl_->accept_connections : false;
}

void TcpListener::set_accept_connections(bool accept) const {
  if (impl_) {
    impl_->accept_connections = accept;
  }
}

void TcpListener::shutdown() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

void TcpListener::set_on_listening(std::function<void()> callback, bool instant) {
  SET_CALLBACK_SAFE(on_listening);
}

void TcpListener::set_on_error(std::function<void(Status)> callback, bool instant) {
  SET_CALLBACK_SAFE(on_error);
}

void TcpListener::set_on_accept(std::function<void(Status, TcpConnection)> callback, bool instant) {
  SET_CALLBACK_SAFE(on_accept);
}

}  // namespace async_net