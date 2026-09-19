# Sets up an isolated venv with cairosvg (the one engine with no native
# counterpart) and runs the actual timing/rendering pass, which also drives
# the 5 native_*_bench binaries built under ../native. Kept separate from
# report/ so `novasvg_bench_run` can be re-run on its own (e.g. with a
# different --runs) without regenerating anything else until you ask for it.

set(NOVASVG_BENCH_VENV "${CMAKE_CURRENT_BINARY_DIR}/venv" CACHE PATH
    "Python venv used to install and run cairosvg")
set(NOVASVG_BENCH_RUNS "5" CACHE STRING "Renders per (engine, sample) cell; median is reported")
set(NOVASVG_BENCH_WIDTH "320" CACHE STRING "Render width in px, shared by every engine")
set(NOVASVG_BENCH_HEIGHT "320" CACHE STRING "Render height in px, shared by every engine")

if(WIN32)
    set(NOVASVG_BENCH_PYTHON "${NOVASVG_BENCH_VENV}/Scripts/python.exe")
else()
    set(NOVASVG_BENCH_PYTHON "${NOVASVG_BENCH_VENV}/bin/python")
endif()

set(NOVASVG_BENCH_RESULTS_DIR "${NOVASVG_BENCH_RESULTS_DIR_DEFAULT}" CACHE PATH
    "Where results.json + per-engine renders/ land (build/ by default -- intermediate, not committed)")

add_custom_command(
    OUTPUT "${NOVASVG_BENCH_PYTHON}"
    COMMAND "${Python3_EXECUTABLE}" -m venv "${NOVASVG_BENCH_VENV}"
    COMMENT "novasvg_bench: creating venv at ${NOVASVG_BENCH_VENV}"
    VERBATIM
)

set(NOVASVG_BENCH_DEPS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/deps.stamp")
add_custom_command(
    OUTPUT "${NOVASVG_BENCH_DEPS_STAMP}"
    DEPENDS "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    COMMAND "${NOVASVG_BENCH_PYTHON}" -m pip install -q --upgrade pip
    COMMAND "${NOVASVG_BENCH_PYTHON}" -m pip install -q -r "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    COMMAND "${CMAKE_COMMAND}" -E touch "${NOVASVG_BENCH_DEPS_STAMP}"
    COMMENT "novasvg_bench: installing cairosvg + playwright"
    VERBATIM
)
add_custom_target(novasvg_bench_deps DEPENDS "${NOVASVG_BENCH_DEPS_STAMP}")

set(NOVASVG_BENCH_RESULTS_JSON "${NOVASVG_BENCH_RESULTS_DIR}/results.json")
add_custom_command(
    OUTPUT "${NOVASVG_BENCH_RESULTS_JSON}"
    DEPENDS novasvg_bench_deps
            novasvg_native_bench resvg_native_bench lunasvg_native_bench
            thorvg_native_bench nanosvg_native_bench
            "${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/engines.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/corpus.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/native_bench.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/ground_truth.py"
    COMMAND "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.py"
            "${NOVASVG_BENCH_RESULTS_DIR}"
            --runs "${NOVASVG_BENCH_RUNS}"
            --width "${NOVASVG_BENCH_WIDTH}"
            --height "${NOVASVG_BENCH_HEIGHT}"
            --novasvg-native-bench "$<TARGET_FILE:novasvg_native_bench>"
            --resvg-native-bench "$<TARGET_FILE:resvg_native_bench>"
            --lunasvg-native-bench "$<TARGET_FILE:lunasvg_native_bench>"
            --thorvg-native-bench "$<TARGET_FILE:thorvg_native_bench>"
            --nanosvg-native-bench "$<TARGET_FILE:nanosvg_native_bench>"
    COMMENT "novasvg_bench: rendering every (engine, sample) pair and timing it"
    VERBATIM
)
add_custom_target(novasvg_bench_run DEPENDS "${NOVASVG_BENCH_RESULTS_JSON}")

set(NOVASVG_BENCH_RESULTS_JSON "${NOVASVG_BENCH_RESULTS_JSON}" PARENT_SCOPE)
set(NOVASVG_BENCH_PYTHON "${NOVASVG_BENCH_PYTHON}" PARENT_SCOPE)
set(NOVASVG_BENCH_RESULTS_DIR "${NOVASVG_BENCH_RESULTS_DIR}" PARENT_SCOPE)
