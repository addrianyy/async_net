#pragma once
#include "Json.hpp"

#include <cstdint>
#include <string>

namespace base::json {

namespace detail {
void print(std::string& output, const Value& value, uint32_t indentation);
void print(std::string& output, const Object& value, uint32_t indentation);
void print(std::string& output, const Array& value, uint32_t indentation);
void print(std::string& output, const String& value, uint32_t indentation);
void print(std::string& output, const Number& value, uint32_t indentation);
void print(std::string& output, const Boolean& value, uint32_t indentation);
void print(std::string& output, const Null& value, uint32_t indentation);
}  // namespace detail

template <typename T>
void print(std::string& output, const T& value, uint32_t indentation = 0) {
  detail::print(output, value, indentation);
}

template <typename T>
std::string print(const T& value, uint32_t indentation = 0) {
  std::string output;
  print(output, value, indentation);
  return output;
}

template <typename T>
void pretty_print(std::string& output, const T& value, uint32_t indentation = 2) {
  detail::print(output, value, indentation);
}

template <typename T>
std::string pretty_print(const T& value, uint32_t indentation = 2) {
  std::string output;
  pretty_print(output, value, indentation);
  return output;
}

}  // namespace base::json