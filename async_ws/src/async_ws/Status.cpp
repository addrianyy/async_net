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
  if (net_status != sock::Status::Ok) {
    result += ' ';
    result += sock::status_to_string(net_status);
  }
  return result;
}

}  // namespace async_ws