cmake_minimum_required(VERSION 3.19)

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

# Project options
option(NOVASVG_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_PYTHON "Build Python" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_DOCS "Build documentation" OFF)


set(NOVASVG_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# Header-only library (interface target)
add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)

target_include_directories(novasvg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add CLI executable
add_executable(novasvg_cli src/novasvg_cli.cpp)
target_link_libraries(novasvg_cli PRIVATE novasvg::novasvg)
set_target_properties(novasvg_cli PROPERTIES OUTPUT_NAME novasvg-cli)


# target_compile_features(novasvg INTERFACE cxx_std_17)

function(generate_single_header_distribution dist_path)
    add_custom_target(dist_header
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dist_path}/${PROJECT_NAME}
        COMMAND quom ${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h 
            ${NOVdist_pathASVG_DIST_DIR}/${PROJECT_NAME}/novasvg.h 
            -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMENT "Generating single-header distribution: ${dist_path}/${PROJECT_NAME}/novasvg.h"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endfunction()

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
    generate_single_header_distribution(${NOVASVG_DIST_DIR})
endif()

# Tests
if(NOVASVG_BUILD_TESTS)
    enable_testing()
    set(CMAKE_CTEST_ARGUMENTS "--output-on-failure" "-V")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
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


# Documentation (Doxygen)
if(NOVASVG_BUILD_DOCS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/docs)
endif()


# Installation
install(DIRECTORY include DESTINATION include)

