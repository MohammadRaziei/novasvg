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

    # Generate Python stubs
    # Run stubgen from the directory that actually contains the built module.
    # On multi-config generators (Visual Studio, Xcode) the .pyd/.so lands in a
    # per-config subdirectory (e.g. Debug/), so CMAKE_CURRENT_BINARY_DIR is the
    # wrong import path and stubgen fails with ModuleNotFoundError.
    add_custom_command(TARGET novasvg_py POST_BUILD
        COMMAND ${Python_EXECUTABLE} -m nanobind.stubgen
                -m novasvg_py
                -i $<TARGET_FILE_DIR:novasvg_py>
                -o $<TARGET_FILE_DIR:novasvg_py>/novasvg_py.pyi
        WORKING_DIRECTORY $<TARGET_FILE_DIR:novasvg_py>
        COMMENT "Generating Python stubs (.pyi) for novasvg_py..."
    )
    
    # Generate the single-header distribution for the Python package 
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/novasvg/novasvg.h"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dist_path}/novasvg
        COMMAND quom ${CMAKE_CURRENT_SOURCE_DIR}/include/novasvg/novasvg.h 
            "${CMAKE_CURRENT_BINARY_DIR}/novasvg/novasvg.h"
            -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        MAIN_DEPENDENCY novasvg
        COMMENT "Generating: ${CMAKE_CURRENT_BINARY_DIR}/novasvg/novasvg.h"
    )

    # Install rules for the Python package
    # This installs the compiled module (.so or .pyd) and the python source files

    # 1. Install the compiled extension module into the 'novasvg' package directory
    install(TARGETS novasvg_py
        LIBRARY DESTINATION novasvg
    )

    install(FILES $<TARGET_FILE_DIR:novasvg_py>/novasvg_py.pyi
        DESTINATION novasvg
        OPTIONAL
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

# Configuration used for the nested python build. Prefer the single-config
# CMAKE_BUILD_TYPE the user asked for; fall back to Release so multi-config
# generators don't silently produce a Debug .pyd against a release Python.
if(CMAKE_BUILD_TYPE)
    set(NOVASVG_PYTHON_CONFIG ${CMAKE_BUILD_TYPE})
else()
    set(NOVASVG_PYTHON_CONFIG Release)
endif()

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
        -DCMAKE_BUILD_TYPE=${NOVASVG_PYTHON_CONFIG}
        -DPython_EXECUTABLE=${Python_EXECUTABLE}

    # Multi-config generators (Visual Studio, Xcode) ignore CMAKE_BUILD_TYPE:
    # tell the nested build/install which configuration to use explicitly.
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${NOVASVG_PYTHON_CONFIG} --parallel 8
    
    # Ensure the build step runs the install command
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install --config ${NOVASVG_PYTHON_CONFIG} --parallel 8
    
    # Build this target always, useful for testing workflows
    BUILD_ALWAYS 1
)
add_dependencies(novasvg_build_python novasvg)