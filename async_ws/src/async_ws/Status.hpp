#pragma once
#include <async_net/Status.hpp>

#include <string>

namespace async_ws {

enum class Error {
#define X(variant) variant,

#include "Errors.inc"

#undef X
};

struct Status {
  Error error{};
  async_net::Status net_status{};

  std::string stringify() const;
};

}  // namespace async_ws
