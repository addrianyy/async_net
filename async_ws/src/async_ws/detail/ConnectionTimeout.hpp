#pragma once
#include <base/time/PreciseTime.hpp>

namespace async_ws::detail {

constexpr base::PreciseTime connection_timeout = base::PreciseTime::from_seconds(4);

}  // namespace async_ws::detail