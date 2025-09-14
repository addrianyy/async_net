#pragma once
#include <functional>
#include <memory>
#include <optional>

#include <base/macro/ClassTraits.hpp>
#include <base/time/PreciseTime.hpp>

namespace async_net {

class IpResolver;
class Timer;

namespace detail {
class IoContextImpl;
class TcpConnectionImpl;
class TcpListenerImpl;
class UdpSocketImpl;
}  // namespace detail

class IoContext {
  friend detail::IoContextImpl;
  friend detail::TcpConnectionImpl;
  friend detail::TcpListenerImpl;
  friend detail::UdpSocketImpl;
  friend IpResolver;
  friend Timer;

  std::unique_ptr<detail::IoContextImpl> impl_;

 public:
  CLASS_NON_COPYABLE(IoContext)

  enum class RunResult {
    Ok,
    Failed,
    NoMoreWork,
  };

  struct RunParameters {
    std::optional<base::PreciseTime> timeout{};
    bool stop_when_no_work{};
  };

  IoContext();
  ~IoContext();

  void post(std::move_only_function<void()> callback);

  template <typename T>
  void post_destroy(T value) {
    post([value_to_destroy = std::move(value)] { (void)value_to_destroy; });
  }

  void post_atomic(std::move_only_function<void()> callback);

  [[nodiscard]] RunResult run(const RunParameters& parameters);
  [[nodiscard]] bool run_until_no_work();

  void notify();

  void drain();
};

}  // namespace async_net
