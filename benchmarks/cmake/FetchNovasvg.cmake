# Builds the actual novasvg C++ library/CLI that
# native/novasvg_native_bench.cpp links against.
#
# This directory normally lives *inside* a novasvg checkout (it's
# ../benchmarks relative to novasvg's own root CMakeLists.txt), so the
# right thing is to build THAT tree -- not fetch a second, separate copy
# of novasvg from GitHub and build it again alongside the one we're
# already sitting in. So: use the local tree when it's there
# (../CMakeLists.txt + ../include/novasvg present), and only fall back to
# FetchContent from GitHub if this directory has been copied out on its
# own (matching the self-contained-directory convention the other engines'
# Fetch*.cmake modules use, e.g.
# https://github.com/MohammadRaziei/pygixml/tree/main/benchmarks).

set(NOVASVG_LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Keep the build minimal either way: just the header-only lib + novasvg_cli.
set(NOVASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(NOVASVG_BUILD_DOCS OFF CACHE BOOL "" FORCE)

if(EXISTS "${NOVASVG_LOCAL_ROOT}/CMakeLists.txt" AND EXISTS "${NOVASVG_LOCAL_ROOT}/include/novasvg")
    message(STATUS "novasvg_bench: using the local novasvg checkout at ${NOVASVG_LOCAL_ROOT} (not fetching a second copy)")
    add_subdirectory("${NOVASVG_LOCAL_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/_novasvg_local")
else()
    message(STATUS "novasvg_bench: no local novasvg checkout found above this directory -- fetching one from GitHub")
    include(FetchContent)
    set(NOVASVG_BENCH_GIT_URL "https://github.com/MohammadRaziei/novasvg.git" CACHE STRING
        "novasvg source repo to build from when there's no local checkout to reuse")
    set(NOVASVG_BENCH_GIT_TAG "master" CACHE STRING
        "novasvg git ref (branch/tag/commit), only used in the no-local-checkout fallback")
    FetchContent_Declare(
        novasvg_src
        GIT_REPOSITORY "${NOVASVG_BENCH_GIT_URL}"
        GIT_TAG "${NOVASVG_BENCH_GIT_TAG}"
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(novasvg_src)
endif()
# Either way, target `novasvg::novasvg` (header-only) and `novasvg_cli`
# (executable name `novasvg`) now exist; the latter consumed via
# $<TARGET_FILE:novasvg_native_bench> is not needed here -- that's our own
# target, linked against novasvg::novasvg in native/CMakeLists.txt.
