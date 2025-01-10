#include "TcpConnectionImpl.hpp"
#include "IoContextImpl.hpp"

#include <async_net/IoContext.hpp>
#include <async_net/IpResolver.hpp>

#include <base/Log.hpp>

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
  connecting_socket = {};
  cleanup();
}

bool TcpConnectionImpl::prepare_unregister() {
  cleanup();

  if (unregister_pending) {
    return false;
  }
  unregister_pending = true;

  socket = {};
  connecting_socket = {};
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

void TcpConnectionImpl::connect_immediate(std::shared_ptr<TcpConnectionImpl> self,
                                          SocketAddress address) {
  if (state == TcpConnection::State::Shutdown) {
    return cleanup_before_register();
  }

  auto [initiate_status, connection] = sock::ConnectingSocket::initiate_connection(address);
  if (!initiate_status) {
    state = TcpConnection::State::Error;

    if (on_error) {
      on_connection_failed(initiate_status);
    } else {
      log_error("failed to connect to the TCP socket: {}", initiate_status.stringify());
    }

    return cleanup_before_register();
  }

  if (connection.connected) {
    socket = std::move(connection.connected);
    enter_connected_state(true);
  } else {
    state = TcpConnection::State::Connecting;
    connecting_socket = std::make_unique<sock::ConnectingSocket>(std::move(connection.connecting));
  }

  if (state != TcpConnection::State::Shutdown) {
    context.impl_->register_tcp_connection(std::move(self));
  } else {
    cleanup_before_register();
  }
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
  IpResolver::resolve(context, std::move(hostname),
                      [self = std::move(self), port](sock::Status status, IpAddress resolved_ip) {
                        if (self->state == TcpConnection::State::Shutdown) {
                          return self->cleanup_before_register();
                        }

                        if (status) {
                          self->connect_immediate(self, SocketAddress{resolved_ip, port});
                        } else {
                          self->state = TcpConnection::State::Error;

                          if (self->on_error) {
                            self->on_error(status);
                          } else {
                            log_error("failed to connect to the TCP socket: {}",
                                      status.stringify());
                          }

                          self->cleanup_before_register();
                        }
                      });
}

void TcpConnectionImpl::startup(std::shared_ptr<TcpConnectionImpl> self, SocketAddress address) {
  context.post([self = std::move(self), address] { self->connect_immediate(self, address); });
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