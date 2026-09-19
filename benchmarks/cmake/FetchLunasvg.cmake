# Builds lunasvg from source (a real CMake project, unlike thorvg/resvg) so
# native/lunasvg_native_bench.cpp can link lunasvg::lunasvg directly -- same
# in-process-C++-API approach as novasvg, no Python binding for this one.

include(FetchContent)

set(NOVASVG_BENCH_LUNASVG_GIT_URL "https://github.com/sammycage/lunasvg.git" CACHE STRING
    "lunasvg source repo the native bench links against")
set(NOVASVG_BENCH_LUNASVG_GIT_TAG "master" CACHE STRING
    "lunasvg git ref (branch/tag/commit)")

set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    lunasvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_LUNASVG_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_LUNASVG_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lunasvg_src)
# Target `lunasvg::lunasvg` now exists (it pulls in its own plutovg
# dependency via a git submodule CMake resolves automatically).
