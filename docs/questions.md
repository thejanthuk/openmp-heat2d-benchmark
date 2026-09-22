# Candidate research questions

> **Template/planning file written by Claude (an AI assistant).** Entries below are Claude's unless marked otherwise.

The supervisor has said they have no fixed topic yet, so at some point the proposal comes from me. Research
questions are *generated*, not found. One candidate a week, each with the observation that produced
it — by month three there should be twenty and the good one should be obvious.

**The five generators:**

1. **A number that doesn't match its model.**
2. **A mandated choice nobody measured.**
3. **A crossover** — two methods with different scaling always swap somewhere.
4. **A negative result someone should have published.**
5. **The same thing in a harder setting** — 2-D → 3-D, explicit → implicit, one node → many.

## Template

```
### <question, phrased so a measurement could answer it>
**Generator:**    which of the five
**Observation:**  the specific number or event that produced it
**Measurable as:** what you would actually run
**Worth it?:**    honest guess — paragraph, section, or project
```

---

### Why does GCC's nvptx OpenMP offload reach only ~3% of achievable device bandwidth on a 5-point stencil, and what recovers it?
**Generator:** 1 — a number that doesn't match its model.
**Observation:** N=2000, 500 steps moves ~48 GB. GTX 1050 does 112 GB/s → predicted ~0.53 s.
Measured 13.15 s. The PCIe half of the same experiment matches its (corrected) model to ~3%, so the defect is
localised to kernel execution.
**Measurable as:** ablation over launch geometry (`num_teams`/`thread_limit`), `collapse(2)` on/off,
FP32 vs FP64, PTX ISA target, device optimisation level — each predicted before it is run.
**ANSWERED 2026-09-17** (`docs/00-gpu-forensics.md`): GCC's nvptx runtime caps threads per team at
20, defaults to 8, and ignores `thread_limit`, `num_threads`, `OMP_NUM_THREADS`,
`OMP_TEAMS_THREAD_LIMIT` and `OMP_THREAD_LIMIT`. Default launch = 80 threads on a 10,240-slot GPU.
Best achievable is 6.4% of device bandwidth; nothing recovers it.
**Worth it?:** a section at minimum. It became the strongest result in the project — but it is a
finding about *GCC's* offload, not about OpenMP offload as a model.

### Do Clang and NVHPC share GCC's 20-thread-per-team cap, or is this a GCC defect?
**Generator:** 1 — the follow-on question the answer created.
**Observation:** the cap above is not in the OpenMP specification; it is an implementation choice.
If Clang reaches full occupancy on the same hardware and the same directives, GCC's offload is the
story. If every compiler caps similarly, the story is about the offload model itself.
**Measurable as:** the same triad and the same stencil under `llvm-runtimes/openmp[offload]`
(needs installing — `libomptarget` is absent) and/or NVHPC, comparing `omp_get_num_threads()` inside
the kernel and achieved bandwidth.
**Worth it?:** **this is the strongest candidate for the project's headline result.** A
compiler-to-compiler comparison on an identical kernel is exactly the kind of controlled experiment
the project is already built to run, and the answer is genuinely unknown.

### What does the spec-mandated `collapse(2)` actually cost, and when does it stop costing?
**Generator:** 2 — a mandated choice nobody measured.
**Observation:** `cpu-1t` runs at 0.55–0.68x of serial at *every* resolution measured, i.e. the
directive costs ~1.6x before a single extra thread wins any of it back. Suspected cause:
linearising the iteration space inhibits vectorisation and adds integer div/mod per point.
**Measurable as:** `collapse(2)` vs `parallel for` on `i` alone, across resolutions and thread
counts, on CPU and GPU separately — they may not agree, which would itself be the interesting part.
**Worth it?:** a paragraph at minimum, and it is the respectful way to disagree with a supervisor's
spec: bring a table, not an opinion.

**PARTLY ANSWERED 2026-09-22** (N=2000, 100 steps, best of 5, GCC 16 `-O3 -march=native`):

| | serial | `collapse(2)` | `parallel for` on `i` only |
|---|---|---|---|
| 1 thread | 1.048 s | 1.376 s (**0.76x**) | 0.996 s (1.05x) |
| 4 threads | | 0.998 s (1.05x) | 0.955 s (1.10x) |

`collapse(2)` costs ~1.4x at 1 thread by blocking vectorisation (14 -> 2 vector ops in the generated
code) — **not** thread overhead, which measured 0.74-7 us per region, i.e. <0.2% of the gap. The penalty
vanishes at 4 threads because four scalar cores can saturate memory, and both versions then hit the same
wall. **Idle re-run 2026-09-22:** serial moves 12.5–12.8 GB/s against a ~14.5 GB/s ceiling counted the same
way — **~87%** (neither the 99% nor the 73% claimed earlier that day; see audit). Most a thread count could
add is ~1.14x; the best measured is 1.06x (4 threads, no `collapse`). `collapse(2)` idle, 1 thread: **1.42x**
(C), **1.46x** (C++).
Open: does the penalty reappear on a machine with more memory bandwidth, where the kernel stops being
memory-bound? Needs the university server.

**Mechanism confirmed by GCC's own report (2026-09-22, `-fopt-info-vec-all`), after Krit challenged the
claim.** Without `collapse(2)` the kernel's inner loop is `vectorized using 32 byte vectors` in both the C
and C++ versions. With it: C — `not vectorized: unsupported control flow in loop`; C++ — `not vectorized:
vectorization is not profitable` (only a basic-block fragment, 16-byte). The merged loop needs a
row-wraparound branch the vectoriser cannot handle. Krit's original C code, 1 thread, n=2000 (measured under load; idle re-run: **1.42x**): 1.28x
slower with `collapse(2)` (1.324 vs 1.036 s). Compile time identical (0.21 s). Krit's own pre-existing C
benchmark already showed OpenMP-1-thread at 0.55-0.68x of serial at every resolution.
Side finding: the vectorised versions are `loop versioned ... because of possible aliasing` — the case
`restrict` exists for.

### Why is OpenMP 26x slower than serial on a 100x100 grid, when a parallel region costs only ~7 us?
**Generator:** 1 — a number that doesn't match its model.
**Observation (2026-09-22):** `0 100 100 500`, default thread count: serial 0.0023 s, OpenMP 0.061 s —
about 122 us per timestep. An empty `parallel for` measured 0.74 us (1 thread) to 7 us (8 threads) per
region. The overhead model is off by ~17x at this size. (At N=2000 the same model was *too large* to
matter — so the bottleneck changes with problem size.)
**Measurable as:** per-step time vs N (e.g. 50..2000) at 1/4/8 threads; `OMP_WAIT_POLICY=active` vs
`passive`; `OMP_PROC_BIND=close`; a crossover N where OpenMP starts to win.
**Worth it?:** probably a paragraph — "where does parallelism stop paying?" is a crossover (generator 3)
the report should have anyway.

### Why is 8 threads the slowest thread count, while 16 recovers?
**Generator:** 1 — a number that doesn't match its model.
**Observation (reproduced 2026-09-22, idle):** N=2000 stencil, `collapse(2)`: 4 threads 1.04x, **8 threads
0.88x**, 16 threads 0.98x vs serial (under load earlier the same day: 0.69x at 8, 0.96x at 16). 8 threads =
two hyperthreads per core. Neither "more threads = more contention" nor "context switching" predicts that
oversubscribing to 16 would *help*.
**Measurable as:** `OMP_PROC_BIND=close|spread` and `OMP_PLACES=cores|threads` at 8 threads; per-thread
timing; `perf stat` cache misses at 4 / 8 / 16.
**Worth it?:** a paragraph — it's a real, reproducible anomaly in the report's own scaling table.
