# Books for becoming a senior parallel-programming engineer

> **Drafted by Claude (an AI assistant)** at Krit's request, 2026-09-25, after he judged *Using OpenMP*
> (2007) too dated to invest attention in. Editions and dates were checked online on that date; sources
> are listed at the end. This list targets the **career**, not this project — project relevance is noted
> where it exists, but it is not the ranking criterion.

## The short answer

If you buy only three books, buy these:

1. **Hager & Wellein, *Introduction to High Performance Computing for Scientists and Engineers*, 2nd ed.
   (CRC, 2026)** — the performance-model book. Roofline, STREAM, memory-bound versus compute-bound,
   ccNUMA, and how to reason about a machine's ceilings *before* measuring. It is the closest thing in
   print to what this project has been doing by hand. The 1st edition (2010) is still excellent and
   cheap if the 2nd is hard to get; its authors' blog (blogs.fau.de/hager) is the ongoing version.
2. **Bryant & O'Hallaron, *Computer Systems: A Programmer's Perspective*, 3rd ed. (2015)** — the bridge
   between C and hardware: memory hierarchy, linking, exceptions, virtual memory, optimisation. The 4th
   edition is expected Spring 2027, so the 3rd is the current one.
3. **Hwu, Kirk & El Hajj, *Programming Massively Parallel Processors*, 5th ed. (Morgan Kaufmann,
   Feb 2026)** — the standard GPU book, CUDA-based but the architecture and patterns transfer to OpenMP
   offload directly.

Everything else below is by domain.

---

## OpenMP — what replaces the 2007 book

The complaint is correct and the fix exists: the same publisher issued a **sequel** covering exactly what
the 2007 volume lacks.

| Book | Year | What it is |
|---|---|---|
| **van der Pas, Stotzer & Terboven, *Using OpenMP — The Next Step*** (MIT Press) | 2017 | The direct sequel: **affinity, accelerators (`target`), tasking, SIMD**. Same lineage, same author. This is the book to read after the 2007 one — or instead of it. |
| **Deakin & Mattson, *Programming Your GPU with OpenMP*** (MIT Press) | 2023 | The most current book for **versions 3 and 4 of this project**. Builds a "GPU common core" of directives and covers performance portability. Written by the people who wrote the spec. |
| **Mattson, He & Koniges, *The OpenMP Common Core*** (MIT Press) | 2019 | Argues most programmers need ~20 constructs, and teaches those properly. Good if the language reference feels bloated. |
| **Klemm & Cownie, *High Performance Parallel Runtimes*** (De Gruyter) | 2021 | How an OpenMP runtime is *implemented*. Directly relevant to the finding in this repo that GCC's nvptx runtime caps threads per team at 20 — that is a runtime design decision, and this book is about those decisions. |
| **The OpenMP specification + the official Examples document** (openmp.org) | current | Free. For `target`, `teams`, `distribute` and the map clauses, the spec's examples are better than any book. |

**Verdict on the 2007 *Using OpenMP*:** keep it for Ch 5 (sequential performance and measurement) and
Ch 7–8 (races, and what the compiler generates). Those chapters have not aged. Skip the rest and use
*The Next Step* plus the spec.

## MPI — the other half of HPC employment

| Book | Year | Notes |
|---|---|---|
| **Gropp, Lusk & Skjellum, *Using MPI*, 3rd ed.** (MIT Press) | 2014 | Still the standard tutorial, updated for MPI-3. |
| **Gropp, Hoefler & Thakur, *Using Advanced MPI*** (MIT Press) | 2014 | One-sided communication, hybrid MPI+OpenMP, large data. Read after the first. |

MPI moves slowly, so 2014 is not a problem the way 2007 is for OpenMP. Learn MPI even if this project
does not use it: most HPC job adverts assume it, and hybrid MPI+OpenMP is the normal production shape.

## GPU

| Book | Year | Notes |
|---|---|---|
| **Hwu, Kirk & El Hajj, *Programming Massively Parallel Processors*, 5th ed.** | 2026 | The canonical text. CUDA, but teaches the *machine model* — occupancy, coalescing, divergence, shared memory — which is what actually transfers. |
| **Reinders et al., *Data Parallel C++*, 2nd ed.** (Apress) | 2023 | SYCL. **Free, open access.** Worth reading as the vendor-neutral alternative to CUDA. |
| NVIDIA CUDA C++ Programming Guide and Best Practices Guide | current | Free, and more current than any book for API detail. |

## CPU architecture

| Book | Year | Notes |
|---|---|---|
| **Hennessy & Patterson, *Computer Architecture: A Quantitative Approach*, 7th ed.** (Morgan Kaufmann, with Kozyrakis) | Oct 2025 | The graduate-level standard; this edition adds GPU/CPU developments, domain-specific architectures and SoC coverage. Read chapters, not the whole thing: memory hierarchy, ILP, data-level parallelism (SIMD/GPU), warehouse-scale. |
| **Patterson & Hennessy, *Computer Organization and Design* (RISC-V ed.)** | 2020 | The undergraduate prerequisite. Skip if pipelines and caches are already comfortable. |
| **Agner Fog's optimization manuals** (agner.org) | continuously updated | Free. Instruction tables, microarchitecture details per CPU generation. The reference professionals actually keep open. |

## Performance engineering — the skill this project is really teaching

| Book | Year | Notes |
|---|---|---|
| **Bakhvalov, *Performance Analysis and Tuning on Modern CPUs*, 2nd ed.** | 2024 | **Free PDF** (github.com/dendibakh/perf-book). Top-Down Microarchitecture Analysis, `perf`, hardware counters, real case studies. The most practical book on this list. |
| **Gregg, *Systems Performance*, 2nd ed.** | 2020 | Whole-system: CPU, memory, I/O, scheduler, methodology (USE method). More ops-flavoured, still valuable. |
| **Drepper, *What Every Programmer Should Know About Memory*** | 2007 | Free paper, ~100 pages. Dated on specific numbers, unmatched on the mental model. Its DRAM and cache-coherence sections explain most of what we measured here. |

## Operating systems

| Book | Year | Notes |
|---|---|---|
| **Arpaci-Dusseau, *Operating Systems: Three Easy Pieces*** | current | **Free.** Virtualization, concurrency, persistence. The best first OS book, and short. |
| **Kerrisk, *The Linux Programming Interface*** | 2010 | The hands-on counterpart: syscalls, processes, threads, memory, signals. Ageing in places, still the definitive Linux systems-programming reference. |
| **Tanenbaum, *Modern Operating Systems*** | 2014 | Broad academic reference. Optional if OSTEP is done. |

Pick one theory book (OSTEP) and one hands-on book (Kerrisk). Reading all three is not a good use of time.

## Concurrency theory and C++

| Book | Year | Notes |
|---|---|---|
| **Williams, *C++ Concurrency in Action*, 2nd ed.** (Manning) | 2019 | Covers C++17 threading properly: memory model, atomics, lock-free design. No 3rd edition exists yet. Relevant because this project is now C++. |
| **Herlihy, Shavit, Luchangco & Spear, *The Art of Multiprocessor Programming*, 2nd ed.** | 2020 | The theory: linearizability, lock-free algorithms, consensus. Harder, and the thing that separates "can use OpenMP" from "understands concurrency". |
| **McKenney, *Is Parallel Programming Hard, And, If So, What Can You Do About It?*** | continuously updated | **Free** (perfbook). RCU, memory barriers, real kernel-grade parallelism. |

## Broad single-volume introductions

Useful if a single book covering OpenMP + MPI + GPU is wanted:

- **Robey & Zamora, *Parallel and High Performance Computing*** (Manning, 2021) — practical, vectorisation
  through MPI and GPUs, written by Los Alamos practitioners.
- **Pacheco & Malensek, *An Introduction to Parallel Programming*, 2nd ed.** (2021) — textbook covering
  MPI, OpenMP, Pthreads and CUDA.
- **McCool, Reinders & Robison, *Structured Parallel Programming*** (2012) — patterns (map, reduce, scan,
  stencil). Dated on APIs, durable on the patterns; several university HPC courses still use it.

---

## A twelve-month sequence

The order matters more than the list. Roughly two books at a time, one theoretical and one immediately
applicable.

1. **Months 1–3 (during this placement):** *Using OpenMP — The Next Step* and *Programming Your GPU with
   OpenMP*, alongside Bakhvalov (free). These pay off inside the project.
2. **Months 3–6:** Hager & Wellein, plus the memory-hierarchy and optimisation chapters of CSAPP. This is
   where "I measured 87% of bandwidth" becomes a habit rather than an incident.
3. **Months 6–9:** *Programming Massively Parallel Processors*, plus OSTEP for the systems background.
4. **Months 9–12:** *Using MPI*, then Hennessy & Patterson chapters as reference, then *C++ Concurrency in
   Action* or *The Art of Multiprocessor Programming* depending on whether the target is HPC or systems
   engineering more broadly.

**Caveat worth keeping:** every book on this list will be behind the hardware. The habit this project is
building — predict, measure on an idle machine, state how the bytes were counted, explain the gap — is the
part that does not go out of date.

## Sources checked, 2026-09-25

- OpenMP's own book list: <https://www.openmp.org/resources/openmp-books/>
- *Programming Your GPU with OpenMP* (MIT Press, 2023): <https://mitpress.mit.edu/9780262547536/>
- *Programming Massively Parallel Processors*, 5th ed.: <https://shop.elsevier.com/books/programming-massively-parallel-processors/hwu/978-0-443-43900-1>
- *Computer Architecture: A Quantitative Approach*, 7th ed.: <https://shop.elsevier.com/books/computer-architecture/hennessy/978-0-443-15406-5>
- CS:APP 4th edition status: <http://csappbook.blogspot.com/2026/06/the-4th-edition-is-in-works.html>
- Hager & Wellein, 2nd ed.: <https://www.routledge.com/Introduction-to-High-Performance-Computing-for-Scientists-and-Engineers/Hager-Wellein/p/book/9781482252934>
- Bakhvalov, free PDF: <https://github.com/dendibakh/perf-book>
- *Using MPI*, 3rd ed.: <https://mitpress.mit.edu/9780262527392/using-mpi/>
- *Data Parallel C++*, 2nd ed. (open access): <https://link.springer.com/book/10.1007/978-1-4842-9691-2>
- *C++ Concurrency in Action*, 2nd ed.: <https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition>
