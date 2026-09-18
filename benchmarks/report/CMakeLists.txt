set(NOVASVG_BENCH_REPORT_HTML "${NOVASVG_BENCH_RESULTS_DIR}/report.html")

add_custom_command(
    OUTPUT "${NOVASVG_BENCH_REPORT_HTML}"
    DEPENDS novasvg_bench_run "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py"
    COMMAND "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py"
            "${NOVASVG_BENCH_RESULTS_JSON}"
            "${NOVASVG_BENCH_REPORT_HTML}"
    COMMENT "novasvg_bench: writing self-contained report.html"
    VERBATIM
)
add_custom_target(novasvg_bench_report DEPENDS "${NOVASVG_BENCH_REPORT_HTML}")
