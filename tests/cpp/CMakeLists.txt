
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

# The embedded fallback font only matters when the machine has no fonts, so this check is built with system
# font loading compiled out and runs as its own executable (the main one deliberately sees system fonts).
add_executable(test_novasvg_embedded_font embedded_font/check_embedded_font.cpp)
target_include_directories(test_novasvg_embedded_font PRIVATE "${NOVASVG_TEST_INCLUDE_DIR}")
target_compile_definitions(test_novasvg_embedded_font PRIVATE
    NOVASVG_DISABLE_LOAD_SYSTEM_FONTS
    NOVASVG_TEST_FONT_PATH="${PROJECT_SOURCE_DIR}/data/fonts/DejaVuSans.ttf")
add_test(NAME test_novasvg_embedded_font COMMAND $<TARGET_FILE:test_novasvg_embedded_font>)

add_custom_target(novasvg_test_cpp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running c++ tests..."
    COMMAND $<TARGET_FILE:test_novasvg_cpp>
    COMMAND $<TARGET_FILE:test_novasvg_embedded_font>
    WORKING_DIRECTORY ${NOVASVG_TEST_BINARY_DIR}
)
add_dependencies(novasvg_test_cpp test_novasvg_cpp test_novasvg_embedded_font)
add_dependencies(novasvg_test novasvg_test_cpp)


if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    # Enable coverage flags for the test executable
    target_compile_options(test_novasvg_cpp PRIVATE -O0 -g --coverage)
    target_link_options(test_novasvg_cpp PRIVATE --coverage)

    add_custom_target(novasvg_coverage_cpp
        # Clean previous coverage data (only html, keep index.html wrapper)
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html

        # Capture coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
        COMMAND ${LCOV_EXECUTABLE}
                --capture
                --base-directory ${PROJECT_SOURCE_DIR}
                --directory ${CMAKE_CURRENT_BINARY_DIR}
                --exclude "*/tests/*"
                --no-external
                --rc function_coverage=1
                --output-file ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
        # Extract only novasvg source files
        COMMAND ${CMAKE_COMMAND} -E echo "Filtering novasvg source files..."
        COMMAND ${LCOV_EXECUTABLE}
                --extract
                ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
                "*/novasvg/*"
                --output-file ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        COMMAND ${CMAKE_COMMAND} -E remove ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov

        # Generate HTML report
        COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
        COMMAND ${GENHTML_EXECUTABLE} --quiet
                ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                --output-directory ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/html
                --prefix ${PROJECT_SOURCE_DIR}
                --title "NovaSVG C++ Coverage"
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

    # Generate iframe wrapper using GenerateIframe.cmake
    file(MAKE_DIRECTORY ${NOVASVG_COVERAGE_CPP_BINARY_DIR})

    # Add custom command to run GenerateIframe.cmake after coverage target
    add_custom_command(TARGET novasvg_coverage_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DIFRAME_SRC="html/index.html"
                -DSECTION_TITLE="C++%20Core"
                -DLOGO_SVG="${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg"
                -DPROJECT_NAME="${PROJECT_NAME}"
                -DOUTPUT_DIR="${NOVASVG_COVERAGE_CPP_BINARY_DIR}"
                -P "${PROJECT_SOURCE_DIR}/cmake/GenerateIframe.cmake"
        COMMENT "Generating iframe wrapper for C++ coverage report"
        WORKING_DIRECTORY ${NOVASVG_TEST_SOURCE_DIR}
    )
else()
    message(WARNING "LCOV or GENHTML executables not found.")
    message(WARNING "C++ coverage targets will be skipped.")
    message(WARNING "Please install them to enable coverage reports (e.g., 'sudo apt install lcov' on Linux).")
endif()
