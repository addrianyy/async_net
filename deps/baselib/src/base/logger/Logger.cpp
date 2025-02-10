#include "Logger.hpp"
#include "LoggerImpl.hpp"

#include <atomic>

using namespace base;

static std::unique_ptr<LoggerImpl> g_logger;
static std::atomic_uint32_t g_min_reported_level{0};

LoggerImpl* Logger::get() {
  return g_logger.get();
}

std::unique_ptr<LoggerImpl> Logger::set(std::unique_ptr<LoggerImpl> logger) {
  auto previous_logger = std::move(g_logger);
  g_logger = std::move(logger);
  return previous_logger;
}

LogLevel Logger::min_reported_level() {
  return LogLevel(g_min_reported_level.load(std::memory_order_relaxed));
}

void Logger::set_min_reported_level(LogLevel level) {
  g_min_reported_level.store(uint32_t(level), std::memory_order_relaxed);
}

void Logger::log(const char* file,
                 int line,
                 LogLevel level,
                 fmt::string_view fmt,
                 fmt::format_args args) {
  if (g_logger && uint32_t(level) >= g_min_reported_level.load(std::memory_order_relaxed)) {
    g_logger->log(file, line, level, fmt, args);
  }
}

void Logger::log_panic(const char* file, int line, fmt::string_view fmt, fmt::format_args args) {
  if (g_logger) {
    g_logger->log_panic(file, line, fmt, args);
  }
}

bool Logger::supports_color() {
  if (g_logger) {
    return g_logger->supports_color();
  }
  return false;
}