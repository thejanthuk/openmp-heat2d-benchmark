# Parallel FDM 2-D Heat Equation Solver

Four implementations of the same explicit finite-difference stencil for the 2-D heat equation,
benchmarked against each other and against the hardware's own limits.

The solver is the workload, not the result. The deliverable is the *comparison*: what each
parallelisation strategy costs, and how far each lands from the ceiling the machine allows.

| # | Version | Key construct |
|---|---|---|
| 1 | Serial CPU | baseline, single core |
| 2 | CPU OpenMP | `#pragma omp parallel for collapse(2)` |
| 3 | GPU offload, naive | `target teams distribute parallel for collapse(2)`, transferring every timestep |
| 4 | GPU offload, optimized | the same kernel wrapped in `#pragma omp target data` |

Versions 3 and 4 run an identical kernel and differ only in where the mapping clause sits, so the
gap between them measures PCIe transfer cost directly.

## The equation

    dT/dt = alpha * (d2T/dx2 + d2T/dy2)

    T[i][j]^(n+1) = T[i][j]^n + rx*(T[i+1][j] - 2T[i][j] + T[i-1][j])
                              + ry*(T[i][j+1] - 2T[i][j] + T[i][j-1])

with `rx = alpha*dt/dx^2` and `ry = alpha*dt/dy^2`. The explicit scheme is stable only while
**`rx + ry <= 1/2`**; violating it makes the solution diverge rather than fail, so `dt` is derived
from `dx`, `dy` and `alpha`, and the constraint is checked at startup in every build.

## Build and run

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/heat_serial 0 100 100 500      # run_id, nx, ny, timesteps
```

Add `-DHEAT_GPU=ON` to build the two offload variants; they need a GCC with a working nvptx
offload toolchain, and must be run with `OMP_TARGET_OFFLOAD=MANDATORY`. Full flags and options:
`docs/build-and-run.md`.

## Correctness

The solver is checked against the analytic solution
`T = sin(pi x/Lx) sin(pi y/Ly) exp(-alpha pi^2 (1/Lx^2 + 1/Ly^2) t)`, and observed convergence is
second order on both square and non-square grids. Parallel variants are verified differentially
against the serial baseline rather than by sanitizers, for reasons recorded in
`docs/debugging-toolkit.md`.

## Where the work is written down

| File | Contents |
|---|---|
| `NOTES.md` | Lab notebook: dated predicted / observed / explained. Wrong predictions stay in. |
| `docs/design-decisions.md` | Per decision: the forces, the choice, and the alternative rejected. |
| `docs/00-gpu-forensics.md` | Why the GPU kernel measured 25x slower than its roofline, and the root cause. |
| `docs/audit-2026-09-22.md` | Every claim in these docs re-tested, with the wrong ones named. |
| `docs/questions.md` | Candidate research questions, each with the observation that produced it. |
| `docs/sessions/` | What was done each session, what was decided, what is still open. |
| `AUTHORSHIP.md` | Who wrote what. |

## A note on the numbers

Measurements here are tied to one machine and one toolchain, and the docs say which is which:
*measured* means re-tested, *inherited* means carried over and not re-checked, *spec* means a
datasheet figure. Absolute timings are not compared across days — machine state moves them more
than noise does.
