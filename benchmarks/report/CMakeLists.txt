# The only thing this writes into outputs/ is report.html itself -- keeps
# that folder to exactly the one file meant to be shared/committed, with
# everything intermediate (results.json, per-engine renders/) staying in
# build/ via NOVASVG_BENCH_RESULTS_DIR.

set(NOVASVG_BENCH_OUTPUT_DIR "${NOVASVG_BENCH_OUTPUT_DIR_DEFAULT}" CACHE PATH
    "Where the final report.html lands -- the one file meant to be committed/shared")
set(NOVASVG_BENCH_REPORT_HTML "${NOVASVG_BENCH_OUTPUT_DIR}/report.html")

add_custom_command(
    OUTPUT "${NOVASVG_BENCH_REPORT_HTML}"
    DEPENDS novasvg_bench_run "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py"
    COMMAND "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py"
            "${NOVASVG_BENCH_RESULTS_JSON}"
            "${NOVASVG_BENCH_REPORT_HTML}"
    COMMENT "novasvg_bench: writing self-contained report.html to ${NOVASVG_BENCH_OUTPUT_DIR}"
    VERBATIM
)
add_custom_target(novasvg_bench_report DEPENDS "${NOVASVG_BENCH_REPORT_HTML}")
