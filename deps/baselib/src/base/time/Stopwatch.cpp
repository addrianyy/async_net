#include "Stopwatch.hpp"

using namespace base;

Stopwatch::Stopwatch() {
  reset();
}

PreciseTime Stopwatch::reset() {
  const auto now = PreciseTime::now();
  const auto elapsed_result = now - start_time;
  start_time = now;
  pause_start_time = {};
  return elapsed_result;
}

void Stopwatch::pause() {
  pause_start_time = PreciseTime::now();
}

void Stopwatch::resume() {
  const auto pause_time = PreciseTime::now() - pause_start_time;

  pause_start_time = {};
  start_time += pause_time;
}

PreciseTime Stopwatch::elapsed() const {
  return PreciseTime::now() - start_time;
}
