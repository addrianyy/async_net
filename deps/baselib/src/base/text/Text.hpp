#pragma once
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

#include <base/Platform.hpp>

#ifdef PLATFORM_APPLE
#include <cstdlib>
#include <type_traits>
#endif

namespace base::text {

std::string to_lowercase(std::string_view s);
std::string to_uppercase(std::string_view s);
bool equals_case_insensitive(std::string_view a, std::string_view b);

std::string_view lstrip(std::string_view s);
std::string_view rstrip(std::string_view s);
std::string_view strip(std::string_view s);

template <typename T>
bool to_number(std::string_view s, T& value) {
  value = {};

#ifdef PLATFORM_APPLE
  if constexpr (std::is_floating_point_v<T>) {
    const std::string string{s};
    char* end = nullptr;
    const auto result = std::strtod(string.c_str(), &end);
    if (result == HUGE_VAL || result == -HUGE_VAL || end != string.data() + string.size()) {
      return false;
    }
    value = T(result);
    return true;
  } else {
    const auto end = s.data() + s.size();

    const auto result = std::from_chars(s.data(), end, value);
    return result.ec == std::errc{} && result.ptr == end;
  }
#else
  const auto end = s.data() + s.size();

  const auto result = std::from_chars(s.data(), end, value);
  return result.ec == std::errc{} && result.ptr == end;
#endif
}
template <typename T>
bool to_number(const char* s, T& value) {
  return to_number(std::string_view{s}, value);
}

template <typename T>
bool to_number(std::string_view s, T& value, int base) {
  value = {};

  const auto end = s.data() + s.size();

  const auto result = std::from_chars(s.data(), end, value, base);
  return result.ec == std::errc{} && result.ptr == end;
}
template <typename T>
bool to_number(const char* s, T& value, int base) {
  return to_number(std::string_view{s}, value, base);
}

}  // namespace base::text