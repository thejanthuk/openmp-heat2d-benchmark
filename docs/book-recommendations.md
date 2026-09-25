# Books and sources for a senior parallel-performance engineer

> **Drafted by Claude (an AI assistant)** at Krit's request. **Revision 2, 2026-09-25** — the first
> revision was checked against the standard "is this what a senior practitioner reads?" and failed in
> four places (see *What changed* below). Editions and dates verified online on 2026-09-25; sources at
> the end. Targets the career across CPU, GPU, memory, OpenMP, MPI and AI accelerators (TPU-class),
> not this project.

## What changed in revision 2

The first list was honest but junior-shaped:

1. **It was mostly course textbooks.** CSAPP, OSTEP and *Programming Massively Parallel Processors* are
   what a good undergraduate degree covers. They are prerequisites, not the senior shelf.
2. **No accelerator or TPU track at all** — the thing explicitly asked for, and where the field is going.
3. **No measurement methodology**, even though this project's own worst failures were measurement
   failures, not knowledge failures.
4. **No memory consistency or coherence**, which is the actual hard part of shared-memory correctness
   once a problem stops being a `parallel for`.

A fifth omission, less serious: no performance-portability layer (Kokkos, SYCL, `std::execution`), which
is what production HPC codebases increasingly use instead of raw OpenMP or CUDA.

## How senior people actually use this material

Three different modes, and mixing them up wastes months:

- **Read through once, to build a mental model.** Perhaps six books in a career. Marked **[model]**.
- **Keep on the shelf and look things up.** Never read cover to cover. Marked **[reference]**.
- **Papers and specifications.** Beyond a certain level the canon is not books — it is 15-page papers
  and the standard documents. This is the main thing separating senior from mid-level: when a book
  disagrees with the spec, or the hardware, the book is wrong.

---

## Tier 0 — prerequisites (skip any you already have)

Not senior material; just the floor. Skip on sight if the topic is comfortable.

- [X] Bryant & O'Hallaron, **Computer Systems: A Programmer's Perspective**, 3rd ed. (2015). The C-to-hardware
  bridge. 4th ed. expected Spring 2027. **[model]**
- [X] Arpaci-Dusseau, **Operating Systems: Three Easy Pieces** (free). Virtualization, concurrency,
  persistence. **[model]**
- [X] Patterson & Hennessy, **Computer Organization and Design** (RISC-V ed., 2020). Skip if pipelines and
  caches are already comfortable. **[reference]**

---

## Tier 1 — the senior core

### Performance modelling and measurement — the discipline this project has been teaching by accident

- [X] **Hager & Wellein, *Introduction to High Performance Computing for Scientists and Engineers*, 2nd ed.**
  (CRC, 2026; 1st ed. 2010 still excellent). Roofline, STREAM, balance metrics, ccNUMA, and the
  **ECM model** — analytic prediction of a kernel's runtime *before* running it. The single most
  aligned book with what this repo does by hand. **[model]**
- [X] **Bakhvalov, *Performance Analysis and Tuning on Modern CPUs*, 2nd ed.** (2024, **free PDF**).
  Top-Down Microarchitecture Analysis, `perf`, counters, case studies. **[model]**
- **Jain, *The Art of Computer Systems Performance Analysis*** (1991) or **Lilja, *Measuring Computer
  Performance*** (2000). Experiment design, confounding variables, when a difference is real. Old, and
  the only books that address the failure mode this project hit repeatedly: **measuring on a machine
  whose state was not controlled, then believing the number**. **[model, once]**

### Memory — the binding constraint in nearly every parallel program

- **Nagarajan, Sorin, Hill & Wood, *A Primer on Memory Consistency and Cache Coherence*, 2nd ed.** (2020,
  **open access**). Coherence protocols, memory models, and a new chapter on **accelerator/GPU
  coherence**. This is the senior replacement for hand-waving about "cache effects". **[model]**
- **Jacob, Ng & Wang, *Memory Systems: Cache, DRAM, Disk*** (2007). Deep DRAM: banks, rows, refresh,
  scheduling. Dated on capacities, correct on mechanisms. **[reference]**
- **Drepper, *What Every Programmer Should Know About Memory*** (2007, free, ~100 pp.). Still the best
  short treatment. **[model]**

### CPU architecture

- **Hennessy & Patterson, *Computer Architecture: A Quantitative Approach*, 7th ed.** (Oct 2025, now with
  Kozyrakis). Read the chapters on memory hierarchy, data-level parallelism and **domain-specific
  architectures** — the last contains the TPU case study, co-authored by its designers. **[reference]**
- **Agner Fog's optimization manuals** (free, continuously updated). Microarchitecture and instruction
  tables per CPU generation. **[reference]**

### GPU

- **Hwu, Kirk & El Hajj, *Programming Massively Parallel Processors*, 5th ed.** (Feb 2026). The
  programming side. **[model]**
- **Aamodt, Fung & Rogers, *General-Purpose Graphics Processor Architectures*** (Springer, 2018). The
  **architecture** side: SIMT execution, divergence handling, memory systems, scheduling. This is the
  book that explains *why* a GPU behaves as measured — the senior counterpart to PMPP. **[model]**
- NVIDIA CUDA C++ Programming Guide and Best Practices Guide (free, current). **[reference]**

### OpenMP — current, not 2007

- [X] **van der Pas, Stotzer & Terboven, *Using OpenMP — The Next Step*** (MIT Press, 2017). The direct
  sequel to the book in `docs/`: affinity, accelerators, tasking, SIMD. **[model]**
- **Deakin & Mattson, *Programming Your GPU with OpenMP*** (MIT Press, 2023). The book for `target`
  offload and performance portability. Most relevant single book to versions 3–4 of this project.
  **[model]**
- **Klemm & Cownie, *High Performance Parallel Runtimes*** (De Gruyter, 2021). How a runtime is
  *implemented*. Directly explains the class of finding this repo hit — GCC's nvptx runtime capping
  threads per team at 20 is a runtime design decision, not a language one. **[model]**
- **The OpenMP specification and the Examples document** (free). Current release **6.0** (Nov 2024);
  6.1 expected Nov 2026. For `target`/`teams`/`distribute`, the spec's examples beat every book.
  **[reference]**

### MPI and distributed memory

- **Gropp, Lusk & Skjellum, *Using MPI*, 3rd ed.** (2014) and **Gropp, Hoefler & Thakur, *Using Advanced
  MPI*** (2014). Still standard; MPI moves slowly. **[model, then reference]**
- The **MPI standard** itself: 4.1 (2023), with 5.0 finalised — check mpi-forum.org for the current
  document. Senior MPI work lives in one-sided communication, neighbourhood collectives and
  hybrid MPI+OpenMP, which is where *Advanced* and the standard matter. **[reference]**

### Performance portability — what production codes are moving to

- **Kokkos lectures and tutorials** (Sandia, free, video + slides, updated yearly). The dominant C++
  performance-portability layer in US labs; runs on CPU, NVIDIA, AMD and Intel GPUs from one source.
  **[model]**
- **Reinders et al., *Data Parallel C++*, 2nd ed.** (Apress, 2023, **open access**). SYCL, the
  vendor-neutral standard. **[model]**
- C++ `std::execution` / senders-receivers as it lands in compilers. **[reference]**

### Accelerators beyond GPUs — the TPU-class track

- **Sze, Chen, Yang & Emer, *Efficient Processing of Deep Neural Networks*** (2020). Systolic arrays,
  dataflow taxonomies (weight-stationary, output-stationary), and the energy-per-operation reasoning that
  drives every AI accelerator including the TPU. Joel Emer is a processor architect; this is not a
  machine-learning book. **[model]**
- **Austin et al. (Google DeepMind), *How to Scale Your Model: A Systems View of LLMs on TPUs***
  (jax-ml.github.io/scaling-book, free). **The single most useful TPU resource that exists.** TPU
  architecture, rooflines, sharding strategies, collectives, and how to choose parallelism so
  communication does not dominate. It also has a GPU comparison section. The method is exactly this
  project's method — roofline first, then measure — applied at datacentre scale. **[model]**
- **Reddi et al., *Machine Learning Systems*** (Harvard, free at mlsysbook.ai, MIT Press edition 2026).
  The full ML-systems lifecycle; read the hardware-acceleration and benchmarking volumes. **[reference]**
- **Barroso, Hölzle & Ranganathan, *The Datacenter as a Computer*** (3rd ed., 2018, free). What changes
  when the unit of deployment is a warehouse. **[model]**

### Concurrency, when it stops being a `parallel for`

- **Herlihy, Shavit, Luchangco & Spear, *The Art of Multiprocessor Programming*, 2nd ed.** (2020).
  Linearizability, lock-free algorithms, consensus. The theory that separates "uses OpenMP" from
  "understands concurrency". **[model]**
- **McKenney, *Is Parallel Programming Hard...?*** (perfbook, free, continuously updated). RCU, memory
  barriers, real kernel-grade parallelism. **[reference]**
- **Williams, *C++ Concurrency in Action*, 2nd ed.** (2019; no 3rd edition exists). The C++ memory model
  and atomics, done properly. **[model]**

### Numerical foundations, for HPC specifically

- **Golub & Van Loan, *Matrix Computations*, 4th ed.** (2013). The reference for the kernels HPC actually
  runs. **[reference]**
- **Eijkhout, *The Art of HPC*** (4 volumes, free, theartofhpc.com). Volume 1 (science of computing) and
  Volume 2 (parallel programming) overlap with the above but are free and well-taught. **[reference]**

### Compilers — because this project already hit this wall

- **Allen & Kennedy, *Optimizing Compilers for Modern Architectures*** (2001). Dependence analysis and
  loop transformations. Dated on machines, exactly right on why a directive like `collapse(2)` can
  prevent vectorisation: it is a loop-transformation legality question. **[reference]**

---

## Papers and specifications — the senior differentiator

A reading habit worth building now: one paper a week, in a notebook, with the measurement it predicts.

- Williams, Waterman & Patterson, **"Roofline: An Insightful Visual Performance Model"** (CACM 2009).
- Hager, Treibig & Wellein on the **ECM model** — roofline's more predictive successor.
- Jouppi et al., **"In-Datacenter Performance Analysis of a Tensor Processing Unit"** (ISCA 2017), and
  the TPUv4 follow-ups. The TPU papers are short and unusually clear.
- Gholami et al., **"AI and Memory Wall"** — why accelerator progress is bounded by bandwidth, which is
  the same constraint measured in this repo at 0.42 flop/byte.
- **Amdahl (1967)** and **Gustafson (1988)** — both, and the difference between them.
- The **OpenMP 6.0** and **MPI 4.1/5.0** documents; vendor tuning guides (NVIDIA, AMD, Intel).

---

## What to skip, and why

- **Chapman et al., *Using OpenMP* (2007)** — the copy in `docs/`. Keep Ch. 5 (sequential performance and
  measurement) and Ch. 7–8 (races, compiler translation). The rest is superseded by *The Next Step*.
- Quinn, *Parallel Programming in C with MPI and OpenMP* (2003), Chandra et al., *Parallel Programming in
  OpenMP* (2001), Cook's introduction — all listed on openmp.org, all pre-3.0.
- *CUDA by Example* (2010), *Professional CUDA C Programming* (2014) — superseded by PMPP 5th ed. plus
  NVIDIA's current guides.
- Culler, Singh & Gupta, *Parallel Computer Architecture* (1999) — historically important, but the
  coherence and consistency material is better in the 2020 Primer.
- Tanenbaum, *Modern Operating Systems* — fine, but redundant after OSTEP for this trajectory.

---

## A revised twelve-month plan

Two threads at once: one that pays off inside the placement, one that compounds for the career.

### Months 1–3

- ~~**Placement thread.** Ruud van der Pas, Eric Stotzer & Christian Terboven, ***Using OpenMP — The Next
  Step: Affinity, Accelerators, Tasking, and SIMD*** (MIT Press, 2017)~~; ~~Tom Deakin & Timothy G. Mattson,
  ***Programming Your GPU with OpenMP: Performance Portability for GPUs*** (MIT Press, 2023)~~; ~~the
  **OpenMP Application Programming Interface Examples** document for release 6.0 (openmp.org, free).~~
- **Career thread.** ~~Denis Bakhvalov, ***Performance Analysis and Tuning on Modern CPUs: Learn to Write
  Fast Software Like a Pro***, 2nd ed. (2024, free PDF)~~; ~~Samuel Williams, Andrew Waterman & David
  Patterson, **"Roofline: An Insightful Visual Performance Model for Multicore Architectures"**
  (*Communications of the ACM*, 2009)~~.

### Months 3–6

- **Placement thread.** Georg Hager & Gerhard Wellein, ***Introduction to High Performance Computing for
  Scientists and Engineers***, 2nd ed. (CRC Press, 2026) — then re-derive this project's ceilings from
  its models rather than from measurements.
- **Career thread.** Vijay Nagarajan, Daniel J. Sorin, Mark D. Hill & David A. Wood, ***A Primer on
  Memory Consistency and Cache Coherence***, 2nd ed. (Morgan & Claypool / Springer, 2020; open access).

### Months 6–9

- **Placement thread.** Wen-mei W. Hwu, David B. Kirk & Izzat El Hajj, ***Programming Massively Parallel
  Processors: A Hands-on Approach***, 5th ed. (Morgan Kaufmann, 2026); Tor M. Aamodt, Wilson Wai Lun Fung
  & Timothy G. Rogers, ***General-Purpose Graphics Processor Architectures*** (Springer, 2018) for the
  architecture underneath it.
- **Career thread.** Raj Jain, ***The Art of Computer Systems Performance Analysis: Techniques for
  Experimental Design, Measurement, Simulation, and Modeling*** (Wiley, 1991), or David J. Lilja,
  ***Measuring Computer Performance: A Practitioner's Guide*** (Cambridge University Press, 2000). Start
  the one-paper-a-week habit here.

### Months 9–12

- **Placement thread.** William Gropp, Ewing Lusk & Anthony Skjellum, ***Using MPI: Portable Parallel
  Programming with the Message-Passing Interface***, 3rd ed. (MIT Press, 2014), then William Gropp,
  Torsten Hoefler & Rajeev Thakur, ***Using Advanced MPI: Modern Features of the Message-Passing
  Interface*** (MIT Press, 2014); **The Kokkos Lectures** (Sandia National Laboratories, free slides and
  recordings).
- **Career thread.** Jacob Austin et al. (Google DeepMind), ***How to Scale Your Model: A Systems View of
  LLMs on TPUs*** (jax-ml.github.io/scaling-book, free); Vivienne Sze, Yu-Hsin Chen, Tien-Ju Yang & Joel
  S. Emer, ***Efficient Processing of Deep Neural Networks*** (Morgan & Claypool, 2020); Norman P. Jouppi
  et al., **"In-Datacenter Performance Analysis of a Tensor Processing Unit"** (ISCA, 2017) and the TPUv4
  follow-ups.

### After the twelve months

Maurice Herlihy, Nir Shavit, Victor Luchangco & Michael Spear, ***The Art of Multiprocessor Programming***,
2nd ed. (Morgan Kaufmann, 2020), and chapters of John L. Hennessy, David A. Patterson & Christos
Kozyrakis, ***Computer Architecture: A Quantitative Approach***, 7th ed. (Morgan Kaufmann, 2025) as
reference — chosen by whichever direction the work has taken: HPC simulation, ML systems, or systems
engineering generally.

**The caveat that matters most:** every book here trails the hardware by years, and the AI-accelerator
material trails it by months. The durable skill is the loop this project already practises — predict from
a model, measure on a controlled machine, state the accounting, explain the gap, and distrust a
correction as much as the original claim.

## Sources checked, 2026-09-25

- OpenMP book list and spec status: <https://www.openmp.org/resources/openmp-books/>,
  <https://www.openmp.org/articles/openmp-6/>
- *Programming Your GPU with OpenMP*: <https://mitpress.mit.edu/9780262547536/>
- PMPP 5th ed.: <https://shop.elsevier.com/books/programming-massively-parallel-processors/hwu/978-0-443-43900-1>
- H&P 7th ed.: <https://shop.elsevier.com/books/computer-architecture/hennessy/978-0-443-15406-5>
- CS:APP 4th-edition status: <http://csappbook.blogspot.com/2026/06/the-4th-edition-is-in-works.html>
- Hager & Wellein 2nd ed.: <https://www.routledge.com/Introduction-to-High-Performance-Computing-for-Scientists-and-Engineers/Hager-Wellein/p/book/9781482252934>
- Bakhvalov (free PDF): <https://github.com/dendibakh/perf-book>
- Memory consistency primer (open access): <https://link.springer.com/book/10.1007/978-3-031-01764-3>
- GPU architectures (Aamodt et al.): <https://link.springer.com/book/10.1007/978-3-031-01759-9>
- *Efficient Processing of Deep Neural Networks*: <https://link.springer.com/book/10.1007/978-3-031-01766-7>
- *How to Scale Your Model* (TPUs): <https://jax-ml.github.io/scaling-book/>
- *Machine Learning Systems*: <https://mlsysbook.ai/>
- Kokkos tutorials: <https://github.com/kokkos/kokkos-tutorials>
- *The Art of HPC*: <https://theartofhpc.com/>
- *Using MPI* 3rd ed.: <https://mitpress.mit.edu/9780262527392/using-mpi/>
- MPI standard documents: <https://www.mpi-forum.org/>
- *Data Parallel C++* 2nd ed. (open access): <https://link.springer.com/book/10.1007/978-1-4842-9691-2>
