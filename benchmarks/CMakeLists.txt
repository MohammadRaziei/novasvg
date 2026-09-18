cmake_minimum_required(VERSION 3.19)
project(NovaSVGBenchmarks LANGUAGES CXX)

# Compares novasvg against resvg, lunasvg, cairosvg and thorvg on capability,
# rendered output, and speed -- and writes a single self-contained report.html.
# novasvg is driven directly through its own C++ API (novasvg::Document /
# novasvg::Bitmap, native/novasvg_native_bench.cpp), fetched and built from
# source here (cmake/FetchNovasvg.cmake) -- not the novasvg_cli binary, not a
# subprocess per render. Every sample renders inside that one process, same
# as the other 4 engines do inside one Python process via their bindings
# (resvg-py, pylunasvg, cairosvg, thorvg-python), so timings are finally
# apples-to-apples. Directory otherwise follows the self-contained-directory
# convention of https://github.com/MohammadRaziei/pygixml/tree/main/benchmarks.
#
#   cmake -S . -B build
#   cmake --build build --target novasvg_benchmark
#
# Result: results/report.html (self-contained, safe to open with no server
# and no network -- every render is a base64 data: URI).
#
# Tunables (cmake -D<name>=<value>):
#   NOVASVG_BENCH_GIT_URL / NOVASVG_BENCH_GIT_TAG   which novasvg checkout the CLI is built from
#   NOVASVG_BENCH_RUNS    renders per (engine, sample) cell, median reported (default 5)
#   NOVASVG_BENCH_WIDTH / NOVASVG_BENCH_HEIGHT   shared output size in px (default 320x320)
# See README.md for the full rundown and what's still ponytail-scoped for later.

set(NOVASVG_BENCH_RESULTS_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/results" CACHE PATH
    "Where the final standalone report.html (and results.json) are written -- the artifacts meant to be committed/shared")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(FetchNovasvg)

add_subdirectory(native)
add_subdirectory(python)
add_subdirectory(report)

add_custom_target(novasvg_benchmark
  COMMENT "novasvg_benchmark: running the full 5-engine benchmark suite and building the report"
)
add_dependencies(novasvg_benchmark novasvg_bench_report)
