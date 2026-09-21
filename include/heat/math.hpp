#pragma once

#include "heat/dtype.hpp"

namespace heat {

constexpr real pi = 3.14159265358979323846;

[[nodiscard]] constexpr real square(real x) noexcept
{
  return x * x;
}

} // namespace heat
