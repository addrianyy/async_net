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

  constexpr bool success() const { return error == Error::Ok; }
  constexpr explicit operator bool() const { return success(); }

  std::string stringify() const;
};

}  // namespace async_ws
