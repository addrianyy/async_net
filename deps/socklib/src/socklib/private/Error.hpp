#pragma once
#include "../Status.hpp"
#include "System.hpp"

#include <cstdint>

namespace sock::detail {

#if defined(SOCKLIB_WINDOWS)
static constexpr int default_error_value = SOCKET_ERROR;
#else
static constexpr int default_error_value = -1;
#endif

template <typename T>
bool is_error(T value) {
  return value == default_error_value;
}

Status system_error_to_status(int error_num);
Status system_last_error_to_status();

}  // namespace sock::detail
