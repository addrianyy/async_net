#pragma once
#include "Lexer.hpp"

#include <base/json/Json.hpp>

#include <optional>

namespace base::json::detail {

class Parser {
  Lexer lexer;
  Token current_token{};

  void next_token();

  std::optional<Value> parse_object();
  std::optional<Value> parse_array();
  std::optional<Value> parse_number();
  std::optional<Value> parse_value();
  std::optional<Value> parse_root_value();

 public:
  explicit Parser(Lexer lexer);

  std::optional<Value> parse(Token::Location& error_location);
};

}  // namespace base::json::detail