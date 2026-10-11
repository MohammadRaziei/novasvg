# Brings in the novasvg C++ library that native/novasvg_native_bench.cpp links
# against, via FetchContent. This directory is self-contained: it does not
# look at, include or depend on the directory it happens to sit in (only the
# sample data is shared with the parent).
#
# Which novasvg gets built:
#   default                   : NOVASVG_BENCH_GIT_URL @ NOVASVG_BENCH_GIT_TAG
#   to benchmark a local tree : -DFETCHCONTENT_SOURCE_DIR_NOVASVG_SRC=/path/to/novasvg
#                               (standard FetchContent override; also what to use
#                               to measure uncommitted changes)
#
# novasvg is header-only and carries its optimisation flags on the novasvg::novasvg
# INTERFACE target, so linking it (native/CMakeLists.txt) is enough. Those flags are
# config-specific, so cmake/DefaultBuildType.cmake guarantees a Release build.

include(FetchContent)

# Keep the build minimal: just the header-only lib + novasvg_cli.
set(NOVASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_DOCS OFF CACHE BOOL "" FORCE)

set(NOVASVG_BENCH_GIT_URL "https://github.com/MohammadRaziei/novasvg.git" CACHE STRING
    "novasvg source repo to build and benchmark")
set(NOVASVG_BENCH_GIT_TAG "master" CACHE STRING
    "novasvg git ref (branch/tag/commit) to build and benchmark")

FetchContent_Declare(
    novasvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(novasvg_src)
# Target `novasvg::novasvg` (header-only) and `novasvg_cli` now exist.
