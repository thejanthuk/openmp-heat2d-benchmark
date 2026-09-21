# How to prove stability for *any* scheme

> **Written by Claude (an AI assistant)** as teaching/reference material for this project. Measurements were run on Krit's machine. Not Krit's own writing.

A procedure, not a result. Von Neumann analysis is mechanical once you know the substitution table;
the only real skill is knowing when it applies and what it does not tell you.

## When this method is valid

Von Neumann assumes **a linear PDE with constant coefficients on a uniform grid, with periodic (or
effectively infinite) boundaries.** It ignores boundary conditions entirely, so it gives a
**necessary** condition that is not always sufficient once real boundaries are attached.

- **Variable coefficients / nonlinear:** freeze the coefficients locally and apply it pointwise.
  That is standard practice and usually right, but it is an approximation.
- **Boundaries matter:** switch to the matrix method below.

## The five steps

1. **Write the scheme with every time level explicit.** Get `T^{n+1}` on the left, everything else
   on the right. Implicit schemes keep `T^{n+1}` terms on both sides — that is fine.
2. **Substitute a single Fourier mode.** In 2-D:

       T^n[j][l] = G^n * exp( I*(theta_x*j + theta_y*l) )       theta = k*dx,  theta in [-pi, pi]

   The only mechanical fact you need: **shifting by one cell multiplies by `exp(I*theta)`.**
3. **Divide through by `G^n * exp(I*(...))`.** Everything collapses to algebra in `G` and `theta`.
4. **Solve for `G(theta)`** — the amplification factor, the amount a mode is multiplied by per step.
5. **Require `|G| <= 1` for every `theta`.** Find the worst `theta`, read off the condition.

## The substitution table — memorise this, the rest is algebra

| Stencil term | Becomes | Range |
|---|---|---|
| `T[j+1] - 2T[j] + T[j-1]` (2nd difference) | `-4*sin^2(theta/2)` | **[-4, 0]** |
| `(T[j+1] - T[j-1]) / 2` (centred 1st) | `I*sin(theta)` | pure **imaginary**, [-i, i] |
| `T[j] - T[j-1]` (upwind 1st) | `1 - exp(-I*theta)` | |
| `T[j+1] + T[j-1]` | `2*cos(theta)` | [-2, 2] |

Two identities do all the work: `exp(I*t) + exp(-I*t) = 2*cos(t)`, and `1 - cos(t) = 2*sin^2(t/2)`.

**The second difference becoming `-4*sin^2(theta/2)` is the single most useful fact here.** It is
always real and never positive, and it is maximised in magnitude at `theta = pi` — the checkerboard.

## The same recipe, four schemes, three different verdicts

Let `r = alpha*dt/dx^2` and `C = a*dt/dx`. Write `s = sin^2(theta/2)`, which ranges over `[0, 1]`.

| Scheme | `G(theta)` | Verdict |
|---|---|---|
| **FTCS diffusion** (yours) | `1 - 4*r*s` | `G >= -1` needs **`r <= 1/2`** — conditionally stable |
| **FTCS advection** | `1 - I*C*sin(theta)`, so `\|G\|^2 = 1 + C^2*sin^2(theta)` | `> 1` for every `theta != 0` — **unconditionally UNSTABLE at any dt** |
| **Upwind advection** | `1 - C*(1 - exp(-I*theta))`; at `theta=pi`, `\|G\| = \|1-2C\|` | **`0 <= C <= 1`** — this is the **CFL** condition |
| **BTCS (implicit)** | `1 / (1 + 4*r*s)` | denominator `>= 1` always → **unconditionally stable** |
| **Crank-Nicolson** | `(1 - 2*r*s) / (1 + 2*r*s)` | `\|G\| <= 1` always → **unconditionally stable** (can go negative → bounded ringing) |

Three lessons in one table:

- **FTCS advection is consistent and still useless.** It approximates the PDE correctly as
  `dx, dt -> 0`, and it blows up at every timestep. *Consistency does not imply usability* —
  that is why stability is a separate question.
- **CFL genuinely is about speed**, and it lives with advection, not diffusion. Keep the two apart.
- **Implicit schemes buy unconditional stability**, which is the proof of the claim that they free
  `dt` from `dx`. The price is a linear solve per step.

## Three shortcuts worth knowing

**1. Positive coefficients (explicit schemes only).** If the update is a weighted sum of old values
and every weight is `>= 0` while summing to 1, the scheme is stable in the max norm — a blend cannot
leave the range of what it blended. For FTCS diffusion, `1 - 2rx - 2ry >= 0` gives `rx + ry <= 1/2`
in one line. **Caveat:** sufficient, not necessary — some stable schemes have negative coefficients —
and it says nothing about implicit schemes.

**2. Guess `theta = pi`.** For diffusion-type schemes the worst mode is nearly always the
checkerboard, so setting `s = 1` immediately gives the condition. **Verify it is actually the worst**
— for advection-diffusion the maximum can sit elsewhere.

**3. Method of lines — the one that generalises best.** Discretise space only:

       dT/dt = L*T

Then stability is: **`dt * lambda` must lie inside your time integrator's stability region, for
every eigenvalue `lambda` of `L`.** This decouples "which spatial operator" from "which time
stepper", so a new combination costs no new analysis.

For the 1-D Laplacian, `lambda(theta) = -(4*alpha/dx^2) * s`, so `lambda` spans `[-4*alpha/dx^2, 0]`.

- **Forward Euler** is stable for `|1 + dt*lambda| <= 1`, i.e. `dt*|lambda| <= 2`, giving
  `dt <= dx^2/(2*alpha)` — that is `r <= 1/2`. **Same answer, different route.**
- **RK4** reaches about `-2.785` along the negative real axis, so `r <= 2.785/4 = 0.696`.
  A ~39% larger timestep, obtained without redoing any Fourier algebra.

This is the framing to default to once the schemes get interesting — and it is exactly the ODE
stability/stiffness material in Chapra Ch 25-26, which is why those chapters are on the list.

## What stability analysis does NOT tell you

- **Nothing about accuracy.** BTCS is unconditionally stable and only first-order in time. Stable
  and wrong is a real place to end up.
- **Nothing about boundary-induced instability.** Von Neumann assumes periodicity. If a scheme is
  von-Neumann-stable but still blows up, suspect the boundaries and use the matrix method: write
  `T^{n+1} = A*T^n` with the real boundary rows included, and require spectral radius `rho(A) <= 1`.
- **Nothing about nonlinear instability** (aliasing, energy cascade to the grid scale).

**Lax equivalence theorem** ties it together: for a well-posed linear initial-value problem,
**consistency + stability <=> convergence.** That is why stability is worth proving — it is one of
exactly two things needed for the solution to converge, and the convergence study measures the other.

## Always cross-check numerically

The algebra is easy to get wrong. The empirical check takes two minutes: sweep `dt` and find where
growth begins.

**Seed the checkerboard, not a smooth profile.** An unstable run at `rx+ry = 0.55` started from
`sin(pi x) sin(pi y)` still looked *plausibly decaying at 150 steps* and only exploded past 250 — the
smooth initial condition holds almost no energy at the unstable wavenumber, so the instability had to
grow out of round-off. Initialise with `T[i][j] = (-1)^(i+j)` instead and the worst mode is fully
excited from step zero.

**This also makes the analysis directly falsifiable.** At `theta = pi` the theory says
`G = 1 - 4*(rx+ry)` exactly, so the centre value after `n` steps should be `G^n`. Measured on a
32x32 grid:

| `rx+ry` | `G = 1 - 4*(rx+ry)` | steps | predicted `G^n` | **measured** |
|---|---|---|---|---|
| 0.55 | **-1.2** | 5 | -2.48832 | **-2.48832** |
| 0.55 | -1.2 | 10 | +6.19174 | **+6.19174** |
| 0.55 | -1.2 | 20 | +38.3376 | **+38.3376** |
| 0.40 | **-0.6** | 5 | -0.07776 | **-0.07776** |
| 0.40 | -0.6 | 20 | +3.65616e-5 | **+3.65611e-5** |

Six significant figures, **sign included** — the negative `G` is why the values alternate. (The
small drift by 40 steps is real: the checkerboard is not an exact eigenmode of a *finite* grid with
Dirichlet boundaries, only of the infinite/periodic one von Neumann assumes. That deviation is the
method's own stated limitation showing up in the data.)

So the cross-check is not merely "did it blow up" — it is a quantitative test of the amplification
factor you derived. If `G^n` does not match, the algebra is wrong, and you will know within minutes.
