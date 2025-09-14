#include "Parser.hpp"
#include "detail/Lexer.hpp"
#include "detail/Parser.hpp"

using namespace base::json;

std::optional<Value> base::json::parse(
  std::string_view text, ParsingErrorLocation* error_location
) {
  detail::Lexer lexer{text};
  detail::Parser parser{lexer};
  detail::Token::Location token_error_location{};

  auto value = parser.parse(token_error_location);
  if (!value) {
    if (error_location) {
      *error_location = {
        .line = token_error_location.line,
        .column = token_error_location.column,
      };
    }
  }
  return value;
}
