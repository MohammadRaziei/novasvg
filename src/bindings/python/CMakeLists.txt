find_package(Python COMPONENTS Interpreter Development REQUIRED)

if(DEFINED SKBUILD)

    # Find Python and nanobind
    find_package(nanobind CONFIG REQUIRED)

    # Define the source file for the binding
    set(BINDING_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/binding.cpp)

    # Create the Python extension module
    # Note: The module name here ('novasvg_py') should match the NB_MODULE name in binding.cpp
    nanobind_add_module(novasvg_py
        ${BINDING_SOURCE_FILE}
    )

    # Link against the novasvg library
    target_link_libraries(novasvg_py PRIVATE novasvg::novasvg)


    # Install rules for the Python package
    # This installs the compiled module (.so or .pyd) and the python source files

    # 1. Install the compiled extension module into the 'novasvg' package directory
    install(TARGETS novasvg_py
        LIBRARY DESTINATION novasvg
    )

    # 2. Install the Python source files (.py) into the package directory
    # We use DIRECTORY to copy the contents of the 'novasvg' folder
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/novasvg/
        DESTINATION novasvg
        FILES_MATCHING PATTERN "*.py"
    )

    # Installation
    install(TARGETS novasvg_cli
        RUNTIME DESTINATION novasvg/bin
    )

    return()
endif()


# Include the ExternalProject module
include(ExternalProject)

# --- Check for nanobind dependency ---
# We need to find the nanobind cmake directory using the python module.
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import nanobind; print(nanobind.cmake_dir())"
    OUTPUT_VARIABLE NANOBIND_CMAKE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE NANOBIND_CHECK_RESULT
)

if(NOT NANOBIND_CHECK_RESULT EQUAL 0)
    message(WARNING "nanobind python module is not installed or not found.")
    message(WARNING "Please install it using: pip install nanobind")
    # We set a dummy variable to prevent further errors if find_package is called conditionally later,
    # but ideally we should stop configuration if it's strictly required.
    # For now, we let it fail naturally in ExternalProject if not found, or handle it there.
endif()

# Setup the external project
# We use the current source directory but build it in a separate binary directory
# to avoid conflicts with the main build.

set(NOVASVG_PYTHON_INSTALL_DIR ${CMAKE_BINARY_DIR}/python/install)
ExternalProject_Add(novasvg_build_python
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/python/build
    INSTALL_DIR ${NOVASVG_PYTHON_INSTALL_DIR}
    
    # Pass configuration arguments to CMake
    CMAKE_ARGS  
        --no-warn-unused-cli
        -DNOVASVG_BUILD_PYTHON="ON"
        -DSKBUILD=2
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/python/install
        -Dnanobind_DIR=${NANOBIND_CMAKE_DIR}
    
    # Ensure the build step runs the install command
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install --parallel 8
    
    # Build this target always, useful for testing workflows
    BUILD_ALWAYS 1
)
add_dependencies(novasvg_build_python novasvg)