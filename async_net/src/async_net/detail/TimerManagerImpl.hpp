#pragma once
#include <base/macro/ClassTraits.hpp>
#include <base/time/PreciseTime.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <set>

namespace async_net::detail {

class IoContextImpl;

class TimerManagerImpl {
  struct TimerEntry {
    uint64_t id{};
    base::PreciseTime deadline{};
    mutable std::move_only_function<void()> callback;

    bool operator<(const TimerEntry& other) const {
      if (deadline == other.deadline) {
        return id < other.id;
      } else {
        return deadline < other.deadline;
      }
    }

    bool operator==(const TimerEntry& other) const {
      return deadline == other.deadline && id == other.id;
    }
  };

  std::set<TimerEntry> timers;
  uint64_t next_timer_id{};

  std::vector<std::move_only_function<void()>> pending_callbacks;

 public:
  CLASS_NON_COPYABLE_NON_MOVABLE(TimerManagerImpl)

  TimerManagerImpl() = default;

  struct TimerKey {
    uint64_t id{};
    base::PreciseTime deadline;
  };

  TimerKey register_timer(base::PreciseTime deadline, std::move_only_function<void()> callback);
  std::move_only_function<void()> unregister_timer(const TimerKey& key);

  void poll(base::PreciseTime now);
  void drain();

  std::optional<base::PreciseTime> earliest_deadline() const;

  bool empty() const { return timers.empty(); }
  bool pending(base::PreciseTime now) const;
};

}  // namespace async_net::detail
