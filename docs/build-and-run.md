# Build and run — cheat sheet

> **Written by Claude (an AI assistant)** as a reference card for this project. Not Krit's own writing.

CMake is two phases: **configure** writes build instructions, **build** compiles. Then `ctest` tests.

```bash
cmake -S . -B build -G Ninja                  # configure (first time, or to change an option)
cmake --build build                           # compile — only what changed
ctest --test-dir build --output-on-failure    # run tests; ALWAYS --output-on-failure
./build/heat_serial 0 100 100 500             # expect Centre = 0.82085...
```

| Flag | Meaning |
|---|---|
| `-S .` | source dir (where `CMakeLists.txt` is) |
| `-B build` | build dir — everything generated goes here (gitignored, safe to delete) |
| `-G Ninja` | write `build.ninja` instead of a `Makefile` |
| `-DHEAT_GPU=ON` | also build the two GPU variants — **fails to configure until `src/openmp_gpu_naive/main.cpp` and `src/openmp_gpu_optimized/main.cpp` exist** |
| `-DHEAT_NATIVE=OFF` | portable binary (no `-march=native`) for another machine |
| `-DCMAKE_BUILD_TYPE=Debug` | asserts live, `-g`, no `-O3` — use a separate dir |

**Options persist** in `build/CMakeCache.txt`. Configuring again without `-DHEAT_GPU=...` keeps the
old value; only an explicit `-DHEAT_GPU=OFF` resets it.

**Release adds `-DNDEBUG`**, so `assert` vanishes there — which is why input checks use `require()`
(ADR-003).

## Situations

| Situation | Do |
|---|---|
| Debugging | `cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug` — keep it beside `build/` |
| See the real compiler commands | `cmake --build build -v` |
| Build one program | `cmake --build build --target heat_serial` |
| Run one test | `ctest --test-dir build -R config --output-on-failure` |
| Switch Ninja <-> Make | refused in an existing dir — `rm -rf build` first |
| Anything confusing | `rm -rf build` and configure again; it is all regenerable |
| GPU run | build with `-DHEAT_GPU=ON`, run with `OMP_TARGET_OFFLOAD=MANDATORY ./build/heat_gpu_naive ...` |
