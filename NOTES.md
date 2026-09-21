# Lab notebook

A running record of what I predicted, what actually happened, and what I did not understand until
I did. Three purposes, in order of how much they matter:

1. **It is how the learning sticks.** Writing a prediction down before measuring is the whole loop.
2. **It is evidence of authorship.** A dated trail of my own reasoning, alongside the git history.
3. **It is raw material for the report.** Methodology and discussion are already half written if
   this file is kept honestly.

## How to use it

One entry whenever something surprises me, confuses me, or gets resolved. Two minutes, not twenty.
**Keep the failures in** — a wrong prediction that I then explained is worth more than a right one.

```
### YYYY-MM-DD — short title
**Predicted:**  what I expected, and why
**Observed:**   what actually happened
**Explained:**  the resolution, or OPEN if still unresolved
```

Do not tidy this up later. The messy version is the useful one.

---

## Week 1 — GPU forensics

Goal: find why the GPU kernel measures 25x slower than its roofline, and learn the method that
found the discrepancy in the first place.

| # | Checkpoint | Verified when | Done |
|---|---|---|---|
| 0 | Toolchain re-verified | `OMP_TARGET_OFFLOAD=MANDATORY` + in-kernel `omp_is_initial_device()==0` both pass | [ ] |
| 1 | Host bandwidth ceiling | STREAM triad measured; compare to my written prediction | [ ] |
| 2 | Device bandwidth ceiling | trivial `omp target` triad measured | [ ] |
| 3 | PCIe ceiling | `target enter data` on 32 MB timed | [ ] |
| 4 | Prediction table filled in **before** any ablation | all four versions predicted at N=2000 | [ ] |
| 5 | Ablations a–g, one variable at a time | each one predicted before it was run | [ ] |
| 6 | CPU side: `-O3 -march=native`, `collapse` on/off, hoisted parallel region, `restrict` | vectorisation confirmed via `-fopt-info-vec` | [ ] |
| 7 | `docs/00-gpu-forensics.md` written | prediction vs measurement within ~20%, or the gap explained | [ ] |
| 8 | First post-mortem written | `docs/postmortems/` — on my own reported result | [ ] |

### Ceilings — MEASURED 2026-09-17 (no longer a prediction exercise; the numbers were already in the plan)

| Ceiling | Measured | Theoretical | Achieved |
|---|---|---|---|
| Host memory (triad) | 9.27 GB/s | 19.2 | 48% |
| PCIe host->device | 3.66 GB/s | 15.75 | 23% |
| GPU triad, resident | **7.15 GB/s** | **112** | **6.4%** |

### Ablations — deferred, not abandoned

The stencil ablations (`-march=native`, `collapse(2)` on/off, float vs double) still matter, but the
25x is already explained and they are not blocking the C++ rewrite. Do them when the rewrite gives a
clean harness to run them in.

**The one rule that replaces the prediction ritual:** before accepting any number, compute what the
hardware allows. Arithmetic, not intuition — 48 GB / 9.27 GB/s, or 3*N*8 bytes / elapsed. If the
measurement beats the ceiling, the measurement is broken. That caught four silent benchmark failures
on 2026-09-17; none of them errored.

### Gate questions — answer unaided before Week 2

Without looking anything up:

1. Why can the grid not be updated in place?
2. Where does `rx + ry <= 1/2` come from, and what happens if I ignore it?
3. Why is `j` the inner loop and not `i`?
4. What does `restrict` promise, and what does the compiler do differently because of it?
5. My N=1024 run took T seconds. What will N=2048 take at the same step count, and why is it not 4T?

Krit's answers (at 2026-09-16):

1. I assume that the question ask about why the grid cannot just update using the same array over and over again. It's
   because the propagated values will not be the new temperature respected to the next time step, instead it'll be a
   mess of cummulative values.
2. I don't know. I can't even remember what rx and ry are, but I'm pretty sure it's the gap between blocks for
   propagating. All I know about, is if it's violated, the number we got will be messed up.
3. Better cache coherency since the CPU (or OS, I can't remember) will automatically alloted the nearby elements to
   caches.
4. I don't know for both of the questions. But if you talking about restricting the threads, I can tell you that it
   promise either make the threads running closely to the it core as much as possible or far away for better cache
   localization. I don't know about the compiler, through.
5. I don't know. All I can think about is 4T since it doubles the resolution for both axis.

---

## Entries

### 2026-09-15 — the prediction I did not make
> **DRAFTED BY CLAUDE as a format example — rewrite in your own words or delete it.**
> This file is authorship evidence. Nothing should stay in it that you did not write.

**Predicted:** nothing. I benchmarked all four versions and reported the numbers without ever
computing what the hardware was capable of first.
**Observed:** serial CPU 3.97 s at N=2000/500 steps; GPU optimized 13.15 s. I concluded that GPU
offload was ~3x slower than a single CPU core and wrote that down as a result.
**Explained:** OPEN as to root cause, but the conclusion was already wrong and I could have known.
The kernel moves ~48 GB. My DRAM does ~11.9 GB/s → serial *should* take 4.0 s, and it took 3.97,
so the serial code is at ~100% of its ceiling. The GTX 1050 does 112 GB/s → the same work *should*
take ~0.53 s. Measuring 13.15 s means the GPU kernel is at ~3% of its ceiling, which is not a
finding about GPUs, it is a defect. Separately, naive minus optimized = 12.64 s against a PCIe
model predicting 12.8 s, so the transfer half of the experiment is sound to 1% and the
`target data` analysis stands. **The lesson: every number needs a ceiling before it needs an
explanation, and modelling the whole experiment tells you which half to distrust.**
