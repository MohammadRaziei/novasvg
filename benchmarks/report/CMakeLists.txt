# Two small steps over what novasvg_bench_run left in outputs/:
#   gather.py           renders/<engine>/result.json (x N)  ->  render_benchmark.json  (+ RMSE vs Chromium, + summary)
#   generate_report.py  render_benchmark.json               ->  index.html            ("At a glance" drawn from the JSON)
# Neither renders anything, so `cmake --build build --target novasvg_bench_report` is a quick re-run
# after editing the report or after redoing a single engine's folder.

set(NOVASVG_BENCH_REPORT_HTML "${NOVASVG_BENCH_OUTPUT_DIR}/index.html")
set(NOVASVG_BENCH_REPORT_JSON "${NOVASVG_BENCH_OUTPUT_DIR}/render_benchmark.json")

add_custom_command(
    OUTPUT "${NOVASVG_BENCH_REPORT_JSON}" "${NOVASVG_BENCH_REPORT_HTML}"
    # the stamp FILE, not just the target: a target-only dependency orders the steps but does not
    # notice a fresh run, so a rerun of run_benchmark.py would leave a stale render_benchmark.json behind
    DEPENDS novasvg_bench_run "${NOVASVG_BENCH_RUN_STAMP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/gather.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/charts.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../python/compare.py"
    COMMAND "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/gather.py" "${NOVASVG_BENCH_OUTPUT_DIR}"
    COMMAND "${NOVASVG_BENCH_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py" "${NOVASVG_BENCH_REPORT_JSON}"
    COMMENT "novasvg_bench: gathering renders/*/result.json -> render_benchmark.json -> index.html in ${NOVASVG_BENCH_OUTPUT_DIR}"
    VERBATIM
)
add_custom_target(novasvg_bench_report DEPENDS "${NOVASVG_BENCH_REPORT_JSON}" "${NOVASVG_BENCH_REPORT_HTML}")
