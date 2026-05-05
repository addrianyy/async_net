#include "Poller.hpp"
#include "ConnectedPair.hpp"
#include "Socket.hpp"
#include "StreamSocket.hpp"
#include "private/Error.hpp"
#include "private/Initialization.hpp"
#include "private/RawSocketAccessor.hpp"
#include "private/SocketHelpers.hpp"
#include "private/System.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace sock {

#if defined(SOCKLIB_WINDOWS)

class PollCanceller {
  StreamSocket write_socket;
  StreamSocket read_socket;
  bool initialized{false};

 public:
  PollCanceller() {
    auto pair = ConnectedPair::create({
      .non_blocking = true,
    });
    if (pair) {
      write_socket = std::move(pair->first);
      read_socket = std::move(pair->second);
      initialized = true;
    }
  }

  bool valid() const { return initialized; }

  detail::RawSocket cancel_socket() const { return detail::RawSocketAccessor::get(read_socket); }

  bool drain() {
    uint8_t buffer[32];

    bool first = true;

    while (true) {
      const auto received_bytes = read_socket.receive(buffer);
      if (first) {
        if (!received_bytes) {
          return false;
        }
        first = false;
      }

      if (received_bytes.error() == Status::WouldBlock ||
          (received_bytes && *received_bytes < sizeof(buffer))) {
        return true;
      }

      if (!received_bytes) {
        return false;
      }
    }
  }

  bool signal() {
    uint8_t buffer[1]{};
    const auto sent_bytes = write_socket.send(buffer);
    return sent_bytes && *sent_bytes == 1;
  }
};

#else

class PollCanceller {
  int read_pipe{-1};
  int write_pipe{-1};
  bool initialized{false};

 public:
  PollCanceller() {
    int pipe_fds[2]{};
    if (::pipe(pipe_fds) == 0) {
      read_pipe = pipe_fds[0];
      write_pipe = pipe_fds[1];

      if (::fcntl(read_pipe, F_SETFL, O_NONBLOCK | (::fcntl(read_pipe, F_GETFL))) == -1) {
        return;
      }

      if (::fcntl(write_pipe, F_SETFL, O_NONBLOCK | (::fcntl(write_pipe, F_GETFL))) == -1) {
        return;
      }

      initialized = true;
    }
  }

  ~PollCanceller() {
    if (read_pipe != -1) {
      ::close(read_pipe);
    }
    if (write_pipe != -1) {
      ::close(write_pipe);
    }
  }

  bool valid() const { return initialized; }

  detail::RawSocket cancel_socket() const { return read_pipe; }

  bool drain() {
    uint8_t buffer[32];

    bool first = true;

    while (true) {
      const auto result =
        detail::retry_on_eintr([&] { return ::read(read_pipe, buffer, sizeof(buffer)); });
      if (first) {
        if (result <= 0) {
          return false;
        }
        first = false;
      }

      if ((result == -1 && errno == EWOULDBLOCK) || result < sizeof(buffer)) {
        return true;
      }

      if (result <= 0) {
        return false;
      }
    }
  }

  bool signal() {
    uint8_t buffer[1]{};
    const auto result = detail::retry_on_eintr([this, buffer] {
      return ::write(write_pipe, buffer, sizeof(buffer));
    });
    return result == 1;
  }
};

#endif

class PollerImpl : public Poller {
#if defined(SOCKLIB_WINDOWS)
  using RawEntry = WSAPOLLFD;
#else
  using RawEntry = pollfd;
#endif

  std::vector<RawEntry> raw_entries;

  std::unique_ptr<PollCanceller> canceller;
  std::atomic_bool cancel_pending{false};

 public:
  explicit PollerImpl(const CreateParameters& create_parameters) {
    if (create_parameters.enable_cancellation) {
      canceller = std::make_unique<PollCanceller>();
    }
  }

  operator bool() const {
    if (canceller && !canceller->valid()) {
      return false;
    }
    return true;
  }

  Result<size_t> poll(std::span<PollEntry> entries, int timeout_ms) override {
    if (entries.empty() && !canceller) {
      if (timeout_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
      } else if (timeout_ms < 0) {
        std::this_thread::sleep_for(std::chrono::hours(24));
      }
      return 0;
    }

    raw_entries.resize(entries.size());

    for (size_t i = 0; i < entries.size(); i++) {
      auto& source = entries[i];
      auto& dest = raw_entries[i];

      source.status_events = {};

      dest.fd =
        source.socket ? detail::RawSocketAccessor::get(*source.socket) : detail::RawSocket{-1};
      dest.events = {};

      if ((source.query_events & QueryEvents::CanReceiveFrom) == QueryEvents::CanReceiveFrom) {
        dest.events |= POLLIN;
      }
      if ((source.query_events & QueryEvents::CanSendTo) == QueryEvents::CanSendTo) {
        dest.events |= POLLOUT;
      }
    }

    if (canceller) {
      if (cancel_pending) {
        if (canceller->drain()) {
          cancel_pending = false;
          return 0;
        }
      }

      raw_entries.push_back({
        .fd = canceller->cancel_socket(),
        .events = POLLIN,
      });
    }

#if defined(SOCKLIB_WINDOWS)
    const auto poll_result = detail::retry_on_eintr([&] {
      return ::WSAPoll(raw_entries.data(), ULONG(raw_entries.size()), timeout_ms);
    });
#else
    const auto poll_result = detail::retry_on_eintr([&] {
      return ::poll(raw_entries.data(), int(raw_entries.size()), timeout_ms);
    });
#endif

    if (detail::is_error(poll_result)) {
      return std::unexpected{detail::system_last_error_to_status()};
    }

    auto signaled_entries = size_t(poll_result);

    if (canceller) {
      if (const auto& last = raw_entries[entries.size()]; last.revents) {
        const bool signaled_input = last.revents & POLLIN;
        bool cancellation_error = last.revents & (POLLERR | POLLHUP | POLLNVAL);

        if (signaled_input) {
          if (canceller->drain()) {
            cancel_pending.store(false);
          } else {
            cancellation_error = true;
          }
        }

        if (cancellation_error) {
          return std::unexpected{sock::Status::CancellationFailed};
        }

        signaled_entries -= 1;
      }
    }

    if (signaled_entries > 0) {
      for (size_t i = 0; i < entries.size(); i++) {
        const auto& source = raw_entries[i];
        if (!source.revents) {
          continue;
        }

        auto& dest = entries[i];

        const auto set_event = [&](int raw_flag, StatusEvents flag) {
          if (source.revents & raw_flag) {
            dest.status_events = dest.status_events | flag;
          }
        };

        set_event(POLLERR, StatusEvents::Error);
        set_event(POLLHUP, StatusEvents::Disconnected);
        set_event(POLLNVAL, StatusEvents::InvalidSocket);
        set_event(POLLIN, StatusEvents::CanReceiveFrom);
        set_event(POLLOUT, StatusEvents::CanSendTo);
      }
    }

    return signaled_entries;
  }

  bool cancel() override {
    if (canceller) {
      if (!cancel_pending.exchange(true)) {
        return canceller->signal();
      }
      return true;
    } else {
      return false;
    }
  }
};

std::unique_ptr<Poller> Poller::create(const CreateParameters& create_parameters) {
  if (!detail::initialize_socket_library()) {
    return nullptr;
  }
  auto impl = std::make_unique<PollerImpl>(create_parameters);
  if (!*impl) {
    return nullptr;
  }
  return impl;
}

bool Poller::PollEntry::has_events(StatusEvents events) const {
  return (status_events & events) == events;
}
bool Poller::PollEntry::has_any_event(StatusEvents events) const {
  return (status_events & events) != StatusEvents::None;
}

}  // namespace sock
