#include "Status.hpp"

namespace async_ws {

static std::string_view error_to_string(Error error) {
  switch (error) {
#define X(variant)     \
  case Error::variant: \
    return #variant;

#include "Errors.inc"

#undef X

    default:
      return "unknown";
  }
}

std::string Status::stringify() const {
  std::string result{error_to_string(error)};
  if (!net_status) {
    result += ' ';
    result += net_status.stringify();
  }
  return result;
}

}  // namespace async_ws