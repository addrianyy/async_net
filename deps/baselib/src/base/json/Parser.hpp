#pragma once
#include "Json.hpp"

#include <optional>
#include <string_view>

#include <base/text/Format.hpp>

namespace base::json {

struct ParsingErrorLocation {
  uint32_t line{};
  uint32_t column{};
};

std::optional<Value> parse(std::string_view text, ParsingErrorLocation* error_location = nullptr);

}  // namespace base::json

template <>
struct fmt::formatter<base::json::ParsingErrorLocation> {
  constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
    return ctx.begin();
  };

  auto format(const base::json::ParsingErrorLocation& v, format_context& ctx) const
    -> format_context::iterator {
    return format_to(ctx.out(), "line {}, column {}", v.line, v.column);
  }
};
