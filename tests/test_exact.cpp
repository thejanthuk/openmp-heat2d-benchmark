#include <cmath>
#include <cstdio>
#include <cstddef>
#include <memory>

#include "heat/dtype.hpp"

#include "heat/config.hpp"
#include "heat/field.hpp"
#include "heat/exact.hpp"

static void fill_with_known_error(heat::real *field, heat::real t,
                                  heat::real epsilon, bool alternative,
                                  const heat::Config& config)
{
  const heat::Indexer index{config.n_point_y};

  for (int i = 0; i < config.n_point_x; i++) {
    for (int j = 0; j < config.n_point_y; j++) {
      const heat::real x = i * config.delta_x;
      const heat::real y = j * config.delta_y;

      const heat::real s = alternative && (i + j) % 2 == 1 ? -1 : 1;

      field[index(i, j)] = heat::exact(x, y, t, config) + s * epsilon;
    }
  }
}

static int check(const char *what, double got, double want)
{
  if (!(std::abs(got - want) <= 1e-9 * std::abs(want))) {
    std::printf("FAIL: %s = %.17g, want %.17g\n", what, got, want);
    return 1;
  }
  return 0;
}

int main()
{
  int fail = 0;

  const heat::Config config = heat::make_config(0, 100, 100, 500);

  const heat::real t = 0.01;
  const heat::real epsilon = 1e-3;

  const std::size_t n_point_xy = static_cast<std::size_t>
                                 (config.n_point_x) * config.n_point_y;

  std::unique_ptr<heat::real[]> field(new heat::real[n_point_xy]);

  // interior area
  const double area = (config.n_point_x - 2) * config.delta_x
                      * (config.n_point_y - 2) * config.delta_y;

  const double want_l2  = epsilon * std::sqrt(area);
  const double want_max = epsilon;

  fill_with_known_error(field.get(), t, +epsilon, false, config);
  fail += check("l2_error all +",
                heat::l2_error(field.get(), t, config), want_l2);
  fail += check("max_error all +",
                heat::max_error(field.get(), t, config), want_max);
  
  fill_with_known_error(field.get(), t, -epsilon, false, config);
  fail += check("l2_error all -",
                heat::l2_error(field.get(), t, config), want_l2);
  fail += check("max_error all -",
                heat::max_error(field.get(), t, config), want_max);

  fill_with_known_error(field.get(), t, +epsilon, true, config);
  fail += check("l2_error all +-",
                heat::l2_error(field.get(), t, config), want_l2);
  fail += check("max_error all +-",
                heat::max_error(field.get(), t, config), want_max);

  return fail;
}
