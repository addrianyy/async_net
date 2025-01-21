#pragma once
#include <functional>
#include <memory>
#include <optional>

#include <base/macro/ClassTraits.hpp>

namespace async_net {

class IpResolver;
class Timer;

namespace detail {
class IoContextImpl;
class TcpConnectionImpl;
class TcpListenerImpl;
}  // namespace detail

class IoContext {
  friend detail::IoContextImpl;
  friend detail::TcpConnectionImpl;
  friend detail::TcpListenerImpl;
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
    std::optional<uint32_t> timeout{};
    bool stop_when_no_work{};
  };

  IoContext();
  ~IoContext();

  void post(std::function<void()> callback);

  template <typename T>
  void post_destroy(T value) {
    post([value_to_destroy = std::move(value)] { (void)value_to_destroy; });
  }

  void post_atomic(std::function<void()> callback);

  [[nodiscard]] RunResult run(const RunParameters& parameters);
  [[nodiscard]] bool run_until_no_work();

  void notify();

  void drain();
};

}  // namespace async_net