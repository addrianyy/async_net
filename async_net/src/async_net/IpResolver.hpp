#pragma once
#include "IpAddress.hpp"
#include "Status.hpp"

#include <functional>
#include <string>
#include <vector>

namespace async_net {

class IoContext;

class IpResolver {
 public:
  static void resolve(IoContext& context,
                      std::string hostname,
                      std::move_only_function<void(sock::Status, std::vector<IpAddress>)> callback);
};

}  // namespace async_net
