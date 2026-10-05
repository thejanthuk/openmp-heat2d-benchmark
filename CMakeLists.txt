# CMake is a build-system GENERATOR: you declare targets and their properties,
# and it writes the Makefile/ninja file. You never write compiler command lines.
cmake_minimum_required(VERSION 3.20)
project(heat_fdm_2d LANGUAGES CXX)

# C++17, not 20: the university server may have an older GCC, and nothing here needs 20.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17, not -std=gnu++17

# TRAP: if CMAKE_BUILD_TYPE is empty, CMake adds NO optimisation flag at all --
# you get -O0 and every benchmark number is garbage. Default it explicitly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
# Release is -O3 -DNDEBUG. Do not override it to -O2 "to be safe"; -O3 is the
# build every reported number must come from.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # compile_commands.json, for clangd

option(HEAT_NATIVE "Tune for this exact CPU (-march=native)" ON)
option(HEAT_GPU    "Build the OpenMP GPU offload variants"   OFF)

# --- shared header-only code -------------------------------------------------
# An INTERFACE library has no sources and compiles nothing. It is a bundle of
# properties that propagates to whatever links it. This is exactly what ADR-001
# needs: shared code compiled INTO each variant, never linked as a library,
# because GCC's nvptx offload is unreliable across translation units.
add_library(heat_common INTERFACE)
target_include_directories(heat_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(heat_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(heat_common INTERFACE -Wall -Wextra -Wpedantic -Wfloat-conversion -Werror)
if(HEAT_NATIVE)
  # -march=native unlocks AVX2 + FMA on this machine: 4 doubles per instruction
  # instead of SSE2's 2, plus fused multiply-add. Off by default on unknown
  # hardware -- the binary will not run on an older CPU.
  target_compile_options(heat_common INTERFACE -march=native)
endif()

find_package(OpenMP REQUIRED)          # provides the OpenMP::OpenMP_CXX target

add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config PRIVATE heat_common)
add_test(NAME config COMMAND test_config)

add_executable(test_exact tests/test_exact.cpp)
target_link_libraries(test_exact PRIVATE heat_common)
add_test(NAME exact COMMAND test_exact)


# --- version 1: serial -------------------------------------------------------
# Deliberately does NOT link OpenMP. The baseline should contain no OpenMP at
# all, so no runtime startup or parallel-region overhead can leak into it.
# timer.hpp therefore uses std::chrono::steady_clock, not omp_get_wtime().
add_executable(heat_serial src/serial/main.cpp)
target_link_libraries(heat_serial PRIVATE heat_common)

# --- version 2: CPU OpenMP ---------------------------------------------------
add_executable(heat_openmp_cpu src/openmp_cpu/main.cpp)
target_link_libraries(heat_openmp_cpu PRIVATE heat_common OpenMP::OpenMP_CXX)

# --- versions 3 and 4: GPU offload ------------------------------------------
# OFF by default so the project still configures on a machine with no nvptx
# toolchain. Enable with: cmake -S . -B build -DHEAT_GPU=ON
if(HEAT_GPU)
  # One place for the device flags, so the variants and the differential test
  # cannot drift apart. A target built with different device flags from the one
  # it is compared against is not a controlled experiment.
  function(heat_add_offload_flags target)
    # -fno-stack-protector: Gentoo hardens GCC by default; nvptx cannot do it.
    # -foffload-options=...=-lm: GCC does NOT forward -lm to the device link,
    #   so sin() inside a target region fails to link without this.
    # -foffload-options=...=-O3: CMAKE_BUILD_TYPE's -O3 applies to the HOST
    #   compiler only. Without this the device kernel is built unoptimised,
    #   which leaves absolute GPU times meaningless (added 2026-10-05; the gap
    #   between the naive and optimized variants is unaffected, since both
    #   shared the omission).
    target_compile_options(${target} PRIVATE
      -foffload=nvptx-none
      -fno-stack-protector
      -foffload-options=nvptx-none=-O3
      -foffload-options=nvptx-none=-fno-stack-protector
      -foffload-options=nvptx-none=-lm)

    # -fopenmp must appear on the LINK line too, not just compile: it activates
    # the offload linker plugin. CMake links libgomp by path, which is NOT
    # enough. Verified 2026-09-17 -- without it the link fails loudly with
    #   undefined reference to `__offload_func_table'
    # so this one cannot be got wrong silently. (Unlike a missing
    # OMP_TARGET_OFFLOAD=MANDATORY, which CAN silently run on the host.)
    # GCC generates device code during the LINK step (via mkoffload), so the
    # device options must appear here as well as on the compile line.
    target_link_options(${target} PRIVATE
      -fopenmp
      -foffload=nvptx-none
      -fno-stack-protector
      -foffload-options=nvptx-none=-O3
      -foffload-options=nvptx-none=-fno-stack-protector
      -foffload-options=nvptx-none=-lm)
  endfunction()

  # Each variant is added only if its source exists, so the naive version can
  # be built and tested before the optimized one has been written. Previously
  # the loop declared both targets and configuring failed with
  #   No SOURCES given to target: heat_gpu_optimized
  foreach(variant naive optimized)
    set(variant_source ${CMAKE_CURRENT_SOURCE_DIR}/src/openmp_gpu_${variant}/main.cpp)
    if(EXISTS ${variant_source})
      add_executable(heat_gpu_${variant} ${variant_source})
      target_link_libraries(heat_gpu_${variant} PRIVATE heat_common OpenMP::OpenMP_CXX)
      heat_add_offload_flags(heat_gpu_${variant})
    else()
      message(STATUS "HEAT_GPU: src/openmp_gpu_${variant}/main.cpp not written yet -- skipping")
    endif()
  endforeach()

  # Full-field differential test, GPU naive vs the serial oracle. Built only
  # with HEAT_GPU because it includes the offload solver. OMP_TARGET_OFFLOAD is
  # set on the test itself: without it a host fallback would be timed and
  # compared as though it were the GPU. The solver's own require() then refuses
  # to return a result it did not compute on the device.
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/openmp_gpu_naive/solver.hpp)
    add_executable(test_differential_gpu tests/differential_gpu.cpp)
    target_link_libraries(test_differential_gpu PRIVATE heat_common OpenMP::OpenMP_CXX)
    heat_add_offload_flags(test_differential_gpu)
    add_test(NAME differential_gpu COMMAND test_differential_gpu)
    set_tests_properties(differential_gpu PROPERTIES
      ENVIRONMENT "OMP_TARGET_OFFLOAD=MANDATORY")
  endif()
endif()

# --- tests -------------------------------------------------------------------
# No test framework: a main() returning non-zero is enough for a convergence
# study. Run with `ctest --test-dir build`.
enable_testing()
add_executable(test_convergence tests/convergence.cpp)
target_link_libraries(test_convergence PRIVATE heat_common)
add_test(NAME convergence COMMAND test_convergence)
