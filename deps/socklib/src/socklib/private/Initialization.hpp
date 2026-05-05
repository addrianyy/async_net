#pragma once
#include "../Status.hpp"
#include "System.hpp"

namespace sock::detail {

#if defined(SOCKLIB_WINDOWS)
bool initialize_socket_library();
#else
inline bool initialize_socket_library() {
  return true;
}
#endif

}  // namespace sock::detail

#if defined(SOCKLIB_WINDOWS)
#define SOCKLIB_ENSURE_INITIALIZED()                              \
  if (!::sock::detail::initialize_socket_library()) {             \
    return std::unexpected{::sock::Status::InitializationFailed}; \
  }
#else
#define SOCKLIB_ENSURE_INITIALIZED()
#endif
