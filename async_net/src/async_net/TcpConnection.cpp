#include "TcpConnection.hpp"
#include "IoContext.hpp"
#include "detail/TcpConnectionImpl.hpp"
#include "detail/UpdateCallback.hpp"

namespace async_net {

base::BinaryBuffer* TcpConnection::acquire_send_buffer() {
  return impl_ ? &impl_->acquire_send_buffer() : nullptr;
}

TcpConnection::TcpConnection(IoContext& context, sock::StreamSocket socket)
    : impl_(std::make_shared<detail::TcpConnectionImpl>(context)) {
  impl_->startup(impl_, std::move(socket));
}

TcpConnection::TcpConnection(IoContext& context, std::string hostname, uint16_t port)
    : impl_(std::make_shared<detail::TcpConnectionImpl>(context)) {
  impl_->startup(impl_, std::move(hostname), port);
}

TcpConnection::TcpConnection(IoContext& context, const IpAddress& address, uint16_t port)
    : TcpConnection(context, SocketAddress{address, port}) {}

TcpConnection::TcpConnection(IoContext& context, const SocketAddress& address)
    : impl_(std::make_shared<detail::TcpConnectionImpl>(context)) {
  impl_->startup(impl_, address);
}

TcpConnection::TcpConnection(async_net::IoContext& context, std::vector<SocketAddress> addresses)
    : impl_(std::make_shared<detail::TcpConnectionImpl>(context)) {
  impl_->startup(impl_, std::move(addresses));
}

TcpConnection::~TcpConnection() {
  if (impl_) {
    impl_->shutdown(impl_);
  }
}

TcpConnection::TcpConnection(TcpConnection&& other) noexcept {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) noexcept {
  if (this != &other) {
    shutdown();

    impl_ = std::move(other.impl_);
    other.impl_ = nullptr;
  }
  return *this;
}

IoContext* TcpConnection::io_context() {
  return impl_ ? &impl_->context : nullptr;
}
const IoContext* TcpConnection::io_context() const {
  return impl_ ? &impl_->context : nullptr;
}

uint64_t TcpConnection::total_bytes_sent() const {
  return impl_ ? impl_->total_bytes_sent : 0;
}

uint64_t TcpConnection::total_bytes_received() const {
  return impl_ ? impl_->total_bytes_received : 0;
}

SocketAddress TcpConnection::local_address() const {
  return impl_ ? impl_->local_address : SocketAddress{};
}

SocketAddress TcpConnection::peer_address() const {
  return impl_ ? impl_->peer_addreess : SocketAddress{};
}

TcpConnection::State TcpConnection::state() const {
  return impl_ ? impl_->state : State::Shutdown;
}

size_t TcpConnection::send_buffer_remaining_size() const {
  return impl_ ? impl_->send_buffer_remaining_size() : 0;
}

size_t TcpConnection::pending_to_send() const {
  return impl_ ? impl_->send_buffer_size() : 0;
}

bool TcpConnection::is_send_buffer_empty() const {
  return impl_ ? impl_->send_buffer_size() == 0 : true;
}

bool TcpConnection::is_send_buffer_full() const {
  return impl_ ? impl_->send_buffer_size() >= impl_->send_buffer_max_size : true;
}

size_t TcpConnection::max_send_buffer_size() const {
  return impl_ ? impl_->send_buffer_max_size : 0;
}

void TcpConnection::set_max_send_buffer_size(size_t size) {
  if (impl_) {
    impl_->send_buffer_max_size = size;
  }
}

bool TcpConnection::block_on_send_buffer_full() const {
  return impl_ ? impl_->block_on_send_buffer_full : false;
}

void TcpConnection::set_block_on_send_buffer_full(bool block) {
  if (impl_) {
    impl_->block_on_send_buffer_full = block;
  }
}

bool TcpConnection::receive_packets() const {
  return impl_ ? impl_->receive_packets : false;
}

void TcpConnection::set_receive_packets(bool receive) const {
  if (impl_) {
    impl_->receive_packets = receive;
  }
}

bool TcpConnection::send_data(std::span<const uint8_t> data) {
  return send([&](base::BinaryBuffer& buffer) { buffer.append(data); });
}

bool TcpConnection::send_data_force(std::span<const uint8_t> data) {
  return send_force([&](base::BinaryBuffer& buffer) { buffer.append(data); });
}

void TcpConnection::shutdown() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

void TcpConnection::set_on_connection_succeeded(std::function<void()> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_connection_succeeded, std::move(callback));
  }
}
void TcpConnection::set_on_connection_failed(std::function<void(Status)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_connection_failed, std::move(callback));
  }
}

void TcpConnection::set_on_disconnected(std::function<void()> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_disconnected, std::move(callback));
  }
}
void TcpConnection::set_on_error(std::function<void(Status)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_error, std::move(callback));
  }
}

void TcpConnection::set_on_data_received(std::function<size_t(std::span<const uint8_t>)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_data_received, std::move(callback));
  }
}
void TcpConnection::set_on_data_sent(std::function<void()> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_data_sent, std::move(callback));
  }
}

}  // namespace async_net