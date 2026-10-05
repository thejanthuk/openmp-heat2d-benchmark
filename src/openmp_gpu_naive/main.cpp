#include <cstdio>
#include <exception>

#include "heat/config.hpp"
#include "heat/field.hpp"

#include "openmp_gpu_naive/solver.hpp"

int main(int argc, char** argv)
{
  try {
    const heat::Config config = heat::parse_args(argc, argv);

    const auto field = heat::openmp_gpu_naive::run(config);

    const heat::Indexer index{config.n_point_y};
    std::printf("Centre = %.17g\nTime = %.17g s\n",
                              field.field[index(config.n_point_x / 2,
                                                config.n_point_y / 2)],
                              field.seconds);
    return 0;
  }
  catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
