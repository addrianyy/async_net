#pragma once
#include "IpResolverImpl.hpp"
#include "TimerManagerImpl.hpp"

#include <async_net/IoContext.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <socklib/Socket.hpp>

#include <base/macro/ClassTraits.hpp>

namespace async_net::detail {

class TcpListenerImpl;
class TcpConnectionImpl;

class IoContextImpl {
  std::vector<std::shared_ptr<TcpListenerImpl>> tcp_listeners;
  std::vector<std::shared_ptr<TcpConnectionImpl>> tcp_connections;

  std::unique_ptr<sock::Poller> poller;
  std::vector<sock::Poller::PollEntry> poll_entries;

  IpResolverImpl ip_resolver;
  TimerManagerImpl timer_manager;

  std::vector<std::function<void()>> deferred_work_write;
  std::vector<std::function<void()>> deferred_work_read;

  std::mutex deferred_work_atomic_mutex;
  std::vector<std::function<void()>> deferred_work_atomic_write;
  std::vector<std::function<void()>> deferred_work_atomic_read;

  enum class PendingConnectionStatus {
    StillWaiting,
    Connected,
    Failed,
  };

  void register_poll_entries();

  void handle_listener_events(const sock::Poller::PollEntry& entry,
                              const std::shared_ptr<TcpListenerImpl>& listener);
  PendingConnectionStatus handle_pending_connection_events(
    const sock::Poller::PollEntry& entry,
    const std::shared_ptr<TcpConnectionImpl>& connection);
  void handle_connection_events(const sock::Poller::PollEntry& entry,
                                const std::shared_ptr<TcpConnectionImpl>& connection);

  void handle_poll_events();

  void run_deferred_work();
  void run_deferred_work_atomic();

  void drain_tcp_listeners();
  void drain_tcp_connections();

  void drain_deferred_work();
  void drain_deferred_work_atomic();

  bool has_any_non_atomic_work() const;

 public:
  CLASS_NON_COPYABLE_NON_MOVABLE(IoContextImpl)

  IoContextImpl();

  void register_tcp_listener(std::shared_ptr<TcpListenerImpl> listener);
  void unregister_tcp_listener(TcpListenerImpl* listener);

  void register_tcp_connection(std::shared_ptr<TcpConnectionImpl> connection);
  void unregister_tcp_connection(TcpConnectionImpl* connection);

  void queue_deferred_work(std::function<void()> callback);
  void queue_deferred_work_atomic(std::function<void()> callback);

  void queue_ip_resolve(std::string hostname,
                        std::function<void(Status, std::vector<IpAddress>)> callback);

  TimerManagerImpl::TimerKey register_timer(base::PreciseTime deadline,
                                            std::function<void()> callback);
  void unregister_timer(const TimerManagerImpl::TimerKey& key);

  [[nodiscard]] IoContext::RunResult run(const IoContext::RunParameters& parameters);
  void notify();

  void drain();
};

}  // namespace async_net::detail
