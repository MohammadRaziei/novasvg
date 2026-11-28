# EmbedFile.cmake
#
# Compiles binary files into the program: at CMake CONFIGURE time every file is read and written into one generated
# C++ header, as
#
#     inline constexpr unsigned char <n>[] = {0x2f, 0x2a, ...};   // the file's bytes
#     inline constexpr std::size_t   <n>_size = sizeof(<n>);      // its length
#
# so there is nothing to ship or to find on disk at run time, and `<n>` lives in .rodata. Byte arrays, not string
# literals: MSVC refuses long literals and the others warn (-Woverlength-strings) for anything over 64 KB. The
# files are embedded exactly as they are: to embed something compressed, commit it compressed.
# (Adapted from httpp's EmbedFile.cmake, without its compression.)
#
# Usage:
#   include(EmbedFile)
#   embed_files(
#       OUTPUT    ${CMAKE_CURRENT_BINARY_DIR}/generated/data.hpp
#       NAMESPACE nova::vg::detail
#       FILES     some_name /path/to/file   other_name /path/to/other)

set(_EMBED_FILE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# one file -> the C++ declarations in <out_var>
function(_embed_one NAME PATH OUT_VAR)
    if(NOT EXISTS "${PATH}")
        message(FATAL_ERROR "embed_files: ${PATH} does not exist")
    endif()
    file(SIZE "${PATH}" _size)
    if(_size EQUAL 0) # a zero-size array is ill-formed (MSVC C2466): one dummy byte, the size stays 0
        set(${OUT_VAR} "inline constexpr unsigned char ${NAME}[1] = {0};\ninline constexpr std::size_t ${NAME}_size = 0;" PARENT_SCOPE)
        return()
    endif()
    file(READ "${PATH}" _hex HEX)

    # the data is cut into whole lines of _chunk bytes plus a shorter tail, each handled on its own (the tail can't
    # be matched by a regex without a line-sized repeat, and CMake's regex has no {n})
    set(_chunk 24)
    math(EXPR _rem "${_size} % ${_chunk}")
    math(EXPR _head_chars "(${_size} - ${_rem}) * 2")
    string(SUBSTRING "${_hex}" 0 ${_head_chars} _head)
    string(SUBSTRING "${_hex}" ${_head_chars} -1 _tail)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _head "${_head}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _tail "${_tail}")
    string(REPEAT "0x[0-9a-f][0-9a-f]," ${_chunk} _unit)
    string(REGEX REPLACE "(${_unit})" "    \\1\n" _head "${_head}")
    if(_rem GREATER 0)
        set(_tail "    ${_tail}\n")
    endif()
    set(${OUT_VAR} "inline constexpr unsigned char ${NAME}[] = {\n${_head}${_tail}};\ninline constexpr std::size_t ${NAME}_size = sizeof(${NAME});" PARENT_SCOPE)
endfunction()

function(embed_files)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "OUTPUT;NAMESPACE" "FILES")
    if(ARG_UNPARSED_ARGUMENTS OR ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "embed_files: unknown or incomplete arguments: ${ARG_UNPARSED_ARGUMENTS} ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(NOT ARG_OUTPUT OR NOT ARG_NAMESPACE OR NOT ARG_FILES)
        message(FATAL_ERROR "embed_files: OUTPUT, NAMESPACE and FILES <n> <path> ... are required")
    endif()
    list(LENGTH ARG_FILES _n)
    math(EXPR _odd "${_n} % 2")
    if(_odd)
        message(FATAL_ERROR "embed_files: FILES takes pairs of <n> <path>")
    endif()

    set(CONTENT "")
    set(_i 0)
    while(_i LESS _n)
        math(EXPR _j "${_i} + 1")
        list(GET ARG_FILES ${_i} _name)
        list(GET ARG_FILES ${_j} _path)
        _embed_one("${_name}" "${_path}" _decl)
        string(APPEND CONTENT "${_decl}\n\n")
        # a changed data file re-runs CMake, which regenerates the header
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_path}")
        math(EXPR _i "${_i} + 2")
    endwhile()

    set(NAMESPACE "${ARG_NAMESPACE}")
    get_filename_component(_outdir "${ARG_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_outdir}")
    # configure_file only rewrites the header when its content changed, so an unchanged file costs no rebuild
    configure_file("${_EMBED_FILE_MODULE_DIR}/utils/embedded_files.in.hpp" "${ARG_OUTPUT}" @ONLY)
endfunction()
