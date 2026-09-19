# resvg is a Rust project; its C API (crates/c-api) is what
# native/resvg_native_bench.cpp links against, built here via `cargo build
# --release` as a custom command -- no Python binding, no prebuilt wheel.
#
# Pinned to v0.45.1: it's the newest tag whose c-api crate declares
# rust-version <= what a stock `apt install cargo rustc` gives you (1.67.1
# vs. HEAD's 1.85.0 requirement at the time this was written). Bump
# NOVASVG_BENCH_RESVG_GIT_TAG once a newer toolchain is available.

include(FetchContent)

set(NOVASVG_BENCH_RESVG_GIT_URL "https://github.com/linebender/resvg.git" CACHE STRING
    "resvg source repo the native bench is built from")
set(NOVASVG_BENCH_RESVG_GIT_TAG "v0.45.1" CACHE STRING
    "resvg git ref (branch/tag/commit) -- must have crates/c-api rust-version <= your cargo/rustc")

find_program(CARGO_EXECUTABLE cargo)
if(NOT CARGO_EXECUTABLE)
    message(FATAL_ERROR
        "resvg needs cargo to build its C API from source (it's a Rust project). "
        "Install it, e.g. `apt install cargo rustc`.")
endif()

FetchContent_Declare(
    resvg_src
    GIT_REPOSITORY "${NOVASVG_BENCH_RESVG_GIT_URL}"
    GIT_TAG "${NOVASVG_BENCH_RESVG_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_Populate(resvg_src)

set(RESVG_CAPI_DIR "${resvg_src_SOURCE_DIR}/crates/c-api")
set(RESVG_CARGO_TARGET_DIR "${resvg_src_BINARY_DIR}/cargo-target")
set(RESVG_LIB "${RESVG_CARGO_TARGET_DIR}/release/libresvg.a")
set(RESVG_INCLUDE_DIR "${RESVG_CAPI_DIR}")

add_custom_command(
    OUTPUT "${RESVG_LIB}"
    COMMAND "${CARGO_EXECUTABLE}" build --release
            --manifest-path "${RESVG_CAPI_DIR}/Cargo.toml"
            --target-dir "${RESVG_CARGO_TARGET_DIR}"
    COMMENT "novasvg_bench: building resvg's C API from source (cargo build --release)"
    VERBATIM
)
add_custom_target(resvg_build DEPENDS "${RESVG_LIB}")
# Consumers: link "${RESVG_LIB}" (plus -lpthread -ldl -lm), include
# "${RESVG_INCLUDE_DIR}", and add_dependencies(<target> resvg_build).
