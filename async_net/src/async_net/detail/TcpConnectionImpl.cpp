#include "TcpConnectionImpl.hpp"
#include "IoContextImpl.hpp"

#include <async_net/IoContext.hpp>
#include <async_net/IpResolver.hpp>

#include <base/Log.hpp>
#include <base/Panic.hpp>

namespace async_net::detail {

base::BinaryBuffer& TcpConnectionImpl::acquire_send_buffer() {
  if (send_buffer_offset > 0) {
    send_buffer.trim_front(send_buffer_offset);
    send_buffer_offset = 0;
  }
  return send_buffer;
}

size_t TcpConnectionImpl::send_buffer_size() const {
  return send_buffer.size() - send_buffer_offset;
}

size_t TcpConnectionImpl::send_buffer_remaining_size() const {
  const auto used_size = send_buffer_size();
  const auto max_size = send_buffer_max_size;
  if (used_size > max_size) {
    return 0;
  } else {
    return max_size - used_size;
  }
}

void TcpConnectionImpl::cleanup() {
  on_connection_succeeded = nullptr;
  on_connection_failed = nullptr;
  on_disconnected = nullptr;
  on_error = nullptr;
  on_data_received = nullptr;
  on_data_sent = nullptr;
}

void TcpConnectionImpl::cleanup_before_register() {
  socket = {};
  connecting_state = {};
  cleanup();
}

bool TcpConnectionImpl::prepare_unregister() {
  cleanup();

  if (unregister_pending) {
    return false;
  }
  unregister_pending = true;

  socket = {};
  connecting_state = {};
  can_send_packets = false;

  if (state != TcpConnection::State::Disconnected && state != TcpConnection::State::Error) {
    state = TcpConnection::State::Shutdown;
  }

  return true;
}

void TcpConnectionImpl::enter_connected_state(bool invoke_callbacks) {
  if (const auto result = socket.peer_address<SocketAddress>()) {
    peer_addreess = result.value;
  }

  state = TcpConnection::State::Connected;
  can_send_packets = true;

  if (invoke_callbacks && on_connection_succeeded) {
    on_connection_succeeded();
  }
}

void TcpConnectionImpl::setup_connecting_timeout(const std::shared_ptr<TcpConnectionImpl>& self) {
  auto selfW = std::weak_ptr(self);
  connecting_state->timeout = Timer::invoke_after(
    context, base::PreciseTime::from_milliseconds(300), [selfW = std::move(selfW)] {
      if (auto selfS = selfW.lock()) {
        if (selfS->state == TcpConnection::State::Connecting) {
          // Don't use TimedOut here to make the result consistent with OSes that have sane
          // connection timeouts.
          const auto succeeded = selfS->attempt_next_address(
            selfS, {
                     .error = sock::Error::ConnectFailed,
                     .system_error = sock::SystemError::ConnectionRefused,
                   });
          if (!succeeded) {
            selfS->unregister_during_runloop(selfS);
          }
        }
      }
    });
}

bool TcpConnectionImpl::attempt_next_address(const std::shared_ptr<TcpConnectionImpl>& self,
                                             Status previous_connection_status) {
  if (connecting_state->error_status) {
    connecting_state->error_status = previous_connection_status;
  }

  for (size_t i = connecting_state->next_address_index; i < connecting_state->addresses.size();
       ++i) {
    const auto address = connecting_state->addresses[i];

    auto [initiate_status, connection] = sock::ConnectingStreamSocket::initiate_connection(address);
    if (!initiate_status) {
      if (connecting_state->error_status) {
        connecting_state->error_status = initiate_status;
      }
      continue;
    }

    if (connection.connected) {
      connecting_state = {};
      socket = std::move(connection.connected);
      enter_connected_state(true);
    } else {
      connecting_state->socket = std::move(connection.connecting);
      connecting_state->next_address_index = i + 1;

      setup_connecting_timeout(self);
    }

    return true;
  }

  const auto status = connecting_state->error_status;

  verify(!status, "expected error status");

  state = TcpConnection::State::Error;
  connecting_state = {};

  if (on_connection_failed) {
    on_connection_failed(status);
  } else {
    log_error("failed to connect to the TCP socket: {}", status.stringify());
  }

  return false;
}

void TcpConnectionImpl::connect_immediate(std::shared_ptr<TcpConnectionImpl> self,
                                          std::vector<SocketAddress> addresses) {
  verify(!addresses.empty(), "address list is empty");

  if (state == TcpConnection::State::Shutdown) {
    return cleanup_before_register();
  }

  Status error_status{};

  for (size_t i = 0; i < addresses.size(); ++i) {
    const auto address = addresses[i];

    auto [initiate_status, connection] = sock::ConnectingStreamSocket::initiate_connection(address);
    if (!initiate_status) {
      if (error_status) {
        error_status = initiate_status;
      }
      continue;
    }

    if (connection.connected) {
      socket = std::move(connection.connected);
      enter_connected_state(true);
    } else {
      state = TcpConnection::State::Connecting;
      connecting_state = std::make_unique<ConnectingState>(
        std::move(connection.connecting), std::move(addresses), i + 1, error_status);
    }

    if (state != TcpConnection::State::Shutdown) {
      setup_connecting_timeout(self);
      context.impl_->register_tcp_connection(std::move(self));
    } else {
      cleanup_before_register();
    }

    return;
  }

  verify(!error_status, "expected error status");

  state = TcpConnection::State::Error;

  if (on_connection_failed) {
    on_connection_failed(error_status);
  } else {
    log_error("failed to connect to the TCP socket: {}", error_status.stringify());
  }

  cleanup_before_register();
}

TcpConnectionImpl::TcpConnectionImpl(IoContext& context) : context(context) {}

void TcpConnectionImpl::startup(std::shared_ptr<TcpConnectionImpl> self,
                                sock::StreamSocket connection) {
  socket = std::move(connection);
  enter_connected_state(false);

  context.post([self = std::move(self)] {
    if (self->state == TcpConnection::State::Shutdown) {
      return self->cleanup_before_register();
    }

    self->context.impl_->register_tcp_connection(self);
  });
}

void TcpConnectionImpl::startup(std::shared_ptr<TcpConnectionImpl> self,
                                std::string hostname,
                                uint16_t port) {
  IpResolver::resolve(
    context, std::move(hostname),
    [self = std::move(self), port](sock::Status status, std::vector<IpAddress> resolved_ips) {
      if (self->state == TcpConnection::State::Shutdown) {
        return self->cleanup_before_register();
      }

      if (status) {
        std::vector<SocketAddress> socket_addresses;
        socket_addresses.reserve(resolved_ips.size());

        for (const auto ip : resolved_ips) {
          socket_addresses.emplace_back(ip, port);
        }

        self->connect_immediate(self, std::move(socket_addresses));
      } else {
        self->state = TcpConnection::State::Error;

        if (self->on_error) {
          self->on_error(status);
        } else {
          log_error("failed to connect to the TCP socket: {}", status.stringify());
        }

        self->cleanup_before_register();
      }
    });
}

void TcpConnectionImpl::startup(std::shared_ptr<TcpConnectionImpl> self,
                                std::vector<SocketAddress> addresses) {
  context.post([self = std::move(self), addresses = std::move(addresses)]() mutable {
    self->connect_immediate(self, std::move(addresses));
  });
}

void TcpConnectionImpl::startup(std::shared_ptr<TcpConnectionImpl> self, SocketAddress address) {
  context.post([self = std::move(self), address] {
    std::vector<SocketAddress> socket_addresses;
    socket_addresses.emplace_back(address);
    self->connect_immediate(self, std::move(socket_addresses));
  });
}

void TcpConnectionImpl::shutdown(std::shared_ptr<TcpConnectionImpl> self) {
  if (state != TcpConnection::State::Shutdown) {
    const auto should_unregister =
      state != TcpConnection::State::Error && state != TcpConnection::State::Disconnected;

    state = TcpConnection::State::Shutdown;

    if (should_unregister) {
      context.post([self = std::move(self)] {
        self->cleanup();

        if (self->send_buffer_size() == 0) {
          if (self->prepare_unregister()) {
            self->context.impl_->unregister_tcp_connection(self.get());
          }
        }
      });
    }
  }
}

void TcpConnectionImpl::unregister_during_runloop(std::shared_ptr<TcpConnectionImpl> self) {
  if (prepare_unregister()) {
    std::weak_ptr selfW(self);
    context.post([selfW = std::move(selfW)] {
      if (const auto selfS = selfW.lock()) {
        selfS->context.impl_->unregister_tcp_connection(selfS.get());
      }
    });
  }
}

}  // namespace async_net::detail