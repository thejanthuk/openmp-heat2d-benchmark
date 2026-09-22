#pragma once

#include <memory>

#include "dtype.hpp"

namespace heat {

struct RunResult {
  std::unique_ptr<real[]> field;

  double seconds;
};

} // namespace heat
