#pragma once
#include "IpAddress.hpp"
#include "Status.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <base/containers/BinaryBuffer.hpp>
#include <base/macro/ClassTraits.hpp>

namespace sock {
class StreamSocket;
}

namespace async_net {

class IoContext;

namespace detail {
class IoContextImpl;
class TcpConnectionImpl;
}  // namespace detail

class TcpConnection {
  friend detail::IoContextImpl;

  std::shared_ptr<detail::TcpConnectionImpl> impl_;

  base::BinaryBuffer* acquire_send_buffer();

  TcpConnection(IoContext& context, sock::StreamSocket socket);

 public:
  enum class State {
    Connecting,
    Connected,
    Disconnected,
    Error,
    Shutdown,
  };

  CLASS_NON_COPYABLE(TcpConnection)

  TcpConnection() = default;

  TcpConnection(IoContext& context, std::string hostname, uint16_t port);
  TcpConnection(IoContext& context, const IpAddress& address, uint16_t port);
  TcpConnection(IoContext& context, const SocketAddress& address);
  TcpConnection(IoContext& context, std::vector<SocketAddress> addresses);
  ~TcpConnection();

  TcpConnection(TcpConnection&& other) noexcept;
  TcpConnection& operator=(TcpConnection&& other) noexcept;

  IoContext* io_context();
  const IoContext* io_context() const;

  bool valid() const { return impl_ != nullptr; }
  operator bool() const { return valid(); }

  uint64_t total_bytes_sent() const;
  uint64_t total_bytes_received() const;
  SocketAddress local_address() const;
  SocketAddress peer_address() const;
  State state() const;

  bool is_connected() const { return state() == State::Connected; }

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

  [[nodiscard]] bool send_data(std::span<const uint8_t> data);
  bool send_data_force(std::span<const uint8_t> data);

  template <typename Fn>
  [[nodiscard]] bool send(Fn&& fn) {
    if (!is_send_buffer_full()) {
      const auto buffer = acquire_send_buffer();
      if (buffer) {
        fn(*buffer);
        return true;
      }
    }

    return false;
  }

  template <typename Fn>
  bool send_force(Fn&& fn) {
    const auto buffer = acquire_send_buffer();
    if (buffer) {
      fn(*buffer);
      return true;
    }

    return false;
  }

  void shutdown();

  void set_on_connection_succeeded(std::function<void()> callback);
  void set_on_connection_failed(std::function<void(Status)> callback);

  void set_on_disconnected(std::function<void()> callback);
  void set_on_error(std::function<void(Status)> callback);

  void set_on_data_received(std::function<size_t(std::span<const uint8_t>)> callback);
  void set_on_data_sent(std::function<void()> callback);
};

}  // namespace async_net