# Next three quests — ALL DONE (2026-09-22)

> **Template/planning file written by Claude (an AI assistant).** Entries below are Claude's unless marked otherwise.

Order matters: make it run, make it right, make it recorded.

## 1. Get a serial solver actually running — DONE 2026-09-21

Nothing executes yet. Five sessions of scaffolding, zero output. Fix that first.

**Go minimal — no `Field` class.** Two `unique_ptr<real[]>`, raw `real*` working pointers, the free
`index()`, `std::swap`. Add the class later only if `p[index(i,j,ny)]` genuinely grates when writing
`exact.hpp`.

- `include/heat/field.hpp` — just the free `index(i, j, n_point_y)` for now
- `src/serial/main.cpp` — parse args, allocate, initial condition, timestep loop with pointer swap.
  **No `#pragma omp` here** — `heat_serial` does not link OpenMP. (Corrected 2026-09-21: an earlier
  version of this line said "parallel first touch"; that belongs in the `openmp_cpu` variant.)
- `tests/convergence.cpp` — a stub `int main(){return 0;}` so CMake configures

**Done when:** `cmake -S . -B build -G Ninja && cmake --build build && ./build/heat_serial 0 100 100 500`
runs and the centre value **decays** rather than growing or turning NaN.

## 2. Prove it is correct — DONE 2026-09-22 (order 2.000 on square and 1x0.5, nx != ny)

This is the checkpoint that validates everything else. Do it before any timing.

- `include/heat/exact.hpp` — analytic solution and L2 error. **Generalised for Lx != Ly**:
  `T = sin(pi x/Lx) sin(pi y/Ly) exp(-alpha pi^2 (1/Lx^2 + 1/Ly^2) t)`
- `tests/convergence.cpp` — run at N = 20, 40, 80, 160 to a fixed physical time, print L2 and the
  observed order

**Done when:** observed order converges to **2.0**. If it comes out ~1, suspect `dx`; if ~0, suspect
an in-place update. Both failure signatures are in an earlier local guide.

**Watch for:** `exact.hpp` and the kernel must agree on which index is contiguous. Disagree and the
solver runs, converges at order 1, and says nothing about why.

## 3. Make the record real — DONE (git history since 2026-09-21)

- `git init`, then commit in logical steps with the reasoning in each message. Five sessions of work
  is currently untracked and one `rm` from gone — and it is the authorship evidence.
- Write your own `NOTES.md` entry: `config.hpp` is done, and the Week 1 checkpoints are all still
  unticked. In your words, not Claude's.

## Still open, not for today

- Whether to add a `Field` class at all (ADR-004 unwritten — decide after quest 2 tells you whether
  the raw indexing grates)
- CPU OpenMP variant
- ParaView still not installed
