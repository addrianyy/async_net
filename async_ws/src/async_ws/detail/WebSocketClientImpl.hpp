#pragma once
#include "MaskingSettings.hpp"

#include <async_ws/Status.hpp>
#include <async_ws/WebSocketClient.hpp>

#include <async_net/TcpConnection.hpp>

#include <websocket/Http.hpp>
#include <websocket/Packet.hpp>

#include <base/rng/Xorshift.hpp>

#include <memory>
#include <optional>
#include <utility>

namespace async_ws::detail {

class WebSocketConnectorImpl;

class WebSocketClientImpl : public std::enable_shared_from_this<WebSocketClientImpl> {
  async_net::IoContext& context;

  async_net::TcpConnection connection;
  std::unique_ptr<WebSocketConnectorImpl> connector;

  MaskingSettings masking_settings{};
  std::optional<base::Xorshift> mask_generator;

  WebSocketClient::State state_{WebSocketClient::State::Connecting};

  struct PendingPacket {
    websocket::Packet packet;
    size_t header_size{};
    size_t total_size{};
  };
  std::optional<PendingPacket> pending_packet;

  enum class ReceiveState {
    Idle,
    ReceivingTextFrame,
    ReceivingBinaryFrame,
  };
  ReceiveState receive_state{ReceiveState::Idle};

  base::BinaryBuffer message_buffer;

  bool can_register_data_sent_callback{true};
  bool is_data_sent_callback_registered{false};

  uint64_t pending_pings{};
  uint64_t pending_pongs{};

  std::function<void(Status)> on_connected;
  std::function<void(Status)> on_closed;
  std::function<void(std::string_view)> on_text_message_received;
  std::function<void(std::span<const uint8_t>)> on_binary_message_received;
  std::function<void()> on_data_sent;

  std::optional<websocket::MaskingKey> generate_masking_key_if_needed();

  bool try_send_ping_pong(bool ping);
  void queue_ping_pong(bool ping);
  void send_queued_ping_pongs();

  bool send_packet(const websocket::Packet& packet,
                   std::span<const uint8_t> payload,
                   bool force = false);
  void dispatch_message(std::span<const uint8_t> payload);

  bool handle_websocket_message_payload(const websocket::Packet& packet,
                                        std::span<const uint8_t> payload);

  bool handle_websocket_packet(const websocket::Packet& packet, std::span<const uint8_t> payload);
  size_t handle_websocket_data(std::span<const uint8_t> data);

  std::pair<size_t, bool> handle_connector_data(std::span<const uint8_t> data);

  size_t on_data_received(std::span<const uint8_t> data);

  void on_ws_disconnected();
  void on_ws_error(Status status);

  void on_tcp_connected(async_net::Status status);
  void on_tcp_closed(async_net::Status status);

  void on_tcp_data_sent();

  void on_connector_succeeded(const MaskingSettings& negotiated_masking_settings);

  void cleanup();

  void cleanup_immediate();
  void close_and_cleanup_immediate(bool is_connected);

  void request_tcp_data_sent_callback();

 public:
  explicit WebSocketClientImpl(async_net::TcpConnection connection,
                               MaskingSettings masking_settings);
  ~WebSocketClientImpl();

  WebSocketClient::State state() const { return state_; }

  async_net::IoContext& io_context();
  const async_net::IoContext& io_context() const;

  uint64_t total_bytes_sent() const;
  uint64_t total_bytes_received() const;
  async_net::SocketAddress local_address() const;
  async_net::SocketAddress peer_address() const;

  size_t send_buffer_remaining_size() const;
  size_t pending_to_send() const;
  bool is_send_buffer_empty() const;
  bool is_send_buffer_full() const;

  size_t max_send_buffer_size() const;
  void set_max_send_buffer_size(size_t size);

  bool block_on_send_buffer_full() const;
  void set_block_on_send_buffer_full(bool block);

  bool receive_packets() const;
  void set_receive_packets(bool receive) const;

  bool can_send_message(size_t size) const;

  bool send_text_message(std::string_view payload, bool force);
  bool send_binary_message(std::span<const uint8_t> payload, bool force);

  void send_ping();

  void startup(std::shared_ptr<WebSocketClientImpl> self);
  void startup(std::shared_ptr<WebSocketClientImpl> self, std::string uri);
  void shutdown(std::shared_ptr<WebSocketClientImpl> self);

  void set_on_connected(std::function<void(Status)> callback);
  void set_on_closed(std::function<void(Status)> callback);

  void set_on_text_message_received(std::function<void(std::string_view)> callback);
  void set_on_binary_message_received(std::function<void(std::span<const uint8_t>)> callback);

  void set_on_data_sent(std::function<void()> callback);
};

}  // namespace async_ws::detail
