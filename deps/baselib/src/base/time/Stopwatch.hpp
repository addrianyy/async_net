#pragma once
#include "PreciseTime.hpp"

namespace base {

class Stopwatch {
  PreciseTime start_time{};
  PreciseTime pause_start_time{};

 public:
  Stopwatch();

  PreciseTime reset();

  void pause();
  void resume();

  PreciseTime elapsed() const;
};

}  // namespace base
