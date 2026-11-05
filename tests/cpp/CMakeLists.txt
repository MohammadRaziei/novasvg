
# Find all test source files matching test_*.cpp pattern
file(GLOB NOVA_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp
)

# TEMPORARY refactor hook: verification tests that only make sense while the refactor runs live in the
# gitignored /refactor/temp-tests and are compiled in when present. Remove this block when the refactor ends.
file(GLOB NOVA_REFACTOR_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/refactor/temp-tests/test_*.cpp)
list(APPEND NOVA_TEST_SOURCES ${NOVA_REFACTOR_TEST_SOURCES})

add_executable(test_novasvg_cpp ${NOVA_TEST_SOURCES})

# Default: test against normal headers (source tree)
set(NOVA_TEST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

# test against amalgamated/single-header output
if(NOVA_DIST_DIR)
    message(STATUS "Testing against dist headers at: ${NOVA_DIST_DIR}")
    set(NOVA_TEST_INCLUDE_DIR "${NOVA_DIST_DIR}")
    add_dependencies(test_novasvg_cpp novasvg_dist_header)
endif()

target_include_directories(test_novasvg_cpp PRIVATE "${NOVA_TEST_INCLUDE_DIR}" "${PROJECT_SOURCE_DIR}/src/sources")
# These two take their headers from NOVA_TEST_INCLUDE_DIR (which can be the dist header), so they link
# libstb directly instead of going through the novasvg target.
target_link_libraries(test_novasvg_cpp PRIVATE novasvg::noflag)

add_test(NAME test_novasvg_cpp COMMAND $<TARGET_FILE:test_novasvg_cpp>)

# The embedded fallback font only matters when the machine has no fonts, so this check is built with system
# font loading compiled out and runs as its own executable (the main one deliberately sees system fonts).
add_executable(test_novasvg_embedded_font embedded_font/check_embedded_font.cpp)
target_include_directories(test_novasvg_embedded_font PRIVATE "${NOVA_TEST_INCLUDE_DIR}" "${PROJECT_SOURCE_DIR}/src/sources")
target_link_libraries(test_novasvg_embedded_font PRIVATE novasvg::noflag)
target_compile_definitions(test_novasvg_embedded_font PRIVATE
    NOVA_DISABLE_LOAD_SYSTEM_FONTS
    NOVA_TEST_FONT_PATH="${PROJECT_SOURCE_DIR}/data/fonts/DejaVuSans.ttf")
add_test(NAME test_novasvg_embedded_font COMMAND $<TARGET_FILE:test_novasvg_embedded_font>)

# novasvg links libstb dynamically. This one is built against novasvg::noflag only, so it proves the
# include path and link line travel with the target and that libstb is a separate shared object.
add_executable(test_novasvg_libstb libstb_link/check_libstb_link.cpp)
target_link_libraries(test_novasvg_libstb PRIVATE novasvg::noflag ${CMAKE_DL_LIBS})
add_test(NAME test_novasvg_libstb COMMAND $<TARGET_FILE:test_novasvg_libstb>)

add_custom_target(novasvg_test_cpp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running c++ tests..."
    COMMAND $<TARGET_FILE:test_novasvg_cpp>
    COMMAND $<TARGET_FILE:test_novasvg_embedded_font>
    COMMAND $<TARGET_FILE:test_novasvg_libstb>
    WORKING_DIRECTORY ${NOVA_TEST_BINARY_DIR}
)
add_dependencies(novasvg_test_cpp test_novasvg_cpp test_novasvg_embedded_font test_novasvg_libstb)
add_dependencies(novasvg_test novasvg_test_cpp)


if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    # Enable coverage flags for the test executable
    target_compile_options(test_novasvg_cpp PRIVATE -O0 -g --coverage)
    target_link_options(test_novasvg_cpp PRIVATE --coverage)

    add_custom_target(novasvg_coverage_cpp
        # Clean previous coverage data (only html, keep index.html wrapper)
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${NOVA_COVERAGE_CPP_BINARY_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVA_COVERAGE_CPP_BINARY_DIR}/html

        # Capture coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
        COMMAND ${LCOV_EXECUTABLE}
                --capture
                --base-directory ${PROJECT_SOURCE_DIR}
                --directory ${CMAKE_CURRENT_BINARY_DIR}
                --exclude "*/tests/*"
                --no-external
                --rc function_coverage=1
                --output-file ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
        # Extract only novasvg source files
        COMMAND ${CMAKE_COMMAND} -E echo "Filtering novasvg source files..."
        COMMAND ${LCOV_EXECUTABLE}
                --extract
                ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
                "*/novasvg/*"
                --output-file ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        COMMAND ${CMAKE_COMMAND} -E remove ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov

        # Generate HTML report
        COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
        COMMAND ${GENHTML_EXECUTABLE} --quiet
                ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                --output-directory ${NOVA_COVERAGE_CPP_BINARY_DIR}/html
                --prefix ${PROJECT_SOURCE_DIR}
                --title "NovaSVG C++ Coverage"
                --css-file ${NOVA_TEST_SOURCE_DIR}/genhtml.css
                --dark-mode
        COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${NOVA_COVERAGE_CPP_BINARY_DIR}/html/index.html"

        # Fix paths to be relative (for SonarQube, etc.)
        COMMAND ${CMAKE_COMMAND} -E echo "-- Fixing paths to relative..."
        COMMAND ${CMAKE_COMMAND}
                -DCOV_FILE=${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                -DSOURCE_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
        # Summary
        COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
        COMMAND ${LCOV_EXECUTABLE}
                --list
                ${NOVA_COVERAGE_CPP_BINARY_DIR}/coverage.lcov

        WORKING_DIRECTORY ${NOVA_TEST_BINARY_DIR}
        USES_TERMINAL
    )

    add_dependencies(novasvg_coverage_cpp novasvg_test_cpp)
    add_dependencies(novasvg_coverage novasvg_coverage_cpp)

    # Generate iframe wrapper using GenerateIframe.cmake
    file(MAKE_DIRECTORY ${NOVA_COVERAGE_CPP_BINARY_DIR})

    # Add custom command to run GenerateIframe.cmake after coverage target
    add_custom_command(TARGET novasvg_coverage_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DIFRAME_SRC="html/index.html"
                -DSECTION_TITLE="C++%20Core"
                -DLOGO_SVG="${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg"
                -DPROJECT_NAME="${PROJECT_NAME}"
                -DOUTPUT_DIR="${NOVA_COVERAGE_CPP_BINARY_DIR}"
                -P "${PROJECT_SOURCE_DIR}/cmake/GenerateIframe.cmake"
        COMMENT "Generating iframe wrapper for C++ coverage report"
        WORKING_DIRECTORY ${NOVA_TEST_SOURCE_DIR}
    )
else()
    message(WARNING "LCOV or GENHTML executables not found.")
    message(WARNING "C++ coverage targets will be skipped.")
    message(WARNING "Please install them to enable coverage reports (e.g., 'sudo apt install lcov' on Linux).")
endif()
