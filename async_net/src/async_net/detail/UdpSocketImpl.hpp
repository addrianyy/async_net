#pragma once
#include "Common.hpp"

#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>
#include <async_net/UdpSocket.hpp>

#include <socklib/Socket.hpp>

#include <base/containers/BinaryBuffer.hpp>

#include <functional>
#include <vector>

namespace async_net {

class IoContext;
class UdpSocket;

namespace detail {

class IoContextImpl;
class ContextEntryRegistration;

class UdpSocketImpl {
  friend UdpSocket;
  friend IoContextImpl;
  friend ContextEntryRegistration;

  constexpr static size_t send_entries_max_size = 32 * 1024;

  struct SendEntry {
    SocketAddress destination;
    uint32_t datagram_size{};
  };

  IoContext& context;
  size_t context_index{invalid_context_index};

  UdpSocket::State state{UdpSocket::State::Binding};
  bool unregister_pending{};
  bool can_send_packets{};

  sock::DatagramSocket socket;

  bool receive_packets{true};

  base::BinaryBuffer send_buffer;
  size_t send_buffer_offset{};
  size_t send_buffer_max_size{default_send_buffer_max_size};
  bool block_on_send_buffer_full{true};

  std::vector<SendEntry> send_entries;

  SocketAddress local_address{};
  uint64_t total_bytes_received{};
  uint64_t total_bytes_sent{};

  std::move_only_function<void(Status)> on_bound;
  std::move_only_function<void(Status)> on_closed;
  std::move_only_function<void(const SocketAddress&, std::span<const uint8_t>)> on_data_received;
  std::move_only_function<void()> on_data_sent;
  std::move_only_function<void(Status)> on_send_error;

  size_t send_buffer_size() const;
  size_t send_buffer_remaining_size() const;
  bool is_send_buffer_full() const;

  void cleanup();
  void cleanup_before_register();
  bool prepare_unregister();

  void enter_bound_state();

  void bind_immediate(std::shared_ptr<UdpSocketImpl> self,
                      std::span<const SocketAddress> addresses,
                      UdpSocket::BindParameters parameters);

  void bind_anonymous_immediate(std::shared_ptr<UdpSocketImpl> self,
                                UdpSocket::BindParameters parameters);

 public:
  explicit UdpSocketImpl(IoContext& context);

  void startup(std::shared_ptr<UdpSocketImpl> self,
               std::string hostname,
               uint16_t port,
               UdpSocket::BindParameters parameters);
  void startup(std::shared_ptr<UdpSocketImpl> self,
               std::vector<SocketAddress> addresses,
               UdpSocket::BindParameters parameters);
  void startup(std::shared_ptr<UdpSocketImpl> self,
               SocketAddress address,
               UdpSocket::BindParameters parameters);
  void startup(std::shared_ptr<UdpSocketImpl> self, UdpSocket::BindParameters parameters);
  void shutdown(std::shared_ptr<UdpSocketImpl> self);

  void unregister_during_runloop(std::shared_ptr<UdpSocketImpl> self);

  bool send_data(const SocketAddress& destination, std::span<const uint8_t> data);
};

}  // namespace detail

}  // namespace async_net
