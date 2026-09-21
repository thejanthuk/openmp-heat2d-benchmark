#include <cmath>
#include <cstdio>
#include <cstddef>
#include <exception>
#include <memory>
#include <utility>

#include "heat/config.hpp"
#include "heat/field.hpp"
#include "heat/math.hpp"

int main(int argc, char** argv)
{
  try {
    const heat::Config config = heat::parse_args(argc, argv);

    const int n_point_x = config.n_point_x;
    const int n_point_y = config.n_point_y;

    const std::size_t n_point_xy = static_cast<std::size_t>(n_point_x)
                                   * n_point_y;

    std::unique_ptr<heat::real[]> buffer_a(new heat::real[n_point_xy]);
    std::unique_ptr<heat::real[]> buffer_b(new heat::real[n_point_xy]);

    heat::real *cur = buffer_a.get();
    heat::real *nxt = buffer_b.get();

    const heat::Indexer index{config.n_point_y};

    for (int i = 0; i < n_point_x; i++) {
      for (int j = 0; j < n_point_y; j++) {
        bool boundary_i = i == 0 || i == n_point_x - 1;
        bool boundary_j = j == 0 || j == n_point_y - 1;

        if (boundary_i || boundary_j) {
          cur[index(i, j)] = 0.0;
          nxt[index(i, j)] = 0.0;
        }
        else {
          heat::real x = i * config.delta_x;
          heat::real y = j * config.delta_y;

          cur[index(i, j)] = std::sin(heat::pi * x / config.length_x)
                             * std::sin(heat::pi * y / config.length_y);
        }
      }
    }

    for (int step = 0; step < config.n_timestep; step++) {
      for (int i = 1; i < n_point_x - 1; i++) {
        for (int j = 1; j < n_point_y - 1; j++) {
          nxt[index(i, j)] = cur[index(i, j)] + config.rate_x *
                                                (cur[index(i + 1, j)]
                                                 - cur[index(i, j)] * 2
                                                 + cur[index(i - 1, j)])
                                              + config.rate_y *
                                                (cur[index(i, j + 1)]
                                                 - cur[index(i, j)] * 2
                                                 + cur[index(i, j - 1)]);
        }
      }
      
      std::swap(cur, nxt);
    }

    std::printf("Centre = %.17g\n", cur[index(n_point_x / 2,
                                              n_point_y / 2)]);
    return 0;
  }
  catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
