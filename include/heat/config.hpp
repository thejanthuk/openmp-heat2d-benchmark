#pragma once

#include <string>
#include <stdexcept>

#include "heat/check.hpp"

#include "heat/dtype.hpp"

#include "heat/math.hpp"

namespace heat {

constexpr real default_alpha  = 1.0;
constexpr real default_length = 1.0;
constexpr real default_coeff  = 0.8;

struct Config {
  int run_id;

  int n_interval_x;
  int n_interval_y;

  int n_point_x;
  int n_point_y;

  int n_timestep;

  real alpha;

  real length_x;
  real length_y;

  real delta_x;
  real delta_y;

  real coeff;
  real delta_t;

  real rate_x;
  real rate_y;
};

inline Config make_config(int run_id, int n_interval_x, int n_interval_y,
                          int n_timestep, real alpha = default_alpha,
                          real length_x = default_length,
                          real length_y = default_length,
                          real coeff = default_coeff)
{
  Config config{};

  config.run_id = run_id;

  require(n_interval_x > 0,
         "n_interval_x must be > 0");
  require(n_interval_y > 0,
         "n_interval_y must be > 0");
  config.n_interval_x = n_interval_x;
  config.n_interval_y = n_interval_y;

  config.n_point_x = config.n_interval_x + 1;
  config.n_point_y = config.n_interval_y + 1;

  require(n_timestep > 0,
         "n_timestep must be > 0");
  config.n_timestep = n_timestep;

  require(alpha > 0,
         "alpha must be > 0");
  config.alpha = alpha;

  require(length_x > 0,
         "length_x must be > 0");
  require(length_y > 0,
         "length_y must be > 0");
  config.length_x = length_x;
  config.length_y = length_y;

  config.delta_x = config.length_x / config.n_interval_x;
  config.delta_y = config.length_y / config.n_interval_y;

  require(coeff > 0 && coeff < 1,
         "coeff must be > 0 and < 1");
  config.coeff = coeff;

  const real inverse_delta_xx = 1.00 / square(config.delta_x);
  const real inverse_delta_yy = 1.00 / square(config.delta_y);
  const real inverse_delta    = inverse_delta_xx + inverse_delta_yy;
  config.delta_t = config.coeff / (config.alpha * inverse_delta * 2);
  
  config.rate_x = (config.alpha * config.delta_t) * inverse_delta_xx;
  config.rate_y = (config.alpha * config.delta_t) * inverse_delta_yy;
  require(config.rate_x + config.rate_y <= 0.5,
         "Unstable: rate_x + rate_y > 0.5");

  return config;
}

inline Config parse_args(int argc, char** argv)
{
  if (argc < 5 || argc > 9)
    throw std::runtime_error(
        std::string("Usage: ") + argv[0] +
        " <run_id> <n_interval_x> <n_interval_y> <n_timestep>"
        " <alpha = 1.0> <length_x = 1.0> <length_y = 1.0>"
        " <coeff = 0.8>\n"
        " Mandatory:\n"
        "  run_id        integer tag\n"
        "  n_interval_x  grid intervals in x   (> 0)\n"
        "  n_interval_y  grid intervals in y   (> 0)\n"
        "  n_timestep    number of timesteps   (> 0)\n"
        " Optional:\n"
        "  alpha         thermal diffusivity   (> 0)\n"
        "  length_x      grid size in x        (> 0)\n"
        "  length_y      grid size in y        (> 0)\n"
        "  coeff         stability coefficient (> 0, < 1)\n"
        "Example: " + std::string(argv[0]) + " 0 1000 1000 500\n"
    );

  int run_id = std::stoi(argv[1]);

  int n_interval_x = std::stoi(argv[2]);
  int n_interval_y = std::stoi(argv[3]);

  int n_timestep = std::stoi(argv[4]);

  real alpha = argc < 6 ? default_alpha : std::stod(argv[5]);

  real length_x = argc < 7 ? default_length : std::stod(argv[6]);
  real length_y = argc < 8 ? default_length : std::stod(argv[7]);

  real coeff = argc < 9 ? default_coeff : std::stod(argv[8]);

  return make_config(run_id, n_interval_x, n_interval_y, n_timestep,
                     alpha, length_x, length_y, coeff);
}

} // namespace heat
