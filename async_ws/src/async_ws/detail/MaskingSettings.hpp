#pragma once

namespace async_ws::detail {

struct MaskingSettings {
  bool send_masked{};
  bool receive_masked{};
};

}  // namespace async_ws::detail