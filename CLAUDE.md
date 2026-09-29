# CLAUDE.md

Guidance for Claude Code working in this repository.

## What this project is

The deliverable for an 8-month placement under an academic supervisor: a 2-D heat-equation solver by
explicit finite differences, implemented four ways and benchmarked against itself.
`docs/private/project-spec.pdf` is the supervisor-issued spec.

**It looks like a physics project. It is not.** It is a performance-engineering experiment that
uses a physics kernel as its workload. The scientific content is the *difference* between the
versions, so the comparison — not the solver — is the deliverable.

| # | Version | Key construct |
|---|---|---|
| 1 | Serial CPU | baseline, single core |
| 2 | CPU OpenMP | `#pragma omp parallel for collapse(2)` |
| 3 | GPU offload, naive | `target teams distribute parallel for collapse(2)`, transferring every timestep |
| 4 | GPU offload, optimized | the same kernel wrapped in `#pragma omp target data` |

Versions 3 and 4 must run the **identical** kernel and differ only in where the mapping clause
sits, so the gap between them is a clean experimental measurement of PCIe transfer cost. Version 3's
per-timestep transfer is the control group, **not a bug to fix in place**.

Governing equation and update, with `rx = alpha*dt/dx^2`, `ry = alpha*dt/dy^2`:

    dT/dt = alpha * (d2T/dx2 + d2T/dy2)

    T[i][j]^(n+1) = T[i][j]^n + rx*(T[i+1][j] - 2T[i][j] + T[i-1][j])
                              + ry*(T[i][j+1] - 2T[i][j] + T[i][j-1])

Stability: **`rx + ry <= 1/2`**. Violating it makes the solution diverge rather than error out, so
derive `dt` from `dx`, `dy` and `alpha` and assert the constraint at startup.

## Working agreement — read before touching src/

**Krit writes the solver by hand and may be assessed on defending it.**

**Default behaviour: do not produce solver code.** Review it, question it, run it, explain concepts
around it. Hand over a finished implementation only when he explicitly asks, or says he is stuck
and wants to be unblocked. Infrastructure, notes, and analysis tooling are fine to write.

He picks a mode; honour it and stay in it. Default to **Review** if unstated.

| Mode | Behaviour |
|---|---|
| **Socratic** | Ask questions instead of answering. Use when he is close. |
| **Review** | He writes; you say what is wrong, *why it matters*, and **the recommended action** (which file, which approach, why) — without typing the fix for him. A finding with no action stalls him. |
| **Explain** | Teach a concept. Prose and diagrams, no solver code. |
| **Verify** | Compile and run his code; check convergence; confirm or refute a specific claim. |
| **Unblock** | He asked explicitly. Give the answer **with the reasoning**, then backfill understanding. |

Standing rules:

- **Predict → Measure → Explain.** The prediction is written down *before* running anything.
  A wrong prediction that then gets explained is worth more than a right one.
- **Teach origination, not answers.** The curriculum is five methods — derive an architecture, know
  what to measure, design in C++, generate a research question, run a post-mortem. When a structure
  is needed, have him propose and defend it *before* showing yours. See the plan at
  a teaching plan kept outside the repository.
- **Reading is assigned after a measurement surprises us**, never as advance homework, and always
  as specific pages with a question attached.
- **If the supervisor teaches a different convention** (indexing order, `dx = L/n`, boundary handling),
  **the supervisor's wins.** Help reconcile; do not "correct" toward this guide.
- Encourage incremental commits with the reasoning in the message. That history is authorship
  evidence.
- Keep the record current — see below. Draft `docs/sessions/YYYY-MM-DD.md` at the end of a session.

## The record

| File | Contents |
|---|---|
| `NOTES.md` | Lab notebook: dated *predicted / observed / explained*. Wrong predictions stay in. |
| `docs/sessions/YYYY-MM-DD.md` | Per session: what we did, what was decided and why, what is open. |
| `docs/design-decisions.md` | Per design session: claim, forces, choice, **alternative rejected**. |
| `docs/questions.md` | Candidate research questions + the observation that generated each. |
| `docs/postmortems/` | Written when a reported result was wrong, a bug took >2 h, or a decision was reversed. |

**Attribution — the repo is Krit's authorship evidence and is pushed to GitHub, publicly.**
Every file Claude writes or drafts carries a one-line attribution at the top, and is listed in
`AUTHORSHIP.md`. Session logs are written in **neutral third person** ("Krit asked…", "Claude
measured…"), never in Krit's first-person voice. Never write in `NOTES.md` except in an entry clearly
marked as Claude's. (2026-09-21: logs written as "I said…" had been pushed unlabelled.)

**Cold-start test:** a session opening with no context should be able to reconstruct where the work
stands from `NOTES.md` + `docs/sessions/` + `docs/design-decisions.md` alone.

## Environment — checked against this machine (audit 2026-09-22: `docs/audit-2026-09-22.md`)

**Before quoting any number below, check its label.** *Measured* = re-tested by Claude on this
machine. *Inherited* = copied from an earlier local guide and never re-checked. *Spec* = datasheet.

Gentoo, kernel 7.1.7 (`CONFIG_PREEMPT=y`, `HZ=1000`; transparent huge pages compiled in,
mode **madvise** since the 2026-09-16 kernel rebuild — plain allocations get none unless they ask).
GCC 16.2.0, Clang 23.1.1, CMake 4.3.5, Python 3.14.7, gnuplot, gdb 17.2, rr 5.9.0, perf 7.2,
Valgrind 3.27.1. Not installed: ParaView, `libomptarget`/Archer.

- **i7-7700HQ**, 4 cores / 8 threads. L1d 32 KiB 8-way, L2 256 KiB 4-way per core, **L3 6 MiB 12-way
  shared**, 64 B lines, AVX2+FMA. *(measured)*
- **16 GiB DDR4-2400, single channel** *(inherited — from an earlier `dmidecode`; needs root to
  re-check)*. 19.2 GB/s theoretical.
- **Memory bandwidth — idle, 2026-09-22, always state how bytes were counted.** Triad: 10.6–10.9 GB/s
  STREAM-counted (24 B/element) = **14.2–14.5 GB/s actually moved** (32 B incl. write-allocate) at 1–4
  threads, 13.9 at 8. The serial stencil moves 24 B/point actually: **12.5–12.8 GB/s ≈ 87%** of that
  ceiling. *(Withdrawn: "99%" and "~11.9 GB/s" mixed the two ways of counting; "73%" and "the triad slows
  with more threads" were measured with a video player running.)*
- **OpenMP scaling of the stencil is tiny — idle, N=2000:** best **1.06x** (4 threads, `parallel for` on
  `i`), 1.04x with `collapse(2)`; **8 threads is the slowest** (0.88x with `collapse(2)`), 16 recovers
  (0.98x) — reproduced, unexplained (`docs/questions.md`). Consistent with the ~87% above: the most any
  thread count could add is ~1.14x.
- **GTX 1050**, GP107, **sm_61**, 4 GiB, PCIe gen3 **x16**, memory 7.0 Gbps -> **112 GB/s**
  *(measured via `nvidia-smi`; 128-bit bus and 5 SMs / 640 cores are spec)*. FP64 = 1/32 of FP32
  ≈ 76 GFLOPS *(derived from spec, not measured)*.
- **PCIe: measured 3.66 GB/s host->device, 3.73 device->host** (32 MB, pageable, 2026-09-17), against
  15.75 GB/s theoretical. The earlier "~5–6 GB/s" is withdrawn.

Kernel arithmetic intensity ~10 flops / 24 B ≈ **0.42 flop/byte** → bandwidth-bound on both.

**Machine state changes the numbers more than noise does.** Within one idle run, 10 repeats spread
**+3.7% median, +10% max** over the fastest. Between states it is far larger: Krit's C code, same source
and `-O2` flags, ran 7.94 ms/step (~09-11), **11.42 ms/step with a video player running**, and 8.52 ms/step
once it was closed. **Rules: check the machine is idle before measuring — by `top`'s CPU idle %, not the load average alone (here
`i915_flip` display workers sit in disk-wait and hold the load average near 1–2 on an idle machine); compare only
numbers from the same run; never compare absolute times across days.** (The inherited "median +22%,
max +83%" was not reproduced within a run.)

**Cache boundary — build-dependent (idle, same session, 2026-09-22).** 500 -> 1000 is 4.0x the work but
costs **4.92x** with Krit's `-O2` C and **9.59x** with the `-O3 -march=native` C++. The optimised build's
cliff is twice as large — so the old guide's "11x" was plausible for an optimised build, and an earlier
correction here that called it unreproduced (by comparing it with `-O2` data) was wrong. At N=1000 the
optimised kernel still moves 16.3 GB/s, above the DRAM ceiling, so **draw DRAM-bound conclusions only
from N >= 2000**. Data: `docs/stability-and-scaling.md`.

**Measured findings to keep:**
- `collapse(2)` costs **1.42x (Krit's C) / 1.46x (C++)** at 1 thread, `-O3 -march=native`, idle, because
  it stops GCC vectorising the kernel — confirmed by GCC's own `-fopt-info-vec` report ("unsupported
  control flow" / "not profitable"). Mandated by the spec, so it stays, reported with its cost.
- Padding the grid row does **not** help this stencil: **0.97x** (row 2001), **0.99x** (row 2048), idle.
  It *does* help column traversal (1.4–1.9x).
- DRAM latency 86 ns (64 MB) to 105 ns (1 GB); second-level TLB knee **measured** between 1536 and 2048
  pages (6.2 -> 11.2 ns). All in `docs/memory-hierarchy-from-zero.md`.

## GPU offload — works, but fails silently

Offload is functional here via GCC's nvptx path (**re-verified 2026-09-15**: probe compiles, reports 1 device, and runs on the GPU under `OMP_TARGET_OFFLOAD=MANDATORY`). **The trap:** host GCC is built
`--enable-offload-defaulted`, so a missing or mismatched offload toolchain makes
`#pragma omp target` compile fine, report `omp_get_num_devices() == 1`, and then **run on the host**,
producing plausible and entirely bogus GPU numbers.

**Every GPU timing needs both, never either alone:**
1. `OMP_TARGET_OFFLOAD=MANDATORY` in the environment, and
2. `omp_is_initial_device() == 0` asserted **inside** the kernel, or a harness that refuses to emit
   a GPU row when it is 1.

Build flags (the device-side `-lm` is required — GCC does not forward `-lm` to the device link, so
`sin()` inside a `target` region fails to link):

```sh
gcc -O3 -march=native -fopenmp -fno-stack-protector -foffload=nvptx-none \
    -foffload-options=nvptx-none="-O3 -fno-stack-protector -lm" src.c -lm -o out
```

With CMake, `-fopenmp` must be on the **link** line too, not just compile — it activates the
offload linker plugin. Verified 2026-09-17: omitting it fails **loudly** at link with
``undefined reference to `__offload_func_table'``, so this particular mistake cannot pass silently.
(The silent failure mode is a missing `OMP_TARGET_OFFLOAD=MANDATORY`, which is a different trap.)
A verified, commented `CMakeLists.txt` is in the repo root.

Fragile: host GCC and `cross-nvptx-none/gcc` must stay the same version; a plain `emerge -u gcc`
breaks offload silently. Check
`/usr/libexec/gcc/x86_64-pc-linux-gnu/16/accel/nvptx-none/mkoffload` exists. CUDA is **not** needed
for this path (GCC emits PTX, the driver JITs it). Clang offload is unavailable — no `libomptarget`.

## Correctness tooling — `docs/debugging-toolkit.md`

Read that file before recommending a debugging approach. Load-bearing points, all measured here
on 2026-09-15:

- **Pick the tool from the symptom**, not from habit. Wrong-only-with-threads → TSan. Crash → gdb.
  Heap corruption → ASan. "Worked last week" → `git bisect`. Slow-not-wrong → `perf`.
- **Installed and verified 2026-09-16:** GDB 17.2 (sees OpenMP thread teams), rr 5.9.0
  (record/replay/reverse-execution all confirmed), perf 7.2, Valgrind 3.27.1. ParaView not yet.
- **No kernel change is needed.** `kernel.perf_event_paranoid=2` already permits everything used
  here: user-space HW counters (`cycles:u`, `L1-dcache-load-misses:u`, `LLC-load-misses:u`,
  `dTLB-load-misses:u`), `perf record` on a process, and rr. Only kernel-space events and
  system-wide `perf record -a` are blocked, and neither is needed. If ever wanted:
  `sudo sysctl kernel.perf_event_paranoid=1` — a sysctl, not a rebuild.
- **rr serialises threads onto one core** (measured 9.4x slowdown on 4 threads), so it can neither
  find races nor time parallel code. Division of labour: **TSan finds races (not for OpenMP here — see below), rr debugs
  logic, perf measures speed.**
- **TSan does NOT work for OpenMP on this machine (corrected 2026-09-22).** Both libgomp and libomp
  synchronise their thread pools invisibly to TSan, so every cross-thread hand-off is reported — even
  a single `parallel for` followed by a serial read (GCC 1 report, Clang 2); Krit's correct solver got
  20. The false positives name the user's own lines. The earlier "GCC is clean" claim came from one
  `reduction` test, whose atomics TSan can see. **Race check for OpenMP here = differential testing
  vs the serial solver at 1/2/4/8 threads.** Real TSan needs LLVM OpenMP with Archer (not installed).
  See `docs/postmortems/2026-09-22-tsan-openmp-false-positives.md`.
- Costs: ASan+UBSan ~5x (leave on in debug builds), **TSan ~71x** (N=64 only). The two are
  **mutually exclusive** — the compiler refuses to combine them.
- **A result that changes with thread count or `schedule(dynamic)` is usually NOT a race** — it is
  floating-point non-associativity, and TSan correctly reports nothing. Verified: `schedule(static)`
  at fixed threads is bit-identical across runs; `schedule(dynamic)` differs every run. The L2 error
  is a reduction, so **compare versions to a tolerance, never with `==`**.
- **No sanitizer or debugger reaches OpenMP offload code.** GPU correctness is differential against
  the serial oracle, plus the two device assertions above.

## RESOLVED finding — the GPU 25x  (`docs/00-gpu-forensics.md`)

**Root cause found 2026-09-17: GCC's nvptx OpenMP runtime caps threads per team at 20 and defaults
to 8.** A default launch is `num_teams=10 x 8 threads = 80 threads` on a GPU with 640 cores and
~10,240 thread slots — under 1% occupancy. `thread_limit`, an explicit `num_threads` clause,
`OMP_NUM_THREADS`, `OMP_TEAMS_THREAD_LIMIT` and `OMP_THREAD_LIMIT` were **all tested and all ignored**.

Measured ceilings on this machine:

| Ceiling | Measured | Theoretical | Achieved |
|---|---|---|---|
| Host memory (triad, STREAM-counted) | **10.1–11.0 GB/s** (paired program below, 09-22; 9.27 on 09-17, state unknown) | 19.2 | 53–57% |
| PCIe host->device / device->host | 3.66 / 3.73 GB/s | 15.75 | 23% |
| **GPU triad, resident, best geometry** | **7.15 GB/s** (7.11–7.37 re-run 09-22) | **112** | **6.4%** |

So on this toolchain **the GPU is worth ~0.7x the CPU** (0.65–0.71x: both triads in one process, run twice on
09-22, the second on a quiet machine (CPU ~95% idle, load settled at ~2); source in the audit's appendix; the earlier "0.77x"
divided by a lower 09-17 host figure) and no tuning changes it. The host range here comes from that paired program;
the Environment section's 10.6–10.9 is a separate CPU-only triad the same afternoon — two runs of one quantity. The anomaly
reproduces in a ten-line triad with no stencil, no `collapse(2)` and no indexing, which eliminated
those hypotheses outright.

**The original result was right; the original explanation was wrong.** Report it as *"GCC's OpenMP
offload reaches 6% of device bandwidth"*, never as *"the GPU is slow"*.

**Say "GCC's offload", not "OpenMP offload".** Clang/NVHPC use different runtimes and may not share
the cap — untestable here (`libomptarget` absent). Still open: `-misa=sm_61` (GCC 16 defaults to
**sm_52** — checked in the emitted PTX, 2026-09-22; an earlier "sm_30" was wrong).

**The naive-vs-optimized comparison is unaffected** — both share the cap, so their difference is
still a clean PCIe measurement. Corrected model (audit 2026-09-22): the naive kernel moves **3** arrays
per step (`to: tt`, `tofrom: tt_`) = 48 GB over 500 steps at the measured 3.66/3.73 GB/s -> **13.0 s**,
against the measured 12.64 s gap (~3%). The earlier "matched to 1%" used 4 transfers at an assumed
5 GB/s — two errors that cancelled.

**Any future GPU number is reported against the 6.4% ceiling, not the 112 GB/s datasheet figure.**
