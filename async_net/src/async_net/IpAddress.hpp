#pragma once
#include <socklib/Address.hpp>

namespace async_net {

using SocketAddress = sock::SocketIpV6Address;
using IpAddress = SocketAddress::Ip;

}  // namespace async_net