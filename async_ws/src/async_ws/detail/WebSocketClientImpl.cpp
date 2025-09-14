#include "WebSocketClientImpl.hpp"
#include "ConnectionTimeout.hpp"
#include "UpdateCallback.hpp"
#include "WebSocketConnectorImpl.hpp"
#include "WebSocketServerImpl.hpp"

#include <websocket/PacketSerialization.hpp>

#include <base/Log.hpp>
#include <base/Panic.hpp>

namespace async_ws::detail {

constexpr static size_t max_packet_size = 64 * 1024 * 1024;
constexpr static size_t max_message_size = 128 * 1024 * 1024;

std::optional<websocket::MaskingKey> WebSocketClientImpl::generate_masking_key_if_needed() {
  if (masking_settings.send_masked) {
    if (!mask_generator) {
      mask_generator = base::Xorshift{base::SeedFromSystemRng{}};
    }
    const auto value = mask_generator->gen();
    return websocket::MaskingKey{
      uint8_t(value >> 0),
      uint8_t(value >> 8),
      uint8_t(value >> 16),
      uint8_t(value >> 24),
    };
  }
  return {};
}

bool WebSocketClientImpl::try_send_ping_pong(bool ping) {
  return send_packet(
    websocket::Packet{
      .packet_type = ping ? websocket::PacketType::Ping : websocket::PacketType::Pong,
      .final = true,
      .masking_key = generate_masking_key_if_needed(),
    },
    {});
}

void WebSocketClientImpl::queue_ping_pong(bool ping) {
  if (!try_send_ping_pong(ping)) {
    request_tcp_data_sent_callback();

    if (ping) {
      pending_pings++;
    } else {
      pending_pongs++;
    }
  }
}

void WebSocketClientImpl::send_queued_ping_pongs() {
  if (pending_pings == 0 && pending_pongs == 0) {
    return;
  }

  const auto pings_to_send = pending_pings;
  const auto pongs_to_send = pending_pongs;

  // First send pongs to make sure that peer won't drop the connection.
  for (uint64_t i = 0; i < pongs_to_send; ++i) {
    if (!try_send_ping_pong(false)) {
      return;
    }
    pending_pongs--;
  }

  // Then pings.
  for (uint64_t i = 0; i < pings_to_send; ++i) {
    if (!try_send_ping_pong(true)) {
      return;
    }
    pending_pings--;
  }
}

bool WebSocketClientImpl::send_packet(const websocket::Packet& packet,
                                      std::span<const uint8_t> payload,
                                      bool force) {
  if (force) {
    return connection.send_force([&](base::BinaryBuffer& buffer) {
      websocket::Serializer::serialize(packet, payload, buffer);
    });
  } else {
    if (websocket::Serializer::serialized_packet_size(payload.size(),
                                                      packet.masking_key.has_value()) >
        connection.send_buffer_remaining_size()) {
      return false;
    }

    return connection.send([&](base::BinaryBuffer& buffer) {
      websocket::Serializer::serialize(packet, payload, buffer);
    });
  }
}

void WebSocketClientImpl::dispatch_message(std::span<const uint8_t> payload) {
  switch (receive_state) {
    case ReceiveState::ReceivingTextFrame: {
      if (on_text_message_received) {
        on_text_message_received(
          std::string_view{reinterpret_cast<const char*>(payload.data()), payload.size()});
      } else {
        log_warn("WebSocketClient: ignored text message");
      }
      break;
    }

    case ReceiveState::ReceivingBinaryFrame: {
      if (on_binary_message_received) {
        on_binary_message_received(payload);
      } else {
        log_warn("WebSocketClient: ignored binary message");
      }
      break;
    }

    default:
      panic_unreachable();
  }

  receive_state = ReceiveState::Idle;
}

bool WebSocketClientImpl::handle_websocket_message_payload(const websocket::Packet& packet,
                                                           std::span<const uint8_t> payload) {
  // Fast path for single, non-fragmented messages without masking.
  if (packet.final && !packet.masking_key && message_buffer.empty()) {
    dispatch_message(payload);
    return true;
  }

  if (message_buffer.size() + payload.size() > max_message_size) {
    on_ws_error({.error = Error::MessageTooLarge});
    return false;
  }

  const auto size_before = message_buffer.size();
  message_buffer.append(payload);
  if (packet.masking_key) {
    websocket::mask_packet_payload(*packet.masking_key,
                                   message_buffer.span().subspan(size_before, payload.size()));
  }

  if (packet.final) {
    dispatch_message(message_buffer);
    message_buffer.clear();
  }

  return true;
}

bool WebSocketClientImpl::handle_websocket_packet(const websocket::Packet& packet,
                                                  std::span<const uint8_t> payload) {
  if (state_ != WebSocketClient::State::Connected) {
    return true;
  }

  if (masking_settings.receive_masked != packet.masking_key.has_value()) {
    on_ws_error({.error = Error::MaskingViolation});
    return false;
  }

  switch (packet.packet_type) {
    case websocket::PacketType::ContinuationFrame: {
      if (receive_state == ReceiveState::ReceivingTextFrame ||
          receive_state == ReceiveState::ReceivingBinaryFrame) {
        return handle_websocket_message_payload(packet, payload);
      } else {
        on_ws_error({.error = Error::UnexpectedPacket});
        return false;
      }
    }

    case websocket::PacketType::TextFrame: {
      if (receive_state == ReceiveState::Idle) {
        receive_state = ReceiveState::ReceivingTextFrame;
        return handle_websocket_message_payload(packet, payload);
      } else {
        on_ws_error({.error = Error::UnexpectedPacket});
        return false;
      }
    }

    case websocket::PacketType::BinaryFrame: {
      if (receive_state == ReceiveState::Idle) {
        receive_state = ReceiveState::ReceivingBinaryFrame;
        return handle_websocket_message_payload(packet, payload);
      } else {
        on_ws_error({.error = Error::UnexpectedPacket});
        return false;
      }
    }

    case websocket::PacketType::ConnectionClose: {
      if (!packet.final) {
        on_ws_error({.error = Error::FragmentedControlPacket});
        return false;
      }

      // Echo the packet.
      {
        base::BinaryBuffer buffer{payload};

        // Unmask.
        if (packet.masking_key) {
          websocket::mask_packet_payload(*packet.masking_key, buffer);
        }

        auto echo_packet = packet;
        echo_packet.masking_key = generate_masking_key_if_needed();

        // Remask.
        if (echo_packet.masking_key) {
          websocket::mask_packet_payload(*echo_packet.masking_key, buffer);
        }

        send_packet(echo_packet, buffer);
      }

      on_ws_disconnected();

      return true;
    }

    case websocket::PacketType::Ping: {
      if (!packet.final) {
        on_ws_error({.error = Error::FragmentedControlPacket});
        return false;
      }

      queue_ping_pong(false);

      return true;
    }

    case websocket::PacketType::Pong: {
      if (!packet.final) {
        on_ws_error({.error = Error::FragmentedControlPacket});
        return false;
      }

      // Ignore the pongs.

      return true;
    }

    default:
      panic_unreachable();
  }
}

size_t WebSocketClientImpl::handle_websocket_data(std::span<const uint8_t> data) {
  if (pending_packet) {
    const auto& pending = *pending_packet;

    const auto packet_total_size = pending.total_size;
    if (packet_total_size > data.size()) {
      return 0;
    }

    const auto payload =
      data.subspan(pending.header_size, pending.total_size - pending.header_size);
    const auto result = handle_websocket_packet(pending_packet->packet, payload);

    pending_packet = {};

    if (!result) {
      return data.size();
    }

    return packet_total_size;
  }

  const auto result = websocket::Deserializer::deserialize(data);
  switch (result.error) {
    case websocket::Deserializer::Error::NeedMoreData: {
      return 0;
    }

    case websocket::Deserializer::Error::Ok: {
      const auto packet_total_size = result.header_size + result.packet.payload_size;
      if (packet_total_size > max_packet_size) {
        on_ws_error(Status{.error = Error::PacketTooLarge});
        return data.size();
      }

      if (packet_total_size > data.size()) {
        pending_packet = {
          .packet = result.packet,
          .header_size = result.header_size,
          .total_size = packet_total_size,
        };

        return 0;
      } else {
        const auto payload = data.subspan(result.header_size, result.packet.payload_size);
        if (!handle_websocket_packet(result.packet, payload)) {
          return data.size();
        }

        return packet_total_size;
      }
    }

    default: {
      on_ws_error(Status{.error = Error::CannotDeserializePacket});
      return data.size();
    }
  }
}

std::pair<size_t, bool> WebSocketClientImpl::handle_connector_data(std::span<const uint8_t> data) {
  const auto result = connector->on_data_received(data);
  if (result.error != Error::Ok) {
    on_ws_error({.error = result.error});
    return {data.size(), false};
  }

  if (result.finished) {
    on_connector_succeeded(result.negotiated_masking_settings);
    return {result.bytes_consumed, true};
  }

  return {result.bytes_consumed, false};
}

size_t WebSocketClientImpl::on_data_received(std::span<const uint8_t> data) {
  size_t total_consumed = 0;

  if (connector) {
    const auto [consumed, estabilished] = handle_connector_data(data);
    if (estabilished) {
      total_consumed += consumed;
      data = data.subspan(consumed);
    } else {
      return consumed;
    }
  }

  while (!data.empty()) {
    const auto consumed = handle_websocket_data(data);
    if (consumed == 0) {
      break;
    }
    total_consumed += consumed;
    data = data.subspan(consumed);
  }

  return total_consumed;
}

void WebSocketClientImpl::on_ws_disconnected() {
  connector = nullptr;

  if (state_ == WebSocketClient::State::Connecting) {
    state_ = WebSocketClient::State::Error;

    const Status ws_status{.error = Error::DisconnectedDuringHandshake};
    if (on_connected) {
      on_connected(ws_status);
    } else {
      log_error("WebSocket client: disconnected during handshake");
    }
  } else if (state_ == WebSocketClient::State::Connected) {
    state_ = WebSocketClient::State::Disconnected;

    if (on_closed) {
      on_closed({});
    }
  }

  cleanup();
}

void WebSocketClientImpl::on_ws_error(Status status) {
  connector = nullptr;

  if (state_ == WebSocketClient::State::Connecting) {
    state_ = WebSocketClient::State::Error;

    if (on_connected) {
      on_connected(status);
    } else {
      log_error("WebSocket client: error during connecting: {}", status.stringify());
    }
  } else if (state_ == WebSocketClient::State::Connected) {
    state_ = WebSocketClient::State::Error;

    if (on_closed) {
      on_closed(status);
    } else {
      log_error("WebSocket client: error: {}", status.stringify());
    }
  }

  cleanup();
}

void WebSocketClientImpl::on_tcp_connected(async_net::Status status) {
  if (state_ == WebSocketClient::State::Connecting) {
    if (status) {
      if (!connector || !connector->send_request(connection)) {
        on_ws_error({.error = Error::FailedToSendRequest});
      }
    } else {
      on_ws_error({.error = Error::NetworkError, .net_status = status});
    }
  }
}

void WebSocketClientImpl::on_tcp_closed(async_net::Status status) {
  if (status) {
    on_ws_disconnected();
  } else {
    on_ws_error({.error = Error::NetworkError, .net_status = status});
  }
}

void WebSocketClientImpl::on_tcp_data_sent() {
  if (state_ == WebSocketClient::State::Connected) {
    send_queued_ping_pongs();

    if (on_data_sent) {
      on_data_sent();
    }
  }
}

void WebSocketClientImpl::on_connector_succeeded(
  const MaskingSettings& negotiated_masking_settings) {
  masking_settings = negotiated_masking_settings;
  connector = nullptr;
  state_ = WebSocketClient::State::Connected;

  if (on_connected) {
    on_connected({});
  }
}

void WebSocketClientImpl::cleanup() {
  can_register_data_sent_callback = false;

  // We don't want to clean up immediately because we may free ourselves if we are only referenced
  // by the callbacks.
  context.post([self = shared_from_this()] { self->cleanup_immediate(); });
}

void WebSocketClientImpl::cleanup_immediate() {
  can_register_data_sent_callback = false;

  connection.shutdown();
  connector = nullptr;

  on_connected = nullptr;
  on_closed = nullptr;
  on_text_message_received = nullptr;
  on_binary_message_received = nullptr;
  on_data_sent = nullptr;
}

void WebSocketClientImpl::close_and_cleanup_immediate(bool is_connected) {
  if (is_connected) {
    (void)send_packet(
      websocket::Packet{
        .packet_type = websocket::PacketType::ConnectionClose,
        .final = true,
        .masking_key = generate_masking_key_if_needed(),
      },
      {});
  }

  cleanup_immediate();
}

void WebSocketClientImpl::request_tcp_data_sent_callback() {
  // Lazily set data sent callback to improve performance when it's not used.
  if (!is_data_sent_callback_registered && can_register_data_sent_callback) {
    auto self = shared_from_this();
    connection.set_on_data_sent([self] { return self->on_tcp_data_sent(); });

    is_data_sent_callback_registered = true;
  }
}

WebSocketClientImpl::WebSocketClientImpl(async_net::TcpConnection connection,
                                         MaskingSettings masking_settings)
    : context(*connection.io_context()),
      connection(std::move(connection)),
      masking_settings(masking_settings) {}
WebSocketClientImpl::~WebSocketClientImpl() = default;

async_net::IoContext& WebSocketClientImpl::io_context() {
  return context;
}
const async_net::IoContext& WebSocketClientImpl::io_context() const {
  return context;
}

uint64_t WebSocketClientImpl::total_bytes_sent() const {
  return connection.total_bytes_sent();
}

uint64_t WebSocketClientImpl::total_bytes_received() const {
  return connection.total_bytes_received();
}

async_net::SocketAddress WebSocketClientImpl::local_address() const {
  return connection.local_address();
}

async_net::SocketAddress WebSocketClientImpl::peer_address() const {
  return connection.peer_address();
}

size_t WebSocketClientImpl::send_buffer_remaining_size() const {
  return connection.send_buffer_remaining_size();
}

size_t WebSocketClientImpl::pending_to_send() const {
  return connection.pending_to_send();
}

bool WebSocketClientImpl::is_send_buffer_empty() const {
  return connection.is_send_buffer_empty();
}

bool WebSocketClientImpl::is_send_buffer_full() const {
  return connection.is_send_buffer_full();
}

size_t WebSocketClientImpl::max_send_buffer_size() const {
  return connection.max_send_buffer_size();
}

void WebSocketClientImpl::set_max_send_buffer_size(size_t size) {
  connection.set_max_send_buffer_size(size);
}

bool WebSocketClientImpl::block_on_send_buffer_full() const {
  return connection.block_on_send_buffer_full();
}

void WebSocketClientImpl::set_block_on_send_buffer_full(bool block) {
  connection.set_block_on_send_buffer_full(block);
}

bool WebSocketClientImpl::receive_packets() const {
  return connection.receive_packets();
}

void WebSocketClientImpl::set_receive_packets(bool receive) const {
  connection.set_receive_packets(receive);
}

bool WebSocketClientImpl::can_send_message(size_t size) const {
  if (state_ != WebSocketClient::State::Connected) {
    return false;
  }

  return connection.send_buffer_remaining_size() >=
         websocket::Serializer::serialized_packet_size(size, masking_settings.send_masked);
}

bool WebSocketClientImpl::send_text_message(std::string_view payload, bool force) {
  if (state_ != WebSocketClient::State::Connected) {
    return false;
  }

  return send_packet(
    websocket::Packet{
      .packet_type = websocket::PacketType::TextFrame,
      .final = true,
      .masking_key = generate_masking_key_if_needed(),
    },
    std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
    force);
}

bool WebSocketClientImpl::send_binary_message(std::span<const uint8_t> payload, bool force) {
  if (state_ != WebSocketClient::State::Connected) {
    return false;
  }

  return send_packet(
    websocket::Packet{
      .packet_type = websocket::PacketType::BinaryFrame,
      .final = true,
      .masking_key = generate_masking_key_if_needed(),
    },
    payload, force);
}

void WebSocketClientImpl::send_ping() {
  if (state_ != WebSocketClient::State::Connected) {
    return;
  }

  queue_ping_pong(true);
}

void WebSocketClientImpl::startup(std::shared_ptr<WebSocketClientImpl> self) {
  state_ = WebSocketClient::State::Connected;

  connection.set_on_closed(
    [self](async_net::Status status) { return self->on_tcp_closed(status); });

  connection.set_on_data_received(
    [self](std::span<const uint8_t> data) { return self->on_data_received(data); });
}

void WebSocketClientImpl::startup(std::shared_ptr<WebSocketClientImpl> self, std::string uri) {
  state_ = WebSocketClient::State::Connecting;

  async_net::Timer timeout;
  {
    std::weak_ptr<WebSocketClientImpl> selfW = self;
    timeout = async_net::Timer::invoke_after(*connection.io_context(), connection_timeout, [selfW] {
      if (auto selfS = selfW.lock()) {
        if (selfS->state_ == WebSocketClient::State::Connecting) {
          selfS->on_ws_error({.error = Error::Timeout});
        }
      }
    });
  }

  connector = std::make_unique<WebSocketConnectorImpl>(std::move(timeout), std::move(uri));

  connection.set_on_connected(
    [self](async_net::Status status) { return self->on_tcp_connected(status); });
  connection.set_on_closed(
    [self](async_net::Status status) { return self->on_tcp_closed(status); });

  connection.set_on_data_received(
    [self](std::span<const uint8_t> data) { return self->on_data_received(data); });
}

void WebSocketClientImpl::shutdown(std::shared_ptr<WebSocketClientImpl> self) {
  if (state_ != WebSocketClient::State::Shutdown) {
    const auto should_cleanup =
      state_ != WebSocketClient::State::Error && state_ != WebSocketClient::State::Disconnected;
    const auto is_connected = state_ == WebSocketClient::State::Connected;

    state_ = WebSocketClient::State::Shutdown;

    if (should_cleanup) {
      context.post([self = std::move(self), is_connected] {
        self->close_and_cleanup_immediate(is_connected);
      });
    }
  }
}

void WebSocketClientImpl::set_on_connected(std::move_only_function<void(Status)> callback) {
  detail::update_callback(context, on_connected, std::move(callback));
}

void WebSocketClientImpl::set_on_closed(std::move_only_function<void(Status)> callback) {
  detail::update_callback(context, on_closed, std::move(callback));
}

void WebSocketClientImpl::set_on_text_message_received(
  std::move_only_function<void(std::string_view)> callback) {
  detail::update_callback(context, on_text_message_received, std::move(callback));
}
void WebSocketClientImpl::set_on_binary_message_received(
  std::move_only_function<void(std::span<const uint8_t>)> callback) {
  detail::update_callback(context, on_binary_message_received, std::move(callback));
}

void WebSocketClientImpl::set_on_data_sent(std::move_only_function<void()> callback) {
  const auto is_callback_present = callback != nullptr;
  detail::update_callback(context, on_data_sent, std::move(callback));
  if (is_callback_present) {
    request_tcp_data_sent_callback();
  }
}

}  // namespace async_ws::detail
