#include <stdexcept>
#include <cmath>
#include <cstdio>

#include "heat/dtype.hpp"

#include "heat/config.hpp"

template <class F> bool throws(F f)
{
  try {
    f();
  }
  catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

int main()
{
  int fail = 0;

  /* n_interval_x */
  if (!throws([]{
        heat::make_config(0, -1, 1, 1);
  })) {
    std::puts("FAIL: n_interval_x < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 0, 1, 1);
  })) {
    std::puts("FAIL: n_interval_x = 0 accepted");
    fail++;
  }

  /* n_interval_y */
  if (!throws([]{
        heat::make_config(0, 1, -1, 1);
  })) {
    std::puts("FAIL: n_interval_y < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 0, 1);
  })) {
    std::puts("FAIL: n_interval_y = 0 accepted");
    fail++;
  }

  /* n_timestep */
  if (!throws([]{
        heat::make_config(0, 1, 1, -1);
  })) {
    std::puts("FAIL: n_timestep < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 0);
  })) {
    std::puts("FAIL: n_timestep = 0 accepted");
    fail++;
  } 

  /* alpha */
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, -0.1);
  })) {
    std::puts("FAIL: alpha < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 0.0);
  })) {
    std::puts("FAIL: alpha = 0 accepted");
    fail++;
  }

  /* length_x */
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, -0.1);
  })) {
    std::puts("FAIL: length_x < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 0.0);
  })) {
    std::puts("FAIL: length_x = 0 accepted");
    fail++;
  }

  /* length_y */
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, -0.1);
  })) {
    std::puts("FAIL: length_y < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, 0.0);
  })) {
    std::puts("FAIL: length_y = 0 accepted");
    fail++;
  }

  /* coeff */
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, 1.0, -0.1);
  })) {
    std::puts("FAIL: coeff < 0 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, 1.0, 0.0);
  })) {
    std::puts("FAIL: coeff = 0 accepted");
    fail++;
  } 
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, 1.0, 1.0);
  })) {
    std::puts("FAIL: coeff = 1 accepted");
    fail++;
  }
  if (!throws([]{
        heat::make_config(0, 1, 1, 1, 1.0, 1.0, 1.0, 1.1);
  })) {
    std::puts("FAIL: coeff > 1 accepted");
    fail++;
  }

  /* valid config */
  try {
    const auto config = heat::make_config(0, 100, 100, 500);

    const heat::real rate = config.rate_x + config.rate_y;
    const heat::real want = 0.5 * heat::default_coeff;

    if (std::abs(rate - want) > 1e-12) {
      std::printf("FAIL: rate_x + rate_y = %.17g, want %.17g\n", rate, want);
      fail++;
    }
  }
  catch (const std::exception& error) {
    std::printf("FAIL: valid config rejected: %s\n", error.what());
    fail++;
  }

  return fail;
}
