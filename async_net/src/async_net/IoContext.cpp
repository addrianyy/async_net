#include "IoContext.hpp"
#include "detail/IoContextImpl.hpp"

namespace async_net {

IoContext::IoContext() : impl_(std::make_unique<detail::IoContextImpl>()) {}
IoContext::~IoContext() {
  impl_->drain();
}

void IoContext::post(std::function<void()> callback) {
  impl_->queue_deferred_work(std::move(callback));
}

void IoContext::post_atomic(std::function<void()> callback) {
  impl_->queue_deferred_work_atomic(std::move(callback));
}

IoContext::RunResult IoContext::run(const RunParameters& parameters) {
  return impl_->run(parameters);
}

bool IoContext::run_until_no_work() {
  const RunParameters parameters{
    .timeout = std::nullopt,
    .stop_when_no_work = true,
  };
  while (true) {
    const auto result = impl_->run(parameters);
    if (result == RunResult::Failed) {
      return false;
    }
    if (result == RunResult::NoMoreWork) {
      return true;
    }
  }
}

void IoContext::notify() {
  impl_->notify();
}

void IoContext::drain() {
  impl_->drain();
}

}  // namespace async_net