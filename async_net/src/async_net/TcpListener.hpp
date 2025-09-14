#pragma once
#include "IpAddress.hpp"
#include "Status.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <base/macro/ClassTraits.hpp>

namespace async_net {

class IoContext;
class TcpConnection;

namespace detail {
class TcpListenerImpl;
}

class TcpListener {
  std::shared_ptr<detail::TcpListenerImpl> impl_;

 public:
  enum class State {
    Waiting,
    Listening,
    Error,
    Shutdown,
  };

  CLASS_NON_COPYABLE(TcpListener)

  TcpListener() = default;

  TcpListener(IoContext& context, std::string hostname, uint16_t port);
  TcpListener(IoContext& context, const IpAddress& address, uint16_t port);
  TcpListener(IoContext& context, const SocketAddress& address);
  TcpListener(IoContext& context, std::vector<SocketAddress> addresses);
  TcpListener(IoContext& context, uint16_t port);
  ~TcpListener();

  TcpListener(TcpListener&& other) noexcept;
  TcpListener& operator=(TcpListener&& other) noexcept;

  IoContext* io_context();
  const IoContext* io_context() const;

  bool valid() const { return impl_ != nullptr; }
  explicit operator bool() const { return valid(); }

  State state() const;

  bool is_listening() const { return state() == State::Listening; }

  bool accept_connections() const;
  void set_accept_connections(bool accept) const;

  void shutdown();

  void set_on_listening(std::move_only_function<void()> callback);
  void set_on_error(std::move_only_function<void(Status)> callback);
  void set_on_accept(std::move_only_function<void(Status, TcpConnection)> callback);
};

}  // namespace async_net
