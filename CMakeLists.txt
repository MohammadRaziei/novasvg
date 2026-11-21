cmake_minimum_required(VERSION 3.19)

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

# Header-only library: two INTERFACE targets.
#   novasvg::noflag   headers + compile definitions only
#   novasvg::novasvg  noflag + optimisation flags (-O3 / /O2 ...): what nearly everybody wants.
#                     Link novasvg::noflag instead to opt out of the flags.
add_library(novasvg_noflag INTERFACE)
add_library(novasvg::noflag ALIAS novasvg_noflag)
set_target_properties(novasvg_noflag PROPERTIES EXPORT_NAME noflag)

if(NOT NOVASVG_EMBED_DEFAULT_FONT)
    target_compile_definitions(novasvg_noflag INTERFACE NOVASVG_DISABLE_EMBEDDED_FONT)
endif()

target_include_directories(novasvg_noflag INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")

add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)
target_link_libraries(novasvg INTERFACE novasvg_noflag)
novasvg_optimize(novasvg)   # flags travel to every consumer of novasvg::novasvg

# Add CLI executable
add_executable(novasvg_cli src/novasvg_cli.cpp)
target_link_libraries(novasvg_cli PRIVATE novasvg::novasvg)
set_target_properties(novasvg_cli PROPERTIES OUTPUT_NAME novasvg)
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

# Generate single-header distribution if NOVASVG_DIST_DIR is defined
if(DEFINED NOVASVG_DIST_DIR)
    add_custom_target(novasvg_dist_header
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_DIST_DIR}/novasvg
        COMMAND quom ${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h 
            ${NOVASVG_DIST_DIR}/novasvg/novasvg.h 
            -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMENT "Generating single-header distribution: ${NOVASVG_DIST_DIR}/novasvg/novasvg.h"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

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

    install(TARGETS novasvg_noflag novasvg EXPORT novasvgTargets)
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
