#pragma once
#include <socklib/Status.hpp>

namespace async_net {

using Status = sock::Status;

inline std::string_view status_to_string(Status status) {
  return sock::status_to_string(status);
}

}  // namespace async_net