#pragma once
#include <expected>
#include <string_view>

namespace sock {

enum class Status {
#define X(variant) variant,

#include "Status.inc"

#undef X
};

template <typename Value>
using Result = std::expected<Value, Status>;

std::string_view status_to_string(Status status);

}  // namespace sock
