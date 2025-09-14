#include "IpResolverImpl.hpp"
#include "IoContextImpl.hpp"

#include <socklib/Socket.hpp>

namespace async_net::detail {

void IpResolverImpl::worker_run() {
  std::vector<Request> requests;
  while (true) {
    if (!worker_request_queue.pop_front_blocking(requests)) {
      return;
    }

    for (auto& request : requests) {
      auto [status, ips] = sock::IpResolver::ForIp<IpAddress>::resolve(request.hostname);
      worker_response_queue.push_back_one({
        .status = status,
        .resolved_ips = std::move(ips),
        .callback = std::move(request.callback),
      });
      io_context.notify();
    }

    requests.clear();
  }
}

IpResolverImpl::IpResolverImpl(IoContextImpl& io_context) : io_context(io_context) {
  worker = std::thread{[this] { this->worker_run(); }};
}

IpResolverImpl::~IpResolverImpl() {
  exit();
}

void IpResolverImpl::exit() {
  if (worker.joinable()) {
    worker_request_queue.request_exit();
    worker.join();
  }
}

void IpResolverImpl::resolve(
  std::string hostname,
  std::move_only_function<void(Status, std::vector<IpAddress>)> callback) {
  worker_request_queue.push_back_one({
    .hostname = std::move(hostname),
    .callback = std::move(callback),
  });
  pending_requests++;
}

void IpResolverImpl::poll() {
  worker_response_queue.pop_front_non_blocking(response_buffer);

  pending_requests -= response_buffer.size();

  for (auto& response : response_buffer) {
    response.callback(response.status, std::move(response.resolved_ips));
  }
  response_buffer.clear();
}

void IpResolverImpl::drain() {
  worker_response_queue.pop_front_non_blocking(response_buffer);

  pending_requests -= response_buffer.size();

  response_buffer.clear();
}

}  // namespace async_net::detail
