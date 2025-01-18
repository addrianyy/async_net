#include "IpResolver.hpp"
#include "IoContext.hpp"
#include "detail/IoContextImpl.hpp"

namespace async_net {

void IpResolver::resolve(IoContext& context,
                         std::string hostname,
                         std::function<void(sock::Status, std::vector<IpAddress>)> callback) {
  context.impl_->queue_ip_resolve(std::move(hostname), std::move(callback));
}

}  // namespace async_net