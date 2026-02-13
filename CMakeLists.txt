cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(PyProject)

project(${PyProject_NAME} LANGUAGES CXX VERSION ${PyProject_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")
include(Startup)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(NOVASVG_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(NOVASVG_BUILD_DOCS "Build documentation" ${PROJECT_IS_TOP_LEVEL})




# Header-only library (interface target)
add_library(novasvg INTERFACE)
add_library(novasvg::novasvg ALIAS novasvg)

target_include_directories(novasvg INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# target_compile_features(novasvg INTERFACE cxx_std_17)

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

# Tests
if(NOVASVG_BUILD_TESTS)
    enable_testing()
    
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
        message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
        
        include(FetchContent)
        FetchContent_Declare(
            doxygen-awesome-css
            URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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

