# DynamicVersion.cmake
#
# CMake module for extracting version information from C/C++ header files
# using #define directives with optional configurable version prefix.
#
# Provides two functions:
#   - extract_version_components: extracts MAJOR, MINOR, PATCH separately
#   - extract_version_string: extracts full semantic version string (MAJOR.MINOR.PATCH)

# =============================================================================
# Function: extract_version_components
#
# Extracts version components (MAJOR, MINOR, PATCH) separately from a header file.
#
# Usage:
#   extract_version_components(
#       HEADER_FILE <path_to_header>
#       [VERSION_PREFIX <version_prefix>]  # Optional, defaults to ""
#       [MAJOR_VAR <output_var_for_major>]
#       [MINOR_VAR <output_var_for_minor>]
#       [PATCH_VAR <output_var_for_patch>]
#   )
# =============================================================================
function(extract_version_components)
    set(oneValueArgs HEADER_FILE VERSION_PREFIX MAJOR_VAR MINOR_VAR PATCH_VAR)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "extract_version_components: HEADER_FILE argument is required")
    endif()

    if(NOT EXISTS "${ARG_HEADER_FILE}")
        message(FATAL_ERROR "Header file '${ARG_HEADER_FILE}' not found")
    endif()

    file(READ "${ARG_HEADER_FILE}" HEADER_CONTENT)

    # Robust regex: [[:space:]] handles spaces/tabs; allows comments after value
    set(MAJOR_REGEX "#define[[:space:]]+${ARG_VERSION_PREFIX}VERSION_MAJOR[[:space:]]+([0-9]+)")
    set(MINOR_REGEX "#define[[:space:]]+${ARG_VERSION_PREFIX}VERSION_MINOR[[:space:]]+([0-9]+)")
    set(PATCH_REGEX "#define[[:space:]]+${ARG_VERSION_PREFIX}VERSION_PATCH[[:space:]]+([0-9]+)")

    # Extract MAJOR
    if(NOT HEADER_CONTENT MATCHES "${MAJOR_REGEX}")
        message(FATAL_ERROR "Could not find ${ARG_VERSION_PREFIX}VERSION_MAJOR in ${ARG_HEADER_FILE}\n"
                            "Regex used: ${MAJOR_REGEX}")
    endif()
    set(VERSION_MAJOR "${CMAKE_MATCH_1}")
    if(ARG_MAJOR_VAR)
        set(${ARG_MAJOR_VAR} "${VERSION_MAJOR}" PARENT_SCOPE)
    endif()

    # Extract MINOR
    if(NOT HEADER_CONTENT MATCHES "${MINOR_REGEX}")
        message(FATAL_ERROR "Could not find ${ARG_VERSION_PREFIX}VERSION_MINOR in ${ARG_HEADER_FILE}\n"
                            "Regex used: ${MINOR_REGEX}")
    endif()
    set(VERSION_MINOR "${CMAKE_MATCH_1}")
    if(ARG_MINOR_VAR)
        set(${ARG_MINOR_VAR} "${VERSION_MINOR}" PARENT_SCOPE)
    endif()

    # Extract PATCH
    if(NOT HEADER_CONTENT MATCHES "${PATCH_REGEX}")
        message(FATAL_ERROR "Could not find ${ARG_VERSION_PREFIX}VERSION_PATCH in ${ARG_HEADER_FILE}\n"
                            "Regex used: ${PATCH_REGEX}")
    endif()
    set(VERSION_PATCH "${CMAKE_MATCH_1}")
    if(ARG_PATCH_VAR)
        set(${ARG_PATCH_VAR} "${VERSION_PATCH}" PARENT_SCOPE)
    endif()
endfunction()

# =============================================================================
# Function: extract_version_string
#
# Extracts full semantic version string (MAJOR.MINOR.PATCH) from a header file.
#
# Usage:
#   extract_version_string(
#       HEADER_FILE <path_to_header>
#       [VERSION_PREFIX <version_prefix>]  # Optional, defaults to ""
#       OUTPUT_VAR <output_variable_name>
#   )
# =============================================================================
function(extract_version_string)
    set(oneValueArgs HEADER_FILE VERSION_PREFIX OUTPUT_VAR)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "extract_version_string: HEADER_FILE argument is required")
    endif()
    if(NOT ARG_OUTPUT_VAR)
        message(FATAL_ERROR "extract_version_string: OUTPUT_VAR argument is required")
    endif()

    # CRITICAL FIX: Pass VERSION_PREFIX (not PREFIX!) to child function
    extract_version_components(
        HEADER_FILE "${ARG_HEADER_FILE}"
        VERSION_PREFIX "${ARG_VERSION_PREFIX}"  # ← CORRECT ARGUMENT NAME
        MAJOR_VAR _ver_major
        MINOR_VAR _ver_minor
        PATCH_VAR _ver_patch
    )

    set(VERSION_STRING "${_ver_major}.${_ver_minor}.${_ver_patch}")
    set(${ARG_OUTPUT_VAR} "${VERSION_STRING}" PARENT_SCOPE)
endfunction()