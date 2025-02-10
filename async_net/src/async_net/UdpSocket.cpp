#include "UdpSocket.hpp"
#include "detail/UdpSocketImpl.hpp"
#include "detail/UpdateCallback.hpp"

namespace async_net {

UdpSocket::UdpSocket(IoContext& context,
                     std::string hostname,
                     uint16_t port,
                     BindParameters parameters)
    : impl_(std::make_shared<detail::UdpSocketImpl>(context)) {
  impl_->startup(impl_, std::move(hostname), port, parameters);
}

UdpSocket::UdpSocket(IoContext& context,
                     const IpAddress& address,
                     uint16_t port,
                     BindParameters parameters)
    : UdpSocket(context, SocketAddress{address, port}, parameters) {}

UdpSocket::UdpSocket(IoContext& context, const SocketAddress& address, BindParameters parameters)
    : impl_(std::make_shared<detail::UdpSocketImpl>(context)) {
  impl_->startup(impl_, address, parameters);
}

UdpSocket::UdpSocket(IoContext& context,
                     std::vector<SocketAddress> addresses,
                     BindParameters parameters)
    : impl_(std::make_shared<detail::UdpSocketImpl>(context)) {
  impl_->startup(impl_, std::move(addresses), parameters);
}

UdpSocket::UdpSocket(IoContext& context, uint16_t port, UdpSocket::BindParameters parameters)
    : UdpSocket(context, SocketAddress{IpAddress::unspecified(), port}, parameters) {}

UdpSocket::UdpSocket(IoContext& context, BindParameters parameters)
    : impl_(std::make_shared<detail::UdpSocketImpl>(context)) {
  impl_->startup(impl_, parameters);
}

UdpSocket::~UdpSocket() {
  if (impl_) {
    impl_->shutdown(impl_);
  }
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
  if (this != &other) {
    shutdown();

    impl_ = std::move(other.impl_);
    other.impl_ = nullptr;
  }
  return *this;
}

IoContext* UdpSocket::io_context() {
  return impl_ ? &impl_->context : nullptr;
}
const IoContext* UdpSocket::io_context() const {
  return impl_ ? &impl_->context : nullptr;
}

uint64_t UdpSocket::total_bytes_sent() const {
  return impl_ ? impl_->total_bytes_sent : 0;
}

uint64_t UdpSocket::total_bytes_received() const {
  return impl_ ? impl_->total_bytes_received : 0;
}

SocketAddress UdpSocket::local_address() const {
  return impl_ ? impl_->local_address : SocketAddress{};
}

UdpSocket::State UdpSocket::state() const {
  return impl_ ? impl_->state : State::Shutdown;
}

size_t UdpSocket::send_buffer_remaining_size() const {
  return impl_ ? impl_->send_buffer_remaining_size() : 0;
}

size_t UdpSocket::pending_to_send() const {
  return impl_ ? impl_->send_buffer_size() : 0;
}

bool UdpSocket::is_send_buffer_empty() const {
  return impl_ ? impl_->send_entries.empty() : true;
}

bool UdpSocket::is_send_buffer_full() const {
  return impl_ ? impl_->is_send_buffer_full() : true;
}

size_t UdpSocket::max_send_buffer_size() const {
  return impl_ ? impl_->send_buffer_max_size : 0;
}

void UdpSocket::set_max_send_buffer_size(size_t size) {
  if (impl_) {
    impl_->send_buffer_max_size = size;
  }
}

bool UdpSocket::block_on_send_buffer_full() const {
  return impl_ ? impl_->block_on_send_buffer_full : false;
}

void UdpSocket::set_block_on_send_buffer_full(bool block) {
  if (impl_) {
    impl_->block_on_send_buffer_full = block;
  }
}

bool UdpSocket::receive_packets() const {
  return impl_ ? impl_->receive_packets : false;
}

void UdpSocket::set_receive_packets(bool receive) const {
  if (impl_) {
    impl_->receive_packets = receive;
  }
}

bool UdpSocket::send_data(const SocketAddress& destination, std::span<const uint8_t> data) {
  return impl_ ? impl_->send_data(destination, data) : false;
}

void UdpSocket::shutdown() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

void UdpSocket::set_on_bound(std::function<void(Status)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_bound, std::move(callback));
  }
}

void UdpSocket::set_on_closed(std::function<void(Status)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_closed, std::move(callback));
  }
}

void UdpSocket::set_on_data_received(
  std::function<void(const SocketAddress&, std::span<const uint8_t>)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_data_received, std::move(callback));
  }
}

void UdpSocket::set_on_data_sent(std::function<void()> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_data_sent, std::move(callback));
  }
}

void UdpSocket::set_on_send_error(std::function<void(Status)> callback) {
  if (impl_) {
    detail::update_callback(impl_->context, impl_->on_send_error, std::move(callback));
  }
}

}  // namespace async_net