# thorvg ships only a meson build, no CMake support -- so we fetch its
# source ourselves and drive meson+ninja directly via a custom command,
# producing a static lib that native/thorvg_native_bench.cpp links against
# like any other target. Requires meson + ninja on PATH (apt install meson
# ninja-build covers both).
#
# Built with the minimal feature set this benchmark actually needs (SVG
# loader + software rasterizer only) to keep the build fast: no text/font
# loaders, no savers, no language bindings, no threading.

include(FetchContent)

set(NOVASVG_BENCH_THORVG_GIT_URL "https://github.com/thorvg/thorvg.git" CACHE STRING
    "thorvg source repo the native bench is built from")
set(NOVASVG_BENCH_THORVG_GIT_TAG "main" CACHE STRING
    "thorvg git ref (branch/tag/commit)")

find_program(MESON_EXECUTABLE meson)
find_program(NINJA_EXECUTABLE ninja)
if(NOT MESON_EXECUTABLE OR NOT NINJA_EXECUTABLE)
    message(FATAL_ERROR
        "thorvg needs meson + ninja to build from source (it has no CMake build of its own). "
        "Install both, e.g. `apt install meson ninja-build`.")
endif()

FetchContent_Declare(
    thorvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_THORVG_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_THORVG_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_Populate(thorvg_src)

set(THORVG_BUILD_DIR "${thorvg_src_BINARY_DIR}/mesonbuild")
set(THORVG_LIB "${THORVG_BUILD_DIR}/src/libthorvg-1.a")
set(THORVG_INCLUDE_DIR "${thorvg_src_SOURCE_DIR}/inc")

add_custom_command(
    OUTPUT "${THORVG_LIB}"
    COMMAND "${MESON_EXECUTABLE}" setup "${THORVG_BUILD_DIR}"
            -Dloaders=svg -Dsavers= -Dbindings= -Dthreads=false -Dlog=false
            -Ddefault_library=static -Dtests=false --buildtype=release
    COMMAND "${NINJA_EXECUTABLE}" -C "${THORVG_BUILD_DIR}"
    WORKING_DIRECTORY "${thorvg_src_SOURCE_DIR}"
    COMMENT "novasvg_bench: building thorvg from source (meson+ninja, svg loader + sw engine only)"
    VERBATIM
)
add_custom_target(thorvg_build DEPENDS "${THORVG_LIB}")
# Consumers: link "${THORVG_LIB}", include "${THORVG_INCLUDE_DIR}", and
# add_dependencies(<target> thorvg_build) so the meson build runs first.
