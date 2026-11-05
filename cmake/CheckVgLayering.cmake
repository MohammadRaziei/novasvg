# nova::vg (src/sources/vg, include/nova/vg, include/nova/config.h) must not know the svg layer (ADR-0009).
# Run as: cmake -DNOVA_ROOT=<source dir> -P CheckVgLayering.cmake
file(GLOB_RECURSE _files
    "${NOVA_ROOT}/src/sources/vg/*.cpp" "${NOVA_ROOT}/src/sources/vg/*.h"
    "${NOVA_ROOT}/include/nova/vg/*.h" "${NOVA_ROOT}/include/nova/config.h")
set(_bad "")
foreach(_f IN LISTS _files)
    file(STRINGS "${_f}" _lines)
    foreach(_line IN LISTS _lines)
        if(_line MATCHES "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"]*(svg/|nova/svg|svgelement|svgparser|novasvg\\.h)")
            list(APPEND _bad "${_f}: ${_line}")
        elseif(_line MATCHES "nova::svg|namespace svg")
            list(APPEND _bad "${_f}: ${_line}")
        endif()
    endforeach()
endforeach()
if(_bad)
    list(JOIN _bad "\n  " _text)
    message(FATAL_ERROR "nova::vg refers to the svg layer:\n  ${_text}")
endif()
list(LENGTH _files _n)
message(STATUS "nova::vg layering OK (${_n} files checked)")
