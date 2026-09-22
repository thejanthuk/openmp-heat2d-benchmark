# TSan cannot check OpenMP code on this machine — the toolkit said it could

> **Written by Claude (an AI assistant)**, about Claude's own reported result. Third person throughout.

## Timeline

- **2026-09-15** — Claude tested ThreadSanitizer on OpenMP: one *correct* program (a `reduction`) and
  one *racy* program (an unsynchronised `+=`). GCC: 0 reports on the correct one, 2 on the racy one.
  Clang: 1 report on the correct one (runtime frames only). Conclusion recorded in
  `docs/debugging-toolkit.md`, `CLAUDE.md` and memory: **"use GCC for TSan on OpenMP — it is clean on
  correct code."** A second demo (in-place stencil, 3 timesteps) was reported as TSan "catching" the
  in-place race at the right line.
- **2026-09-22** — Krit's first OpenMP solver (double-buffered, bit-identical to serial at 1/2/4/8
  threads on four configurations) produced **20 TSan reports** under GCC, all pointing at his own
  source lines.
- Same day — a trivially correct program (one `parallel for` writing an array, then a *serial* read)
  was flagged: GCC 1 report, Clang 2. With two back-to-back regions: GCC 3, Clang 4.

## What made the original conclusion reasonable

Claude did test both directions — a correct program and a racy one — which is the right discipline.
The correct program simply happened to be a `reduction`, whose combining step uses atomic operations
that TSan *can* see. That incidental synchronisation hid the real problem.

## Cause

The OpenMP runtimes (GCC's libgomp, LLVM's libomp) reuse a pool of threads and synchronise them at the
end of every parallel region with mechanisms TSan does not understand (futex-based barriers). TSan
therefore never sees that region *n* finished before anything after it began, and reports **every**
cross-thread hand-off — any data written in a parallel region and later read by another thread — as a
race. Only thread *creation* (the first region) is visible to it.

## Contributing factors

- **A sample of one** correct program generalised to "clean on correct code".
- The report-reading rule "does any frame name a file you wrote?" is useless here: these false
  positives name the user's own lines, because the missing piece is the synchronisation, not the access.
- The 2026-09-15 in-place demo ran 3 timesteps (3 regions), so it would have been flagged **even
  without the bug**. It demonstrated nothing about detecting the in-place race.

## Detection

Five days later, on the first real multi-region OpenMP code. What gave it away: **the reports
contradicted a stronger piece of evidence** — bit-identical results across thread counts, which a real
race in a stencil almost never produces.

## What went right

The disagreement was investigated rather than accepted in either direction: a minimal, obviously
correct program was built to test the hypothesis, and both compilers were checked before concluding.

## Action items

1. **Done** — `docs/debugging-toolkit.md`, `CLAUDE.md` and memory corrected: TSan is unusable for
   OpenMP here without Archer.
2. **The race check for OpenMP code on this machine is differential**: compare every grid point
   against the serial solver at several thread counts and across repeated runs. A deterministic kernel
   with a race rarely stays bit-identical under that.
3. **Open decision for Krit**: install LLVM's OpenMP runtime with **Archer** (the TSan-aware OpenMP
   tool) and use Clang for race checks — a Gentoo rebuild, worth it only if OpenMP work goes beyond
   loops (tasks, `nowait`, shared accumulators), where differential testing gets weaker.
