set(NOVASVG_TEST_BINARY_DIR ${CMAKE_BINARY_DIR})
set(NOVASVG_COVERAGE_BINARY_DIR ${CMAKE_BINARY_DIR}/coverage)

set(NOVASVG_COVERAGE_CPP_BINARY_DIR ${NOVASVG_COVERAGE_BINARY_DIR}/cpp)
set(NOVASVG_COVERAGE_PYTHON_BINARY_DIR ${NOVASVG_COVERAGE_BINARY_DIR}/python)

add_custom_target(novasvg_test
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running All tests..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_TEST_BINARY_DIR}
)

add_custom_target(novasvg_coverage
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running coverage for all languages ..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_COVERAGE_BINARY_DIR}
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/python)

add_custom_target(novasvg_coverage_report
    COMMAND ${CMAKE_COMMAND} -E echo "-- Finalize coverage reports..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${NOVASVG_COVERAGE_BINARY_DIR}
)
add_dependencies(novasvg_coverage_report novasvg_coverage_cpp novasvg_coverage_python)
add_dependencies(novasvg_coverage novasvg_coverage_report)