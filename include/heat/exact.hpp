#pragma once

#include <cmath>

#include "heat/dtype.hpp"

#include "heat/config.hpp"
#include "heat/field.hpp"
#include "heat/math.hpp"

namespace heat {

[[nodiscard]] inline real exact(real x, real y, real t,
                                const Config &config) noexcept
{
  return std::sin(pi * x / config.length_x)
         * std::sin(pi * y / config.length_y)
         * std::exp(-config.alpha * pi * pi * t
                    * (1.0 / square(config.length_x)
                       + 1.0 / square(config.length_y)));
}

[[nodiscard]] inline real l2_error(const real *hh, real t,
                                   const Config& config) noexcept
{
  const Indexer index{config.n_point_y};

  double sum = 0.0;
  for (int i = 1; i < config.n_point_x - 1; i++) {
    for (int j = 1; j < config.n_point_y - 1; j++) {
      real x = i * config.delta_x;
      real y = j * config.delta_y;

      const double diff = hh[index(i, j)] - exact(x, y, t, config);

      sum += diff * diff;
    }
  }

  return std::sqrt(sum * config.delta_x * config.delta_y);
}

[[nodiscard]] inline real max_error(const real *hh, real t,
                                    const Config& config) noexcept
{
  const Indexer index{config.n_point_y};

  double max = 0.0;
  for (int i = 1; i < config.n_point_x - 1; i++) {
    for (int j = 1; j < config.n_point_y - 1; j++) {
      real x = i * config.delta_x;
      real y = j * config.delta_y;

      const real diff = std::abs(hh[index(i, j)]
                                 - exact(x, y, t, config));

      if (max < diff)
        max = diff;
    }
  }

  return max;
}

} // namespace heat
