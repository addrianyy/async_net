#pragma once
#include "IpAddress.hpp"
#include "Status.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <base/macro/ClassTraits.hpp>

namespace async_net {

class IoContext;

namespace detail {
class IoContextImpl;
class UdpSocketImpl;
}  // namespace detail

class UdpSocket {
  friend detail::IoContextImpl;

  std::shared_ptr<detail::UdpSocketImpl> impl_;

 public:
  enum class State {
    Binding,
    Bound,
    Error,
    Shutdown,
  };

  struct BindParameters {
    bool reuse_port = false;
    bool allow_broadcast = false;

    static constexpr BindParameters default_parameters() { return BindParameters{}; }
  };

  CLASS_NON_COPYABLE(UdpSocket)

  UdpSocket() = default;

  UdpSocket(IoContext& context,
            std::string hostname,
            uint16_t port,
            BindParameters parameters = BindParameters::default_parameters());
  UdpSocket(IoContext& context,
            const IpAddress& address,
            uint16_t port,
            BindParameters parameters = BindParameters::default_parameters());
  UdpSocket(IoContext& context,
            const SocketAddress& address,
            BindParameters parameters = BindParameters::default_parameters());
  UdpSocket(IoContext& context,
            std::vector<SocketAddress> addresses,
            BindParameters parameters = BindParameters::default_parameters());
  UdpSocket(IoContext& context,
            uint16_t port,
            BindParameters parameters = BindParameters::default_parameters());
  explicit UdpSocket(IoContext& context,
                     BindParameters parameters = BindParameters::default_parameters());
  ~UdpSocket();

  UdpSocket(UdpSocket&& other) noexcept;
  UdpSocket& operator=(UdpSocket&& other) noexcept;

  IoContext* io_context();
  const IoContext* io_context() const;

  bool valid() const { return impl_ != nullptr; }
  operator bool() const { return valid(); }

  uint64_t total_bytes_sent() const;
  uint64_t total_bytes_received() const;
  SocketAddress local_address() const;
  State state() const;

  bool is_bound() const { return state() == State::Bound; }

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

  bool send_data(const SocketAddress& destination, std::span<const uint8_t> data);

  void shutdown();

  void set_on_binding_succeeded(std::function<void()> callback);
  void set_on_binding_failed(std::function<void(Status)> callback);

  void set_on_error(std::function<void(Status)> callback);

  void set_on_data_received(
    std::function<void(const SocketAddress&, std::span<const uint8_t>)> callback);
  void set_on_data_sent(std::function<void()> callback);

  void set_on_send_failed(std::function<void(Status)> callback);
};

}  // namespace async_net