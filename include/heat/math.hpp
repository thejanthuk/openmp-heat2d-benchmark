#pragma once

#include "heat/dtype.hpp"

namespace heat {

[[nodiscard]] constexpr real square(real x) noexcept
{
  return x * x;
}

} // namespace heat
