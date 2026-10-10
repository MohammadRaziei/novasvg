set(NOVASVG_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(NOVASVG_TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
set(NOVASVG_COVERAGE_BINARY_DIR ${CMAKE_BINARY_DIR}/coverage)

set(NOVASVG_COVERAGE_TOTAL_BINARY_DIR ${NOVASVG_COVERAGE_BINARY_DIR}/total CACHE PATH "Total coverage output directory")
set(NOVASVG_COVERAGE_CPP_BINARY_DIR ${NOVASVG_COVERAGE_BINARY_DIR}/cpp CACHE PATH "C++ coverage output directory")
set(NOVASVG_COVERAGE_PYTHON_BINARY_DIR ${NOVASVG_COVERAGE_BINARY_DIR}/python CACHE PATH "Python coverage output directory")

file(REMOVE_RECURSE ${NOVASVG_COVERAGE_BINARY_DIR})
file(MAKE_DIRECTORY ${NOVASVG_COVERAGE_BINARY_DIR})

add_custom_target(novasvg_test
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running All tests..."
)

add_custom_target(novasvg_coverage
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running coverage for all languages ..."
)


find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

add_subdirectory(${NOVASVG_TEST_SOURCE_DIR}/cpp)
add_subdirectory(${NOVASVG_TEST_SOURCE_DIR}/python)

if(TARGET novasvg_coverage_cpp AND TARGET novasvg_coverage_python)
    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)

        # Build the list of --add-tracefile arguments dynamically so more
        # languages can be added later by appending to these lists.
        set(LCOV_MERGE_ARGS
            --add-tracefile ${NOVASVG_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
            --add-tracefile ${NOVASVG_COVERAGE_PYTHON_BINARY_DIR}/coverage.lcov
        )
        set(LCOV_MERGE_DEPS novasvg_coverage_cpp novasvg_coverage_python)
        set(LCOV_MERGE_ECHO "-- Merging C++ and Python coverage...")

        add_custom_target(novasvg_coverage_total
            # Clean previous coverage data
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/html
            # Merge all coverage info files
            COMMAND ${CMAKE_COMMAND} -E echo ${LCOV_MERGE_ECHO}
            COMMAND ${LCOV_EXECUTABLE}
                    ${LCOV_MERGE_ARGS}
                    --output-file ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
            COMMAND ${CMAKE_COMMAND} -E echo "-- Total coverage info created at: ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov"

            # Generate HTML report
            COMMAND ${CMAKE_COMMAND}
                -DCOV_FILE=${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
                -DPREFIX_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
            COMMAND ${GENHTML_EXECUTABLE} --quiet
                    ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
                    --output-directory ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/html
                    --prefix ${PROJECT_SOURCE_DIR}
                    --title "NovaSVG Total Coverage"
                    --css-file ${NOVASVG_TEST_SOURCE_DIR}/genhtml.css
                    --dark-mode
            COMMAND ${CMAKE_COMMAND}
                -DCOV_FILE=${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
                -DSOURCE_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
            COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}/html/index.html"

            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            USES_TERMINAL
        )

        foreach(_dep IN LISTS LCOV_MERGE_DEPS)
            add_dependencies(novasvg_coverage_total ${_dep})
        endforeach()

        add_dependencies(novasvg_coverage novasvg_coverage_total)

        add_custom_command(TARGET novasvg_coverage_total POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DIFRAME_SRC="html/index.html"
                -DSECTION_TITLE="Total%20Coverage"
                -DLOGO_SVG="${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg"
                -DPROJECT_NAME="${PROJECT_NAME}"
                -DOUTPUT_DIR="${NOVASVG_COVERAGE_TOTAL_BINARY_DIR}"
                -P "${PROJECT_SOURCE_DIR}/cmake/GenerateIframe.cmake"
            COMMENT "Generating iframe wrapper for total coverage report"
            WORKING_DIRECTORY ${NOVASVG_TEST_SOURCE_DIR}
        )
    endif()
endif()

# --- Generate Coverage Dashboard HTML ---
set(COVERAGE_DATA_JSON "[")

if(TARGET novasvg_coverage_total)
    set(COVERAGE_DATA_JSON "${COVERAGE_DATA_JSON}
        {
            \"id\": \"total\",
            \"title\": \"Total Coverage\",
            \"badge\": \"Merged\",
            \"badgeClass\": \"total\",
            \"color\": \"#b07dd0\",
            \"desc\": \"Combined coverage report for the C++ and Python codebases. Useful for SonarQube analysis.\",
            \"htmlLink\": \"total/index.html\",
            \"lcovLink\": \"total/coverage.lcov\"
        },")
endif()

if(TARGET novasvg_coverage_cpp)
    set(COVERAGE_DATA_JSON "${COVERAGE_DATA_JSON}
        {
            \"id\": \"cpp\",
            \"title\": \"C++ Core\",
            \"badge\": \"Native\",
            \"color\": \"#5aa7f0\",
            \"desc\": \"Detailed coverage report for the core C++ library, including headers and implementation files generated by LCOV.\",
            \"htmlLink\": \"cpp/index.html\",
            \"lcovLink\": \"cpp/coverage.lcov\"
        },")
endif()

if(TARGET novasvg_coverage_python)
    set(COVERAGE_DATA_JSON "${COVERAGE_DATA_JSON}
        {
            \"id\": \"py\",
            \"title\": \"Python Bindings\",
            \"badge\": \"Interpreter\",
            \"color\": \"#e06bbd\",
            \"desc\": \"Coverage analysis for Python bindings and high-level logic, generated by coverage.py.\",
            \"htmlLink\": \"python/index.html\",
            \"lcovLink\": \"python/coverage.lcov\"
        },")
endif()

set(COVERAGE_DATA_JSON "${COVERAGE_DATA_JSON}]")

set(NOVASVG_LOGO_SVG "${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg")
if(EXISTS "${NOVASVG_LOGO_SVG}")
    file(READ "${NOVASVG_LOGO_SVG}" LOGO_CONTENT)
else()
    set(LOGO_CONTENT "")
endif()

configure_file(
    ${NOVASVG_TEST_SOURCE_DIR}/coverage_dashboard.html.in
    ${NOVASVG_COVERAGE_BINARY_DIR}/index.html
    @ONLY
)