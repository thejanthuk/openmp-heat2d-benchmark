#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <omp.h>

#include "heat/dtype.hpp"
#include "heat/check.hpp"

#include "heat/config.hpp"
#include "heat/field.hpp"
#include "heat/exact.hpp"
#include "heat/result.hpp"
#include "heat/timer.hpp"

namespace heat::openmp_gpu_naive {

inline RunResult run(const Config& config)
{

  const int n_point_x = config.n_point_x;
  const int n_point_y = config.n_point_y;

  const std::size_t n_point_xy = static_cast<std::size_t>(n_point_x)
                                 * n_point_y;

  std::unique_ptr<real[]> cur(new real[n_point_xy]);
  std::unique_ptr<real[]> nxt(new real[n_point_xy]);

  const Indexer index{config.n_point_y};

  real *uu = cur.get();
  real *vv = nxt.get();
  #pragma omp target teams distribute parallel for collapse(2) \
      map(from: uu[0:n_point_xy]) map(from: vv[0:n_point_xy])
  for (int i = 0; i < n_point_x; i++) {
    for (int j = 0; j < n_point_y; j++) {
      bool bound_i = i == 0 || i == n_point_x - 1;
      bool bound_j = j == 0 || j == n_point_y - 1;

      if (bound_i || bound_j) {
        uu[index(i, j)] = 0.0;
        vv[index(i, j)] = 0.0;
      }
      else {
        real x = i * config.delta_x;
        real y = j * config.delta_y;

        uu[index(i, j)] = exact(x, y, 0.0, config);
      }
    }
  }

  int on_host = 1;
  const auto start = Clock::now();
  for (int step = 0; step < config.n_timestep; step++) {
    #pragma omp target teams distribute parallel for collapse(2) \
      map(to: uu[0:n_point_xy]) map(tofrom: vv[0:n_point_xy]) \
      map(tofrom: on_host)
    for (int i = 1; i < n_point_x - 1; i++) {
      for (int j = 1; j < n_point_y - 1; j++) {
        if (i == 1 && j == 1)
          on_host = omp_is_initial_device();

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

    std::swap(uu, vv);
  }
  const double seconds = seconds_since(start);
  require(on_host == 0, "kernel ran on the host, not the GPU");

  return RunResult{uu == cur.get() ? std::move(cur) : std::move(nxt), seconds};
}

} // namespace heat::openmp_gpu_naive
