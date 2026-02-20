cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(PyProject)
include(DynamicVersion)

extract_version_string(
    HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h"
    VERSION_PREFIX "NOVASVG_"
    OUTPUT_VAR PROJECT_VERSION
)

project(${PyProject_NAME} LANGUAGES CXX VERSION ${PROJECT_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")
include(Startup)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(NOVASVG_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_CLI "Build command line interface" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_DOCS "Build documentation" OFF)




# Header-only library (interface target)
add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)

target_include_directories(novasvg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# target_compile_features(novasvg INTERFACE cxx_std_17)


# Generate single-header distribution if NOVASVG_DIST_DIR is defined
if(DEFINED NOVASVG_DIST_DIR)
    add_custom_target(dist_header
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_DIST_DIR}/${PROJECT_NAME}
        COMMAND quom ${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h 
            ${NOVASVG_DIST_DIR}/${PROJECT_NAME}/novasvg.h 
            -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMENT "Generating single-header distribution: ${NOVASVG_DIST_DIR}/${PROJECT_NAME}/novasvg.h"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Tests
if(NOVASVG_BUILD_TESTS)
    enable_testing()

    # Find all test source files matching test_*.cpp pattern
    file(GLOB NOVASVG_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/test_*.cpp
    )
    
    add_executable(test_novasvg ${NOVASVG_TEST_SOURCES})

    # Default: test against normal headers (source tree)
    set(NOVASVG_TEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

    # test against amalgamated/single-header output
    if(NOVASVG_DIST_DIR)
        message(STATUS "Testing against dist headers at: ${NOVASVG_DIST_DIR}")
        set(NOVASVG_TEST_INCLUDE_DIR "${NOVASVG_DIST_DIR}")
        add_dependencies(test_novasvg dist_header)
        
    endif()

    target_include_directories(test_novasvg PRIVATE "${NOVASVG_TEST_INCLUDE_DIR}")

    add_test(NAME test_novasvg COMMAND $<TARGET_FILE:test_novasvg>)


    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)


    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        target_compile_options(test_novasvg PRIVATE -O0 -g --coverage)
        target_link_options(test_novasvg PRIVATE --coverage)

        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Running tests..."
            COMMAND $<TARGET_FILE:test_novasvg>

            COMMAND ${CMAKE_COMMAND} -E echo "Capturing coverage..."
            COMMAND ${LCOV_EXECUTABLE}
                    --capture
                    --directory ${CMAKE_BINARY_DIR}
                    --base-directory ${CMAKE_SOURCE_DIR}
                    --no-external
                    --rc function_coverage=1
                    --output-file ${CMAKE_BINARY_DIR}/coverage.info

            COMMAND ${CMAKE_COMMAND} -E echo "Filtering novasvg files..."
            COMMAND ${LCOV_EXECUTABLE}
                    --extract
                    ${CMAKE_BINARY_DIR}/coverage.info
                    "*/include/novasvg/*"
                    --output-file ${CMAKE_BINARY_DIR}/coverage_novasvg.info

            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage_html
            COMMAND ${GENHTML_EXECUTABLE}
                    ${CMAKE_BINARY_DIR}/coverage_novasvg.info
                    --output-directory ${CMAKE_BINARY_DIR}/coverage_html

            COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${CMAKE_BINARY_DIR}/coverage_html/index.html"

            COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
            COMMAND ${LCOV_EXECUTABLE}
                    --list
                    ${CMAKE_BINARY_DIR}/coverage_novasvg.info

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
        )
        add_dependencies(coverage test_novasvg)

    endif()
endif()


# Examples
if(NOVASVG_BUILD_EXAMPLES)
    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/cpp/sample*.cpp
    )

    foreach(SOURCE_FILE IN LISTS EXAMPLE_SOURCES)
        get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
        add_executable(${TARGET_NAME} ${SOURCE_FILE})
        target_link_libraries(${TARGET_NAME} PRIVATE novasvg)
        message(STATUS "Added example target: ${TARGET_NAME}")
    endforeach()
endif()


# Command Line Interface
if(NOVASVG_BUILD_CLI)
    # Add CLI executable
    add_executable(novasvg_cli src/novasvg_cli.cpp)
    
    target_link_libraries(novasvg_cli PRIVATE novasvg)
    
    # Set output name to just 'novasvg' for easier use
    set_target_properties(novasvg_cli PROPERTIES OUTPUT_NAME novasvg)
    
    message(STATUS "Added CLI target: novasvg_cli (output: novasvg)")
    
    # Installation
    install(TARGETS novasvg_cli
        RUNTIME DESTINATION bin
    )
endif()

# Documentation (Doxygen)
if(NOVASVG_BUILD_DOCS)
    # Find Doxygen
    find_package(Doxygen)
    
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
        cmake_policy(SET CMP0135 NEW)
        
        include(FetchContent)
        FetchContent_Declare(
            doxygen-awesome-css
            URL https://github.com/jothepro/doxygen-awesome-css/archive/main.zip
            # DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            URL_HASH SHA256=286f8008c08bb80f88910b3333af763a20724e90c2fd70dd9ee24071e1856965
        )
        FetchContent_MakeAvailable(doxygen-awesome-css)

        # Save the location the files were cloned into
        # This allows us to get the path to doxygen-awesome.css
        FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)


        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        
        # Print documentation location after build
        add_custom_command(TARGET docs POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Documentation generated at: ${CMAKE_CURRENT_BINARY_DIR}/docs/html/index.html"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found - documentation will not be generated")
    endif()
endif()


# Installation
install(DIRECTORY include DESTINATION include)

