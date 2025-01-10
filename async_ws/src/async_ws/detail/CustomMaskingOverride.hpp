#pragma once
#include <websocket/Http.hpp>

namespace async_ws::detail {

class CustomMaskingOverride {
 public:
  static void inject_masking_override(websocket::http::Headers& headers);
  static bool contain_masking_override(const websocket::http::Headers& headers);
};

}  // namespace async_ws::detail