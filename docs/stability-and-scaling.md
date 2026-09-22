# Where `rx + ry <= 1/2` comes from, and how cost scales with N

> **Written by Claude (an AI assistant)** as teaching/reference material for this project. Measurements were run on Krit's machine. Not Krit's own writing.

Gate questions 2 and 5. Both are derivations rather than facts, so they live here — you will need
to reproduce them for the report, and possibly at a whiteboard.

## 1. The stability limit, derived

`rx` and `ry` are **not** geometric gaps. They are dimensionless numbers:

    rx = alpha*dt/dx^2        ry = alpha*dt/dy^2

Read `rx` as *"how far heat diffuses in one timestep, measured in grid cells squared."* An explicit
stencil only looks one cell out, so if the physical diffusion in one step outruns the stencil's
reach, the scheme cannot represent what is happening — and it fails loudly rather than gracefully.
That is the intuition. Here is the proof.

### The intuition: the update is a weighted average

Regroup the update — pure algebra, nothing changes:

    T_new[i][j] = (1 - 2*rx - 2*ry) * T[i][j]      <- centre weight
                + rx*T_E + rx*T_W + ry*T_N + ry*T_S

Five weights summing to exactly 1, so the new value is a **blend** of the old centre and its four
neighbours. **While every weight is positive, a blend must land between the smallest and largest of
the values blended** — peaks flatten, valleys fill, nothing new appears. That is what diffusion does.

The limit is the moment the centre weight goes negative:

    1 - 2*rx - 2*ry >= 0     =>     rx + ry <= 1/2

Same condition, no Fourier analysis. The two derivations agreeing is how you know the intuition is
not hand-waving.

**What a negative centre weight means physically.** `rx` is the fraction of a cell's heat flowing to
each neighbour per step; with four neighbours you export `2rx + 2ry`. Exceed 1 and you are exporting
more heat than the cell contains, so it goes into deficit. A 1-D spike `[0, 0, 1, 0, 0]`, one step:

| r | centre | neighbours | result |
|---|---|---|---|
| 0.25 (stable) | `1 + 0.25*(-2) = 0.5` | `0.25` | `[0, 0.25, 0.5, 0.25, 0]` — spreads, all within [0,1] |
| 0.60 (unstable) | `1 + 0.6*(-2) = -0.2` | `0.6` | `[0, 0.6, -0.2, 0.6, 0]` — **centre is -0.2** |

Both conserve total heat. The second **created a temperature colder than anything that existed**;
the hottest point became the coldest. Next step that cell pulls in too hard and overshoots upward,
and every cell does this out of phase with its neighbours — hot/cold/hot/cold, flipping each step,
growing each time. **That is the checkerboard**, and it is why `G` below comes out negative: the
sign flip per step *is* the alternation.

**Do not confuse this with CFL.** The "information outruns the stencil's reach" picture belongs to
*wave* equations, where signals travel at finite speed. Diffusion is mathematically instantaneous —
every point feels every other immediately — so the constraint is not about speed but about **how
much moves per step**. Keep the two separate; they will both appear.

**Why implicit schemes escape:** they solve for all new values simultaneously, so a cell's new value
already accounts for what its neighbours are doing. Nothing over-drains, weights stay positive for
any `dt`, and stability stops constraining the timestep — at the cost of a linear solve per step.
That is the whole reason anyone tolerates the extra complexity (Chapra 30.3-30.5).

### Von Neumann analysis

Take a single Fourier mode on the grid and ask whether it grows or shrinks per step:

    T^n[i][j] = G^n * exp( I*(kx*i*dx + ky*j*dy) )          I = sqrt(-1)

`G` is the **amplification factor**: the amount the mode is multiplied by each timestep. Substitute
into the update equation. Every neighbour contributes a shift, and the pair in each direction
combines through `exp(I*theta) + exp(-I*theta) = 2*cos(theta)`:

    G = 1 + rx*(2*cos(kx*dx) - 2) + ry*(2*cos(ky*dy) - 2)

Using the identity `1 - cos(theta) = 2*sin^2(theta/2)`:

    G = 1 - 4*rx*sin^2(kx*dx/2) - 4*ry*sin^2(ky*dy/2)

**Stability means no mode grows:** `|G| <= 1` for *every* wavenumber.

- `G <= 1` is automatic — `rx, ry > 0`, so the subtracted terms only reduce `G`.
- `G >= -1` is the binding constraint. Both `sin^2` terms are worst at their maximum of 1, which is
  the **highest** representable wavenumber — a checkerboard alternating every cell:

      G_worst = 1 - 4*rx - 4*ry  >=  -1
      =>  4*(rx + ry) <= 2
      =>  rx + ry <= 1/2                                    <-- the condition

With `dx = dy` this gives `dt <= dx^2/(4*alpha)`, which is exactly the formula in the solver —
`dt = safety * dx*dx / (4*alpha)` with `safety = 0.8`.

### What violating it actually does — and why you must assert it

`G_worst < -1` means the checkerboard mode is multiplied by a number of magnitude > 1 **and flips
sign every step**. Nothing raises an error; the array just fills with growing oscillation.

The dangerous part is that **it does not fail immediately.** Measured on a 32x32 grid with the
smooth initial condition `sin(pi x) sin(pi y)`, at `rx+ry = 0.55` (10% over the limit):

| Steps | Centre value | Looks like |
|---|---|---|
| 60 | 0.727 | fine |
| 150 | 0.451 | **still fine — plausibly decaying** |
| 250 | 180.05 | obviously broken |
| 400 | 6.89e13 | exploded |

The stable run at `rx+ry = 0.40` decays smoothly over the same span: 0.793 → 0.381 → 0.214.

Why the delay? The initial condition is the *smoothest* mode, so it contains almost no energy at
the checkerboard wavenumber that is actually unstable. The only seed is round-off, ~1e-16. With
`|G| = 4*(0.55) - 1 = 1.2`, that seed reaches O(1) after

    n = 16 / log10(1.2) = 202 steps

and the measurement puts the failure between 150 and 250. **The theory predicts the onset.**

The practical lesson: a short test run of an unstable scheme can look completely correct. This is
why `dt` is *derived* from the stability condition and the condition is *asserted* at startup —
never hardcoded and eyeballed.

## 2. How cost scales with N

Two different questions, and the answer depends on **what you hold fixed**.

### Holding the step count fixed

Work is proportional to the number of grid points, so N -> 2N is **4x the work**. Measured serial
runtimes, from `result-summary-by-resolution.csv`:

| Transition | Work | Time | Working set (2 arrays) |
|---|---|---|---|
| 250 -> 500 | x4.00 | **x4.06** | 1.0 MB -> 3.8 MB — both fit 6 MiB L3 |
| 500 -> 1000 | x4.00 | **x5.02** | 3.8 MB -> 15.3 MB — **crosses out of L3** |
| 1000 -> 2000 | x4.00 | **x4.00** | 15.3 MB -> 61.1 MB — both in DRAM |

So it *is* ~4x whenever both sizes sit on the same side of the cache boundary, and it is **5.02x
across the boundary** — the extra 25% is the L3-to-DRAM transition, not arithmetic.

**Open prediction, worth testing in Week 1 Step 3:** those numbers were built `-O2` with no
`-march=native`, i.e. SSE2, 2 doubles per vector. If the code is currently instruction-limited
rather than bandwidth-limited, it cannot exploit L3's higher bandwidth — which would mean the
cache cliff is being *masked*. Rebuilding `-O3 -march=native` should therefore make the 500->1000
excess **larger**, not smaller. If it does, that is strong evidence the baseline was never
bandwidth-bound at N=500 in the first place.

**Tested 2026-09-22 on an idle machine — confirmed.** Per-step time, same session, both builds:

| Transition | work | C, `-O2` (original) | C++, `-O3 -march=native` |
|---|---|---|---|
| 250 -> 500 | 4.02x | 4.20x | 5.27x |
| 500 -> 1000 | 4.01x | 4.92x | **9.59x** |
| 1000 -> 2000 | 4.00x | 4.44x | 5.10x |

The optimised build shows a cliff about twice as large. So the old guide's "11x cache cliff" was probably
right *for an optimised build*; it was never comparable with Krit's `-O2` 5.02x, and an earlier "correction"
that called it unreproduced was itself wrong. Also: at N=1000 the optimised kernel moves 16.3 GB/s, *above* the
~14.5 GB/s memory ceiling, so part of that grid is still served from L3. **Only N >= 2000 is fully
memory-bound** in the optimised build.

### Holding the physical end time fixed

This is where it stops being 4x. Stability ties `dt` to `dx`:

    dt <= dx^2/(4*alpha)     and     dx = L/N     =>     dt ~ 1/N^2

Halving `dx` therefore **quarters** `dt`, so reaching the same physical time `t_end` needs **4x more
steps**. Combined with 4x the work per step:

    total cost ~ N^2 (points) * N^2 (steps) = N^4

**N -> 2N is 16x the work, not 4x.** That factor is the single strongest argument for implicit
schemes (Chapra 30.3-30.5): they are unconditionally stable, so `dt` is chosen for accuracy rather
than dictated by `dx`, at the price of solving a linear system every step.

### The trap in the question

"At the same step count" fixes the steps, so ~4x is right — and the data confirms it to 0.1% between
1000 and 2000. The interesting answer is that **the question is underdetermined until you say what
is held fixed**, and the two readings differ by a factor of four. Always ask which one is meant.
