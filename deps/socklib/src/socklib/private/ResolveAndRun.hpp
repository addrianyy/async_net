#pragma once
#include "../Resolver.hpp"
#include "../Socket.hpp"

namespace sock::detail {

template <typename Result, typename Fn>
Result resolve_and_run(
  IpVersion ip_version, std::string_view hostname, uint16_t port, Fn&& callback
) {
  switch (ip_version) {
    case IpVersion::V4: {
      const auto resolved = IpResolver::resolve_ipv4(hostname);
      if (!resolved) {
        return std::unexpected{Status::HostnameNotFound};
      }
      Status error_status{};
      for (const auto& ip : *resolved) {
        auto result = callback(SocketIpV4Address{ip, port});
        if (result) {
          return result;
        }
        if (error_status == Status::Ok) {
          error_status = result.error();
        }
      }
      return std::unexpected{error_status};
    }
    case IpVersion::V6: {
      const auto resolved = IpResolver::resolve_ipv6(hostname);
      if (!resolved) {
        return std::unexpected{Status::HostnameNotFound};
      }
      Status error_status{};
      for (const auto& ip : *resolved) {
        auto result = callback(SocketIpV6Address{ip, port});
        if (result) {
          return result;
        }
        if (error_status == Status::Ok) {
          error_status = result.error();
        }
      }
      return std::unexpected{error_status};
    }
    default: {
      return std::unexpected{Status::HostnameNotFound};
    }
  }
}

}  // namespace sock::detail
