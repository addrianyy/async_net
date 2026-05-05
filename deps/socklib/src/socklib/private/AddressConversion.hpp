#pragma once
#include "../Address.hpp"
#include "System.hpp"

#include <cstring>
#include <span>

namespace sock::detail {

struct SockaddrBuffer {
  alignas(32) uint8_t data[sizeof(sockaddr_storage)];
};

inline int address_type_to_protocol(SocketAddress::Type type) {
  switch (type) {
    case SocketAddress::Type::IpV4: return AF_INET;
    case SocketAddress::Type::IpV6: return AF_INET6;
    case SocketAddress::Type::Unix: return AF_UNIX;
    default:                        return AF_MAX;
  }
}

class AddressConverter {
  constexpr static size_t sockaddr_un_max_path_size = sizeof(::sockaddr_un{}.sun_path);
  static_assert(
    sockaddr_un_max_path_size >= (sock::SocketUnixAddress::max_path_size + 1),
    "unix socket path is shorter than expected"
  );
  constexpr static size_t sockaddr_un_header_size =
    sizeof(::sockaddr_un{}) - sockaddr_un_max_path_size;

 public:
  template <typename Fn>
  static void to_raw(const SocketAddress& address, Fn&& callback) {
    switch (address.type()) {
      case SocketAddress::Type::IpV4: {
        const auto& address_ipv4 = static_cast<const SocketIpV4Address&>(address);
        const auto components = address_ipv4.ip().components();

        sockaddr_in sockaddr_in{};
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_port = htons(address_ipv4.port());
        sockaddr_in.sin_addr.s_addr =
          (uint32_t(components[0]) << 0) | (uint32_t(components[1]) << 8) |
          (uint32_t(components[2]) << 16) | (uint32_t(components[3]) << 24);

        callback(reinterpret_cast<const sockaddr*>(&sockaddr_in), sizeof(sockaddr_in));

        break;
      }

      case SocketAddress::Type::IpV6: {
        const auto& address_ipv6 = static_cast<const SocketIpV6Address&>(address);
        const auto components = address_ipv6.ip().components();

        sockaddr_in6 sockaddr_in6{};
        sockaddr_in6.sin6_family = AF_INET6;
        sockaddr_in6.sin6_port = htons(address_ipv6.port());

        for (size_t i = 0; i < 8; ++i) {
#if defined(SOCKLIB_WINDOWS)
          sockaddr_in6.sin6_addr.u.Word[i] = htons(components[i]);
#elif defined(SOCKLIB_APPLE)
          sockaddr_in6.sin6_addr.__u6_addr.__u6_addr16[i] = htons(components[i]);
#else
          reinterpret_cast<uint16_t*>(sockaddr_in6.sin6_addr.s6_addr)[i] = htons(components[i]);
#endif
        }

        callback(reinterpret_cast<const sockaddr*>(&sockaddr_in6), sizeof(sockaddr_in6));

        break;
      }

      case SocketAddress::Type::Unix: {
        const auto& address_unix = static_cast<const SocketUnixAddress&>(address);
        const auto address_path = address_unix.path();

        sockaddr_un sockaddr_un{};
        sockaddr_un.sun_family = AF_UNIX;

        {
          // We have zeroed the sockaddr_un structure so we don't need to null terminate or set zero
          // byte at the beginning.
          const auto offset =
            address_unix.socket_namespace() == SocketUnixAddress::Namespace::Abstract ? 1 : 0;
          std::memcpy(sockaddr_un.sun_path + offset, address_path.data(), address_path.size());
        }

        // Always add one because we are either null terminating or adding 0 prefix for abstract
        // sockets.
        const auto address_path_bytes = address_path.size() + 1;

        callback(
          reinterpret_cast<const sockaddr*>(&sockaddr_un),
          socklen_t(sockaddr_un_header_size + address_path_bytes)
        );

        break;
      }

      default: break;
    }
  }

  static bool from_raw(
    const sockaddr* sockaddr_buffer, socklen_t sockaddr_size, SocketAddress& address
  ) {
    switch (address.type()) {
      case SocketAddress::Type::IpV4: {
        if (sockaddr_buffer->sa_family != AF_INET || sockaddr_size < sizeof(sockaddr_in)) {
          return false;
        }

        const auto dest = reinterpret_cast<SocketIpV4Address*>(&address);
        const auto source = reinterpret_cast<const sockaddr_in*>(sockaddr_buffer);

        const auto ip = source->sin_addr.s_addr;
        const std::array<uint8_t, 4> components{
          uint8_t(ip >> 0), uint8_t(ip >> 8), uint8_t(ip >> 16), uint8_t(ip >> 24)
        };

        *dest = SocketIpV4Address{IpV4Address{components}, ntohs(source->sin_port)};

        return true;
      }

      case SocketAddress::Type::IpV6: {
        if (sockaddr_buffer->sa_family != AF_INET6 || sockaddr_size < sizeof(sockaddr_in6)) {
          return false;
        }

        const auto dest = reinterpret_cast<SocketIpV6Address*>(&address);
        const auto source = reinterpret_cast<const sockaddr_in6*>(sockaddr_buffer);

        std::array<uint16_t, 8> components{};
        for (size_t i = 0; i < 8; ++i) {
#if defined(SOCKLIB_WINDOWS)
          components[i] = ntohs(source->sin6_addr.u.Word[i]);
#elif defined(SOCKLIB_APPLE)
          components[i] = ntohs(source->sin6_addr.__u6_addr.__u6_addr16[i]);
#else
          components[i] = ntohs(reinterpret_cast<const uint16_t*>(source->sin6_addr.s6_addr)[i]);
#endif
        }

        *dest = SocketIpV6Address{IpV6Address{components}, ntohs(source->sin6_port)};

        return true;
      }

      case SocketAddress::Type::Unix: {
        // Fail on zero sized path.
        if (sockaddr_buffer->sa_family != AF_UNIX || sockaddr_size <= sockaddr_un_header_size) {
          return false;
        }

        // Always > 0.
        const auto unix_path_buffer = std::span<const char>(
          reinterpret_cast<const sockaddr_un*>(sockaddr_buffer)->sun_path,
          size_t(sockaddr_size - sockaddr_un_header_size)
        );

        SocketUnixAddress::Namespace socket_namespace{};
        std::string_view unix_path{};

        if (unix_path_buffer[0] == 0) {
          const auto actual_path = unix_path_buffer.subspan(1);

          socket_namespace = SocketUnixAddress::Namespace::Abstract;
          unix_path = std::string_view{actual_path.data(), actual_path.size()};
        } else {
          socket_namespace = SocketUnixAddress::Namespace::Filesystem;
          unix_path = std::string_view{unix_path_buffer.data(), unix_path_buffer.size()};

          const auto null_terminator = unix_path.find_first_of('\0');
          if (null_terminator != std::string_view::npos) {
            unix_path = unix_path.substr(0, null_terminator);
          }
        }

        const auto converted_address = SocketUnixAddress::create(socket_namespace, unix_path);
        if (converted_address) {
          *reinterpret_cast<SocketUnixAddress*>(&address) = *converted_address;
        } else {
          return false;
        }
      }

      default: return false;
    }
  }

  static bool from_raw(
    const SockaddrBuffer& sockaddr_buffer, socklen_t sockaddr_size, SocketAddress& address
  ) {
    return from_raw(
      reinterpret_cast<const sockaddr*>(sockaddr_buffer.data), sockaddr_size, address
    );
  }
};

}  // namespace sock::detail
