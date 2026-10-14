cmake_minimum_required(VERSION 3.19)
project(NovaSVGBenchmarks LANGUAGES CXX)

# Compares novasvg against resvg, lunasvg, thorvg, nanosvg and cairosvg on
# capability, rendered output, and speed -- and writes report.html plus
# render_benchmark.json and the renders/ PNGs it loads. 5 of the 6 engines are fetched and built from source here and
# driven directly through their own C/C++ API, each in one process
# (native/*_native_bench.cpp): novasvg is pulled in by
# cmake/FetchNovasvg.cmake (FetchContent; point it at a local tree with
# -DFETCHCONTENT_SOURCE_DIR_NOVASVG_SRC=...);
# lunasvg is a real CMake project (FetchContent); nanosvg is a single-header
# C library compiled straight in; thorvg ships only a meson build, so
# cmake/FetchThorvg.cmake drives meson+ninja itself; resvg is Rust, so
# cmake/FetchResvg.cmake runs `cargo build --release` on its C API crate
# itself. No CLI, no subprocess per render for any of them. Only cairosvg has
# no native library of its own to link against and runs through its Python
# binding (python/engines.py) instead. Directory otherwise follows the
# self-contained-directory convention of
# https://github.com/MohammadRaziei/pygixml/tree/main/benchmarks.
#
#   cmake -S . -B build
#   cmake --build build --target novasvg_benchmark
#
# Needs on PATH: a C++17 compiler, meson + ninja (thorvg), cargo (resvg),
# Python 3.
#
# Result, in outputs/ (keep them together; no server or network needed):
#   renders/<engine>/<sample>.png + result.json   one folder per engine, written by python/run_benchmark.py
#   render_benchmark.json                         report/gather.py merging those result.json files, plus RMSE vs Chromium
#   report.html                                   report/generate_report.py, built from render_benchmark.json
#
# Tunables (cmake -D<name>=<value>):
#   NOVASVG_BENCH_RUNS    renders per (engine, sample) cell, median reported (default 5)
#   NOVASVG_BENCH_WIDTH / NOVASVG_BENCH_HEIGHT   shared output size in px (default 320x320)
#   NOVASVG_BENCH_LUNASVG_GIT_TAG / _THORVG_GIT_TAG / _RESVG_GIT_TAG
#     which checkout each fetched engine is built from (novasvg itself:
#     NOVASVG_BENCH_GIT_URL / _GIT_TAG, see above; README.md has resvg's rust-version caveat)
# See README.md for the full rundown and what's still ponytail-scoped for later.

set(NOVASVG_BENCH_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/outputs" CACHE PATH
    "Where everything lands: renders/<engine>/ (PNGs + that engine's result.json), render_benchmark.json and report.html -- what is meant to be committed/shared")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(DefaultBuildType)   # before any target is created
include(FetchNovasvg)
include(FetchLunasvg)
include(FetchNanosvg)
include(FetchThorvg)
include(FetchResvg)

add_subdirectory(native)
add_subdirectory(python)
add_subdirectory(report)

add_custom_target(novasvg_benchmark
  COMMENT "novasvg_benchmark: running the full 6-engine benchmark suite and building the report"
)
add_dependencies(novasvg_benchmark novasvg_bench_report)
