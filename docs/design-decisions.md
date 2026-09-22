# Design decisions

> **Template/planning file written by Claude (an AI assistant).** Entries below are Claude's unless marked otherwise.

One entry per design session. The point is not to record *what* was chosen but *why it was forced*,
and what was given up. Structure is derived, not taste — so if I cannot name the force, I have not
finished designing.

Written before the code, not after. If an entry is written after the code, say so — that is a
different (weaker) kind of evidence.

## Template

```
## NNN — <the decision, in a phrase>
**Date:**
**Claim this code must prove:**
**Forces:**            what varies · what must stay provably identical · what the toolchain
                       demands · what a reviewer must verify quickly · what will change later
**Decision:**
**Alternative rejected, and why:**
**Stress test:**       three plausible future changes, and how many files each would touch
**Status:**            current | superseded by NNN | reversed (see postmortem)
```

The **Alternative rejected** line is the one that matters in a viva. "Why did you do it this way?"
is answered by naming what you did *not* do, and what it would have cost.

---

## 001 — Repository layout for the four-variant solver

**Date:** 2026-09-17
**Author:** proposed by Claude at Krit's request (Unblock mode), to be defended or amended by Krit.
Written *before* the code.

**Claim this code must prove:** these four programs run identical numerics and differ only in
parallelisation strategy, so any runtime difference between them is caused by that strategy alone.

**Forces:**
- *The diff is the experiment.* Versions 3 and 4 must run an identical kernel and differ only in
  where the mapping clause sits, or the PCIe measurement is not clean.
- *Shared numerics exactly once.* Four copies of the `dt` formula means one typo silently
  invalidates every speedup number, with nothing to signal it.
- *GPU offload forbids a linked library.* GCC's nvptx is fragile with `declare target` across
  translation units, so shared code must be header-only and compiled into each variant.
- *`std::vector` cannot cross the offload boundary.* `map(to: v[0:n])` does not work; the grid
  wrapper must expose a raw `double*`.
- *Defensibility.* Krit hand-writes this and may be assessed on it. Every file must be small enough
  to explain from memory. ~500 lines total.
- *Expensive/cheap boundary.* Generating benchmark data is slow and machine-specific; analysing it
  is fast and iterated. A persisted CSV between them means re-plotting never re-runs a sweep.

### Decision

```
CMakeLists.txt            (rename: current file has no .txt, so CMake cannot see it)
include/heat/
  config.hpp     grid size, alpha, dx/dy, dt DERIVED from stability, assert rx+ry<=0.5, CLI args
  field.hpp      2-D field over 1-D storage; RAII; move-only; .data() -> raw pointer; swap()
  exact.hpp      analytic solution, L2 error, observed convergence order
  timer.hpp      wall-clock, used INSIDE the timed region
  csv.hpp        append-mode row writer, used OUTSIDE it
src/
  serial/main.cpp  openmp_cpu/main.cpp  openmp_gpu_naive/main.cpp  openmp_gpu_optimized/main.cpp
tests/
  convergence.cpp   order -> 2.0; boundaries stay 0; no NaN; cross-version agreement
benchmarks/run_sweep.py   correctness gate, then best-of-N sweep -> data/raw/*.csv
analysis/report.py        data/raw -> data/processed/summary.csv + figures/
data/raw/ (gitignored)  data/processed/ + figures/ (committed — report evidence)
```

**The kernel loop lives in each `main.cpp`, deliberately duplicated.** It is the one place where
duplication is correct: hoisting it into a shared header would make the four variants unable to
differ, which is the entire experiment. Flag it in a comment so nobody "fixes" it later.

**`timer.hpp` and `csv.hpp` stay separate** despite both being tiny, because one is used inside the
timed region and one must never be. Separating them makes the mistake harder to make.

### Alternatives rejected

| Rejected | Why |
|---|---|
| **One `main.cpp` with `--mode=serial\|cpu\|gpu`** | DRY, but destroys force 1 — the diff stops being the experiment. Also puts four kernels in one translation unit for the offload compiler to handle, and risks surrounding code changing optimisation decisions. The spec lists four versions. |
| **A compiled `libheat.a`** | Conventional, but GCC's nvptx `declare target` across translation units is unreliable. Header-only is forced by the toolchain, not chosen for elegance. |
| **`Solver` base class + virtual `step()`** | The obvious OO decomposition and the worst option here: an indirect call per grid point cannot be inlined or vectorised. No virtual dispatch anywhere near the kernel. |
| **`std::vector<std::vector<double>>`** | Non-contiguous — destroys prefetching, and cannot be mapped to the device at all. |
| **`template <typename real_t>` from day one** | Right eventually (float-vs-double is still an open ablation, and FP64 is 1/32 on this GPU), but it complicates the offload code for a beginner. **Start with `using real = double;` in `config.hpp`** — flipping one alias switches the whole build, and it can be promoted to a template later without restructuring. 90% of the benefit, 10% of the cost. |
| **A unit-test framework (Catch2/GoogleTest)** | Premature. A `main()` returning non-zero is enough for a convergence study. Add one only if the tests outgrow it. |
| **Five headers collapsed into one `heat.hpp`** | Tempting at ~500 lines, but `config.hpp` is where a reviewer looks for the physics. Keeping it alone means the supervisor finds the discretisation in 30 seconds. |

### Stress test — three plausible changes

| Change | Files touched |
|---|---|
| Sweep float vs double | 1 (`config.hpp` alias) |
| Add an MPI variant | 1 new `src/mpi/main.cpp`; shared headers untouched |
| Change the initial condition | 1 (`exact.hpp`); all four variants move together |

Passes.

### Status
current


## 002 — `Field` owns its storage with `std::unique_ptr<real[]>`

**Date:** 2026-09-18
**Author:** drafted by Claude from a decision reached with Krit in review.
**Claim:** the grid must be allocated once, freed automatically, never accidentally copied, and must
hand a raw pointer to the OpenMP offload boundary.

**Forces:**
- *No accidental copies.* The C version's rule "DO NOT COPY ARRAYS" was something to remember; a
  silent copy of a 32 MB grid is expensive and invisible.
- *Offload.* `map(to: v[0:n])` cannot take a `std::vector`. Whatever owns the memory must expose a
  raw `real*`.
- *First touch.* The initial condition should be written by a parallel loop so pages land near the
  thread that will use them. Anything that value-initialises in its constructor does a **serial**
  memset first and fixes page placement on one thread.
- *Exception safety.* The C code needed `if (p == NULL) { free(other); exit(1); }` at every
  allocation. That should be automatic.

**Decision:** `std::unique_ptr<real[]> data_;`, constructed as `new real[n]` (not `make_unique`).
**No destructor, no copy/move declarations** — the implicit ones are correct.

**Alternatives rejected:**

| Rejected | Why |
|---|---|
| **raw `new[]` / `delete[]`** | Writing a destructor silently takes on the Rule of Five. Measured: copy-without-handling gives `free(): double free detected`, abort at **runtime**. With `unique_ptr` the same mistake is `error: use of deleted function` at **compile time**. |
| **`malloc`/`free`** | Needs a cast in C++, manual `sizeof` arithmetic, and returns `NULL` instead of throwing — the failure-handling the C version had to write by hand. |
| **`std::vector<real>`** | Copyable, so an accidental 32 MB copy compiles silently. Also value-initialises: 0.0749 s vs 0.0330 s for 128 MB, and that memset is *serial*, which is wrong for first touch. |
| **`std::make_unique<real[]>(n)`** | Value-initialises too (0.0781 s) — a genuine surprise. Use `unique_ptr<real[]>(new real[n])` for uninitialised storage. |
| **Writing `~Field() {}`** | Looks harmless, silently **deletes the implicit move constructor**, making `Field` non-swappable (`std::swap` fails to compile). Verified via `is_move_constructible`. |

**Cost:** zero. `sizeof(unique_ptr)` equals a raw pointer; accessors inline away. Measured at
N=2048, assert-live / `-DNDEBUG` / raw-pointer versions ran 0.0057 / 0.0058 / 0.0063 s — within
noise, because the kernel is memory-bound.

**Status:** current

## 003 — Input validation throws; `assert` is only for internal invariants

**Date:** 2026-09-18
**Author:** drafted by Claude from a decision reached with Krit in review.
**Claim:** a `Config` that exists is a `Config` that is numerically stable — **in the build that
produces the benchmark numbers.**

**Force:** `assert` is compiled out by `NDEBUG`, and CMake's Release config is `-O3 -DNDEBUG`.
Verified: an unstable configuration (`rx+ry = 1.0`) aborted at `-O0` and passed silently with
`exit=0` at `-O3 -DNDEBUG`.

**Decision:** `require(cond, msg)` throwing `std::invalid_argument` for anything a caller or user
could get wrong — every input, and the stability condition. `assert` kept only for invariants that
are unreachable unless this code is itself buggy.

**Alternative rejected:** leaving the stability check as an `assert`. It is the single guarantee
behind every reported number, and it costs one comparison *per run*.

**Two traps found while doing this, both now design constraints:**
- **Validate inputs, not derived values.** `assert(delta_x > 0)` does not catch `n_interval_x = 0`,
  because `1.0/0` is `inf` and `inf > 0` is true — and the resulting config *looks healthy*
  (`rx+ry = 0.4`, because `1/inf^2 = 0`).
- **An upper bound says nothing about sign.** `rx + ry <= 0.5` accepted `stability_coeff = -1.0`,
  giving `rx+ry = -0.5` and `dt = -2.5e-05` — time running backwards.

**Status:** current

## 004 — The timestep loop lives in a per-variant `run(const Config&)`; `main()` is thin

**Date:** 2026-09-21
**Author:** drafted by Claude at Krit's request, after Krit twice asked for the next step rather than
choosing between A/B/C. To be accepted or amended by Krit.

**Claim:** the convergence test exercises the *same* kernel that `main.cpp` runs, at many grid sizes,
without a command line.

**Forces:**
1. The test must run the solver with no command line — so the solver takes a `Config`, not `argc/argv`.
2. ADR-001: the four variants differ only in the pragma — each variant still owns its own loop.
3. Shared code must be header-only (GCC nvptx offload across translation units).
4. The test must exercise the real kernel, not a copy of it.

**Decision:** each variant gets `src/<variant>/solver.hpp` with `run(const Config&)` that allocates,
applies the initial condition, steps to the end and **returns the final field**. `main.cpp` becomes
`parse_args` -> `run` -> print. `tests/convergence.cpp` includes `src/serial/solver.hpp` and calls
`run` at each N. The diff between variants moves from `main.cpp` to `solver.hpp`, and stays
pragma-only.

**Alternatives rejected:**

| Rejected | Why |
|---|---|
| **B. Test runs the binary and parses stdout** | Tests the real program, but depends on the print format, only sees what `main` prints (needs `main` to compute L2), and the later cross-version check would need full fields dumped to files. |
| **C. Copy the stencil into the test** | Breaks force 4 silently: the test passes while proving only that its own copy converges. |
| **One shared `run()` in `include/heat/`** | Breaks force 2: variants sharing one loop cannot differ by their pragma. |

**The trap inside this design, measured (throwaway, N = 20..160, fixed t_end):** returning the wrong one
of the two buffers — one timestep stale — **still converges at order 2** (1.973, 1.984, 1.992 vs the
correct 1.970, 1.984, 1.991). Only the error *size* differs: 5.9e-05 vs 3.6e-06 at N=160. Staleness
costs O(dt) = O(dx^2), the same order as the discretisation error. Consequences:
- **Design:** swap the `unique_ptr`s themselves each step (not just raw pointers), so one named buffer
  always owns the latest field and is the one returned.
- **Test:** check the error *magnitude* against a bound, not only the observed order.

**Amendments (2026-09-22, verified before implementation):**
- **`run()` lives in a per-variant namespace — `heat::serial::run`, `heat::openmp_cpu::run`, …** The
  cross-version agreement test must include two variants' `solver.hpp` in one translation unit; both
  defining `heat::run` is `redefinition of 'heat::run(const Config&)'`. Nested namespaces compile.
- **The kernel reads through `const real* u`.** Writing to the current field — the in-place update
  from gate question 1 — then fails to compile instead of silently converging at order 0.
- **Acceptance: moving the code must not change a single digit.** With the initial condition taken
  from `exact(x, y, 0.0, config)` (`exp(0)` is exactly 1.0) and the owners swapped, the centre is
  still `0.82085005668074662` — bit-identical to the pre-refactor `main.cpp`.
- `tests/convergence.cpp` reaches the header via
  `target_include_directories(test_convergence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)` and
  `#include "serial/solver.hpp"`.

**Status:** approved, current

## 005 — Time the timestep loop inside `run()`, and return the time with the field

**Date:** 2026-09-22
**Author:** decision and reasoning by Krit; drafted by Claude.

**Claim:** the reported time is the cost of the timestep loop and nothing else, measured identically
in all four variants.

**Forces:**
- The comparison is about the kernel. Allocation, the initial condition and output are not.
- In the OpenMP variant the initial condition is itself parallel, so timing around it would partly
  measure first touch, not the kernel.
- Measurements on this machine drift between sessions (same code 44% slower on a different day, audit #19); unrelated work inside the timed region
  adds variance that has nothing to do with the question.
- All four variants must time exactly the same region, or a difference in *what* is timed shows up as a
  difference in performance.

**Decision:** start a `std::chrono::steady_clock` immediately before the `for (step …)` loop in `run()`,
stop it immediately after, and return the elapsed seconds alongside the field. `steady_clock`, not
`omp_get_wtime()`, so the serial variant stays OpenMP-free.

**Krit's reasons:** (1) returning the inner time loses nothing — `main` can still time the whole call if
asked, but the inner time can never be recovered from the outer one; (2) timing the whole call adds
variance from work that is not being measured, so measure immediately around the work.

**Alternative rejected:** timing the whole `run()` call from `main` — simpler, but it folds allocation and
(for OpenMP) parallel first touch into the kernel time.

**Side effect worth knowing:** because the OpenMP initial condition is parallel, the thread pool is
created *before* the clock starts, so thread start-up is excluded from the timed loop.

**Status:** current (implemented 2026-09-22: `include/heat/result.hpp`, `include/heat/timer.hpp`; centre unchanged, tests pass)
