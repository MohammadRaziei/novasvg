# DynamicVersion.cmake
#
# CMake module for extracting version information from C/C++ header files
# using #define directives with optional configurable prefix.
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
#       [PREFIX <version_prefix>]          # Optional, defaults to ""
#       [MAJOR_VAR <output_var_for_major>]
#       [MINOR_VAR <output_var_for_minor>]
#       [PATCH_VAR <output_var_for_patch>]
#   )
#
# Arguments:
#   HEADER_FILE - Path to the header file containing version defines (required)
#   PREFIX      - Prefix used in version defines (e.g., "NOVASVG_"). Optional, defaults to empty string.
#   MAJOR_VAR   - Variable name to store MAJOR version (optional)
#   MINOR_VAR   - Variable name to store MINOR version (optional)
#   PATCH_VAR   - Variable name to store PATCH version (optional)
#
# Examples:
#   # With prefix
#   extract_version_components(
#       HEADER_FILE "include/novasvg/novasvg.h"
#       PREFIX "NOVASVG_"
#       MAJOR_VAR VER_MAJOR
#   )
#
#   # Without prefix (defines like VERSION_MAJOR)
#   extract_version_components(
#       HEADER_FILE "version.h"
#       MAJOR_VAR VER_MAJOR
#   )
# =============================================================================
function(extract_version_components)
    set(oneValueArgs HEADER_FILE PREFIX MAJOR_VAR MINOR_VAR PATCH_VAR)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    # Validate required arguments
    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "extract_version_components: HEADER_FILE argument is required")
    endif()

    # Set default prefix to empty string if not provided
    if(NOT DEFINED ARG_PREFIX)
        set(ARG_PREFIX "")
    endif()

    # Check file existence
    if(NOT EXISTS "${ARG_HEADER_FILE}")
        message(FATAL_ERROR "Header file '${ARG_HEADER_FILE}' not found")
    endif()

    # Read header content
    file(READ "${ARG_HEADER_FILE}" HEADER_CONTENT)

    # Build regex patterns using the provided prefix (may be empty)
    set(MAJOR_REGEX "#define[ \t]+${ARG_PREFIX}VERSION_MAJOR[ \t]+([0-9]+)")
    set(MINOR_REGEX "#define[ \t]+${ARG_PREFIX}VERSION_MINOR[ \t]+([0-9]+)")
    set(PATCH_REGEX "#define[ \t]+${ARG_PREFIX}VERSION_PATCH[ \t]+([0-9]+)")

    # Extract MAJOR version
    if(HEADER_CONTENT MATCHES "${MAJOR_REGEX}")
        set(VERSION_MAJOR "${CMAKE_MATCH_1}")
        if(ARG_MAJOR_VAR)
            set(${ARG_MAJOR_VAR} "${VERSION_MAJOR}" PARENT_SCOPE)
        endif()
    else()
        message(FATAL_ERROR "Could not find ${ARG_PREFIX}VERSION_MAJOR in ${ARG_HEADER_FILE}")
    endif()

    # Extract MINOR version
    if(HEADER_CONTENT MATCHES "${MINOR_REGEX}")
        set(VERSION_MINOR "${CMAKE_MATCH_1}")
        if(ARG_MINOR_VAR)
            set(${ARG_MINOR_VAR} "${VERSION_MINOR}" PARENT_SCOPE)
        endif()
    else()
        message(FATAL_ERROR "Could not find ${ARG_PREFIX}VERSION_MINOR in ${ARG_HEADER_FILE}")
    endif()

    # Extract PATCH version
    if(HEADER_CONTENT MATCHES "${PATCH_REGEX}")
        set(VERSION_PATCH "${CMAKE_MATCH_1}")
        if(ARG_PATCH_VAR)
            set(${ARG_PATCH_VAR} "${VERSION_PATCH}" PARENT_SCOPE)
        endif()
    else()
        message(FATAL_ERROR "Could not find ${ARG_PREFIX}VERSION_PATCH in ${ARG_HEADER_FILE}")
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
#       [PREFIX <version_prefix>]          # Optional, defaults to ""
#       OUTPUT_VAR <output_variable_name>
#   )
#
# Arguments:
#   HEADER_FILE - Path to the header file containing version defines (required)
#   PREFIX      - Prefix used in version defines (e.g., "NOVASVG_"). Optional, defaults to empty string.
#   OUTPUT_VAR  - Variable name to store the full version string (required)
#
# Examples:
#   # With prefix
#   extract_version_string(
#       HEADER_FILE "include/novasvg/novasvg.h"
#       PREFIX "NOVASVG_"
#       OUTPUT_VAR PROJECT_VERSION
#   )
#
#   # Without prefix
#   extract_version_string(
#       HEADER_FILE "version.h"
#       OUTPUT_VAR PROJECT_VERSION
#   )
# =============================================================================
function(extract_version_string)
    set(oneValueArgs HEADER_FILE PREFIX OUTPUT_VAR)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    # Validate required arguments
    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "extract_version_string: HEADER_FILE argument is required")
    endif()
    if(NOT ARG_OUTPUT_VAR)
        message(FATAL_ERROR "extract_version_string: OUTPUT_VAR argument is required")
    endif()

    # Set default prefix to empty string if not provided
    if(NOT DEFINED ARG_PREFIX)
        set(ARG_PREFIX "")
    endif()

    # Reuse the component extraction function internally
    extract_version_components(
        HEADER_FILE "${ARG_HEADER_FILE}"
        PREFIX "${ARG_PREFIX}"
        MAJOR_VAR _ver_major
        MINOR_VAR _ver_minor
        PATCH_VAR _ver_patch
    )

    # Construct full version string
    set(VERSION_STRING "${_ver_major}.${_ver_minor}.${_ver_patch}")
    set(${ARG_OUTPUT_VAR} "${VERSION_STRING}" PARENT_SCOPE)
endfunction()