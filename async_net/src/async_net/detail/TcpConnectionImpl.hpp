#pragma once
#include "Common.hpp"

#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>
#include <async_net/TcpConnection.hpp>
#include <async_net/Timer.hpp>

#include <socklib/Socket.hpp>

#include <base/containers/BinaryBuffer.hpp>

#include <functional>
#include <memory>

namespace async_net {

class IoContext;
class TcpConnection;

namespace detail {

class IoContextImpl;
class ContextEntryRegistration;

class TcpConnectionImpl {
  friend TcpConnection;
  friend IoContextImpl;
  friend ContextEntryRegistration;

  struct ConnectingState {
    sock::ConnectingStreamSocket socket;
    std::vector<SocketAddress> addresses;
    size_t next_address_index{};
    Status error_status{};
    async_net::Timer timeout;

    ConnectingState(sock::ConnectingStreamSocket socket,
                    std::vector<SocketAddress> addresses,
                    size_t next_address_index,
                    Status error_status)
        : socket(std::move(socket)),
          addresses(std::move(addresses)),
          next_address_index(next_address_index),
          error_status(error_status) {}
  };

  IoContext& context;
  size_t context_index{invalid_context_index};

  TcpConnection::State state{TcpConnection::State::Connecting};
  bool unregister_pending{};
  bool can_send_packets{};

  sock::StreamSocket socket;
  std::unique_ptr<ConnectingState> connecting_state;

  bool receive_packets{true};

  base::BinaryBuffer receive_buffer;
  base::BinaryBuffer send_buffer;
  size_t send_buffer_offset{};
  size_t send_buffer_max_size{default_send_buffer_max_size};
  bool block_on_send_buffer_full{true};

  SocketAddress local_address{};
  SocketAddress peer_addreess{};
  uint64_t total_bytes_received{};
  uint64_t total_bytes_sent{};

  std::function<void()> on_connection_succeeded;
  std::function<void(Status)> on_connection_failed;
  std::function<void()> on_disconnected;
  std::function<void(Status)> on_error;
  std::function<size_t(std::span<const uint8_t>)> on_data_received;
  std::function<void()> on_data_sent;

  base::BinaryBuffer& acquire_send_buffer();
  size_t send_buffer_size() const;
  size_t send_buffer_remaining_size() const;

  void cleanup();
  void cleanup_before_register();
  bool prepare_unregister();

  void enter_connected_state(bool invoke_callbacks);

  void setup_connecting_timeout(const std::shared_ptr<TcpConnectionImpl>& self);
  bool attempt_next_address(const std::shared_ptr<TcpConnectionImpl>& self,
                            Status previous_connection_status);
  void connect_immediate(std::shared_ptr<TcpConnectionImpl> self,
                         std::vector<SocketAddress> addresses);

 public:
  explicit TcpConnectionImpl(IoContext& context);

  void startup(std::shared_ptr<TcpConnectionImpl> self, sock::StreamSocket connection);
  void startup(std::shared_ptr<TcpConnectionImpl> self, std::string hostname, uint16_t port);
  void startup(std::shared_ptr<TcpConnectionImpl> self, std::vector<SocketAddress> addresses);
  void startup(std::shared_ptr<TcpConnectionImpl> self, SocketAddress address);
  void shutdown(std::shared_ptr<TcpConnectionImpl> self);

  void unregister_during_runloop(std::shared_ptr<TcpConnectionImpl> self);
};

}  // namespace detail

}  // namespace async_net