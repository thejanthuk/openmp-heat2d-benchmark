#pragma once

#include <cstddef>

namespace heat {

[[nodiscard]] constexpr std::size_t index(int i, int j,
                                          int n_point_y) noexcept
{
  return static_cast<std::size_t>(i) * n_point_y + j;
}

struct Indexer {
  int n_point_y;

  [[nodiscard]] constexpr std::size_t operator()(int i,
                                                 int j) const noexcept
  {
    return heat::index(i, j, n_point_y);
  }
};

} // namespace heat
