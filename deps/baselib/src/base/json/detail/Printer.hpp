#pragma once
#include <cstdint>
#include <string>

#include <base/json/Json.hpp>

namespace base::json::detail {

class Printer {
  std::string& output;

  const uint32_t indentation{};
  const bool pretty{};

  uint32_t current_indentation{};

  void indent();
  void deindent();

  void print_indentation();
  void print_string(std::string_view string);

 public:
  Printer(std::string& output, uint32_t indentation);

  void print(const Value& value);

  void print(const Object& object);
  void print(const Array& array);
  void print(const Number& number);
  void print(const String& string);
  void print(const Boolean& boolean);
  void print(const Null& null);
};

}  // namespace base::json::detail
