#include "TcpListenerImpl.hpp"
#include "IoContextImpl.hpp"

#include <async_net/IoContext.hpp>
#include <async_net/IpResolver.hpp>

#include <base/Log.hpp>
#include <base/Panic.hpp>

namespace async_net::detail {

void TcpListenerImpl::cleanup() {
  on_listening = nullptr;
  on_error = nullptr;
  on_accept = nullptr;
}

void TcpListenerImpl::cleanup_before_register() {
  socket = {};
  cleanup();
}

bool TcpListenerImpl::prepare_unregister() {
  cleanup();

  if (unregister_pending) {
    return false;
  }
  unregister_pending = true;

  socket = {};

  if (state != TcpListener::State::Error) {
    state = TcpListener::State::Shutdown;
  }

  return true;
}

void TcpListenerImpl::listen_immediate(std::shared_ptr<TcpListenerImpl> self,
                                       std::span<const SocketAddress> addresses) {
  verify(!addresses.empty(), "address list is empty");

  if (state == TcpListener::State::Shutdown) {
    return cleanup_before_register();
  }

  Status error_status{};

  for (const auto& address : addresses) {
    auto [status, listener] = sock::Listener::bind(address, {
                                                              .non_blocking = true,
                                                              .reuse_address = true,
                                                            });

    if (status) {
      state = TcpListener::State::Listening;
      socket = std::move(listener);

      if (on_listening) {
        on_listening();
      }

      if (state != TcpListener::State::Shutdown) {
        self->context.impl_->register_tcp_listener(std::move(self));
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

  state = TcpListener::State::Error;

  if (on_error) {
    on_error(error_status);
  } else {
    log_error("failed to listen on the TCP socket: {}", error_status.stringify());
  }

  cleanup_before_register();
}

TcpListenerImpl::TcpListenerImpl(IoContext& context) : context(context) {}

void TcpListenerImpl::startup(std::shared_ptr<TcpListenerImpl> self,
                              std::string hostname,
                              uint16_t port) {
  IpResolver::resolve(
    context, std::move(hostname),
    [self = std::move(self), port](sock::Status status, std::vector<IpAddress> resolved_ips) {
      if (self->state == TcpListener::State::Shutdown) {
        return self->cleanup_before_register();
      }

      if (status) {
        std::vector<SocketAddress> socket_addresses;
        socket_addresses.reserve(resolved_ips.size());

        for (const auto ip : resolved_ips) {
          socket_addresses.emplace_back(ip, port);
        }

        self->listen_immediate(self, socket_addresses);
      } else {
        self->state = TcpListener::State::Error;

        if (self->on_error) {
          self->on_error(status);
        } else {
          log_error("failed to listen on the TCP socket: {}", status.stringify());
        }

        self->cleanup_before_register();
      }
    });
}

void TcpListenerImpl::startup(std::shared_ptr<TcpListenerImpl> self, SocketAddress address) {
  context.post([self = std::move(self), address] {
    const SocketAddress socket_addresses[]{address};
    self->listen_immediate(self, socket_addresses);
  });
}

void TcpListenerImpl::shutdown(std::shared_ptr<TcpListenerImpl> self) {
  if (state != TcpListener::State::Shutdown) {
    const auto should_unregister = state != TcpListener::State::Error;

    state = TcpListener::State::Shutdown;

    if (should_unregister) {
      context.post([self = std::move(self)] {
        if (self->prepare_unregister()) {
          self->context.impl_->unregister_tcp_listener(self.get());
        }
      });
    }
  }
}

void TcpListenerImpl::unregister_during_runloop(std::shared_ptr<TcpListenerImpl> self) {
  if (prepare_unregister()) {
    std::weak_ptr selfW(self);
    context.post([selfW = std::move(selfW)] {
      if (const auto selfS = selfW.lock()) {
        selfS->context.impl_->unregister_tcp_listener(selfS.get());
      }
    });
  }
}

}  // namespace async_net::detail