#pragma once
#include "IpAddress.hpp"
#include "Status.hpp"

#include <functional>
#include <string>

namespace async_net {

class IoContext;

class IpResolver {
 public:
  static void resolve(IoContext& context,
                      std::string hostname,
                      std::function<void(sock::Status, IpAddress)> callback);
};

}  // namespace async_net