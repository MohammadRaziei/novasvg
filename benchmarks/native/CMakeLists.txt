# A small executable that links novasvg directly (novasvg::novasvg, the
# same target novasvg_cli links, brought in by ../cmake/FetchNovasvg.cmake)
# and renders the whole corpus in one process via its C++ API -- see
# novasvg_native_bench.cpp for why this replaces shelling out to the CLI.

add_executable(novasvg_native_bench novasvg_native_bench.cpp)
target_link_libraries(novasvg_native_bench PRIVATE novasvg::novasvg)
