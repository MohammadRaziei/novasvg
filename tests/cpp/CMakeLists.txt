
# Find all test source files matching test_*.cpp pattern
file(GLOB NOVASVG_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp
)

add_executable(test_novasvg_cpp ${NOVASVG_TEST_SOURCES})

# Default: test against normal headers (source tree)
set(NOVASVG_TEST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

# test against amalgamated/single-header output
if(NOVASVG_DIST_DIR)
    message(STATUS "Testing against dist headers at: ${NOVASVG_DIST_DIR}")
    set(NOVASVG_TEST_INCLUDE_DIR "${NOVASVG_DIST_DIR}")
    add_dependencies(test_novasvg_cpp novasvg_dist_header)
endif()

target_include_directories(test_novasvg_cpp PRIVATE "${NOVASVG_TEST_INCLUDE_DIR}")

add_test(NAME test_novasvg_cpp COMMAND $<TARGET_FILE:test_novasvg_cpp>)

add_custom_target(novasvg_test_cpp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running c++ tests..."
    COMMAND $<TARGET_FILE:test_novasvg_cpp>
    WORKING_DIRECTORY ${NOVASVG_TEST_BINARY_DIR}
)
add_dependencies(novasvg_test_cpp test_novasvg_cpp)
add_dependencies(novasvg_test novasvg_test_cpp)


if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    # Enable coverage flags for the test executable
    target_compile_options(test_novasvg_cpp PRIVATE -O0 -g --coverage)
    target_link_options(test_novasvg_cpp PRIVATE --coverage)

    add_custom_target(novasvg_coverage_cpp
        # Clean previous coverage data
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html
                
        # Capture coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
        COMMAND ${LCOV_EXECUTABLE}
                --capture
                --directory ${NOVASVG_TEST_BINARY_DIR}
                --base-directory ${PROJECT_SOURCE_DIR}
                --no-external
                --rc function_coverage=1
                --output-file ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        
        # Filter for novasvg files only
        COMMAND ${CMAKE_COMMAND} -E echo "Filtering novasvg files..."
        COMMAND ${LCOV_EXECUTABLE}
                --extract
                ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                "*/novasvg/*"
                --output-file ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
        COMMAND ${CMAKE_COMMAND} -E remove ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        COMMAND ${LCOV_EXECUTABLE}
            --remove
            ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
            "*/tests/*"
            --output-file ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        COMMAND ${CMAKE_COMMAND} -E remove ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov

        # Generate HTML report
        COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
        COMMAND ${GENHTML_EXECUTABLE} --quiet 
                ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                --output-directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html
                --prefix ${PROJECT_SOURCE_DIR}
                --title "Novasvg C++ Coverage"
                --css-file ${NOVASVG_TEST_SOURCE_DIR}/genhtml.css
                --dark-mode 
        COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html/index.html"
        
        # Fix paths to be relative (for SonarQube, etc.)
        COMMAND ${CMAKE_COMMAND} -E echo "-- Fixing paths to relative..."
        COMMAND ${CMAKE_COMMAND} 
                -DCOV_FILE=${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                -DSOURCE_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
        # Summary
        COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
        COMMAND ${LCOV_EXECUTABLE}
                --list
                ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                
        WORKING_DIRECTORY ${NOVASVG_TEST_BINARY_DIR}
        USES_TERMINAL
    )
    
    add_dependencies(novasvg_coverage_cpp novasvg_test_cpp)
    add_dependencies(novasvg_coverage novasvg_coverage_cpp)
else()
    message(WARNING "LCOV or GENHTML executables not found.")
    message(WARNING "C++ coverage targets will be skipped.")
    message(WARNING "Please install them to enable coverage reports (e.g., 'sudo apt install lcov' on Linux).")
endif()