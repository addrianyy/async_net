#include "Status.hpp"

namespace sock {

std::string_view status_to_string(Status status) {
  switch (status) {
#define X(variant) \
  case Status::variant: return #variant;

#include "Status.inc"

#undef X

    default: return "<unknown>";
  }
}

}  // namespace sock
