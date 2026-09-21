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

**Attribution — the repo is Krit's authorship evidence and is pushed to the university GitLab.**
Every file Claude writes or drafts carries a one-line attribution at the top, and is listed in
`AUTHORSHIP.md`. Session logs are written in **neutral third person** ("Krit asked…", "Claude
measured…"), never in Krit's first-person voice. Never write in `NOTES.md` except in an entry clearly
marked as Claude's. (2026-09-21: logs written as "I said…" had been pushed unlabelled.)

**Cold-start test:** a session opening with no context should be able to reconstruct where the work
stands from `NOTES.md` + `docs/sessions/` + `docs/design-decisions.md` alone.

## Verified environment — measured on this machine, not assumed

Gentoo, kernel 7.1.7 (`CONFIG_PREEMPT=y`, `HZ=1000`, no transparent huge pages).
GCC 16.2.0, Clang 23.1.1, CMake 4.3.5, Python 3.14.7, gnuplot. Not installed: `perf`, `valgrind`,
ParaView.

- **i7-7700HQ**, 4 cores / 8 threads. L1d 32 KiB/core, L2 256 KiB/core, **L3 6 MiB shared**,
  AVX2+FMA.
- **16 GiB DDR4-2400, single channel** (one SODIMM slot empty). 19.2 GB/s theoretical,
  **~11.9 GB/s measured.** This is why the kernel is bandwidth-bound and 4 cores buy only ~12–24%.
  That is the expected result, not a bug.
- **GTX 1050**, GP107, **sm_61**, 5 SMs, 4 GiB, **112 GB/s**. Consumer Pascal, so
  **FP64 = 1/32 of FP32 ~= 76 GFLOPS**.
- PCIe 3.0 x16: 15.75 GB/s theoretical, ~5–6 GB/s effective.

Kernel arithmetic intensity ~10 flops / 24 B ~= **0.42 flop/byte** → bandwidth-bound on both.

**Benchmark noise is high** (spread: median +22%, max +83% over min). Best-of-N on an idle machine,
never single-shot.

**Cache boundary — use the measured number, not the guide's.** `an earlier local guide/docs/08` claims an
"11x cache cliff" at the L3 boundary. **Krit's own serial data does not reproduce that**: 500 -> 1000
costs **5.02x** where the 4.00x work increase predicts 4.00x, i.e. a **25% excess**, while 250 -> 500
(both in L3) and 1000 -> 2000 (both in DRAM) are 4.06x and 4.00x. So the boundary is real but far
smaller than 11x at `-O2`. Open hypothesis: the `-O2`/no-`march=native` build is instruction-limited
(SSE2) and therefore cannot exploit L3's higher bandwidth, *masking* the cliff — which predicts the
excess should **grow** after rebuilding `-O3 -march=native`. Test this in Week 1 Step 3. Derivation
and data: `docs/stability-and-scaling.md`.

**Known negative results. Do not "fix" these:**
- Padding the grid row (`ny+8`) does not help this stencil (0.86–0.99x): 4 streams into an 8-way
  cache, and conflicts need >8.
- `collapse(2)` costs ~1.6x on the CPU by inhibiting vectorisation. It is mandated by the spec, so
  it stays — measured and reported in a table, not silently dropped.

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
  find races nor time parallel code. Division of labour: **TSan finds races, rr debugs logic,
  perf measures speed.**
- **Use GCC for TSan on OpenMP, not Clang** — the reverse of the usual advice. Archer is absent, so
  Clang's `libomp` produces false positives on *correct* code (measured: 1 warning on a clean
  reduction); GCC is clean and still catches real races.
- Reading a TSan report: **does any frame name a file the user wrote?** If every frame is
  `libomp.so`/`libgomp.so`/`libc`, it is runtime noise.
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
| Host memory (triad) | **9.27 GB/s** | 19.2 | 48% |
| PCIe host->device / device->host | 3.66 / 3.73 GB/s | 15.75 | 23% |
| **GPU triad, resident, best geometry** | **7.15 GB/s** | **112** | **6.4%** |

So on this toolchain **the GPU is worth 0.77x the CPU** and no tuning changes it. The anomaly
reproduces in a ten-line triad with no stencil, no `collapse(2)` and no indexing, which eliminated
those hypotheses outright.

**The original result was right; the original explanation was wrong.** Report it as *"GCC's OpenMP
offload reaches 6% of device bandwidth"*, never as *"the GPU is slow"*.

**Say "GCC's offload", not "OpenMP offload".** Clang/NVHPC use different runtimes and may not share
the cap — untestable here (`libomptarget` absent). Still open: `-misa=sm_61` (GCC defaults to sm_30).

**The naive-vs-optimized comparison is unaffected** — both share the cap, so their difference is
still a clean PCIe measurement, and it matched its model to 1%.

**Any future GPU number is reported against the 6.4% ceiling, not the 112 GB/s datasheet figure.**
