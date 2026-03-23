
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
    add_dependencies(test_novasvg_cpp dist_header)
endif()

target_include_directories(test_novasvg_cpp PRIVATE "${NOVASVG_TEST_INCLUDE_DIR}")

add_test(NAME test_novasvg_cpp COMMAND $<TARGET_FILE:test_novasvg_cpp>)


find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)


if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    target_compile_options(test_novasvg_cpp PRIVATE -O0 -g --coverage)
    target_link_options(test_novasvg_cpp PRIVATE --coverage)

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "-- Running tests..."
        COMMAND $<TARGET_FILE:test_novasvg_cpp>

        COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
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
                "*/novasvg/*"
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
    add_dependencies(coverage test_novasvg_cpp)

endif()
