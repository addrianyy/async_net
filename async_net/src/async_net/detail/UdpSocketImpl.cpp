#include "UdpSocketImpl.hpp"
#include "IoContextImpl.hpp"

#include <async_net/IoContext.hpp>
#include <async_net/IpResolver.hpp>

#include <base/Log.hpp>
#include <base/Panic.hpp>

namespace async_net::detail {

size_t UdpSocketImpl::send_buffer_size() const {
  return send_buffer.size() - send_buffer_offset;
}

size_t UdpSocketImpl::send_buffer_remaining_size() const {
  const auto used_size = send_buffer_size();
  const auto max_size = send_buffer_max_size;
  if (used_size > max_size) {
    return 0;
  } else {
    return max_size - used_size;
  }
}

bool UdpSocketImpl::is_send_buffer_full() const {
  return send_buffer_size() >= send_buffer_max_size || send_entries.size() >= send_entries_max_size;
}

void UdpSocketImpl::cleanup() {
  on_bound = nullptr;
  on_closed = nullptr;
  on_data_received = nullptr;
  on_data_sent = nullptr;
  on_send_error = nullptr;
}

void UdpSocketImpl::cleanup_before_register() {
  socket = {};
  cleanup();
}

bool UdpSocketImpl::prepare_unregister() {
  cleanup();

  if (unregister_pending) {
    return false;
  }
  unregister_pending = true;

  socket = {};
  can_send_packets = false;

  if (state != UdpSocket::State::Error) {
    state = UdpSocket::State::Shutdown;
  }

  return true;
}

void UdpSocketImpl::enter_bound_state() {
  if (const auto result = socket.local_address<SocketAddress>()) {
    local_address = result.value;
  }

  state = UdpSocket::State::Bound;
  can_send_packets = true;

  if (on_bound) {
    on_bound({});
  }
}

void UdpSocketImpl::bind_immediate(std::shared_ptr<UdpSocketImpl> self,
                                   std::span<const SocketAddress> addresses,
                                   UdpSocket::BindParameters parameters) {
  verify(!addresses.empty(), "address list is empty");

  if (state == UdpSocket::State::Shutdown) {
    return cleanup_before_register();
  }

  Status error_status{};

  for (const auto& address : addresses) {
    auto [status, datagram] =
      sock::DatagramSocket::bind(address, {
                                            .non_blocking = true,
                                            .reuse_address = true,
                                            .reuse_port = parameters.reuse_port,
                                          });

    if (status && parameters.allow_broadcast) {
      status = datagram.set_broadcast_enabled(true);
    }

    if (status) {
      socket = std::move(datagram);
      enter_bound_state();

      if (state != UdpSocket::State::Shutdown) {
        self->context.impl_->register_udp_socket(std::move(self));
      } else {
        cleanup_before_register();
      }

      return;
    } else {
      if (error_status) {
        error_status = status;
      }
    }
  }

  verify(!error_status, "expected error status");

  state = UdpSocket::State::Error;

  if (on_bound) {
    on_bound(error_status);
  } else {
    log_error("failed to bind to the UDP socket: {}", error_status.stringify());
  }

  cleanup_before_register();
}

void UdpSocketImpl::bind_anonymous_immediate(std::shared_ptr<UdpSocketImpl> self,
                                             UdpSocket::BindParameters parameters) {
  auto [status, datagram] =
    sock::DatagramSocket::create(SocketAddress{}.type(), {
                                                           .non_blocking = true,
                                                         });

  if (status && parameters.allow_broadcast) {
    status = datagram.set_broadcast_enabled(true);
  }

  if (status) {
    socket = std::move(datagram);
    enter_bound_state();

    if (state != UdpSocket::State::Shutdown) {
      self->context.impl_->register_udp_socket(std::move(self));
    } else {
      cleanup_before_register();
    }
  } else {
    state = UdpSocket::State::Error;

    if (on_bound) {
      on_bound(status);
    } else {
      log_error("failed to bind to the UDP socket: {}", status.stringify());
    }

    cleanup_before_register();
  }
}

UdpSocketImpl::UdpSocketImpl(IoContext& context) : context(context) {}

void UdpSocketImpl::startup(std::shared_ptr<UdpSocketImpl> self,
                            std::string hostname,
                            uint16_t port,
                            UdpSocket::BindParameters parameters) {
  IpResolver::resolve(context, std::move(hostname),
                      [self = std::move(self), port, parameters](
                        sock::Status status, std::vector<IpAddress> resolved_ips) {
                        if (self->state == UdpSocket::State::Shutdown) {
                          return self->cleanup_before_register();
                        }

                        if (status) {
                          std::vector<SocketAddress> socket_addresses;
                          socket_addresses.reserve(resolved_ips.size());

                          for (const auto ip : resolved_ips) {
                            socket_addresses.emplace_back(ip, port);
                          }

                          self->bind_immediate(self, socket_addresses, parameters);
                        } else {
                          self->state = UdpSocket::State::Error;

                          if (self->on_bound) {
                            self->on_bound(status);
                          } else {
                            log_error("failed to bind to the UDP socket: {}", status.stringify());
                          }

                          self->cleanup_before_register();
                        }
                      });
}

void UdpSocketImpl::startup(std::shared_ptr<UdpSocketImpl> self,
                            std::vector<SocketAddress> addresses,
                            UdpSocket::BindParameters parameters) {
  context.post([self = std::move(self), addresses = std::move(addresses), parameters] {
    self->bind_immediate(self, addresses, parameters);
  });
}

void UdpSocketImpl::startup(std::shared_ptr<UdpSocketImpl> self,
                            SocketAddress address,
                            UdpSocket::BindParameters parameters) {
  context.post([self = std::move(self), address, parameters] {
    const SocketAddress socket_addresses[]{address};
    self->bind_immediate(self, socket_addresses, parameters);
  });
}

void UdpSocketImpl::startup(std::shared_ptr<UdpSocketImpl> self,
                            UdpSocket::BindParameters parameters) {
  context.post(
    [self = std::move(self), parameters] { self->bind_anonymous_immediate(self, parameters); });
}

void UdpSocketImpl::shutdown(std::shared_ptr<UdpSocketImpl> self) {
  if (state != UdpSocket::State::Shutdown) {
    const auto should_unregister = state != UdpSocket::State::Error;

    state = UdpSocket::State::Shutdown;

    if (should_unregister) {
      context.post([self = std::move(self)] {
        self->cleanup();

        if (self->send_entries.empty()) {
          if (self->prepare_unregister()) {
            self->context.impl_->unregister_udp_socket(self.get());
          }
        }
      });
    }
  }
}

void UdpSocketImpl::unregister_during_runloop(std::shared_ptr<UdpSocketImpl> self) {
  if (prepare_unregister()) {
    std::weak_ptr selfW(self);
    context.post([selfW = std::move(selfW)] {
      if (const auto selfS = selfW.lock()) {
        selfS->context.impl_->unregister_udp_socket(selfS.get());
      }
    });
  }
}

bool UdpSocketImpl::send_data(const SocketAddress& destination, std::span<const uint8_t> data) {
  if (data.size() > max_datagram_size || data.size() > std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  if ((send_entries.size() >= send_entries_max_size) ||
      (send_buffer_size() + data.size() > send_buffer_max_size)) {
    return false;
  }

  if (send_buffer_offset > 0) {
    send_buffer.trim_front(send_buffer_offset);
    send_buffer_offset = 0;
  }

  send_buffer.append(data);
  send_entries.push_back({
    .destination = destination,
    .datagram_size = uint32_t(data.size()),
  });

  return true;
}

}  // namespace async_net::detail