#include "CustomMaskingOverride.hpp"

#include <string>
#include <string_view>

namespace async_ws::detail {

static constexpr std::string_view header_key = "Custom-DisableWebSocketMasks";
static constexpr std::string_view header_value = "Disable-Masks";

void CustomMaskingOverride::inject_masking_override(websocket::http::Headers& headers) {
  headers.set(std::string(header_key), std::string(header_value));
}

bool CustomMaskingOverride::contain_masking_override(const websocket::http::Headers& headers) {
  if (const auto value = headers.get(header_key)) {
    return *value == header_value;
  }
  return false;
}

}  // namespace async_ws::detail