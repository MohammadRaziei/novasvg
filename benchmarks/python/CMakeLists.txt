# Sets up an isolated venv with cairosvg (the one engine with no native
# counterpart) and runs the actual timing/rendering pass, which also drives
# the 5 native_*_bench binaries built under ../native. Every engine writes its
# own renders/<engine>/ folder (PNGs + result.json) into NOVA_BENCH_OUTPUT_DIR.
# Kept separate from report/ so `novasvg_bench_run` can be re-run on its own
# (e.g. with a different --runs) without regenerating the report.

set(NOVA_BENCH_VENV "${CMAKE_CURRENT_BINARY_DIR}/venv" CACHE PATH
    "Python venv used to install and run cairosvg")
set(NOVA_BENCH_RUNS "5" CACHE STRING "Renders per (engine, sample) cell; median is reported")
set(NOVA_BENCH_WIDTH "320" CACHE STRING "Render width in px, shared by every engine")
set(NOVA_BENCH_HEIGHT "320" CACHE STRING "Render height in px, shared by every engine")

if(WIN32)
    set(NOVA_BENCH_PYTHON "${NOVA_BENCH_VENV}/Scripts/python.exe")
else()
    set(NOVA_BENCH_PYTHON "${NOVA_BENCH_VENV}/bin/python")
endif()

add_custom_command(
    OUTPUT "${NOVA_BENCH_PYTHON}"
    COMMAND "${Python3_EXECUTABLE}" -m venv "${NOVA_BENCH_VENV}"
    COMMENT "novasvg_bench: creating venv at ${NOVA_BENCH_VENV}"
    VERBATIM
)

set(NOVA_BENCH_DEPS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/deps.stamp")
add_custom_command(
    OUTPUT "${NOVA_BENCH_DEPS_STAMP}"
    DEPENDS "${NOVA_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    COMMAND "${NOVA_BENCH_PYTHON}" -m pip install -q --upgrade pip
    COMMAND "${NOVA_BENCH_PYTHON}" -m pip install -q -r "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    COMMAND "${CMAKE_COMMAND}" -E touch "${NOVA_BENCH_DEPS_STAMP}"
    COMMENT "novasvg_bench: installing cairosvg + playwright"
    VERBATIM
)
add_custom_target(novasvg_bench_deps DEPENDS "${NOVA_BENCH_DEPS_STAMP}")

# run_benchmark.py writes many files into outputs/, so a stamp in the build tree tracks "has it run".
set(NOVA_BENCH_RUN_STAMP "${CMAKE_CURRENT_BINARY_DIR}/run.stamp")
add_custom_command(
    OUTPUT "${NOVA_BENCH_RUN_STAMP}"
    DEPENDS novasvg_bench_deps
            novasvg_native_bench resvg_native_bench lunasvg_native_bench
            thorvg_native_bench nanosvg_native_bench
            "${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/engines.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/corpus.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/native_bench.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/ground_truth.py"
    COMMAND "${NOVA_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.py"
            "${NOVA_BENCH_OUTPUT_DIR}"
            --runs "${NOVA_BENCH_RUNS}"
            --width "${NOVA_BENCH_WIDTH}"
            --height "${NOVA_BENCH_HEIGHT}"
            --novasvg-native-bench "$<TARGET_FILE:novasvg_native_bench>"
            --resvg-native-bench "$<TARGET_FILE:resvg_native_bench>"
            --lunasvg-native-bench "$<TARGET_FILE:lunasvg_native_bench>"
            --thorvg-native-bench "$<TARGET_FILE:thorvg_native_bench>"
            --nanosvg-native-bench "$<TARGET_FILE:nanosvg_native_bench>"
    COMMAND "${CMAKE_COMMAND}" -E touch "${NOVA_BENCH_RUN_STAMP}"
    COMMENT "novasvg_bench: rendering every (engine, sample) pair and timing it -> ${NOVA_BENCH_OUTPUT_DIR}/renders/<engine>/"
    VERBATIM
)
add_custom_target(novasvg_bench_run DEPENDS "${NOVA_BENCH_RUN_STAMP}")

set(NOVA_BENCH_RUN_STAMP "${NOVA_BENCH_RUN_STAMP}" PARENT_SCOPE)
set(NOVA_BENCH_PYTHON "${NOVA_BENCH_PYTHON}" PARENT_SCOPE)
