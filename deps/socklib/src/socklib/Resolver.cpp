#include "Resolver.hpp"
#include "private/AddressConversion.hpp"
#include "private/Initialization.hpp"
#include "private/System.hpp"

#include <algorithm>

namespace sock {

template <typename TSocketAddress>
static Result<std::vector<typename TSocketAddress::Ip>> resolve_ip_generic(
  int family, std::string_view hostname
) {
  SOCKLIB_ENSURE_INITIALIZED();

  addrinfo* resolved{};
  addrinfo hints{};

  hints.ai_family = family;
  hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_ALL;

  const auto hostname_s = std::string{hostname};

  // Different return value semantics, check if != 0.
  const auto addrinfo_result = ::getaddrinfo(hostname_s.c_str(), nullptr, &hints, &resolved);
  if (addrinfo_result != 0) {
    return std::unexpected{Status::HostnameNotFound};
  }

  std::vector<typename TSocketAddress::Ip> ips;

  for (auto current = resolved; current; current = current->ai_next) {
    if (current->ai_family != hints.ai_family) {
      continue;
    }

    constexpr bool is_ipv4 = std::is_same_v<TSocketAddress, SocketIpV4Address>;
    constexpr bool is_ipv6 = std::is_same_v<TSocketAddress, SocketIpV6Address>;
    static_assert(is_ipv4 || is_ipv6, "invalid address type");

    socklen_t sockaddr_size{};
    if constexpr (is_ipv4) {
      reinterpret_cast<sockaddr_in*>(current->ai_addr)->sin_port = 0;
      sockaddr_size = sizeof(sockaddr_in);
    } else if constexpr (is_ipv6) {
      reinterpret_cast<sockaddr_in6*>(current->ai_addr)->sin6_port = 0;
      sockaddr_size = sizeof(sockaddr_in6);
    }

    TSocketAddress address;
    if (detail::AddressConverter::from_raw(current->ai_addr, sockaddr_size, address)) {
      ips.push_back(address.ip());
    }
  }

  ::freeaddrinfo(resolved);

  if (ips.empty()) {
    return std::unexpected{Status::HostnameNotFound};
  }

  {
    std::sort(ips.begin(), ips.end());
    auto it = std::unique(ips.begin(), ips.end());
    ips.erase(it, ips.end());
  }

  return std::move(ips);
}

Result<std::vector<IpV4Address>> IpResolver::resolve_ipv4(std::string_view hostname) {
  return resolve_ip_generic<SocketIpV4Address>(AF_INET, hostname);
}

Result<std::vector<IpV6Address>> IpResolver::resolve_ipv6(std::string_view hostname) {
  return resolve_ip_generic<SocketIpV6Address>(AF_INET6, hostname);
}

}  // namespace sock
