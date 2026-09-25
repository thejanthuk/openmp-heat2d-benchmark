# Reading order for the two reference books

> **Drafted by Claude (an AI assistant)** at Krit's request, 2026-09-25. Ordered by general importance
> to a numerical/parallel programmer first, and relevance to this project second. Page numbers are the
> **printed** page numbers; each book's PDF offset is given so a viewer's "go to page" lands correctly.

Per the working agreement, each item carries a question. Read to answer the question, not to finish the
pages. If a measurement in this project later contradicts something here, the measurement wins and the
contradiction goes in `NOTES.md`.

---

# Book 1 — Chapra & Canale, *Numerical Methods for Engineers*, 4th ed. (2002)

`docs/NUMERICAL METHODS ... Chapra ... .pdf` — 954 PDF pages. **Printed page + 24 = PDF page**
(book p. 840 = PDF p. 864).

## Tier 1 — first (~60 pages)

The foundation of the subject, and all four are already load-bearing in this repo's code.

**1. Ch 4.1–4.3 — Taylor series, error propagation, total numerical error (pp. 73–95).**
The most important chapter in the book. Every finite-difference formula, every "order" claim and every
stability limit is a truncated Taylor series.
*Question:* the convergence test measured order 1.9987. Where does the exponent 2 come from, and why is
it not exactly 2?

**2. Ch 3.4 — round-off errors and machine epsilon (pp. 57–68); skim 3.1–3.3 (pp. 50–57).**
Limited precision, subtractive cancellation, and non-associative addition.
*Question:* the OpenMP solver is bit-identical to serial with `schedule(static)` but differs every run
with `schedule(dynamic)`. Which paragraph explains that, and why is it not a bug?

**3. Ch 23.1–23.2 — high-accuracy differentiation formulas, Richardson extrapolation (pp. 632–637).**
Where the stencil comes from and how to raise its order deliberately.
*Question:* derive `(T[i+1] - 2T[i] + T[i-1])/dx^2` and state its error term. Then: Richardson
extrapolation and the convergence test both compare two step sizes — what is the relationship?

**4. Ch 30.1–30.2 — heat conduction equation, explicit methods, §30.2.1 convergence and stability
(pp. 840–845).** This project's exact scheme in the book's notation.
*Question:* Chapra writes the 1-D limit as `lambda = k*dt/dx^2 <= 1/2`. Show it is the same statement as
`rx + ry <= 1/2`.

## Tier 2 — next (~45 pages)

**5. Ch 30.3–30.5 — implicit methods, Crank-Nicolson, two dimensions (pp. 845–855).**
§30.5 gives the 2-D explicit criterion as `lambda <= 1/4` on a uniform grid — the same as `rx+ry <= 1/2`
— and introduces ADI.
*Question:* Crank-Nicolson is unconditionally stable and second-order in time. Why is it still the wrong
choice for this project? (What is the deliverable?)

**6. Ch 29.1–29.3 — Laplace equation, Liebmann's method, boundary conditions (pp. 820–834).**
The steady state the solver decays toward, and how Neumann boundaries work.
*Question:* Liebmann's method is Gauss-Seidel. What does explicit time-stepping compute that an elliptic
solve does not?

**7. Ch 11.1–11.2 — tridiagonal systems (Thomas algorithm) and Gauss-Seidel (pp. 285–297).**
The solvers behind every implicit method; the practical side of `docs/iterative-solvers.md`.
*Question:* Thomas is O(n) where Gauss elimination is O(n^3). What property of the matrix buys that?

**8. Ch 25.1–25.3 and 26.1 — Euler to RK4, then stiffness (pp. 682–710, 726–730).**
*Question:* RK4 on this problem allows `r <= 0.696` against explicit Euler's 0.5. Using §26.1's idea of
stiffness, why does the bigger stability region not make RK4 the obvious win here?

## Tier 3 — when it comes up (~40 pages)

- **Ch 17.1, linear regression (p. 440)** — fitting a slope on log-log axes: the convergence order, and
  benchmark scaling fits.
- **Ch 21.1 and 21.5, trapezoidal rule and multiple integrals (pp. 586, 608)** — why `l2_error`
  multiplies by `dx*dy`: the norm is a quadrature over the domain.
- **Ch 19, Fourier approximation (p. 507)** — the mathematics under von Neumann stability analysis.

## Skip for now

Ch 2 (Excel/MATLAB/Fortran 90, 2002-era), Ch 5–8 (roots), 13–16 (optimisation), 18/20/22/24/28/32
(interpolation, case studies), 27 (boundary-value ODEs), 31 (finite elements — worth an hour only if
the supervisor raises FEM).

## Two practical notes

- The PDF is a scan; its text layer mangles formulas. Read equations from the page image.
- Chapra's conventions (`lambda` for the diffusion number, `i`/`l` for space/time indices) differ from
  this repo's. **If they clash, the supervisor's convention wins.**

---

# Book 2 — Chapman, Jost & van der Pas, *Using OpenMP* (MIT Press, 2007)

`docs/Using OpenMP ... .pdf` — 378 PDF pages. **Printed page + 25 = PDF page** (book p. 125 = PDF p. 150).

## Read this caveat before anything else

**The book is OpenMP 2.5. It predates both constructs this project is graded on.**

- `collapse` arrived in **OpenMP 3.0 (2008)**. In this book it appears only as a *proposal* for a future
  release — Ch 9, pp. 312–313, Figure 9.2. Worth reading for exactly that reason: it shows what problem
  the clause was invented to solve.
- `target` / `teams` / `distribute` (GPU offload) arrived in **OpenMP 4.0 (2013)** and are **absent**.
  For those, use the OpenMP specification (openmp.org) and the measurements in `docs/00-gpu-forensics.md`.

What the book is still authoritative on: threads, work-sharing, the memory model, scheduling,
synchronisation, false sharing, NUMA and first touch, how to measure, and what the compiler actually
generates. That substrate has not changed.

## Tier 1 — first (~45 pages)

**1. Ch 5.2 — performance of sequential programs: memory access patterns, TLB, loop optimisations,
pointers and contiguous memory in C (pp. 125–138).**
The most valuable section in the book, and it is not even about parallelism: a parallel program that is
slow serially stays slow.
*Question:* §5.2.2 explains the TLB. We measured the second-level TLB knee between 1536 and 2048 pages
(`docs/memory-hierarchy-from-zero.md`). Does the book's explanation predict the 6.2 -> 11.2 ns jump?

**2. Ch 5.3 — measuring OpenMP performance, and the overheads of the OpenMP translation (pp. 138–145).**
How to time a parallel program honestly, and what the runtime costs.
*Question:* we measured an empty parallel region at 0.74 us (1 thread) to 7 us (8 threads) and used it to
*reject* "thread overhead" as the explanation for the slowdown. Does §5.3.2 agree with that order of
magnitude?

**3. Ch 1.2 and 2.3.3 — cache memory is not shared; the OpenMP memory model (pp. 3–6, 28–29).**
Short, and the basis of every correctness argument that follows.
*Question:* what exactly does OpenMP guarantee about when one thread's write becomes visible to another?

**4. Ch 7.2.1, 7.3.4, 7.4 — data races, a well-masked data race, debugging OpenMP (pp. 243–246,
266–276).**
*Question:* the book's advice assumes a race detector that works. Ours does not (TSan reports false
positives on correct OpenMP code — `docs/postmortems/2026-09-22-tsan-openmp-false-positives.md`). Which
of §7.4's techniques still apply when the tool is unavailable?

## Tier 2 — next (~40 pages)

**5. Ch 5.4–5.5 — best practices; avoid false sharing; private versus shared data (pp. 145–156).**
Especially §5.4.4 "maximize parallel regions" and §5.4.5 "avoid parallel regions in inner loops"
(both p. 148).
*Question:* this solver opens a parallel region every timestep. What does §5.4.4 recommend instead, and
what would it cost to restructure the kernel that way?

**6. Ch 6.1–6.2 — scalability challenges, cc-NUMA, memory placement and thread binding (pp. 191–200).**
This is the first-touch material behind the initial-condition pragma.
*Question:* first touch matters on a multi-socket machine. This machine is single-socket. Does the
pragma on the initialisation loop buy anything here, and is it still right to keep it?

**7. Ch 8.3.4, 8.3.9 and 8.5 — translating parallel regions, do idle threads sleep, impact of OpenMP on
compiler optimisations (pp. 286–291, 300–304).**
§8.5 is the general form of the biggest measured finding in this project.
*Question:* we measured `collapse(2)` costing 1.42x/1.46x because GCC stops vectorising the loop. Does
§8.5 explain *why* a directive restricts the optimiser, in general terms?

**8. Ch 4.5.7 and 4.8.4 — the schedule clause; the reduction clause (pp. 79–83, 105–110).**
Read as reference, not cover to cover.
*Question:* why does `schedule(dynamic)` change the L2 error in the last digits while `schedule(static)`
does not?

**9. Ch 6.6 — performance analysis: profiling, interpreting timing, hardware counters (pp. 228–241).**
*Question:* which counter would confirm that this kernel is bandwidth-bound rather than compute-bound?

## Tier 3 — skim or reference

- **Ch 3 (pp. 35–50)** — a first OpenMP program. Skim; this level is already passed.
- **Ch 4 (pp. 51–123)** — the language reference. Look things up; do not read straight through.
- **Ch 5.6–5.8 (pp. 156–189)** — the matrix-times-vector case study. Read the C results (§5.6.3, p. 159);
  §§5.7–5.8 are Fortran-specific.
- **Ch 9.4 (pp. 312–313)** — the `collapse` proposal, as described above.

## Skip

Ch 6.3–6.5 (SPMD, MPI hybrid, nested parallelism), Ch 8.2 and 8.3.1–8.3.3 (compiler front-end detail),
Ch 9 apart from the `collapse` figure, Appendix A (glossary — look up as needed).

---

## Suggested sequence across both books

1. Chapra Tier 1 items 1–2 (error and floating point) — they underpin both books.
2. OpenMP Tier 1 item 1 (Ch 5.2, sequential performance) — it explains numbers already measured here.
3. Chapra Tier 1 items 3–4 (the stencil and its stability) — the solver's mathematics.
4. OpenMP Tier 1 items 2–4 (measuring, memory model, races).
5. Then Tier 2 of whichever book the next milestone needs: Chapra Ch 30.3–30.5 if the discussion turns to
   implicit methods; OpenMP Ch 6 and 8 before the GPU variants.

**Beyond these two books:** `docs/book-recommendations.md` lists what to read for the career goal,
including the 2017 sequel *Using OpenMP — The Next Step* and the 2023 *Programming Your GPU with OpenMP*,
which cover what this 2007 volume lacks.

The GPU chapters do not exist in either book. That material is the OpenMP 4.5+ specification plus this
repo's own measurements.
