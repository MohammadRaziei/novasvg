cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(PyProject)

project(${PyProject_NAME} LANGUAGES CXX VERSION ${PyProject_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")


# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(NOVASVG_BUILD_EXAMPLES "Build examples" ON)
option(NOVASVG_BUILD_TESTS "Build tests" ON)
option(NOVASVG_BUILD_DOCS "Build documentation" ON)




# Header-only library (interface target)
add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)

target_include_directories(novasvg INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(novasvg INTERFACE cxx_std_17)

# Examples
if(NOVASVG_BUILD_EXAMPLES)
    add_executable(example_svg2png examples/cpp/sample01_convert_svg_to_png.cpp)
    add_executable(example_query_size examples/cpp/sample02_query_svg_size.cpp)
    
    target_link_libraries(example_svg2png novasvg)
    target_link_libraries(example_query_size novasvg)
endif()

# Tests
if(NOVASVG_BUILD_TESTS)
    # Add test executable
    add_executable(test_novasvg tests/cpp/test_basics.cpp)
    
    target_link_libraries(test_novasvg novasvg)
    
    # Add test command
    add_test(NAME test_novasvg COMMAND test_novasvg)
endif()

# Documentation (Doxygen)
if(NOVASVG_BUILD_DOCS)
    # Find Doxygen
    find_package(Doxygen)
    
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found - documentation will not be generated")
    endif()
endif()

# Installation
install(DIRECTORY include DESTINATION include)

