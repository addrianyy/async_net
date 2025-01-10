#include "TimerManagerImpl.hpp"

#include <base/Panic.hpp>

namespace async_net::detail {

TimerManagerImpl::TimerKey TimerManagerImpl::register_timer(base::PreciseTime deadline,
                                                            std::function<void()> callback) {
  const auto id = next_timer_id++;

  TimerEntry entry{
    .id = id,
    .deadline = deadline,
    .callback = std::move(callback),
  };

  verify(timers.insert(std::move(entry)).second, "failed to register timer");

  return {id, deadline};
}

std::function<void()> TimerManagerImpl::unregister_timer(const TimerKey& key) {
  TimerEntry entry{
    .id = key.id,
    .deadline = key.deadline,
  };

  std::function<void()> callback;

  if (auto it = timers.find(entry); it != timers.end()) {
    callback = std::move(it->callback);
    timers.erase(it);
  }

  return callback;
}

void TimerManagerImpl::poll(base::PreciseTime now) {
  auto it = timers.begin();

  while (it != timers.end()) {
    if (it->deadline > now) {
      break;
    }
    pending_callbacks.emplace_back(std::move(it->callback));
    it = timers.erase(it);
  }

  for (auto& callback : pending_callbacks) {
    callback();
  }
  pending_callbacks.clear();
}

void TimerManagerImpl::drain() {
  for (auto& timer : timers) {
    pending_callbacks.emplace_back(std::move(timer.callback));
  }

  timers.clear();
  pending_callbacks.clear();
}

std::optional<base::PreciseTime> TimerManagerImpl::earliest_deadline() const {
  if (timers.empty()) {
    return std::nullopt;
  }

  return timers.begin()->deadline;
}

bool TimerManagerImpl::pending(base::PreciseTime now) const {
  if (timers.empty()) {
    return false;
  }

  return timers.begin()->deadline <= now;
}

}  // namespace async_net::detail
