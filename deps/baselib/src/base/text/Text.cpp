#include "Text.hpp"

#include <cctype>

using namespace base;

std::string text::to_lowercase(std::string_view s) {
  std::string result;
  result.reserve(s.size());

  for (const auto ch : s) {
    const auto conv = std::tolower(ch);
    if (conv > 0 && conv < 256) {
      result.push_back(char(conv));
    } else {
      result.push_back('?');
    }
  }

  return result;
}

std::string text::to_uppercase(std::string_view s) {
  std::string result;
  result.reserve(s.size());

  for (const auto ch : s) {
    const auto conv = std::toupper(ch);
    if (conv > 0 && conv < 256) {
      result.push_back(char(conv));
    } else {
      result.push_back('?');
    }
  }

  return result;
}

bool text::equals_case_insensitive(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(a[i]) != std::tolower(b[i])) {
      return false;
    }
  }

  return true;
}

std::string_view text::lstrip(std::string_view s) {
  while (!s.empty() && std::isspace(s.front())) {
    s = s.substr(1);
  }
  return s;
}

std::string_view text::rstrip(std::string_view s) {
  while (!s.empty() && std::isspace(s.back())) {
    s = s.substr(0, s.size() - 1);
  }
  return s;
}

std::string_view text::strip(std::string_view s) {
  return lstrip(rstrip(s));
}
