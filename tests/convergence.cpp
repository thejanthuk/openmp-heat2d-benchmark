#include <cmath>
#include <cstdio>
#include <initializer_list>

#include "heat/dtype.hpp"

#include "heat/exact.hpp"
#include "heat/config.hpp"

#include "serial/solver.hpp"

static heat::Config config_at_time(int n_interval_x, int n_interval_y,
                                   heat::real t_end,
                                   heat::real length_x,
                                   heat::real length_y)
{
  const heat::real alpha = heat::default_alpha;

  const heat::Config probe = heat::make_config(0, n_interval_x,
                                               n_interval_y, 1, alpha,
                                               length_x, length_y);

  const int n_timestep = static_cast<int>
                         (std::ceil(t_end / probe.delta_t));

  const heat::real factor = (t_end / n_timestep) / probe.delta_t;
  const heat::real coeff  = probe.coeff * factor;

  return heat::make_config(0, n_interval_x, n_interval_y, n_timestep,
                           alpha, length_x, length_y, coeff);
}

static int check_convergence(const char *name, int x_multiplier,
                             heat::real length_y)
{
  const heat::real t_end = 0.0123;

  double prev = 0.0;
  double order = 0.0;
  for (int n : {20, 40, 80, 160}) {
    const heat::Config config = config_at_time(x_multiplier * n, n,
                                               t_end, 1.0, length_y);

    const auto field = heat::serial::run(config);

    const heat::real t = config.n_timestep * config.delta_t;
    const double error = heat::l2_error(field.get(), t, config);

    order = std::log2(prev / error);
    if (prev > 0.0)
      std::printf("(n, error, log2(prev / error)): %d %.17g %.17g\n",
                  n, error, order);

    prev = error;
  }

  int fail = 0;
  
  if (!(std::abs(order - 2.0) <= 0.05)) {
    std::printf("FAIL %s: error %.4f isn't within 0.05 of 2\n",
                name, order);
    fail++;
  }

  if (!(prev < 1e-5)) {
    std::printf("FAIL %s: error %.17g at n=160 is not below 1e-5\n",
                name, prev);
    fail++;
  }

  return fail;
}

int main()
{
  int fail = 0;

  fail += check_convergence("square", 1, 1.0);
  fail += check_convergence("non-square", 2, 0.5);

  return fail;
}
