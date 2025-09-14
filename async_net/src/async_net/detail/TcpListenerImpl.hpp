#pragma once
#include "Common.hpp"

#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>
#include <async_net/TcpListener.hpp>

#include <socklib/Socket.hpp>

#include <functional>
#include <vector>

namespace async_net {

class IoContext;
class TcpConnection;
class TcpListener;

namespace detail {

class IoContextImpl;
class ContextEntryRegistration;

class TcpListenerImpl {
  friend TcpListener;
  friend IoContextImpl;
  friend ContextEntryRegistration;

  IoContext& context;
  size_t context_index{invalid_context_index};

  TcpListener::State state{TcpListener::State::Waiting};
  bool unregister_pending{};

  sock::Listener socket;

  bool accept_connections{true};

  std::move_only_function<void()> on_listening;
  std::move_only_function<void(Status)> on_error;
  std::move_only_function<void(Status, TcpConnection)> on_accept;

  void cleanup();
  void cleanup_before_register();
  bool prepare_unregister();

  void listen_immediate(std::shared_ptr<TcpListenerImpl> self,
                        std::span<const SocketAddress> addresses);

 public:
  explicit TcpListenerImpl(IoContext& context);

  bool is_listening() const { return state == TcpListener::State::Listening; }

  void startup(std::shared_ptr<TcpListenerImpl> self, std::string hostname, uint16_t port);
  void startup(std::shared_ptr<TcpListenerImpl> self, std::vector<SocketAddress> addresses);
  void startup(std::shared_ptr<TcpListenerImpl> self, SocketAddress address);
  void shutdown(std::shared_ptr<TcpListenerImpl> self);

  void unregister_during_runloop(std::shared_ptr<TcpListenerImpl> self);
};

}  // namespace detail

}  // namespace async_net
