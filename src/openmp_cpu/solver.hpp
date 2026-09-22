#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include "heat/dtype.hpp"

#include "heat/config.hpp"
#include "heat/field.hpp"
#include "heat/exact.hpp"

namespace heat::openmp_cpu {

inline std::unique_ptr<real[]> run(const Config& config)
{

  const int n_point_x = config.n_point_x;
  const int n_point_y = config.n_point_y;

  const std::size_t n_point_xy = static_cast<std::size_t>(n_point_x)
                                 * n_point_y;

  std::unique_ptr<real[]> cur(new real[n_point_xy]);
  std::unique_ptr<real[]> nxt(new real[n_point_xy]);

  const Indexer index{config.n_point_y};

  #pragma omp parallel for collapse(2)
  for (int i = 0; i < n_point_x; i++) {
    for (int j = 0; j < n_point_y; j++) {
      bool bound_i = i == 0 || i == n_point_x - 1;
      bool bound_j = j == 0 || j == n_point_y - 1;

      if (bound_i || bound_j) {
        cur[index(i, j)] = 0.0;
        nxt[index(i, j)] = 0.0;
      }
      else {
        real x = i * config.delta_x;
        real y = j * config.delta_y;

        cur[index(i, j)] = exact(x, y, 0.0, config);
      }
    }
  }

  for (int step = 0; step < config.n_timestep; step++) {
    const real *uu = cur.get();
    real       *vv = nxt.get();

    #pragma omp parallel for collapse(2)
    for (int i = 1; i < n_point_x - 1; i++) {
      for (int j = 1; j < n_point_y - 1; j++) {
        vv[index(i, j)] = uu[index(i, j)] + config.rate_x *
                                            (uu[index(i + 1, j)]
                                             - uu[index(i, j)] * 2
                                             + uu[index(i - 1, j)])
                                          + config.rate_y *
                                            (uu[index(i, j + 1)]
                                             - uu[index(i, j)] * 2
                                             + uu[index(i, j - 1)]);
      }
    }
    
    std::swap(cur, nxt);
  }

  return cur;
}

} // namespace heat::openmp_cpu
