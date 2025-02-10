#include "Printer.hpp"
#include "detail/Printer.hpp"

using namespace base::json;

template <typename T>
void print_value(std::string& output, const T& value, uint32_t indentation) {
  detail::Printer printer{output, indentation};
  printer.print(value);
}

void detail::print(std::string& output, const Value& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const Object& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const Array& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const String& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const Number& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const Boolean& value, uint32_t indentation) {
  print_value(output, value, indentation);
}
void detail::print(std::string& output, const Null& value, uint32_t indentation) {
  print_value(output, value, indentation);
}