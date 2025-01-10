#include "Timer.hpp"
#include "IoContext.hpp"
#include "detail/IoContextImpl.hpp"

namespace async_net {

Timer::Timer(IoContext& context, uint64_t id, base::PreciseTime deadline)
    : context_(&context), id_(id), deadline_(deadline) {}

Timer::Timer(Timer&& other) noexcept {
  context_ = other.context_;
  id_ = other.id_;
  deadline_ = other.deadline_;

  other.context_ = nullptr;
  other.id_ = invalid_id;
  other.deadline_ = {};
}

Timer& Timer::operator=(Timer&& other) noexcept {
  if (this != &other) {
    reset();

    context_ = other.context_;
    id_ = other.id_;
    deadline_ = other.deadline_;

    other.context_ = nullptr;
    other.id_ = invalid_id;
    other.deadline_ = {};
  }

  return *this;
}

Timer::~Timer() {
  reset();
}

Timer Timer::invoke_at_deadline(IoContext& context,
                                base::PreciseTime deadline,
                                std::function<void()> callback) {
  const auto key = context.impl_->register_timer(deadline, std::move(callback));
  return Timer{context, key.id, key.deadline};
}

Timer Timer::invoke_after(IoContext& context,
                          base::PreciseTime timeout,
                          std::function<void()> callback) {
  return invoke_at_deadline(context, base::PreciseTime::now() + timeout, std::move(callback));
}

void Timer::invoke_at_deadline_detached(IoContext& context,
                                        base::PreciseTime deadline,
                                        std::function<void()> callback) {
  context.impl_->register_timer(deadline, std::move(callback));
}

void Timer::invoke_after_detached(IoContext& context,
                                  base::PreciseTime timeout,
                                  std::function<void()> callback) {
  invoke_at_deadline_detached(context, base::PreciseTime::now() + timeout, std::move(callback));
}

void Timer::reset() {
  if (context_ && id_ != invalid_id) {
    context_->impl_->unregister_timer({
      .id = id_,
      .deadline = deadline_,
    });
    context_ = nullptr;
    id_ = invalid_id;
    deadline_ = {};
  }
}

}  // namespace async_net
