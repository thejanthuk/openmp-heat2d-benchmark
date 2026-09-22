# Debugging toolkit — which tool for which symptom

> **Written by Claude (an AI assistant)** as teaching/reference material for this project. Measurements were run on Krit's machine. Not Krit's own writing.

Reference, not a tutorial. The hard part is not learning `gdb` commands; it is knowing **which tool
answers the question you actually have**, and recognising the bugs that none of them will find.

Every number below was measured on the test machine on 2026-09-15, not quoted from documentation.

## Start here: pick the tool from the symptom

| Symptom | Reach for | Why not the others |
|---|---|---|
| Segfault, immediate crash | **gdb** — `gdb ./solver` then `run`, then `bt` | 10 seconds to a backtrace. Don't add printfs. |
| Wrong answer, but **reproducible and serial** | **assert + print**, then gdb if state is complex | Sanitizers find *undefined behaviour*, not wrong physics. A stable wrong answer is usually a wrong formula. |
| Wrong answer **only with >1 thread** | **Differential test vs serial** at 1/2/4/8 threads | Likely a race. TSan is unusable for OpenMP here (see below); gdb perturbs timing. |
| Answer changes **run to run** | First ask: `schedule(dynamic)`? Then it may be **FP non-associativity, not a bug** — see below. Otherwise TSan. | TSan reports *nothing* for FP non-determinism, because there is no race. |
| Crash far from the cause; heap corruption; `free(): invalid pointer` | **ASan** | gdb shows you where it *manifested*, ASan shows where it *happened*. |
| Reads uninitialised memory; suspicious index arithmetic | **ASan + UBSan** together (5x, cheap) | Leave these on in every debug build. |
| Signed overflow, misaligned load, bad shift | **UBSan** | Nearly free. No reason not to. |
| "It worked last week" | **`git bisect`** | Faster than any debugger. This is why you commit small. |
| Slow, not wrong | **`perf`**, not a debugger | Different question entirely. |
| Wrong **only on the GPU** | **Differential debugging** vs the serial version | No sanitizer and no debugger reaches OpenMP offload code here. See below. |

**Division of labour:** TSan finds races · rr debugs logic · perf measures speed · ASan finds
memory errors · bisect finds regressions. Using one for another's job is the commonest waste.

**The meta-rule:** a debugger answers *"what is the state right now?"* A sanitizer answers *"did I
break a rule?"* Bisection answers *"what changed?"* Print answers *"what is the shape of this?"*
Pick by question, not by habit.

## Sanitizers: measured cost on this machine

| Build | Overhead | Verdict |
|---|---|---|
| `-fsanitize=address,undefined` | **~5x** | Cheap. Default for every debug build. |
| `-fsanitize=thread` | **~71x** at N=512, plus ~1 s fixed startup | Tiny grids and few steps only. N=64, 3 steps is plenty. |

**ASan and TSan cannot be combined** — the compiler refuses:
`error: '-fsanitize=thread' is incompatible with '-fsanitize=address'`. Two separate builds.

Because TSan is ~71x, **never run it on a benchmark-sized grid.** A race at N=64 is the same race
at N=2048; the detector needs the access pattern, not the work.

### Why "ASan finds it, gdb finds the symptom" is not a slogan

Demonstrated 2026-09-16 on one program that writes one element past a `malloc`ed grid, starting at
`i=64`:

| Build | What happened |
|---|---|
| plain `gcc -Og` | printed `survived` — **no crash at all**; the write landed in heap slack |
| `-fsanitize=address` | caught it **at `i=64`**, the first offending write, and named the allocation site |
| `gdb` on the plain build | crashed only at **`i=323`** — 259 iterations after the actual bug |

The plain build is the dangerous one: a real out-of-bounds write that silently succeeds, corrupting
whatever lives in the slack. This is precisely why `-fsanitize=address,undefined` belongs in every
debug build rather than being something you reach for after a crash.

## TSan on OpenMP: it does NOT work on this machine  (corrected 2026-09-22)

**Earlier versions of this file said "use GCC for TSan on OpenMP — it is clean on correct code". That
was wrong.** See `docs/postmortems/2026-09-22-tsan-openmp-false-positives.md`.

Both OpenMP runtimes here synchronise their thread pools with mechanisms TSan cannot see, so TSan
reports **every** cross-thread hand-off as a race — including on trivially correct code:

| Correct program | GCC TSan | Clang TSan |
|---|---|---|
| one `parallel for` writing an array, then a *serial* read | 1 report | 2 reports |
| two back-to-back `parallel for` regions | 3 reports | 4 reports |
| Krit's double-buffered solver (bit-identical to serial) | **20 reports** | — |

The false positives **name your own source lines**, so "does any frame name a file you wrote?" cannot
tell them apart from real races.

**What to use instead:** differential testing — compare every grid point against the serial solver at
1, 2, 4 and 8 threads and across repeated runs. For a deterministic kernel, a real race almost never
stays bit-identical under that. **To get working TSan for OpenMP:** install LLVM's OpenMP runtime with
**Archer** and use Clang (not installed; a Gentoo rebuild).

The in-place-update "demo" that used to be here ran 3 timesteps (3 regions) and would have been flagged
even without the bug, so it demonstrated nothing and has been removed.

## The non-bug that looks like a bug

**A parallel reduction giving a different answer run-to-run is usually not a race.** Floating-point
addition is not associative, so changing the order changes the last bits. Measured here, summing
4,000,000 values:

| Configuration | Result |
|---|---|
| `schedule(static)`, 4 threads, 5 runs | `1.2821090870975775` — **bit-identical every run** |
| `schedule(dynamic)`, 4 threads, 5 runs | 5 **different** values, spread ~2.4e-13 |
| `schedule(static)`, 1 / 2 / 4 / 8 threads | 4 **different** values |

TSan reports nothing for any of these, correctly — there is no race. The lesson:

- **Static scheduling with a fixed thread count is reproducible.** Use it when you need bit-identical
  results for regression testing.
- **Dynamic scheduling is not**, because the partition differs per run.
- **Changing the thread count changes the answer even with static scheduling.** So "my 8-thread run
  doesn't match my 4-thread run in the last 3 digits" is expected, not a bug.

This bites directly here: the L2 error is a reduction. When comparing versions, **compare to a
tolerance, never with `==`** — and pick the tolerance from the discretisation error (~1e-5 here),
not from round-off.

## gdb — and when it is the wrong tool

**Installed and verified 2026-09-16: GDB 17.2.** Sees OpenMP thread teams correctly (`info threads` showed main + 3 `libgomp` workers at `OMP_NUM_THREADS=4`).

**gdb wins** for crashes, for inspecting rich state at a known point, and for reading unfamiliar
code as it runs. **gdb loses** for nondeterministic bugs (attaching perturbs timing), for
"which of 10,000 timesteps went wrong" (use an assert in the loop), and for anything on the GPU.

The commands that cover ~90% of real use:

```
gdb ./solver
  run 0 500 1024          # args after `run`
  bt                      # backtrace — the single most valuable command
  frame 2 / up / down     # move up the stack to your code
  print t[i*m+j]          # evaluate expressions, including arithmetic
  print *t@10             # print 10 elements of an array — essential for grids
  watch t[513]            # break when a value changes: finds silent corruption
  break heat.c:42 if i==500   # conditional break: the timestep-loop workhorse
  finish                  # run until the current function returns
```

Run it on a **debug build** — `-g -O0`, or `-g -Og` if `-O0` is too slow. At `-O3` variables are
optimised into registers and `print` will tell you `<optimized out>`.

**For a crash you cannot reproduce interactively**, enable core dumps
(`ulimit -c unlimited`) and open the corpse: `gdb ./solver core`.

### rr — reverse debugging

**Installed and verified 2026-09-16: rr 5.9.0.** Records an execution once, then replays it
deterministically as many times as you like — including *backwards*. Verified here on a real
SIGSEGV: `reverse-continue` walked back up the loop, hitting the breakpoint at `i=64`, then `i=63`.

```sh
rr record ./solver 0 500 1024     # record once
rr replay                         # gdb prompt, but time runs both ways
  continue / reverse-continue
  step     / reverse-step
  watch t[513]                    # then reverse-continue = "who corrupted this?"
```

**The killer use:** a bug that takes 40 minutes to reproduce. Record once, then investigate for an
hour without ever re-running it. Also: `reverse-continue` on a watchpoint answers "what wrote this
value?" directly, instead of by bisecting printfs.

**When NOT to use rr — it serialises threads onto one core to get determinism.** Measured here: the
4-thread reduction ran **9.4x slower** under `rr record`. That has two consequences that matter:

- **rr cannot find data races.** It removes the concurrency that causes them. Use TSan.
- **rr cannot time parallel code.** Never take a performance number from a recording.

So the division is clean: **TSan finds races, rr debugs logic, perf measures speed.**

## What none of these reach: the GPU

There is **no sanitizer and no practical debugger for OpenMP offload code** on this setup. ASan,
TSan and gdb all stop at the host boundary. So GPU correctness is established differently:

1. **Differential testing.** The serial version is the oracle. Compare fields elementwise and
   report max absolute difference — it should be at round-off (~1e-16 relative), not merely "close".
2. **Assert the kernel actually ran on the device** — `omp_is_initial_device() == 0` inside the
   region, plus `OMP_TARGET_OFFLOAD=MANDATORY`. A silently host-executed kernel passes every
   correctness check and every benchmark, and both are then meaningless.
3. **Shrink until it is hand-checkable.** N=8, one timestep, printed in full.
4. `GOMP_DEBUG=1` to see what libgomp actually launched and transferred.

## perf permissions — no kernel rebuild needed

`kernel.perf_event_paranoid = 2` on this machine. Verified 2026-09-16, that is **sufficient for
everything this project needs**:

| Capability | At paranoid=2 | Needed here? |
|---|---|---|
| `perf stat` user-space HW counters (`cycles:u`, `L1-dcache-load-misses:u`, `LLC-load-misses:u`, `dTLB-load-misses:u`) | **works** | **yes** — these are the Weeks 6–8 counters |
| `perf record` / `perf report` on a process | **works** | yes |
| `gdb`, and `rr` record/replay/reverse | **works** (rr needs only per-process user-space counters, which paranoid=2 permits; it is not setuid and holds no capabilities) | yes |
| kernel-space events (`cycles:k`) | blocked | no |
| system-wide profiling (`perf record -a`) | blocked | no |

So **no kernel option needs changing.** If system-wide profiling is ever wanted it is one sysctl,
not a rebuild: `sudo sysctl kernel.perf_event_paranoid=1`. Note `perf` silently appends `:u` to
events — that is the restriction being applied, and for profiling a compute kernel it is what you
want anyway.

Installed and verified: GDB 17.2, rr 5.9.0, perf 7.2, Valgrind 3.27.1. ParaView not yet present.

## Build presets to keep around

```sh
# debug — default for development
gcc -g -Og -fopenmp -fsanitize=address,undefined  -Wall -Wextra ...

# race hunt — tiny grids only (~71x)
gcc -g -O1 -fopenmp -fsanitize=thread             -Wall -Wextra ...

# benchmarking — the ONLY build that produces reportable numbers
gcc -O3 -march=native -fopenmp ...
```

Never benchmark a sanitizer build, and never trust a correctness result that was only ever run at
`-O3` — optimisation exposes undefined behaviour that `-O0` hides.

## When to reach for this file

- Before the first CPU OpenMP run (Week 3): build once with TSan, at N=64.
- Whenever a result changes with thread count: check the FP-reduction table above *before*
  suspecting a race.
- Whenever a bug survives 30 minutes of staring: you have picked the wrong tool. Come back to the
  first table.
