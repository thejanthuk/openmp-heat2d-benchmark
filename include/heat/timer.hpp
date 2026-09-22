#pragma once

#include <chrono>

namespace heat {

using Clock = std::chrono::steady_clock;

inline double seconds_since(Clock::time_point start)
{
  return std::chrono::duration<double>(Clock::now() - start).count();
}

} // namespace heat
