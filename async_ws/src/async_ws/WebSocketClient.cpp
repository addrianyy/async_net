#include "WebSocketClient.hpp"
#include "detail/MaskingSettings.hpp"
#include "detail/WebSocketClientImpl.hpp"

namespace async_ws {

WebSocketClient::WebSocketClient(async_net::TcpConnection connection,
                                 detail::MaskingSettings masking_settings)
    : impl_(
        std::make_shared<detail::WebSocketClientImpl>(std::move(connection), masking_settings)) {
  impl_->startup(impl_);
}

WebSocketClient::WebSocketClient(async_net::IoContext& context,
                                 std::string hostname,
                                 uint16_t port,
                                 std::string uri)
    : impl_(std::make_shared<detail::WebSocketClientImpl>(
        async_net::TcpConnection{context, std::move(hostname), port},
        detail::MaskingSettings{})) {
  impl_->startup(impl_, std::move(uri));
}

WebSocketClient::~WebSocketClient() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

WebSocketClient::WebSocketClient(WebSocketClient&& other) noexcept {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
}

WebSocketClient& WebSocketClient::operator=(WebSocketClient&& other) noexcept {
  if (this != &other) {
    shutdown();

    impl_ = std::move(other.impl_);
    other.impl_ = nullptr;
  }
  return *this;
}

async_net::IoContext* WebSocketClient::io_context() {
  return impl_ ? &impl_->io_context() : nullptr;
}

const async_net::IoContext* WebSocketClient::io_context() const {
  return impl_ ? &impl_->io_context() : nullptr;
}

WebSocketClient::State WebSocketClient::state() const {
  return impl_ ? impl_->state() : State::Shutdown;
}

uint64_t WebSocketClient::total_bytes_sent() const {
  return impl_ ? impl_->total_bytes_sent() : 0;
}

uint64_t WebSocketClient::total_bytes_received() const {
  return impl_ ? impl_->total_bytes_received() : 0;
}

async_net::SocketAddress WebSocketClient::local_address() const {
  return impl_ ? impl_->local_address() : async_net::SocketAddress{};
}

async_net::SocketAddress WebSocketClient::peer_address() const {
  return impl_ ? impl_->peer_address() : async_net::SocketAddress{};
}

size_t WebSocketClient::send_buffer_remaining_size() const {
  return impl_ ? impl_->send_buffer_remaining_size() : 0;
}

size_t WebSocketClient::pending_to_send() const {
  return impl_ ? impl_->pending_to_send() : 0;
}

bool WebSocketClient::is_send_buffer_empty() const {
  return impl_ ? impl_->is_send_buffer_empty() : true;
}

bool WebSocketClient::is_send_buffer_full() const {
  return impl_ ? impl_->is_send_buffer_full() : true;
}

size_t WebSocketClient::max_send_buffer_size() const {
  return impl_ ? impl_->max_send_buffer_size() : 0;
}

void WebSocketClient::set_max_send_buffer_size(size_t size) {
  if (impl_) {
    impl_->set_max_send_buffer_size(size);
  }
}

bool WebSocketClient::block_on_send_buffer_full() const {
  return impl_ ? impl_->block_on_send_buffer_full() : false;
}

void WebSocketClient::set_block_on_send_buffer_full(bool block) {
  if (impl_) {
    impl_->set_block_on_send_buffer_full(block);
  }
}

bool WebSocketClient::receive_packets() const {
  return impl_ ? impl_->receive_packets() : false;
}

void WebSocketClient::set_receive_packets(bool receive) const {
  if (impl_) {
    impl_->set_receive_packets(receive);
  }
}

bool WebSocketClient::can_send_message(size_t size) const {
  return impl_ ? impl_->can_send_message(size) : false;
}

bool WebSocketClient::send_text_message(std::string_view payload) {
  return impl_ ? impl_->send_text_message(payload, false) : false;
}

bool WebSocketClient::send_binary_message(std::span<const uint8_t> payload) {
  return impl_ ? impl_->send_binary_message(payload, false) : false;
}

bool WebSocketClient::send_text_message_force(std::string_view payload) {
  return impl_ ? impl_->send_text_message(payload, true) : false;
}

bool WebSocketClient::send_binary_message_force(std::span<const uint8_t> payload) {
  return impl_ ? impl_->send_binary_message(payload, true) : false;
}

void WebSocketClient::send_ping() {
  if (impl_) {
    impl_->send_ping();
  }
}

void WebSocketClient::shutdown() {
  if (impl_) {
    impl_->shutdown(impl_);
    impl_ = nullptr;
  }
}

void WebSocketClient::set_on_connection_succeeded(std::function<void()> callback) {
  if (impl_) {
    impl_->set_on_connection_succeeded(std::move(callback));
  }
}
void WebSocketClient::set_on_connection_failed(std::function<void(Status)> callback) {
  if (impl_) {
    impl_->set_on_connection_failed(std::move(callback));
  }
}

void WebSocketClient::set_on_disconnected(std::function<void()> callback) {
  if (impl_) {
    impl_->set_on_disconnected(std::move(callback));
  }
}
void WebSocketClient::set_on_error(std::function<void(Status)> callback) {
  if (impl_) {
    impl_->set_on_error(std::move(callback));
  }
}

void WebSocketClient::set_on_text_message_received(std::function<void(std::string_view)> callback) {
  if (impl_) {
    impl_->set_on_text_message_received(std::move(callback));
  }
}
void WebSocketClient::set_on_binary_message_received(
  std::function<void(std::span<const uint8_t>)> callback) {
  if (impl_) {
    impl_->set_on_binary_message_received(std::move(callback));
  }
}

void WebSocketClient::set_on_data_sent(std::function<void()> callback) {
  if (impl_) {
    impl_->set_on_data_sent(std::move(callback));
  }
}

}  // namespace async_ws