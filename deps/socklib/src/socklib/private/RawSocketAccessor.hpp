#pragma once
#include "../Socket.hpp"

namespace sock::detail {

struct RawSocketAccessor {
  static RawSocket get(const Socket& socket) { return socket.socket_; }
};

}  // namespace sock::detail
