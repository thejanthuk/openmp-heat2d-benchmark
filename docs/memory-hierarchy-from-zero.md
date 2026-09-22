# Why loop order matters: cache lines, pages, stride, SIMD

> **Written by Claude (an AI assistant)** as teaching/reference material for this project. Measurements were run on Krit's machine. Not Krit's own writing.

Built from zero, no vocabulary assumed. Every number here was measured on this machine.

## The fact underneath everything

The CPU runs at ~3.4 GHz — about 3 operations per nanosecond. **A trip to RAM costs roughly 85–105 ns**, which
is **~300–350 CPU cycles** (idle pointer chase, 2026-09-22: 86 ns at 64 MB, 93 ns at 256 MB, 105 ns at 1 GB —
bigger arrays add TLB misses). While waiting for one number, the processor could have done ~300 arithmetic
operations. *(An earlier "60–85 ns, the old ~100 ns not reproduced" compared smaller arrays measured under load
and was wrong; the old guide's ~100 ns holds for large arrays.)* Everything below is hardware avoiding that trip.

## Cache line

A library that lends only **whole books**, never single pages: the cost is the walk to the shelf,
not the weight. Memory is the same. Ask for `t[100]` and the hardware fetches a **64-byte block** —
a **cache line** — holding `t[96]`..`t[103]`. (Measured: `LEVEL1_DCACHE_LINESIZE = 64`.)

**64 bytes / 8 bytes per double = 8 doubles per line.** Everything hinges on that 8.

## Stride

**Stride** = the distance in memory between consecutive things you touch. With `t[i*m + j]`:

| Loop order | Address step | Doubles used per line | Trips per value |
|---|---|---|---|
| `j` inner | 8 bytes (1 double) | **8 of 8** | 1 per **8** values |
| `i` inner | `m*8` bytes — **16,008** at N=2000 | **1 of 8** | 1 per **1** value |

Same arithmetic, **8x the memory traffic**. The kernel is bandwidth-bound, so 8x traffic is 8x time.

## Measured: same array, same work, only loop order swapped

| N | Array | `j` inner | `i` inner | Penalty |
|---|---|---|---|---|
| 256 | 0.5 MB | 0.0001 s | 0.0001 s | **1.3x** |
| 1024 | 8 MB | 0.0014 s | 0.0044 s | **3.2x** |
| 2048 | 32 MB | 0.0049 s | 0.0321 s | **6.5x** |
| 4096 | 128 MB | 0.0202 s | 0.1632 s | **8.1x** |

Converges on the predicted 8x. **It is nearly free at N=256** — when the array fits in cache, the
wasted 7 doubles cost nothing because you return before eviction. The penalty appears only once the
array outgrows cache. Same reason the L3 boundary shows at 500 -> 1000 in the solver data.

## Page and TLB — a second, independent tax

Programs use **virtual** (fake) addresses; the OS maps them to real RAM in 4 KB chunks called
**pages**. That map lives in RAM, so consulting it would itself cost a RAM trip — hence a small
cache *of the map*: the **TLB**. Its second level has **1536 entries** on this CPU — **measured 2026-09-22**:
a one-line-per-page chase costs 6.0 ns at 1024 pages, 6.2 ns at 1536, then **11.2 ns at 2048** with no
data-cache boundary in between. (The first level, 64 entries, shows as a knee between 64 and 128 pages.)
It covers 1536 * 4 KB = **6 MB**.

Stride-`m` access jumps ~16 KB per step — across four page boundaries — so nearly every access needs
a different map entry. Past ~1536 distinct pages the TLB misses, and each miss adds a RAM trip *just
to locate the data*. Column order is billed twice: wasted lines **and** TLB misses.

## Prefetcher — help you forfeit

Hardware watches the access pattern and **fetches ahead** when you march steadily forward, hiding
most of the 100 ns. It recognises only simple patterns and generally will not cross page boundaries.
Stride 1: works. Stride 16 KB: gives up. Sequential access moves less data *and* moves it earlier.

## SIMD / AVX2 — 4 at once, if they are adjacent

Wide registers hold several numbers and operate on all of them with one instruction (**SIMD**):

- `xmm` = 128 bits = **2 doubles**   |   `ymm` (AVX2) = 256 bits = **4 doubles**

**A load instruction grabs one contiguous chunk**, so 4 adjacent doubles load in one go; 4 doubles a
row apart cannot. Generated assembly for `b[i] = a[i]*2 + 1`:

    no -march        addpd       %xmm0, %xmm0            <- 128-bit, 2 doubles, TWO instructions
                     addpd       %xmm1, %xmm0

    -march=native    vfmadd132pd (%rsi,%rax), %ymm1, %ymm0   <- 256-bit, 4 doubles, ONE instruction

Two wins at once: `%ymm` doubles the width, and `vfmadd` is a **fused multiply-add** doing the `*2`
and `+1` in one instruction. GCC defaults to baseline x86-64, which guarantees only SSE2 (2 doubles),
leaving this machine's AVX2+FMA idle — exactly what the `-O2` benchmark builds gave up.

## The three cache levels, and why they do not rescue a bad stride

Measured latency by working-set size (random pointer chase, so the prefetcher cannot help):

| Working set | ns/access | cycles | Level |
|---|---|---|---|
| <= 32 KB | 1.48 | **5** | L1 (32 KB) |
| 64-256 KB | 3.6-4.7 | **12-16** | L2 (256 KB) |
| 512 KB - 4 MB | 9.8-20.6 | **33-70** | L3 (6 MB) |
| >= 8 MB | 59-85 | **200-290** | DRAM |

Steps land exactly at the hardware's 32 KB / 256 KB / 6 MB. L1 is ~58x faster than DRAM.

**Four reasons more cache does not fix a large stride:**

**1. Every level uses the same 64-byte line.** L2 does not store half-lines. So all three levels
hold the whole line with 7 useless doubles in it — they are bigger warehouses for equally wasteful
boxes. The 8x DRAM traffic is decided before any cache gets a say.

**2. Bad stride shrinks every cache 8x in *useful* bytes:** L1 32 KB -> 4 KB, L2 256 KB -> 32 KB,
L3 6 MB -> 768 KB.

**3. The reuse comes too late.** The 7 wasted doubles are wanted on the next `j` iteration, but
between `t[i][j]` and `t[i][j+1]` you walk a whole column — at N=2048 that is 2048 lines = 128 KB
dragged through cache. That *fits* in a 256 KB L2, so capacity alone does not explain the failure.

**4. The real cause: all three caches fail simultaneously.** Each line may live only in one **set**,
holding a few lines (**ways**), and the number of sets is always a power of two. At N=2048 a row is
exactly **256 lines**:

| Level | Sets | stride mod sets | Sets reachable | Lines usable | Of total |
|---|---|---|---|---|---|
| L1 | 64 | 256 mod 64 = **0** | **1** | 8 | **1.6%** |
| L2 | 1024 | 256 | 4 | 16 | **0.4%** |
| L3 | 8192 | — | — | — | *not predictable this way* |

**Correction (audit 2026-09-22):** an earlier version gave L3 as "32 sets reachable, 384 lines, 0.4%".
That is wrong in principle: Intel's L3 is split into slices chosen by an **undocumented address hash**, so
simple stride-mod-sets arithmetic does not predict L3 conflicts. The L1 and L2 rows stand — they use plain
indexing — and they alone explain why the reuse (2048 lines needed) cannot survive in L1 or L2. Whether L3
also suffers is an empirical question; the padding experiment below measures the combined effect.

### Proof: change one number

If this is set mapping rather than capacity, breaking the power of two should fix it — same data,
same per-line traffic, only the addresses shift. Column traversal, row length `N` vs `N+1`:

| N | row = N | row = N+1 | |
|---|---|---|---|
| 1024 | 0.0032 s | 0.0017 s | **1.9x faster** |
| 2048 | 0.0315 s | 0.0220 s | **1.4x faster** |
| 4096 | 0.1613 s | 0.1136 s | **1.4x faster** |

**One extra element per row — 8 bytes — buys up to 1.9x.** Stride went from 256.000 to 256.125
lines, so accesses drift across sets instead of piling into one.

This is a **conflict miss**, distinct from a **capacity miss** (cache genuinely too small) and a
**compulsory miss** (first touch, unavoidable). Conflict misses are fixed by *moving* data, not by
using less of it.

### Why this does not contradict the "padding does not help" note

Both are right; they are different regimes.

- **The stencil**: `j` inner, stride 1, ~4 streams (rows `i-1`, `i`, `i+1`, plus output) into an
  8-way L1. Nothing collides — padding buys nothing and wastes memory. Measured idle 2026-09-22: **0.97x** (row 2001) and **0.99x** (row 2048) — padding does not help this stencil.
(The old guide's lower bound of 0.86x was not reproduced.)
- **Column traversal**: one stream at a pathological power-of-two stride hammering a single set.
  Padding is transformative.

"Does padding help?" has no general answer. It helps when you have **few streams at a power-of-two
stride**. Diagnosing which regime you are in is the skill.

## Why it is four mechanisms, not one

| Mechanism | Cost of getting loop order wrong |
|---|---|
| Cache lines | 8x memory traffic — **measured 8.1x** |
| TLB | extra lookups past ~1536 pages |
| Prefetcher | stops helping; full ~85–105 ns exposed per access |
| SIMD | cannot vectorise — no adjacent values to load |

All four push the same way, which is why the effect is large and reliable rather than marginal.

**Terminology:** this is **spatial locality** — things used together should sit together.
**Cache coherency** is the unrelated protocol (MESI) keeping several cores' caches consistent with
each other. Easy to conflate; they are different problems.
