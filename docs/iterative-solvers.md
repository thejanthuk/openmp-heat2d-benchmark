# Iterative methods for linear systems: which, when, where

> **Written by Claude (an AI assistant)** as teaching/reference material for this project. Measurements were run on Krit's machine. Not Krit's own writing.

Needed because **implicit time-stepping turns every timestep into a linear solve** (the escape from
`rx + ry <= 1/2`), and because the steady state of the heat equation is the Laplace equation. Also
where parallelism stops being easy — the explicit stencil is embarrassingly parallel; Gauss-Seidel
is inherently sequential.

## Measured: iterations to relative residual 1e-8

`-laplacian(u) = 1` on the unit square, zero Dirichlet boundary, matrix-free.

| N | unknowns | Jacobi | Red-black GS | SOR (optimal omega) | **CG** |
|---|---|---|---|---|---|
| 16 | 225 | 942 | 480 | 61 | **27** |
| 32 | 961 | 3779 | 1926 | 125 | **58** |
| 64 | 3969 | 15122 | 7705 | 257 | **118** |
| 128 | 16129 | 60481 | 30816 | 528 | **237** |
| | *ratio as N doubles* | **x4.00** | **x4.00** | **x2.05** | **x2.01** |

Jacobi and GS are **O(N^2)** iterations; SOR and CG are **O(N)**. At N=128 that is **60,481 vs 237
— a factor of 255**, widening with N.

**Benchmark-design warning.** The first version of this test used `f = 2*pi^2*sin(pi x)sin(pi y)`,
whose solution is an **eigenvector** of the discrete Laplacian — CG converged in **1 iteration** and
looked miraculous. A test problem can be too easy in a way that resembles a triumph. Use a
broad-spectrum right-hand side (constant, random, or a point source).

## Decide direct vs iterative FIRST

**Direct wins when:**

- **The matrix is tridiagonal** -> **Thomas algorithm**, O(n), exact. Non-negotiable. This is 1-D
  implicit time-stepping, and it is why **ADI** exists: it splits a 2-D implicit solve into two
  sequences of 1-D tridiagonal solves.
- **Small and dense** (up to a few thousand unknowns) -> LU / Cholesky.
- **Same matrix, many right-hand sides** -> factor once, reuse. **Applies here:** implicit
  time-stepping at fixed `dt` has the same matrix every step.

**Iterative wins for large sparse systems**, because you never form the matrix: for a stencil,
"multiply by A" *is* the stencil. Memory stays O(n) instead of O(n^2). This is **matrix-free**, and
it is why iterative methods dominate PDE work.

## The five, in learning order

**1. Jacobi** — update from *old* neighbours only. O(N^2) iterations, **embarrassingly parallel**.

> **Already written.** Jacobi on `laplacian(u) = 0` is structurally identical to the explicit heat
> kernel. Running the existing solver to steady state **is** a Jacobi solve.

**2. Gauss-Seidel** — use new values immediately. Exactly 2x fewer iterations than Jacobi (measured
at every N). Chapra calls this **Liebmann's method** for Laplace (Ch 29) — same thing, different name.

> **This is the in-place update from gate question 1** — wrong for time-accurate stepping because it
> mixes time levels; correct here, because there are no time levels, only iterates. Same code change,
> opposite verdict, because the goal changed.

**3. SOR** — overshoot the GS correction: `u <- (1-omega)*u_old + omega*u_GS`. Optimal omega drops
O(N^2) to **O(N)**. Catch: `omega_opt = 2/(1 + sin(pi/N))` is known analytically only for model
problems; elsewhere it must be tuned, and it is sensitive.

**4. Conjugate Gradient** — the workhorse. Needs **symmetric positive definite** (the Laplacian is).
O(sqrt(kappa)) iterations; for the Laplacian kappa ~ N^2, so **O(N)** — matches optimal SOR, beat it
by ~2.2x above, and has **no parameter to tune**. Matrix-free.

**5. Preconditioned CG** — what people actually run. Diagonal/Jacobi preconditioning is trivial and
parallel; incomplete Cholesky is stronger but sequential; multigrid-preconditioned CG is the serious
answer.

**Beyond:** **GMRES / BiCGSTAB** once the matrix is nonsymmetric — which happens the moment advection
is added, and CG then fails outright. **Multigrid** is the asymptotic winner at **O(n) total work**:
smooth on the fine grid to kill high-frequency error, restrict the residual to a coarse grid where
the remaining error looks high-frequency, recurse. Not measured here.

## Parallelism — the part that matters for this project

| Method | Parallel? | Catch |
|---|---|---|
| Jacobi | Embarrassingly | Slowest by far |
| Gauss-Seidel | **No** — sequential sweep | |
| **Red-black GS/SOR** | Yes, two phases | Chessboard colouring: red points depend only on black. Same rate, fully parallel |
| CG | Mostly | Mat-vec is local, but **dot products are global reductions** — sync points that do not scale |
| Multigrid | Yes, but | Coarse grids have too little work to fill a GPU |

## Three connections to work already done

- **Red-black ordering is the checkerboard again.** It appeared in von Neumann analysis as the
  *unstable mode*; here the same geometry is the *fix*, breaking the sequential dependency. Same
  structure, opposite role.
- **CG's dot products are reductions**, so floating-point non-associativity applies: a parallel CG
  can take a *different iteration count* run to run, because the convergence test sees slightly
  different residual norms. Not a bug; TSan reports nothing. See `docs/debugging-toolkit.md`.
- **Amplification-factor analysis also analyses solvers.** Substitute a Fourier mode into the Jacobi
  iteration and you get a factor per *iteration* instead of per *timestep*. Modes near `theta = pi`
  damp fast; smooth modes near `theta = 0` barely damp — which is exactly why Jacobi is slow, and
  exactly why multigrid works. One tool, two apparently unrelated questions. See
  `docs/how-to-do-stability-analysis.md`.

## Quick decision table

| Situation | Method |
|---|---|
| Tridiagonal (1-D implicit, ADI sub-solves) | **Thomas** — direct, O(n) |
| Small dense | LU / Cholesky |
| Same matrix, many RHS (fixed-`dt` implicit) | Direct: factor once, reuse |
| Large sparse SPD (2-D/3-D Laplacian) | **Preconditioned CG** — the default |
| Nonsymmetric (advection added) | GMRES / BiCGSTAB |
| Very large elliptic, want optimal | Multigrid, or MG-preconditioned CG |
| Must be embarrassingly parallel, speed secondary | Jacobi |
| Parallel *and* fast, simple to write | **Red-black SOR** |
