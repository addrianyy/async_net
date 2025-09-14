#pragma once
#include "Status.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>

#include <async_net/IoContext.hpp>
#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>

#include <base/macro/ClassTraits.hpp>

namespace async_net {
class TcpConnection;
}

namespace async_ws {

namespace detail {
class WebSocketClientImpl;
class WebSocketAcceptingClientImpl;
struct MaskingSettings;
}  // namespace detail

class WebSocketClient {
  friend detail::WebSocketAcceptingClientImpl;

  std::shared_ptr<detail::WebSocketClientImpl> impl_;

  explicit WebSocketClient(async_net::TcpConnection connection,
                           detail::MaskingSettings masking_settings);

 public:
  enum class State {
    Connecting,
    Connected,
    Disconnected,
    Error,
    Shutdown,
  };

  CLASS_NON_COPYABLE(WebSocketClient)

  WebSocketClient() = default;

  WebSocketClient(async_net::IoContext& context,
                  std::string hostname,
                  uint16_t port,
                  std::string uri);
  ~WebSocketClient();

  WebSocketClient(WebSocketClient&& other) noexcept;
  WebSocketClient& operator=(WebSocketClient&& other) noexcept;

  async_net::IoContext* io_context();
  const async_net::IoContext* io_context() const;

  bool valid() const { return impl_ != nullptr; }
  explicit operator bool() const { return valid(); }

  State state() const;

  bool is_connected() const { return state() == State::Connected; }

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

  [[nodiscard]] bool send_text_message(std::string_view payload);
  [[nodiscard]] bool send_binary_message(std::span<const uint8_t> payload);

  bool send_text_message_force(std::string_view payload);
  bool send_binary_message_force(std::span<const uint8_t> payload);

  void send_ping();

  void shutdown();

  void set_on_connected(std::move_only_function<void(Status)> callback);
  void set_on_closed(std::move_only_function<void(Status)> callback);

  void set_on_text_message_received(std::move_only_function<void(std::string_view)> callback);
  void set_on_binary_message_received(
    std::move_only_function<void(std::span<const uint8_t>)> callback);

  void set_on_data_sent(std::move_only_function<void()> callback);
};

}  // namespace async_ws
