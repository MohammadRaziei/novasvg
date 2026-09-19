add_executable(novasvg_native_bench novasvg_native_bench.cpp)
target_link_libraries(novasvg_native_bench PRIVATE novasvg::novasvg)

add_executable(lunasvg_native_bench lunasvg_native_bench.cpp)
target_link_libraries(lunasvg_native_bench PRIVATE lunasvg::lunasvg)

add_executable(nanosvg_native_bench nanosvg_native_bench.cpp)
target_include_directories(nanosvg_native_bench PRIVATE "${nanosvg_src_SOURCE_DIR}/src")

add_executable(thorvg_native_bench thorvg_native_bench.cpp)
target_include_directories(thorvg_native_bench PRIVATE "${THORVG_INCLUDE_DIR}")
target_link_libraries(thorvg_native_bench PRIVATE "${THORVG_LIB}")
add_dependencies(thorvg_native_bench thorvg_build)
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    # thorvg's software rasterizer uses OpenMP pragmas internally regardless
    # of its own -Dthreads= option, so the static lib needs libgomp at link time.
    target_link_libraries(thorvg_native_bench PRIVATE OpenMP::OpenMP_CXX)
endif()

add_executable(resvg_native_bench resvg_native_bench.cpp)
target_include_directories(resvg_native_bench PRIVATE "${RESVG_INCLUDE_DIR}")
target_link_libraries(resvg_native_bench PRIVATE "${RESVG_LIB}" pthread dl m)
add_dependencies(resvg_native_bench resvg_build)
