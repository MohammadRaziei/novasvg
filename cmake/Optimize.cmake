# Optimize.cmake -- cross-platform optimisation flags for NovaSVG.
#
#   include(Optimize)                 # once, AFTER project()
#   novasvg_optimize(novasvg)         # the header-only INTERFACE target: its consumers inherit the flags
#   novasvg_optimize(<target>)        # an executable/library: flags + LTO on that one target
#
# NovaSVG is header-only: `novasvg::novasvg` is an INTERFACE target and the code is
# compiled inside whatever links it (the CLI, examples, benchmarks, the Python module...).
# Applying the flags to the INTERFACE target therefore optimises every consumer just by
# linking `novasvg::novasvg` -- also through an installed package (find_package(novasvg)),
# because they are ordinary usage requirements. They are config-specific
# ($<CONFIG:Release> ...), so a consumer whose build type is empty or "None" is left alone.
# Opting out: link `novasvg::noflag` instead of `novasvg::novasvg` (same headers and
# definitions, no flags; works from the build tree and from an installed package), or
# build novasvg itself with -DNOVASVG_OPTIMIZE=OFF.
# LTO is a per-target property that cannot travel through an INTERFACE target; call
# novasvg_optimize(<target>) on the consumer if you want it.
#
# What it does
#   * Single-config generators (Makefiles, Ninja): if CMAKE_BUILD_TYPE is empty
#     it defaults to Release (top-level project only). An empty build type
#     means *no* -O flag at all, i.e. -O0. An explicit value is always respected,
#     including "None" (what distro packagers use to inject their own flags).
#   * GCC / Clang / AppleClang / IntelLLVM : -O3 (Release), -O3 -g (RelWithDebInfo),
#     -Os (MinSizeRel), -Og -g3 (Debug).
#   * MSVC / clang-cl : /O2 (Release, RelWithDebInfo), /O1 (MinSizeRel).
#   * Link-time optimisation (LTO/IPO) in the three optimised configs when
#     the toolchain supports it. Debug is never LTO'd.
#   * Optional CPU tuning (-march=native / -mcpu=native), off by default.
#
# Options (all can be given with -D on the command line)
#   NOVASVG_OPTIMIZE             ON   master switch for everything in this file
#   NOVASVG_ENABLE_LTO           ON   LTO/IPO when supported
#   NOVASVG_NATIVE_ARCH          OFF  tune for THIS machine's CPU. The binary may
#                                     crash on other CPUs. Ignored when
#                                     cross-compiling, under scikit-build-core /
#                                     cibuildwheel, for macOS universal builds, and on MSVC.
#   NOVASVG_KEEP_FRAME_POINTERS  OFF  -fno-omit-frame-pointer: much better call
#                                     stacks for `perf record -g` / flamegraphs.
#
# `apply_optimization_flags(<target>)` is kept as an alias of novasvg_optimize().

include_guard(GLOBAL)

if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW) # honour INTERPROCEDURAL_OPTIMIZATION with every compiler
endif()

if(NOT CMAKE_CXX_COMPILER_LOADED)
    message(FATAL_ERROR "Optimize.cmake must be included after project() (the C++ language must be enabled).")
endif()

option(NOVASVG_OPTIMIZE            "Apply NovaSVG's optimisation flags (-O3 / /O2, LTO) to NovaSVG-owned targets" ON)
option(NOVASVG_ENABLE_LTO          "Use link-time optimisation in optimised configurations when supported"       ON)
option(NOVASVG_NATIVE_ARCH         "Tune for the build machine's CPU (binary is NOT portable)"                    OFF)
option(NOVASVG_KEEP_FRAME_POINTERS "Keep frame pointers so perf/flamegraph call stacks are complete"             OFF)

# ---------------------------------------------------------------------------
# Default build type (single-config generators only, top-level project only)
# ---------------------------------------------------------------------------
if(DEFINED PROJECT_IS_TOP_LEVEL)
    set(_novasvg_top_level ${PROJECT_IS_TOP_LEVEL})
elseif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_novasvg_top_level ON)
else()
    set(_novasvg_top_level OFF)
endif()

get_property(_novasvg_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOVASVG_OPTIMIZE AND _novasvg_top_level AND NOT _novasvg_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (defaulted to Release by NovaSVG)" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
    message(STATUS "NovaSVG: CMAKE_BUILD_TYPE was empty -> defaulting to Release")
endif()

# ---------------------------------------------------------------------------
# LTO support probe (runs once per configure; not cached across configures)
# ---------------------------------------------------------------------------
set_property(GLOBAL PROPERTY NOVASVG_IPO_SUPPORTED OFF)
if(NOVASVG_OPTIMIZE AND NOVASVG_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _novasvg_ipo OUTPUT _novasvg_ipo_msg LANGUAGES CXX)
    set_property(GLOBAL PROPERTY NOVASVG_IPO_SUPPORTED ${_novasvg_ipo})
    if(NOT _novasvg_ipo)
        message(STATUS "NovaSVG: LTO unavailable with this toolchain, continuing without it")
        message(VERBOSE "NovaSVG: check_ipo_supported said: ${_novasvg_ipo_msg}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Pick the right "tune for this CPU" flag, or report why we can't.
# Sets <out> to the flag, or to "" if unavailable.
# ---------------------------------------------------------------------------
function(_novasvg_native_arch_flag out)
    set(${out} "" PARENT_SCOPE)

    if(CMAKE_CROSSCOMPILING)
        message(STATUS "NovaSVG: NOVASVG_NATIVE_ARCH ignored (cross-compiling)")
        return()
    endif()
    if(DEFINED SKBUILD OR DEFINED ENV{CIBUILDWHEEL})
        message(STATUS "NovaSVG: NOVASVG_NATIVE_ARCH ignored (building a distributable wheel)")
        return()
    endif()
    list(LENGTH CMAKE_OSX_ARCHITECTURES _n_osx_arch)
    if(_n_osx_arch GREATER 1)
        message(STATUS "NovaSVG: NOVASVG_NATIVE_ARCH ignored (multi-architecture macOS build)")
        return()
    endif()
    if(MSVC)
        message(STATUS "NovaSVG: NOVASVG_NATIVE_ARCH has no MSVC equivalent, ignored")
        return()
    endif()

    include(CheckCXXCompilerFlag)
    # ARM compilers historically accept -mcpu=native; x86 only knows -march=native.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|ARM64)")
        set(_candidates -mcpu=native -march=native)
    else()
        set(_candidates -march=native -mcpu=native)
    endif()
    foreach(_flag IN LISTS _candidates)
        string(MAKE_C_IDENTIFIER "NOVASVG_HAS_${_flag}" _var)
        check_cxx_compiler_flag(${_flag} ${_var})
        if(${_var})
            set(${out} ${_flag} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(STATUS "NovaSVG: compiler accepts neither -march=native nor -mcpu=native, ignored")
endfunction()

# ---------------------------------------------------------------------------
# Build the (config-specific) flag list for the current compiler into <out>.
# ---------------------------------------------------------------------------
function(_novasvg_collect_flags out)
    set(_flags "")

    if(MSVC) # cl.exe and clang-cl (checked first: clang-cl reports ID "Clang" but takes MSVC-style flags)
        list(APPEND _flags
            "$<$<CONFIG:Release>:/O2>"
            "$<$<CONFIG:RelWithDebInfo>:/O2>"
            "$<$<CONFIG:MinSizeRel>:/O1>")

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang|IntelLLVM")
        list(APPEND _flags
            "$<$<CONFIG:Debug>:-Og>"
            "$<$<CONFIG:Debug>:-g3>"
            "$<$<CONFIG:Release>:-O3>"
            "$<$<CONFIG:RelWithDebInfo>:-O3>"
            "$<$<CONFIG:RelWithDebInfo>:-g>"
            "$<$<CONFIG:MinSizeRel>:-Os>")

        if(NOVASVG_KEEP_FRAME_POINTERS)
            list(APPEND _flags -fno-omit-frame-pointer)
        endif()

        if(NOVASVG_NATIVE_ARCH OR ENABLE_NATIVE) # ENABLE_NATIVE: legacy spelling
            _novasvg_native_arch_flag(_native)
            if(_native)
                list(APPEND _flags ${_native})
            endif()
        endif()

    else()
        message(STATUS "NovaSVG: no optimisation flags known for compiler '${CMAKE_CXX_COMPILER_ID}', "
                       "relying on CMake's defaults")
    endif()

    set(${out} "${_flags}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# novasvg_optimize(<target>)
#   INTERFACE library : flags go to INTERFACE_COMPILE_OPTIONS,
#                       so every target linking it is optimised.
#   anything else     : flags PRIVATE to the target, plus LTO when supported.
# ---------------------------------------------------------------------------
function(novasvg_optimize target)
    if(NOT NOVASVG_OPTIMIZE)
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "novasvg_optimize(): '${target}' is not a target")
    endif()

    get_target_property(_type ${target} TYPE)
    get_target_property(_imported ${target} IMPORTED)
    if(_imported)
        return()
    endif()

    _novasvg_collect_flags(_flags)

    if(_type STREQUAL "INTERFACE_LIBRARY")
        foreach(_f IN LISTS _flags)
            target_compile_options(${target} INTERFACE "${_f}")
        endforeach()
        return()
    endif()

    if(_flags)
        target_compile_options(${target} PRIVATE ${_flags})
    endif()

    get_property(_ipo GLOBAL PROPERTY NOVASVG_IPO_SUPPORTED)
    if(NOVASVG_ENABLE_LTO AND _ipo)
        set_target_properties(${target} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
            INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)
    endif()
endfunction()

# Backward-compatible name used by the previous version of this file.
function(apply_optimization_flags target)
    novasvg_optimize(${target})
endfunction()
