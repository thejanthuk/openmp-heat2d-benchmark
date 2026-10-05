// Written by Claude (an AI assistant) at Krit's request, 2026-10-05.
//
// Full-field differential test: the GPU naive variant against the serial
// oracle, over every grid point, on several shapes and both step parities.
//
// WHY A TOLERANCE AND NOT ==
//   The CPU OpenMP variant is bit-identical to serial: same compiler, same
//   instruction set. The GPU variant is not, and cannot be expected to be.
//   Host and device code are produced by two different compilers, which are
//   free to fuse and associate `u + rx*S1 + ry*S2` differently. Measured
//   2026-10-05: agreement is exact when rx == ry (40x40) or when they are
//   comparable (40x24, ly=0.5), and differs by 1 ulp (2.22e-16) at 624 of 7381
//   points on 120x60, where rx=0.32 and ry=0.08. Forcing -ffp-contract=off on
//   both sides makes agreement WORSE, not better (6 of 6 cases then differ).
//
// WHY 1e-12
//   It must sit far above rounding noise (~1e-16, growing to ~1e-15 over
//   hundreds of steps) and far below any logic error. The stale-buffer bug
//   found on 2026-10-05 showed as ~2e-3. Twelve orders of magnitude separate
//   the two, so the exact threshold is not delicate.
//
// THE NEGATIVE CONTROL
//   A test that only passes proves nothing. For every case this file also
//   compares the serial field against serial run for one step fewer, and
//   requires that difference to EXCEED the tolerance. That is the signature a
//   stale-buffer or wrong-buffer-returned bug produces, so if the control ever
//   stops firing, the tolerance has gone slack and the test has stopped being
//   able to detect the very bug it was written for.

#include <cmath>
#include <cstddef>
#include <cstdio>

#include "heat/config.hpp"
#include "heat/dtype.hpp"

#include "openmp_gpu_naive/solver.hpp"
#include "serial/solver.hpp"

namespace {

constexpr heat::real tolerance = 1e-12;

struct Case {
    int    n_interval_x;
    int    n_interval_y;
    int    n_timestep;
    heat::real length_y;
    const char* why;
};

// max |a - b| over every point, written so that NaN fails rather than passes.
heat::real max_difference(const heat::real* a, const heat::real* b, std::size_t n)
{
    heat::real worst = 0.0;
    for (std::size_t k = 0; k < n; k++) {
        const heat::real d = std::fabs(a[k] - b[k]);
        if (!(d <= worst)) worst = d;
    }
    return worst;
}

int check(const Case& c)
{
    const heat::Config config =
        heat::make_config(0, c.n_interval_x, c.n_interval_y, c.n_timestep, 1.0, 1.0, c.length_y);
    const std::size_t n = static_cast<std::size_t>(config.n_point_x) * config.n_point_y;

    const heat::RunResult serial = heat::serial::run(config);
    const heat::RunResult gpu    = heat::openmp_gpu_naive::run(config);

    const heat::real worst = max_difference(serial.field.get(), gpu.field.get(), n);
    const bool agrees = !(worst > tolerance);

    // Negative control: one step fewer must differ by MORE than the tolerance,
    // otherwise this comparison could not detect a stale buffer.
    const heat::Config shorter =
        heat::make_config(0, c.n_interval_x, c.n_interval_y, c.n_timestep - 1, 1.0, 1.0, c.length_y);
    const heat::RunResult off_by_one = heat::serial::run(shorter);
    const heat::real control = max_difference(serial.field.get(), off_by_one.field.get(), n);
    const bool control_fires = control > tolerance;

    std::printf("%4dx%-4d steps=%-4d max|diff|=%-11g %-9s control=%-11g %-9s %s\n",
                config.n_point_x, config.n_point_y, c.n_timestep,
                static_cast<double>(worst), agrees ? "agrees" : "TOO BIG",
                static_cast<double>(control), control_fires ? "fires" : "BLIND",
                c.why);

    return (agrees && control_fires) ? 0 : 1;
}

}  // namespace

int main()
{
    // n_timestep >= 2 everywhere: the negative control runs one step fewer, and
    // make_config requires n_timestep > 0.
    const Case cases[] = {
        {40, 40, 2, 1.0, "square, even steps"},
        {40, 40, 3, 1.0, "square, odd steps -- catches the wrong buffer returned"},
        {40, 40, 101, 1.0, "square, many steps -- rounding has time to grow"},
        {40, 24, 7, 0.5, "non-square domain, rx and ry comparable"},
        {120, 60, 50, 1.0, "non-square cells, rx=0.32 vs ry=0.08 -- host/device differ by 1 ulp"},
        {80, 40, 13, 1.0, "n_point_x != n_point_y on a 1x1 domain"},
    };

    int failures = 0;
    for (const Case& c : cases) failures += check(c);

    if (failures == 0) {
        std::printf("all %zu cases agree within %g, and every negative control fires\n",
                    sizeof(cases) / sizeof(cases[0]), static_cast<double>(tolerance));
        return 0;
    }
    std::printf("%d case(s) failed\n", failures);
    return failures;
}
