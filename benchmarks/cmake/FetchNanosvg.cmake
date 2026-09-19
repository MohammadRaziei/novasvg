# nanosvg is a single-header C library (nanosvg.h + nanosvgrast.h); we
# don't use its own CMakeLists.txt targets (their include dirs are only
# wired up for the *installed* package, not the FetchContent build tree) --
# just fetch the source and point native/nanosvg_native_bench.cpp's include
# path at src/. NANOSVG_IMPLEMENTATION / NANOSVGRAST_IMPLEMENTATION are
# defined once in that one .cpp, stb-header style.

include(FetchContent)

set(NOVASVG_BENCH_NANOSVG_GIT_URL "https://github.com/memononen/nanosvg.git" CACHE STRING
    "nanosvg source repo the native bench compiles against")
set(NOVASVG_BENCH_NANOSVG_GIT_TAG "master" CACHE STRING
    "nanosvg git ref (branch/tag/commit)")

FetchContent_Declare(
    nanosvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_NANOSVG_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_NANOSVG_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_Populate(nanosvg_src)
# ${nanosvg_src_SOURCE_DIR}/src now holds nanosvg.h / nanosvgrast.h.
