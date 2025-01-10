#pragma once
#include <functional>
#include <string>
#include <thread>

#include <base/concurrency/FastConcurrentQueue.hpp>
#include <base/macro/ClassTraits.hpp>

#include <async_net/IpAddress.hpp>
#include <async_net/Status.hpp>

namespace async_net::detail {

class IoContextImpl;

class IpResolverImpl {
  struct Request {
    std::string hostname{};
    std::function<void(Status, IpAddress)> callback;
  };
  struct Response {
    Status status{};
    IpAddress resolved_ip{};
    std::function<void(Status, IpAddress)> callback;
  };

  IoContextImpl& io_context;

  std::thread worker;
  base::FastConcurrentQueue<Request> worker_request_queue;
  base::FastConcurrentQueue<Response> worker_response_queue;

  size_t pending_requests{};
  std::vector<Response> response_buffer;

  void worker_run();

 public:
  CLASS_NON_COPYABLE_NON_MOVABLE(IpResolverImpl)

  explicit IpResolverImpl(IoContextImpl& io_context);
  ~IpResolverImpl();

  void exit();

  void resolve(std::string hostname, std::function<void(Status, IpAddress)> callback);
  void poll();
  void drain();

  bool empty() const { return pending_requests == 0; }
};

}  // namespace async_net::detail
