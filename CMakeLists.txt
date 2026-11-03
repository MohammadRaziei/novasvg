cmake_minimum_required(VERSION 3.21)  # libstb needs 3.21

# novasvg_cli.cpp uses std::filesystem, which libc++ marks unavailable below
# macOS 10.15. cibuildwheel/scikit-build-core can otherwise pass an older
# target (e.g. 10.13, its universal2/x86_64 default) via the
# MACOSX_DEPLOYMENT_TARGET env var, so raise the floor here, before project(),
# without clobbering a caller-supplied target that's already >= 10.15.
if(APPLE)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.15")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target" FORCE)
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(DynamicVersion)

extract_version_string(
    HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h"
    VERSION_PREFIX "NOVASVG_"
    OUTPUT_VAR PROJECT_VERSION
)

project(NovaSVG LANGUAGES CXX VERSION ${PROJECT_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")
include(Startup)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags (default build type, LTO, per-compiler -O flags).
# Must come after project(). The library is header-only, so novasvg_optimize(novasvg)
# below puts the flags on the INTERFACE target and every consumer inherits them.
include(Optimize)

# Project options
option(NOVASVG_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_EMBED_DEFAULT_FONT "Compile a Latin subset of DejaVu Sans in as the last-resort fallback font" ON)
option(NOVASVG_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_PYTHON "Build Python" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_DOCS "Build documentation" OFF)


set(NOVASVG_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# ---- libstb: image/zlib/font engine, linked DYNAMICALLY ---------------------------------------
# Fetched from git with FetchContent. Point FETCHCONTENT_SOURCE_DIR_LIBSTB at a local checkout to
# work on both projects side by side, or pass -DFETCHCONTENT_FULLY_DISCONNECTED=ON to stay offline.
include(FetchContent)
set(NOVASVG_LIBSTB_GIT_REPOSITORY "https://github.com/mohammadraziei/libstb.git" CACHE STRING
    "libstb repository to fetch")
set(NOVASVG_LIBSTB_GIT_TAG "master" CACHE STRING
    "libstb commit, tag or branch to fetch (floats on master for now: temporary, to be replaced)")

# libstb is always built as a shared library here, and none of its extras are wanted.
set(STB_BUILD_SHARED   ON  CACHE BOOL "novasvg links libstb dynamically" FORCE)
set(STB_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(STB_BUILD_PYTHON   OFF CACHE BOOL "" FORCE)
set(STB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STB_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_Declare(libstb
    GIT_REPOSITORY "${NOVASVG_LIBSTB_GIT_REPOSITORY}"
    GIT_TAG        "${NOVASVG_LIBSTB_GIT_TAG}")
# libstb appends its own cmake/ directory to CMAKE_MODULE_PATH AFTER ours, and both projects have
# modules of the same name (Startup, Optimize, DynamicVersion). Left alone, libstb would silently
# include novasvg's Optimize.cmake and be compiled with novasvg's flags (checked: it lost
# -fstrict-aliasing). So our modules are hidden while libstb configures.
set(_novasvg_module_path "${CMAKE_MODULE_PATH}")
set(CMAKE_MODULE_PATH "")
FetchContent_MakeAvailable(libstb)
set(CMAKE_MODULE_PATH "${_novasvg_module_path}")

get_target_property(_novasvg_libstb_type libstb::libstb TYPE)
if(NOT _novasvg_libstb_type STREQUAL "SHARED_LIBRARY")
    message(FATAL_ERROR "novasvg requires libstb as a shared library, got: ${_novasvg_libstb_type}")
endif()

# ---- the compiled core -------------------------------------------------------------------------
# Step 4 of the refactor (ADR-0001) moves implementation out of the headers into src/, one module at a
# time. What has been moved is compiled once, here; what has not is still header-only and is compiled
# into whatever includes it. Each module directory under src/ is globbed.
file(GLOB NOVASVG_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/svg/*.cpp")
add_library(novasvg_core STATIC ${NOVASVG_CORE_SOURCES})
set_target_properties(novasvg_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(novasvg_core PRIVATE NOVASVG_BUILDING_LIBRARY)
get_target_property(_novasvg_core_type novasvg_core TYPE)
if(_novasvg_core_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(novasvg_core PUBLIC NOVASVG_STATIC_DEFINE)   # NOVASVG_API empty for consumers
endif()
target_include_directories(novasvg_core PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
# Everything that links novasvg links libstb too (dynamic).
target_link_libraries(novasvg_core PUBLIC libstb::libstb)
if(NOT NOVASVG_EMBED_DEFAULT_FONT)
    target_compile_definitions(novasvg_core PUBLIC NOVASVG_DISABLE_EMBEDDED_FONT)
endif()
novasvg_optimize(novasvg_core)

# Two consumer-facing targets (kept while the library is still partly header-only):
#   novasvg::noflag   the core, no optimisation flags on the consumer
#   novasvg::novasvg  noflag + optimisation flags (-O3 / /O2 ...): what nearly everybody wants.
#                     Link novasvg::noflag instead to opt out of the flags.
add_library(novasvg_noflag INTERFACE)
add_library(novasvg::noflag ALIAS novasvg_noflag)
set_target_properties(novasvg_noflag PROPERTIES EXPORT_NAME noflag)
target_link_libraries(novasvg_noflag INTERFACE novasvg_core)

add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)
target_link_libraries(novasvg INTERFACE novasvg_noflag)
novasvg_optimize(novasvg)   # flags travel to every consumer of novasvg::novasvg

# Add CLI executable
add_executable(novasvg_cli src/novasvg_cli.cpp)
target_link_libraries(novasvg_cli PRIVATE novasvg::novasvg)
set_target_properties(novasvg_cli PROPERTIES OUTPUT_NAME novasvg)
# The installed CLI finds the libstb it was installed with (bin/ and lib/ are siblings); the build tree
# already has the right RUNPATH. On Windows stb.dll is installed next to novasvg.exe.
if(APPLE)
    set_target_properties(novasvg_cli PROPERTIES INSTALL_RPATH "@loader_path/../lib")
elseif(UNIX)
    set_target_properties(novasvg_cli PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()
novasvg_optimize(novasvg_cli)


# target_compile_features(novasvg INTERFACE cxx_std_17)


if(NOVASVG_BUILD_PYTHON OR DEFINED SKBUILD)
    add_subdirectory(${PROJECT_SOURCE_DIR}/src/bindings/python)
    if(DEFINED SKBUILD)
        return()
    endif()
endif()

# Installation
install(TARGETS novasvg_cli
    RUNTIME DESTINATION bin
)

# Tests
if(NOVASVG_BUILD_TESTS)
    enable_testing()
    set(CMAKE_CTEST_ARGUMENTS "--output-on-failure" "-V")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()


# Examples
if(NOVASVG_BUILD_EXAMPLES)
    add_custom_target(novasvg_build_examples
        COMMAND ${CMAKE_COMMAND} -E echo "Building examples..."
    )

    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/cpp/sample*.cpp
    )

    foreach(SOURCE_FILE IN LISTS EXAMPLE_SOURCES)
        get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
        add_executable(${TARGET_NAME} ${SOURCE_FILE})
        target_link_libraries(${TARGET_NAME} PRIVATE novasvg)
        novasvg_optimize(${TARGET_NAME})
        message(STATUS "Added example target: ${TARGET_NAME}")
        add_dependencies(novasvg_build_examples ${TARGET_NAME})
    endforeach()
endif()


# Documentation (Doxygen)
if(NOVASVG_BUILD_DOCS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/docs)
endif()


# Installation
install(DIRECTORY include/ DESTINATION include)

# CMake package: find_package(novasvg) -> novasvg::novasvg (optimised) / novasvg::noflag.
# Only the top-level project installs it by default, so that embedding novasvg through
# FetchContent/add_subdirectory doesn't change the parent project's install tree.
option(NOVASVG_INSTALL "Install the CMake package (novasvgConfig.cmake, novasvg::novasvg, novasvg::noflag)" ${PROJECT_IS_TOP_LEVEL})
if(NOVASVG_INSTALL)
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)
    set(NOVASVG_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/cmake/novasvg" CACHE STRING
        "Where novasvgConfig.cmake is installed (relative to the install prefix)")

    install(TARGETS novasvg_core novasvg_noflag novasvg EXPORT novasvgTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(EXPORT novasvgTargets
        FILE novasvgTargets.cmake
        NAMESPACE novasvg::
        DESTINATION "${NOVASVG_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/novasvgConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/novasvgConfig.cmake"
        INSTALL_DESTINATION "${NOVASVG_INSTALL_CMAKEDIR}")
    # 0.x: the API is not frozen yet, so only the same minor version is considered compatible.
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/novasvgConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT)
    install(FILES
        "${PROJECT_BINARY_DIR}/novasvgConfig.cmake"
        "${PROJECT_BINARY_DIR}/novasvgConfigVersion.cmake"
        DESTINATION "${NOVASVG_INSTALL_CMAKEDIR}")
endif()
