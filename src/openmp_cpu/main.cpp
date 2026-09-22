#include <cstdio>
#include <exception>

#include "heat/config.hpp"
#include "heat/field.hpp"

#include "openmp_cpu/solver.hpp"

int main(int argc, char** argv)
{
  try {
    const heat::Config config = heat::parse_args(argc, argv);

    const auto field = heat::openmp_cpu::run(config);

    const heat::Indexer index{config.n_point_y};
    std::printf("Centre = %.17g\n", field[index(config.n_point_x / 2,
                                                config.n_point_y / 2)]);
    return 0;
  }
  catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
