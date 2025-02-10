#include "Parser.hpp"

#include <base/Panic.hpp>

using namespace base::json;
using namespace base::json::detail;

void Parser::next_token() {
  if (current_token.type != TokenType::Eof && current_token.type != TokenType::Invalid) {
    current_token = lexer.next();
  }
}

std::optional<Value> Parser::parse_object() {
  verify(current_token.type == TokenType::OpeningBrace, "invalid state for parse_object");
  next_token();

  if (current_token.type == TokenType::ClosingBrace) {
    next_token();
    return Object{};
  }

  Object::Map entries;

  while (true) {
    if (current_token.type != TokenType::String) {
      return std::nullopt;
    }

    auto key = std::move(current_token.string);
    if (entries.contains(key)) {
      return std::nullopt;
    }

    next_token();

    if (current_token.type != TokenType::Colon) {
      return std::nullopt;
    }
    next_token();

    auto value = parse_value();
    if (!value) {
      return std::nullopt;
    }

    entries.emplace(std::move(key), std::move(*value));

    if (current_token.type == TokenType::Comma) {
      next_token();
      if (current_token.type == TokenType::ClosingBrace) {
        next_token();
        break;
      }
    } else if (current_token.type == TokenType::ClosingBrace) {
      next_token();
      break;
    } else {
      return std::nullopt;
    }
  }

  return Object{std::move(entries)};
}

std::optional<Value> Parser::parse_array() {
  verify(current_token.type == TokenType::OpeningBracket, "invalid state for parse_array");
  next_token();

  if (current_token.type == TokenType::ClosingBracket) {
    next_token();
    return Array{};
  }

  Array::Vector entries;

  while (true) {
    auto value = parse_value();
    if (!value) {
      return std::nullopt;
    }

    entries.emplace_back(std::move(*value));

    if (current_token.type == TokenType::Comma) {
      next_token();
      if (current_token.type == TokenType::ClosingBracket) {
        next_token();
        break;
      }
    } else if (current_token.type == TokenType::ClosingBracket) {
      next_token();
      break;
    } else {
      return std::nullopt;
    }
  }

  return Array{std::move(entries)};
}

std::optional<Value> Parser::parse_number() {
  verify(current_token.type == TokenType::Number, "invalid state for parse_number");

  const auto format = current_token.number_format;
  const auto i_value = current_token.number_integer;
  const auto d_value = current_token.number_double;

  next_token();

  switch (format) {
    case Number::Format::Int:
      return Number{int64_t(i_value)};
    case Number::Format::UInt:
      return Number{uint64_t(i_value)};
    case Number::Format::Double:
      return Number{d_value};
    default:
      unreachable();
  }
}

std::optional<Value> Parser::parse_value() {
  const auto consume_token = [&](auto&& result) -> std::optional<Value> {
    next_token();
    return result;
  };

  switch (current_token.type) {
    case TokenType::OpeningBrace:
      return parse_object();
    case TokenType::OpeningBracket:
      return parse_array();
    case TokenType::Number:
      return parse_number();
    case TokenType::String:
      return consume_token(String{std::move(current_token.string)});

    case TokenType::True:
      return consume_token(Boolean{true});
    case TokenType::False:
      return consume_token(Boolean{false});
    case TokenType::Null:
      return consume_token(Null{});

    default:
      return std::nullopt;
  }
}

std::optional<Value> Parser::parse_root_value() {
  auto value = parse_value();
  if (value) {
    if (current_token.type != TokenType::Eof) {
      return std::nullopt;
    }
    return value;
  }
  return std::nullopt;
}

Parser::Parser(Lexer lexer) : lexer(lexer) {
  current_token = this->lexer.next();
}

std::optional<Value> Parser::parse(Token::Location& error_location) {
  auto value = parse_root_value();
  if (!value) {
    error_location = current_token.location;
  }
  return value;
}
