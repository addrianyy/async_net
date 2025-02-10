#include "Lexer.hpp"

#include <base/text/Text.hpp>

using namespace base::json;
using namespace base::json::detail;

static bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_separator(char c) {
  if (is_whitespace(c)) {
    return true;
  }
  return c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':' || c == '/';
}

static TokenType punctuation_to_token_type(char c) {
  switch (c) {
    case '{':
      return TokenType::OpeningBrace;
    case '}':
      return TokenType::ClosingBrace;
    case '[':
      return TokenType::OpeningBracket;
    case ']':
      return TokenType::ClosingBracket;
    case ',':
      return TokenType::Comma;
    case ':':
      return TokenType::Colon;
    default:
      return TokenType::Invalid;
  }
}

static char escape_to_character(char escape) {
  switch (escape) {
    case '"':
      return '"';
    case '\\':
      return '\\';
    case '/':
      return '/';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      return 0;
  }
}

Token::Location Lexer::current_location() const {
  const auto column = line_start.size() - source.size();
  return {
    .line = line_number,
    .column = uint32_t(column),
  };
}

void Lexer::next_line() {
  line_start = source;
  line_number++;
}

bool Lexer::skip_whitespaces() {
  size_t consumed = 0;

  while (!source.empty()) {
    const auto ch = source[0];
    if (!is_whitespace(ch)) {
      break;
    }

    source = source.substr(1);
    if (ch == '\n') {
      next_line();
    }

    consumed++;
  }

  return consumed > 0;
}

bool Lexer::skip_comments() {
  size_t consumed = 0;

  while (source.starts_with("//")) {
    const auto new_line_index = source.find_first_of('\n');
    if (new_line_index == std::string_view::npos) {
      source = {};
    } else {
      source = source.substr(new_line_index + 1);
    }

    next_line();

    consumed++;
  }

  return consumed > 0;
}

void Lexer::skip_characters() {
  while (skip_whitespaces() | skip_comments()) {
  }
}

size_t Lexer::consume_digits() {
  size_t digit_count = 0;

  while (!source.empty() && is_digit(source[0])) {
    source = source.substr(1);
    digit_count++;
  }

  return digit_count;
}

Token Lexer::consume_string() {
  const auto location = current_location();
  const auto invalid_token = [&] {
    return Token{
      .type = TokenType::Invalid,
      .location = location,
    };
  };

  source = source.substr(1);

  std::string string;

  while (true) {
    if (source.empty()) {
      return invalid_token();
    }

    const char ch = source[0];
    source = source.substr(1);

    if (ch == '"') {
      break;
    }

    if (ch == '\\') {
      if (source.empty()) {
        return invalid_token();
      }

      const char escape = source[0];
      source = source.substr(1);

      if (escape == 'u') {
        if (source.size() < 4) {
          return invalid_token();
        }

        const auto code = source.substr(0, 4);
        source = source.substr(4);

        uint32_t parsed_code{};
        if (!base::text::to_number(code, parsed_code, 16)) {
          return invalid_token();
        }

        if (parsed_code >= 128) {
          // TODO: Add support for unicode characters.
          return invalid_token();
        }

        string.push_back(char(parsed_code));
      } else {
        const auto escaped_character = escape_to_character(escape);
        if (escaped_character == 0) {
          return invalid_token();
        } else {
          string.push_back(escaped_character);
        }
      }
    } else {
      // Reject non-escaped control characters in strings.
      if (uint8_t(ch) < 32 && ch != '\t') {
        return invalid_token();
      }

      string.push_back(ch);
    }
  }

  return Token{
    .type = TokenType::String,
    .location = location,
    .string = std::move(string),
  };
}

Token Lexer::consume_number() {
  const auto location = current_location();
  const auto invalid_token = [&] {
    return Token{
      .type = TokenType::Invalid,
      .location = location,
    };
  };

  auto number_start = source;

  bool number_negative{};
  if (source[0] == '-') {
    number_negative = true;
    source = source.substr(1);
  }

  if (source.empty()) {
    return invalid_token();
  }

  if (source[0] == '0') {
    source = source.substr(1);
  } else {
    if (consume_digits() == 0) {
      return invalid_token();
    }
  }

  bool has_fractional_part{};
  if (!source.empty() && source[0] == '.') {
    has_fractional_part = true;
    source = source.substr(1);

    if (consume_digits() == 0) {
      return invalid_token();
    }
  }

  bool has_exponent{};
  if (!source.empty() && (source[0] == 'e' || source[0] == 'E')) {
    has_exponent = true;
    source = source.substr(1);

    if (!source.empty() && (source[0] == '+' || source[0] == '-')) {
      source = source.substr(1);
    }

    if (consume_digits() == 0) {
      return invalid_token();
    }
  }

  if (source.empty() || is_separator(source[0])) {
    const auto number_text = number_start.substr(0, number_start.size() - source.size());

    Number::Format format{};

    uint64_t integer_value{};
    double double_value{};

    if (has_fractional_part || has_exponent) {
      format = Number::Format::Double;

      if (!base::text::to_number(number_text, double_value)) {
        return invalid_token();
      }
    } else {
      if (number_negative) {
        format = Number::Format::Int;

        int64_t i_value{};
        if (!base::text::to_number(number_text, i_value)) {
          return invalid_token();
        }
        integer_value = uint64_t(i_value);
      } else {
        format = Number::Format::UInt;

        if (!base::text::to_number(number_text, integer_value)) {
          return invalid_token();
        }
      }
    }

    return Token{
      .type = TokenType::Number,
      .location = location,
      .number_format = format,
      .number_integer = integer_value,
      .number_double = double_value,
    };
  }

  return invalid_token();
}

Token Lexer::consume_keyword() {
  static constexpr std::string_view true_token = "true";
  static constexpr std::string_view false_token = "false";
  static constexpr std::string_view null_token = "null";

  const auto location = current_location();

  TokenType token_type = TokenType::Invalid;

  if (source.starts_with(true_token)) {
    token_type = TokenType::True;
    source = source.substr(true_token.size());
  } else if (source.starts_with(false_token)) {
    token_type = TokenType::False;
    source = source.substr(false_token.size());
  } else if (source.starts_with(null_token)) {
    token_type = TokenType::Null;
    source = source.substr(null_token.size());
  }

  if (token_type != TokenType::Invalid) {
    if (source.empty() || is_separator(source[0])) {
      return Token{
        .type = token_type,
        .location = location,
      };
    }
  }

  return Token{
    .type = TokenType::Invalid,
    .location = location,
  };
}

Lexer::Lexer(std::string_view source) : source(source), line_start(source) {}

Token Lexer::next() {
  skip_characters();

  const auto location = current_location();

  if (source.empty()) {
    return Token{
      .type = TokenType::Eof,
      .location = location,
    };
  }

  const auto ch = source[0];

  if (const auto token_type = punctuation_to_token_type(ch); token_type != TokenType::Invalid) {
    source = source.substr(1);

    return Token{
      .type = token_type,
      .location = location,
    };
  }

  if (ch == '-' || is_digit(ch)) {
    return consume_number();
  } else if (ch == '"') {
    return consume_string();
  } else if (ch == 't' || ch == 'f' || ch == 'n') {
    return consume_keyword();
  }

  return Token{
    .type = TokenType::Invalid,
    .location = location,
  };
}
