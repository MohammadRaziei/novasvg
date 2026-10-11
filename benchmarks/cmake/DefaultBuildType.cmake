# Timings of an unoptimised build are meaningless: with an empty
# CMAKE_BUILD_TYPE the Makefile/Ninja generators add no -O flag at all (-O0).
# This directory is a top-level project, so it owns the build type for
# everything it builds (the novasvg driver, lunasvg, nanosvg, thorvg's driver...)
# and every engine gets the same, fair Release flags. An explicit
# -DCMAKE_BUILD_TYPE=... always wins. Multi-config generators are untouched
# (build them with --config Release).
get_property(_novasvg_bench_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _novasvg_bench_multi AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (defaulted to Release by the benchmark)" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
    message(STATUS "novasvg_bench: CMAKE_BUILD_TYPE was empty -> defaulting to Release")
endif()
