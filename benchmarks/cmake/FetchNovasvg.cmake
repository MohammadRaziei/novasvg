# Builds the actual novasvg C++ CLI via FetchContent instead of driving
# novasvg through its Python binding -- same convention pygixml/benchmarks
# uses for pygixml itself (fetch a real checkout, don't reuse the local
# source tree, so this directory stays copyable/standalone). All 4 other
# engines still go through their Python bindings in python/engines.py;
# novasvg alone is invoked as a subprocess against this freshly-built CLI.

include(FetchContent)

set(NOVASVG_BENCH_GIT_URL "https://github.com/MohammadRaziei/novasvg.git" CACHE STRING
    "novasvg source repo the CLI is built from")
set(NOVASVG_BENCH_GIT_TAG "master" CACHE STRING
    "novasvg git ref (branch/tag/commit) the CLI is built from")

# Keep the fetched build minimal: just the header-only lib + novasvg_cli.
set(NOVASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    novasvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(novasvg_src)
# Target `novasvg_cli` (executable name `novasvg`) now exists; consumed via
# $<TARGET_FILE:novasvg_cli> in python/CMakeLists.txt.
