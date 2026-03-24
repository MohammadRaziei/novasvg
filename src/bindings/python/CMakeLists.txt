# Find Python and nanobind
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# Define the source file for the binding
set(BINDING_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/binding.cpp)

# Create the Python extension module
# Note: The module name here ('novasvg_py') should match the NB_MODULE name in binding.cpp
nanobind_add_module(novasvg_py
    ${BINDING_SOURCE_FILE}
)

# Include directories
# Assuming 'novasvg.h' is in a parent include directory relative to the project root
# If novasvg is a target in the parent, we link to it (see below).
target_include_directories(novasvg_py
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} # For local headers if any
        ${CMAKE_SOURCE_DIR}/src      # Adjust if your header is in project/src
)

# Link against the novasvg library
# We check if the target 'novasvg' exists (defined in the parent CMakeLists.txt)
# If not, we try to find it via find_package (optional, for standalone builds)
if(TARGET novasvg)
    target_link_libraries(novasvg_py PRIVATE novasvg)
else()
    message(STATUS "Target 'novasvg' not found in parent scope. Trying find_package...")
    # You might need to configure export() in the root CMake for this to work standalone
    # find_package(novasvg CONFIG REQUIRED) 
    # target_link_libraries(novasvg_py PRIVATE novasvg::novasvg)
    message(WARNING "Please ensure 'novasvg' library target is available.")
endif()

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

# Optional: If you want to install the package directly to Python's site-packages
# You would calculate the PYTHON_SITE_PACKAGES location and use that as DESTINATION.
# Example:
# execute_process(
#     COMMAND "${Python_EXECUTABLE}" -c "import site; print(site.getsitepackages()[0])"
#     OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
#     OUTPUT_STRIP_TRAILING_WHITESPACE
# )
# install(TARGETS novasvg_py LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}/novasvg)