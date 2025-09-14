#pragma once
#include <cstdint>
#include <functional>
#include <limits>

#include <base/macro/ClassTraits.hpp>
#include <base/time/PreciseTime.hpp>

namespace async_net {

class IoContext;

class Timer {
  constexpr static uint64_t invalid_id = std::numeric_limits<uint64_t>::max();

  IoContext* context_{};
  uint64_t id_{invalid_id};
  base::PreciseTime deadline_{};

  Timer(IoContext& context, uint64_t id, base::PreciseTime deadline);

 public:
  CLASS_NON_COPYABLE(Timer)

  Timer() = default;
  ~Timer();

  static Timer invoke_at_deadline(IoContext& context,
                                  base::PreciseTime deadline,
                                  std::move_only_function<void()> callback);
  static Timer invoke_after(IoContext& context,
                            base::PreciseTime timeout,
                            std::move_only_function<void()> callback);

  static void invoke_at_deadline_detached(IoContext& context,
                                          base::PreciseTime deadline,
                                          std::move_only_function<void()> callback);
  static void invoke_after_detached(IoContext& context,
                                    base::PreciseTime timeout,
                                    std::move_only_function<void()> callback);

  Timer(Timer&& other) noexcept;
  Timer& operator=(Timer&& other) noexcept;

  IoContext* io_context() { return context_; }
  const IoContext* io_context() const { return context_; }

  explicit operator bool() const { return context_ && id_ != invalid_id; }

  void reset();
};

}  // namespace async_net
