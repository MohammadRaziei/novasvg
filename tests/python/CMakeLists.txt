# --- Testing and Coverage Section ---
find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(TARGET novasvg_build_python)
    # Check for pytest
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m pytest --version
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE PYTEST_CHECK_RESULT
    )

    if(NOT PYTEST_CHECK_RESULT EQUAL 0)
        message(WARNING "pytest is not installed. Python tests will be skipped.")
        message(WARNING "Please install it using: pip install pytest")
    else()
        # Define the test executable target (conceptually)
        # In reality, we run pytest via the Python interpreter found above.
        add_custom_target(novasvg_test_python
            COMMAND ${CMAKE_COMMAND} -E echo "-- Running python tests..."
            COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}
            WORKING_DIRECTORY ${NOVASVG_PYTHON_INSTALL_DIR}
            USES_TERMINAL
        )

        # Ensure tests are run after the custom build is complete
        add_dependencies(novasvg_test_python novasvg_build_python)
        add_dependencies(novasvg_test novasvg_test_python)
    endif()

    # Check for pytest-cov (coverage tool)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m coverage --version
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE COVERAGE_CHECK_RESULT
    )

    if(COVERAGE_CHECK_RESULT EQUAL 0)
        # Enable coverage for the python test target
        # We define a custom target to generate the coverage report.
        add_custom_target(novasvg_coverage_python
            # Clean previous coverage data
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${NOVASVG_COVERAGE_PYTHON_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_COVERAGE_PYTHON_BINARY_DIR}/html
            
            # Run tests with coverage enabled
            # We run from the install directory so Python can find the installed package.
            COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
            COMMAND ${Python3_EXECUTABLE} -m coverage run 
                    --source=${NOVASVG_PYTHON_INSTALL_DIR} 
                    # --omit="*/tests/*" 
                    -m pytest ${CMAKE_CURRENT_SOURCE_DIR}

            # Generate HTML report
            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
            COMMAND ${Python3_EXECUTABLE} -m coverage html -d ${NOVASVG_COVERAGE_PYTHON_BINARY_DIR}/html
            
            # Summary
            COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${NOVASVG_COVERAGE_PYTHON_BINARY_DIR}/html/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
            COMMAND ${Python3_EXECUTABLE} -m coverage report
            
            # Changed working directory to the install directory
            WORKING_DIRECTORY ${NOVASVG_PYTHON_INSTALL_DIR}
            USES_TERMINAL            
        )
        # Ensure coverage runs after the build is complete
        add_dependencies(novasvg_coverage_python novasvg_build_python)
        
        # Add this python coverage target to the main 'novasvg_coverage' target
        add_dependencies(novasvg_coverage novasvg_coverage_python)
    else()
        message(WARNING "coverage module is not installed. Python coverage target will be skipped.")
        message(WARNING "Please install it using: pip install coverage")
    endif()
endif()