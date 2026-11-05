# nanosvg is a single-header C library (nanosvg.h + nanosvgrast.h); we
# don't use its own CMakeLists.txt targets (their include dirs are only
# wired up for the *installed* package, not the FetchContent build tree) --
# just fetch the source and point native/nanosvg_native_bench.cpp's include
# path at src/. NANOSVG_IMPLEMENTATION / NANOSVGRAST_IMPLEMENTATION are
# defined once in that one .cpp, stb-header style.

include(FetchContent)

set(NOVA_BENCH_NANOSVG_GIT_URL "https://github.com/memononen/nanosvg.git" CACHE STRING
    "nanosvg source repo the native bench compiles against")
set(NOVA_BENCH_NANOSVG_GIT_TAG "master" CACHE STRING
    "nanosvg git ref (branch/tag/commit)")

FetchContent_Declare(
    nanosvg_src
    GIT_REPOSITORY "${NOVA_BENCH_NANOSVG_GIT_URL}"
    GIT_TAG "${NOVA_BENCH_NANOSVG_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_Populate(nanosvg_src)
# ${nanosvg_src_SOURCE_DIR}/src now holds nanosvg.h / nanosvgrast.h.

# nanosvg has no version macro and no release tags: the commit is its only real version, so record that.
execute_process(
    COMMAND git -C "${nanosvg_src_SOURCE_DIR}" rev-parse --short=7 HEAD
    OUTPUT_VARIABLE NOVA_BENCH_NANOSVG_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT NOVA_BENCH_NANOSVG_COMMIT)
    set(NOVA_BENCH_NANOSVG_COMMIT "${NOVA_BENCH_NANOSVG_GIT_TAG}")  # not a git checkout: fall back to the ref asked for
endif()
