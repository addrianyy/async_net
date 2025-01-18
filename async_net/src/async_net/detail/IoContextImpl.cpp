#include "IoContextImpl.hpp"
#include "TcpConnectionImpl.hpp"
#include "TcpListenerImpl.hpp"

#include <socklib/Socket.hpp>

#include <base/Panic.hpp>
#include <base/logger/Log.hpp>

#include <async_net/TcpConnection.hpp>

#include <algorithm>

namespace async_net::detail {

class ContextEntryRegistration {
 public:
  template <typename T, typename U>
  static void register_entry(std::vector<T>& entries, std::shared_ptr<U> entry) {
    entry->context_index = entries.size();
    entries.push_back(std::move(entry));
  }

  template <typename T, typename U>
  static void unregister_entry(std::vector<T>& entries, U* entry) {
    if (entry->context_index != invalid_context_index) {
      const auto last_index = entries.size() - 1;
      if (entry->context_index != last_index) {
        std::swap(entries[entry->context_index], entries[last_index]);
        entries[entry->context_index]->context_index = entry->context_index;
      }
      entry->context_index = invalid_context_index;
      entries.pop_back();
    }
  }
};

void IoContextImpl::register_poll_entries() {
  poll_entries.clear();

  for (const auto& listener : tcp_listeners) {
    const auto accepts_connections =
      listener->is_listening() && listener->accept_connections && listener->on_accept;

    poll_entries.emplace_back(sock::Poller::PollEntry{
      .socket = &listener->socket,
      .query_events = accepts_connections ? sock::Poller::QueryEvents::CanAccept
                                          : sock::Poller::QueryEvents::None,
    });
  }

  for (const auto& connection : tcp_connections) {
    auto query_events = sock::Poller::QueryEvents::None;
    sock::Socket* socket = nullptr;

    if (connection->connecting_state) {
      if (connection->state == TcpConnection::State::Connecting) {
        socket = &connection->connecting_state->socket;

        query_events = query_events | sock::Poller::QueryEvents::CanReceiveFrom |
                       sock::Poller::QueryEvents::CanSendTo;
      }
    } else {
      socket = &connection->socket;

      if (connection->state == TcpConnection::State::Connected && connection->on_data_received &&
          (!connection->block_on_send_buffer_full ||
           connection->send_buffer_size() < connection->send_buffer_max_size)) {
        query_events = query_events | sock::Poller::QueryEvents::CanReceiveFrom;
      }
      if (connection->can_send_packets && connection->send_buffer_size() > 0) {
        query_events = query_events | sock::Poller::QueryEvents::CanSendTo;
      }
    }

    poll_entries.emplace_back(sock::Poller::PollEntry{
      .socket = socket,
      .query_events = query_events,
    });
  }
}

void IoContextImpl::handle_listener_events(const sock::Poller::PollEntry& entry,
                                           std::shared_ptr<TcpListenerImpl>& listener) {
  if (entry.has_any_event(sock::Poller::StatusEvents::Disconnected |
                          sock::Poller::StatusEvents::InvalidSocket |
                          sock::Poller::StatusEvents::Error)) {
    if (listener->state == TcpListener::State::Listening) {
      listener->state = TcpListener::State::Error;

      auto error = listener->socket.last_error();
      if (error == SystemError::None) {
        error = SystemError::Unknown;
      }

      const Status status{
        .error = sock::Error::ListenFailed,
        .system_error = error,
      };

      if (listener->on_error) {
        listener->on_error(status);
      } else {
        log_error("failed to listen on the TCP socket: {}", status.stringify());
      }
    }

    listener->unregister_during_runloop(listener);
    return;
  }

  if (entry.has_events(sock::Poller::StatusEvents::CanAccept)) {
    while (listener->is_listening() && listener->accept_connections && listener->on_accept) {
      auto [accept_status, client_socket] = listener->socket.accept();
      if (!accept_status) {
        if (!accept_status.would_block()) {
          listener->on_accept(accept_status, TcpConnection{});
        }
        break;
      }

      if (const auto status = client_socket.set_non_blocking(true)) {
        TcpConnection connection{listener->context, std::move(client_socket)};
        listener->on_accept(Status{}, std::move(connection));
      } else {
        listener->on_accept(status, TcpConnection{});
      }
    }
  }
}

IoContextImpl::PendingConnectionStatus IoContextImpl::handle_pending_connection_events(
  const sock::Poller::PollEntry& entry,
  std::shared_ptr<TcpConnectionImpl>& connection) {
  const auto next_connection = [&](Status status) {
    if (!connection->attempt_next_address(connection, status)) {
      return PendingConnectionStatus::Failed;
    } else {
      return connection->state == TcpConnection::State::Connected
               ? PendingConnectionStatus::Connected
               : PendingConnectionStatus::StillWaiting;
    }
  };

  if (entry.has_any_event(sock::Poller::StatusEvents::Disconnected |
                          sock::Poller::StatusEvents::InvalidSocket |
                          sock::Poller::StatusEvents::Error)) {
    if (connection->state == TcpConnection::State::Connecting) {
      auto error = connection->connecting_state->socket.last_error();
      if (error == SystemError::None) {
        error = SystemError::Unknown;
      }

      const Status status{
        .error = sock::Error::ConnectFailed,
        .system_error = error,
      };

      return next_connection(status);
    }

    return PendingConnectionStatus::Failed;
  }

  if (entry.has_any_event(sock::Poller::StatusEvents::CanSendTo |
                          sock::Poller::StatusEvents::CanReceiveFrom)) {
    if (connection->state == TcpConnection::State::Connecting) {
      auto [connect_status, socket] = connection->connecting_state->socket.connect();

      if (connect_status) {
        connection->connecting_state = {};
        connection->socket = std::move(socket);
        connection->enter_connected_state(true);

        return connection->state == TcpConnection::State::Connected
                 ? PendingConnectionStatus::Connected
                 : PendingConnectionStatus::Failed;
      } else {
        return next_connection(connect_status);
      }
    } else {
      return PendingConnectionStatus::Failed;
    }
  }

  return PendingConnectionStatus::StillWaiting;
}

void IoContextImpl::handle_connection_events(const sock::Poller::PollEntry& entry,
                                             std::shared_ptr<TcpConnectionImpl>& connection) {
  constexpr static size_t base_receive_fragment_size = 16 * 1024;
  constexpr static size_t max_receive_fragment_size = 16 * 1024 * 1024;
  constexpr static size_t max_send_fragment_size = 32 * 1024 * 1024;

  if (connection->connecting_state) {
    const auto status = handle_pending_connection_events(entry, connection);
    if (status == PendingConnectionStatus::Failed) {
      connection->unregister_during_runloop(connection);
      return;
    }
    if (status == PendingConnectionStatus::StillWaiting) {
      return;
    }
  }

  const auto on_socket_error = [&](sock::Status status) {
    verify(!status, "cannot handle non-error status");

    if (connection->state == TcpConnection::State::Connected) {
      if (status.disconnected()) {
        connection->state = TcpConnection::State::Disconnected;
        if (connection->on_disconnected) {
          connection->on_disconnected();
        }
      } else {
        connection->state = TcpConnection::State::Error;
        if (connection->on_error) {
          connection->on_error(status);
        } else {
          log_error("failed to process TCP connection: {}", status.stringify());
        }
      }
    }

    connection->unregister_during_runloop(connection);
  };

  if (entry.has_events(sock::Poller::StatusEvents::CanReceiveFrom) &&
      connection->state == TcpConnection::State::Connected && connection->on_data_received) {
    sock::Status receive_error = {};
    size_t total_bytes_received = 0;

    while (total_bytes_received < max_receive_fragment_size) {
      const auto available_buffer_size =
        std::max(base_receive_fragment_size, connection->receive_buffer.unused_capacity());
      const auto size_left_to_receive = max_receive_fragment_size - total_bytes_received;
      const auto max_receive_amount = std::min(size_left_to_receive, available_buffer_size);

      const auto previous_size = connection->receive_buffer.size();
      const auto current_receive_buffer = connection->receive_buffer.grow(max_receive_amount);

      const auto [receive_status, bytes_received] =
        connection->socket.receive(current_receive_buffer);

      connection->receive_buffer.resize(previous_size + bytes_received);

      if (!receive_status) {
        if (!receive_status.would_block()) {
          receive_error = receive_status;
        }

        break;
      }

      total_bytes_received += bytes_received;

      if (bytes_received < max_receive_amount) {
        break;
      }
    }

    if (total_bytes_received > 0) {
      connection->total_bytes_received += total_bytes_received;

      const auto consumed_bytes = connection->on_data_received(connection->receive_buffer.span());
      if (consumed_bytes > 0) {
        connection->receive_buffer.trim_front(consumed_bytes);
      }
    }

    if (!receive_error) {
      on_socket_error(receive_error);
    }
  }

  if (entry.has_any_event(sock::Poller::StatusEvents::InvalidSocket |
                          sock::Poller::StatusEvents::Error |
                          sock::Poller::StatusEvents::Disconnected)) {
    if (connection->state == TcpConnection::State::Connected) {
      auto error = connection->connecting_state->socket.last_error();
      if (error == SystemError::None) {
        error = SystemError::Unknown;
      }
      if (entry.has_events(sock::Poller::StatusEvents::Disconnected)) {
        error = sock::SystemError::Disconnected;
      }

      on_socket_error(sock::Status{
        .error = sock::Error::PollFailed,
        .system_error = error,
      });
    } else {
      connection->unregister_during_runloop(connection);
    }
  }

  if (entry.has_events(sock::Poller::StatusEvents::CanSendTo) && connection->can_send_packets) {
    const std::span<const uint8_t> initial_send_buffer =
      connection->send_buffer.span().subspan(connection->send_buffer_offset);

    auto send_buffer = initial_send_buffer;

    while (!send_buffer.empty()) {
      auto current_send_buffer = send_buffer;
      if (current_send_buffer.size() > max_send_fragment_size) {
        current_send_buffer = current_send_buffer.subspan(0, max_send_fragment_size);
      }

      const auto [send_status, bytes_sent] = connection->socket.send(current_send_buffer);
      if (!send_status) {
        if (!send_status.would_block()) {
          on_socket_error(send_status);
        }

        break;
      }

      send_buffer = send_buffer.subspan(bytes_sent);

      if (bytes_sent < current_send_buffer.size()) {
        break;
      }
    }

    const auto total_bytes_sent = initial_send_buffer.size() - send_buffer.size();

    if (total_bytes_sent > 0) {
      connection->total_bytes_sent += total_bytes_sent;
      connection->send_buffer_offset += total_bytes_sent;

      if (connection->state == TcpConnection::State::Connected && connection->on_data_sent) {
        connection->on_data_sent();
      }
    }

    if (connection->send_buffer_size() == 0 &&
        connection->state != TcpConnection::State::Connected) {
      connection->unregister_during_runloop(connection);
    }
  }
}

void IoContextImpl::handle_poll_events() {
  size_t entry_index = 0;

  for (size_t i = 0; i < tcp_listeners.size(); ++i) {
    handle_listener_events(poll_entries[entry_index + i], tcp_listeners[i]);
  }
  entry_index += tcp_listeners.size();

  for (size_t i = 0; i < tcp_connections.size(); ++i) {
    handle_connection_events(poll_entries[entry_index + i], tcp_connections[i]);
  }
  entry_index += tcp_connections.size();
}

void IoContextImpl::run_deferred_work() {
  while (!deferred_work_write.empty()) {
    std::swap(deferred_work_write, deferred_work_read);

    for (auto& callback : deferred_work_read) {
      callback();
    }
    deferred_work_read.clear();
  }
}

void IoContextImpl::run_deferred_work_atomic() {
  {
    std::lock_guard lock(deferred_work_atomic_mutex);
    std::swap(deferred_work_atomic_read, deferred_work_atomic_write);
  }

  for (auto& callback : deferred_work_atomic_read) {
    callback();
  }
  deferred_work_atomic_read.clear();
}

void IoContextImpl::drain_tcp_listeners() {
  for (auto& listener : tcp_listeners) {
    listener->state = TcpListener::State::Shutdown;
    listener->context_index = invalid_context_index;
  }

  std::vector<std::shared_ptr<TcpListenerImpl>> temp;
  std::swap(temp, tcp_listeners);
  temp.clear();
}

void IoContextImpl::drain_tcp_connections() {
  for (auto& connection : tcp_connections) {
    connection->state = TcpConnection::State::Shutdown;
    connection->context_index = invalid_context_index;
  }

  std::vector<std::shared_ptr<TcpConnectionImpl>> temp;
  std::swap(temp, tcp_connections);
  temp.clear();
}

void IoContextImpl::drain_deferred_work() {
  while (!deferred_work_write.empty()) {
    std::swap(deferred_work_write, deferred_work_read);
    deferred_work_read.clear();
  }
}

void IoContextImpl::drain_deferred_work_atomic() {
  {
    std::lock_guard lock(deferred_work_atomic_mutex);
    std::swap(deferred_work_atomic_read, deferred_work_atomic_write);
  }

  deferred_work_atomic_read.clear();
}

bool IoContextImpl::has_any_non_atomic_work() const {
  return !(tcp_listeners.empty() && tcp_connections.empty() && deferred_work_write.empty() &&
           timer_manager.empty() && ip_resolver.empty());
}

IoContextImpl::IoContextImpl() : ip_resolver(*this) {
  poller = sock::Poller::create({
    .enable_cancellation = true,
  });
  verify(poller, "failed to create socket poller");
}

void IoContextImpl::register_tcp_listener(std::shared_ptr<TcpListenerImpl> listener) {
  ContextEntryRegistration::register_entry(tcp_listeners, std::move(listener));
}

void IoContextImpl::unregister_tcp_listener(TcpListenerImpl* listener) {
  ContextEntryRegistration::unregister_entry(tcp_listeners, listener);
}

void IoContextImpl::register_tcp_connection(std::shared_ptr<TcpConnectionImpl> connection) {
  ContextEntryRegistration::register_entry(tcp_connections, std::move(connection));
}

void IoContextImpl::unregister_tcp_connection(TcpConnectionImpl* connection) {
  ContextEntryRegistration::unregister_entry(tcp_connections, connection);
}

void IoContextImpl::queue_deferred_work(std::function<void()> callback) {
  deferred_work_write.push_back(std::move(callback));
}

void IoContextImpl::queue_deferred_work_atomic(std::function<void()> callback) {
  {
    std::lock_guard lock(deferred_work_atomic_mutex);
    deferred_work_atomic_write.push_back(std::move(callback));
  }
  notify();
}

void IoContextImpl::queue_ip_resolve(std::string hostname,
                                     std::function<void(Status, std::vector<IpAddress>)> callback) {
  ip_resolver.resolve(std::move(hostname), std::move(callback));
}

TimerManagerImpl::TimerKey IoContextImpl::register_timer(base::PreciseTime deadline,
                                                         std::function<void()> callback) {
  return timer_manager.register_timer(deadline, std::move(callback));
}

void IoContextImpl::unregister_timer(const TimerManagerImpl::TimerKey& key) {
  auto callback = timer_manager.unregister_timer(key);
  if (callback) {
    // Destroy the callback later to avoid freeing something in the middle of run loop.
    queue_deferred_work([callback = std::move(callback)] { (void)callback; });
  }
}

IoContext::RunResult IoContextImpl::run(const IoContext::RunParameters& parameters) {
  const auto now = base::PreciseTime::now();

  // Make sure all deferred work is done before we block on poll().
  while (!deferred_work_write.empty() || timer_manager.pending(now)) {
    run_deferred_work();
    timer_manager.poll(now);
  }

  int timeout_ms = -1;
  {
    if (parameters.timeout) {
      timeout_ms = int(std::min(*parameters.timeout, uint32_t(std::numeric_limits<int>::max())));
    }

    if (const auto timer_deadline = timer_manager.earliest_deadline()) {
      const auto time_to_deadline = *timer_deadline - now;
      verify(time_to_deadline > base::PreciseTime::from_milliseconds(0),
             "earliest timer has already expired");

      const auto time_to_deadline_ms = time_to_deadline.milliseconds() + 1;
      const int time_to_deadline_ms_clamped =
        time_to_deadline_ms > uint64_t(std::numeric_limits<int>::max())
          ? std::numeric_limits<int>::max()
          : int(time_to_deadline_ms);

      if (timeout_ms < 0) {
        timeout_ms = time_to_deadline_ms_clamped;
      } else if (timeout_ms > 0) {
        timeout_ms = std::min(timeout_ms, time_to_deadline_ms_clamped);
      }
    }

    if (parameters.stop_when_no_work && !has_any_non_atomic_work()) {
      return IoContext::RunResult::NoMoreWork;
    }
  }

  register_poll_entries();

  const auto [poll_status, signaled_entries] = poller->poll(poll_entries, timeout_ms);
  if (!poll_status) {
    log_error("poll failed with result: {}", poll_status.stringify());
    return IoContext::RunResult::Failed;
  }

  if (signaled_entries > 0) {
    handle_poll_events();
  }

  ip_resolver.poll();
  run_deferred_work_atomic();

  run_deferred_work();

  return IoContext::RunResult::Ok;
}

void IoContextImpl::notify() {
  verify(poller->cancel(), "failed to cancel IO context run");
}

void IoContextImpl::drain() {
  ip_resolver.exit();
  drain_deferred_work_atomic();

  for (uint32_t i = 0; has_any_non_atomic_work(); ++i) {
    verify(i < 100, "took too many steps to drain the IO context");

    drain_deferred_work();
    drain_tcp_listeners();
    drain_tcp_connections();
    drain_deferred_work_atomic();
    ip_resolver.drain();
    timer_manager.drain();
  }
}

}  // namespace async_net::detail