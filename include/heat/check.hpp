#pragma once

#include <stdexcept>

namespace heat {

inline void require(bool condition, const char* message)
{
  if (!condition)
    throw std::invalid_argument(message);
}

} // namespace heat
