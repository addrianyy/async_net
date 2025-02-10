#pragma once
#include <cstdint>
#include <string>
#include <string_view>

#include <base/json/Json.hpp>

namespace base::json::detail {

enum class TokenType {
  Colon,
  Comma,
  OpeningBracket,
  ClosingBracket,
  OpeningBrace,
  ClosingBrace,

  Null,
  True,
  False,

  String,
  Number,

  Eof,
  Invalid,
};

struct Token {
  struct Location {
    uint32_t line{};
    uint32_t column{};
  };

  TokenType type{};
  Location location{};

  std::string string{};

  Number::Format number_format{};
  uint64_t number_integer{};
  double number_double{};
};

class Lexer {
  std::string_view source;

  std::string_view line_start;
  uint32_t line_number = 1;

  Token::Location current_location() const;

  void next_line();

  bool skip_whitespaces();
  bool skip_comments();
  void skip_characters();

  size_t consume_digits();

  Token consume_string();
  Token consume_number();
  Token consume_keyword();

 public:
  explicit Lexer(std::string_view source);

  Token next();
};

}  // namespace base::json::detail