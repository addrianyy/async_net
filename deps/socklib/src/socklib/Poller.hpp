#pragma once
#include <memory>
#include <span>

#include "Status.hpp"

#define SOCKLIB_IMPLEMENT_ENUM_BIT_OPERATIONS(type) \
  constexpr inline type operator|(type a, type b) { \
    using T = std::underlying_type_t<type>;         \
    return type(T(a) | T(b));                       \
  }                                                 \
  constexpr inline type operator&(type a, type b) { \
    using T = std::underlying_type_t<type>;         \
    return type(T(a) & T(b));                       \
  }                                                 \
  constexpr inline type operator~(type a) {         \
    using T = std::underlying_type_t<type>;         \
    return type(~T(a));                             \
  }

namespace sock {

class Socket;

class Poller {
 public:
  struct CreateParameters {
    bool enable_cancellation = false;

    constexpr static CreateParameters default_parameters() { return CreateParameters{}; }
  };

  static std::unique_ptr<Poller> create(
    const CreateParameters& create_parameters = CreateParameters::default_parameters()
  );

  Poller() = default;

  Poller(Poller&& other) = delete;
  Poller& operator=(Poller&& other) = delete;

  Poller(const Poller& other) = delete;
  Poller& operator=(const Poller& other) = delete;

  virtual ~Poller() = default;

  enum class QueryEvents {
    None = 0,
    CanReceiveFrom = (1 << 0),
    CanSendTo = (1 << 1),
    CanAccept = CanReceiveFrom,
  };

  enum class StatusEvents {
    None = 0,
    Error = (1 << 0),
    Disconnected = (1 << 1),
    InvalidSocket = (1 << 2),
    CanReceiveFrom = (1 << 3),
    CanSendTo = (1 << 4),
    CanAccept = CanReceiveFrom,
  };

  struct PollEntry {
    const Socket* socket{};
    QueryEvents query_events{};
    StatusEvents status_events{};

    bool has_events(StatusEvents events) const;
    bool has_any_event(StatusEvents events) const;
  };

  virtual Result<size_t> poll(std::span<PollEntry> entries, int timeout_ms) = 0;
  virtual bool cancel() = 0;
};

SOCKLIB_IMPLEMENT_ENUM_BIT_OPERATIONS(Poller::QueryEvents)
SOCKLIB_IMPLEMENT_ENUM_BIT_OPERATIONS(Poller::StatusEvents)

}  // namespace sock

#undef SOCKLIB_IMPLEMENT_ENUM_BIT_OPERATIONS
