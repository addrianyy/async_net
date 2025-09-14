#include "Log.hpp"
#include "Logger.hpp"

using namespace base;

void detail::log::do_log(
  const char* file, int line, LogLevel level, fmt::string_view fmt, fmt::format_args args
) {
  Logger::log(file, line, level, fmt, args);
}
