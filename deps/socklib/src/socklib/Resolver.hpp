#pragma once
#include "Address.hpp"
#include "Status.hpp"

#include <string_view>
#include <vector>

namespace sock {

struct IpResolver {
  static Result<std::vector<IpV4Address>> resolve_ipv4(std::string_view hostname);
  static Result<std::vector<IpV6Address>> resolve_ipv6(std::string_view hostname);

  template <typename Ip>
  struct ForIp {};
};

template <>
struct IpResolver::ForIp<IpV4Address> {
  static Result<std::vector<IpV4Address>> resolve(std::string_view hostname) {
    return resolve_ipv4(hostname);
  }
};

template <>
struct IpResolver::ForIp<IpV6Address> {
  static Result<std::vector<IpV6Address>> resolve(std::string_view hostname) {
    return resolve_ipv6(hostname);
  }
};

}  // namespace sock
