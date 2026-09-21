# Week 1 forensics: why the GPU kernel ran 25x slower than its roofline

**Status: root cause identified 2026-09-17.** All measurements on the test machine (i7-7700HQ, single-channel
DDR4-2400, GTX 1050 / GP107 / sm_61), GCC 16.2.0 nvptx offload, every GPU run under
`OMP_TARGET_OFFLOAD=MANDATORY` with `omp_is_initial_device() == 0` asserted inside the kernel.

## The question

The four-version benchmark reported "GPU offload is ~3x slower than a single CPU core". At N=2000 /
500 steps the kernel moves ~48 GB:

| Version | Measured | Roofline | |
|---|---|---|---|
| Serial CPU | 3.97 s | 4.0 s | at ~100% of achievable DRAM bandwidth |
| naive - optimized | 12.64 s | 12.8 s | PCIe transfer model good to 1% |
| GPU optimized | 13.15 s | **0.53 s** | **25x off** |

Two thirds of the experiment matched its model, which localised the defect to kernel execution.

## Step 1 — measure the ceilings

| Ceiling | Measured | Theoretical | Achieved |
|---|---|---|---|
| Host memory (STREAM triad, 32 MB arrays) | **9.27 GB/s** | 19.2 GB/s | 48% |
| PCIe host->device (32 MB) | **3.66 GB/s** | 15.75 GB/s | 23% |
| PCIe device->host | **3.73 GB/s** | | |
| **GPU triad, data resident** | **7.38 GB/s** | **112 GB/s** | **6.6%** |

Host bandwidth is flat at ~9 GB/s across 1, 2, 4 and 8 threads — one core already saturates the
single memory channel, which is the expected signature. (At 24 B/element accounting; counting the
write-allocate read makes it 32 B/element = 12.4 GB/s, consistent with the ~11.9 GB/s on record.)

**The GPU triad is the finding.** It is ten lines: `a[i] = b[i] + s*c[i]`. No stencil, no
`collapse(2)`, no index arithmetic, no FP64 subtlety — and it reaches **6.6% of device bandwidth**.
The anomaly reproduces with the solver removed entirely.

That immediately eliminated three hypotheses: `collapse(2)` integer div/mod, the stencil access
pattern, and grid indexing. None of them exist in a triad.

## Step 2 — launch geometry

Sweeping `num_teams` and `thread_limit` produced two diagnostic facts:

- **`thread_limit` changes nothing.** 32 and 1024 give identical bandwidth.
- **More teams is *slower*** — 7.8 GB/s at 80 teams, 2.04 GB/s at 2560.

Querying the device directly from inside the kernel explains both:

| Configuration | `omp_get_num_teams()` | `omp_get_num_threads()` | Total threads |
|---|---|---|---|
| default | **10** | **8** | **80** |
| `num_teams(160) thread_limit(256)` | 160 | **20** | 3200 |
| `num_teams(160)` + explicit `parallel num_threads(128)` | 160 | **20** | 3200 |

**GCC's nvptx OpenMP runtime defaults to 8 threads per team and will not exceed 20, whatever you
ask for.** `thread_limit`, an explicit `num_threads` clause, `OMP_NUM_THREADS`,
`OMP_TEAMS_THREAD_LIMIT` and `OMP_THREAD_LIMIT` were all tested; **none** has any effect.

The GTX 1050 has 640 CUDA cores and ~10,240 thread slots. The default launch uses **80 threads** —
under 1% occupancy. That is the 25x.

## Step 3 — best achievable, after tuning

| `num_teams` | GB/s | % of 112 |
|---|---|---|
| 5 | 3.83 | 3.4% |
| **10** | **7.15** | **6.4%** |
| 20 / 40 / 80 / 160 | 6.8-7.1 | ~6.3% |

**Best achievable on this toolchain: 7.15 GB/s, 6.4% of the hardware's 112 GB/s.**

Measured host bandwidth is **9.27 GB/s**. So on this toolchain the GPU is worth **0.77x the CPU** —
genuinely slower, and no amount of tuning fixes it, because the thread cap is not tunable.

## Conclusion

**The original result was right; the original explanation was wrong.** "GPU offload is slower than
the CPU here" is a true statement about *this toolchain*. What it is **not** is a statement about
GPUs, about the stencil, about `collapse(2)`, about FP64 on consumer Pascal, or about the code.

The correct claim is narrower and much more interesting:

> **GCC 16.2's nvptx OpenMP offload cannot launch enough threads to use the device.** It caps
> threads per team at 20 (defaulting to 8) and ignores every documented mechanism for raising it,
> so a memory-bound kernel reaches ~6% of device bandwidth regardless of tuning. On hardware whose
> bandwidth advantage over the host is ~12x, that deficit is more than enough to make offload a net
> loss.

## What is NOT established

**This is a finding about GCC's offload implementation, not about OpenMP offload as a model.**
Clang/LLVM and NVHPC use different runtimes and may not share the limit. That cannot be tested here:
`libomptarget` is not installed, so Clang offload is unavailable (`llvm-runtimes/openmp[offload]`
would be required). Until that comparison exists, every conclusion must say **"GCC's offload"**, not
**"OpenMP offload"**.

Also untested: `-misa=sm_61` (GCC defaults to sm_30 PTX). Unlikely to matter for occupancy, but it
is still open.

## Consequences for the report

1. The benchmark table stands, with the explanation corrected.
2. The naive-vs-optimized comparison is **unaffected and still valid** — it isolates PCIe transfer
   cost, it matched its model to 1%, and both versions share the same thread cap, so the difference
   between them remains a clean measurement.
3. The honest headline is *"GCC's OpenMP offload achieves 6% of device bandwidth on this hardware,
   which is why the GPU loses to a single CPU core"* — a much stronger result than "the GPU is slow".
4. Any future GPU number must be reported against the **6.4% ceiling**, not the 112 GB/s datasheet
   figure, or the comparison is meaningless.

## Method note

The ceilings were measured *before* the ablations, which is what made this quick: the triad showed
the anomaly with the solver removed, eliminating three of seven hypotheses in one run. Three separate
benchmarks in this session first produced impossible numbers (**103,562 GB/s** host bandwidth — the
compiler had deleted the loop). The defence is always a number to compare against: 103,562 against a
19.2 GB/s theoretical maximum is not a result, it is a bug report.
