#include "Printer.hpp"

#include <base/Panic.hpp>
#include <base/text/Format.hpp>

#include <algorithm>
#include <iterator>
#include <string_view>

constexpr static std::string_view indentation_string =
  "                                                                                                "
  "                                                                                                "
  "                                                                                                "
  "                                                                                                "
  "                                                                                                "
  "                                                                                                "
  "                                                                                                "
  "                                                                                               ";

using namespace base::json;
using namespace base::json::detail;

static char character_to_escape(char character) {
  switch (character) {
    case '"':
      return '"';
    case '\\':
      return '\\';
    case '\b':
      return 'b';
    case '\f':
      return 'f';
    case '\n':
      return 'n';
    case '\r':
      return 'r';
    case '\t':
      return 't';
    default:
      return 0;
  }
}

void Printer::indent() {
  current_indentation += indentation;
}
void Printer::deindent() {
  current_indentation -= indentation;
}

void Printer::print_indentation() {
  if (current_indentation == 0) {
    return;
  }

  if (indentation_string.size() >= current_indentation) {
    output += indentation_string.substr(0, current_indentation);
  } else {
    output.reserve(output.size() + current_indentation);

    size_t indentation_left = current_indentation;
    while (indentation_left > 0) {
      const auto size = std::min(indentation_left, indentation_string.size());
      output += indentation_string.substr(0, size);
      indentation_left -= size;
    }
  }
}

void Printer::print_string(std::string_view string) {
  output.reserve(string.size() + 2);

  output += '"';
  for (const char c : string) {
    const auto escape = character_to_escape(c);
    if (escape != 0) {
      output += '\\';
      output += escape;
    } else if (uint8_t(c) < 32 || uint8_t(c) >= 128) {
      base::format_to(std::back_insert_iterator(output), "\\u{:04x}", uint32_t(uint8_t(c)));
    } else {
      output += c;
    }
  }
  output += '"';
}

Printer::Printer(std::string& output, uint32_t indentation)
    : output(output), indentation(indentation), pretty(indentation > 0) {}

void Printer::print(const Value& value) {
  value.visit([this](const auto& v) { print(v); });
}

void Printer::print(const Object& object) {
  output += '{';

  if (pretty) {
    output += '\n';
    indent();
  }

  bool first = true;

  for (const auto& e : object.entries()) {
    if (!first) {
      output += ',';

      if (pretty) {
        output += '\n';
      }
    }
    first = false;

    print_indentation();
    print_string(e.first);
    output += ':';

    if (pretty) {
      output += ' ';
    }

    print(e.second);
  }

  if (pretty && !object.empty()) {
    output += '\n';
  }

  if (pretty) {
    deindent();
    print_indentation();
  }
  output += '}';
}

void Printer::print(const Array& array) {
  output += '[';

  if (pretty) {
    output += '\n';
    indent();
  }

  bool first = true;

  for (const auto& e : array.entries()) {
    if (!first) {
      output += ',';

      if (pretty) {
        output += '\n';
      }
    }
    first = false;

    print_indentation();
    print(e);
  }

  if (pretty && !array.empty()) {
    output += '\n';
  }

  if (pretty) {
    deindent();
    print_indentation();
  }
  output += ']';
}

void Printer::print(const Number& number) {
  switch (number.format()) {
    case Number::Format::Int: {
      const auto v = number.get_int();
      verify(v, "invalid format");
      base::format_to(std::back_insert_iterator(output), "{}", *v);
      break;
    }
    case Number::Format::UInt: {
      const auto v = number.get_uint();
      verify(v, "invalid format");
      base::format_to(std::back_insert_iterator(output), "{}", *v);
      break;
    }
    case Number::Format::Double: {
      const auto v = number.get_double();
      verify(v, "invalid format");
      base::format_to(std::back_insert_iterator(output), "{}", *v);
      break;
    }
    default:
      unreachable();
  }
}

void Printer::print(const String& string) {
  print_string(string.get());
}

void Printer::print(const Boolean& boolean) {
  output += bool(boolean) ? "true" : "false";
}

void Printer::print(const Null& null) {
  output += "null";
}
