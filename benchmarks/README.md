# novasvg benchmarks

Compares **novasvg** against **resvg**, **lunasvg**, **thorvg**, **nanosvg**
and **cairosvg** — with **Chromium** rendered alongside as visual ground
truth — on capability, rendered output, and speed. Driven entirely by
CMake, ending in one self-contained `outputs/report.html`.

5 of the 6 compared engines are driven **directly through their own C/C++
API**, in one process each, built from source by this CMake project. Only
cairosvg runs through a Python binding — it has no native library of its
own to link against (cairosvg *is* Python: it wraps libcairo for drawing,
but the SVG/CSS parsing is pure Python). Chromium sits alongside all of
them as ground truth, not a 7th competitor.

## Run it

```
cmake -S . -B build
cmake --build build --target novasvg_benchmark
```

Needs on `PATH`: a C++17 compiler, `meson` + `ninja` (for thorvg), `cargo`
(for resvg), and Python 3, plus a Chromium build Playwright can launch for
ground truth (`playwright install chromium` if you don't already have one
— see below for pinning the pip package to match it). First build takes a
few minutes (compiles resvg via cargo, thorvg via meson, lunasvg/nanosvg
via CMake, novasvg from the local checkout this directory lives in). Open
`outputs/report.html` afterwards — no server, no network needed, every
render is a base64 `data:` URI embedded directly in the page; click any
render to zoom in. Everything else (`results.json`, per-engine renders/)
stays inside `build/` — `outputs/` holds only that one file.

## How it works

- **novasvg** — `cmake/FetchNovasvg.cmake` builds the checkout this
  `benchmarks/` directory already lives inside (`add_subdirectory(..)`),
  falling back to fetching a fresh copy from GitHub only if this directory
  has been copied out on its own. It does **not** fetch a second copy of
  novasvg when run in place — that was a real bug here for a while: the
  benchmark was compiling novasvg twice, once as the actual product
  (`../CMakeLists.txt`'s own `novasvg_cli`) and once again as a throwaway
  duplicate pulled fresh from GitHub master, which could even silently
  drift from whatever local changes were sitting in `../src`/`../include`.
  `native/novasvg_native_bench.cpp` links `novasvg::novasvg` and calls
  `novasvg::Document` / `novasvg::Bitmap` directly either way.
- **lunasvg** — `cmake/FetchLunasvg.cmake` fetches it (a real CMake
  project, pulls in its own `plutovg` dependency via git submodule);
  `native/lunasvg_native_bench.cpp` links `lunasvg::lunasvg` and calls
  `lunasvg::Document` / `lunasvg::Bitmap` directly.
- **nanosvg** — `cmake/FetchNanosvg.cmake` fetches the single-header C
  library; `native/nanosvg_native_bench.cpp` compiles it directly
  (`#define NANOSVG_IMPLEMENTATION`, stb-header style) and calls
  `nsvgParseFromFile` / `nsvgRasterize`.
- **thorvg** — ships *no* CMake build, only meson. `cmake/FetchThorvg.cmake`
  fetches its source and drives `meson setup` + `ninja` itself via a custom
  command (SVG loader + software rasterizer only, to keep the build fast),
  producing a static lib that `native/thorvg_native_bench.cpp` links
  normally and calls through `tvg::SwCanvas` / `tvg::Picture`.
- **resvg** — a Rust project; `cmake/FetchResvg.cmake` fetches its source
  and runs `cargo build --release` on `crates/c-api` itself via a custom
  command, producing a real `libresvg.a` + `resvg.h` that
  `native/resvg_native_bench.cpp` links and calls through the C API
  (`resvg_parse_tree_from_file` / `resvg_render`). Pinned to v0.45.1 — the
  newest tag whose c-api crate still declares `rust-version <= 1.75`
  (what a stock `apt install cargo rustc` gives you); HEAD needs 1.85+.
- **cairosvg** — the one Python binding. `python/CMakeLists.txt` creates an
  isolated venv and installs it; `python/engines.py` wraps `cairosvg.svg2png`.
- **Chromium (ground truth)** — `python/ground_truth.py` drives it through
  Playwright, the same approach as this repo's own `../playwright_render.py`:
  navigate straight to the SVG as a document (its `<svg>` root becomes
  `documentElement`, there's no `<body>`) and screenshot it, Chrome's own
  engine doing the rendering. One shared browser instance covers the whole
  corpus. This is the reference the other 6 are checked against, not
  another competitor — its timing includes full page-navigation overhead
  and isn't meant to be compared against the others' numbers (the report
  says as much). `requirements.txt` pins `playwright==1.56.0` to match
  whatever Chromium build is already on the machine; on a fresh machine,
  `playwright install chromium` first.
- **RMSE against ground truth** — `python/compare.py` computes root mean
  squared error (back on the 0-255 scale, all 4 RGBA channels) between
  every engine's render and Chromium's render of the same sample, shown
  under each render's time in the gallery. Squaring before the square
  root weights a handful of badly-wrong pixels (a missing filter, a wrong
  fill, a shifted shape) far more than the routine anti-aliasing noise
  along every edge — low single digits is essentially imperceptible, tens
  or higher usually means something structural differs.
- Every native engine speaks the same tiny protocol (`native/manifest.h`):
  read a tab-separated job list (name, svg path, out PNG path, w, h), time
  `runs` load+render passes per job with `std::chrono`, print
  `name\tOK\t<median_seconds>` or `name\tFAIL\t<message>` per line. One
  process covers the *entire* corpus for that engine — no process-spawn or
  library-init cost gets paid once per sample the way a CLI-per-render
  design would. `python/native_bench.py` builds the manifest and parses the
  output; the same function drives all 5.
- Raw-pixel-buffer engines (nanosvg, thorvg, resvg) encode to PNG with a
  vendored `stb_image_write.h` (fetched from the real upstream source, not
  hand-transcribed) — novasvg and lunasvg have their own PNG writers.
- `python/corpus.py` reuses 15 files from novasvg's own `../data/` — plain
  shapes, the `tiger.svg` torture test, gradients/filters/clip/mask, CSS
  `transform`, an embedded raster `<image>`, two files with `@font-face`
  embedded fonts, and two real mermaid.js renders (venn and block) —
  instead of inventing a parallel test set.
- `python/run_benchmark.py` resolves every sample's aspect-fit render size
  up front (`corpus.py`'s `fit_box()`, same logic as CSS
  `object-fit: contain` — most of the corpus isn't square, so forcing a
  fixed square render would stretch the artwork itself), then calls each
  native binary once and cairosvg per-file, merging everything into
  `build/results/results.json` (an intermediate build artifact, not
  something meant to be committed -- see Tunables below to relocate it).
- `report/generate_report.py` turns that JSON into the single `report.html`:
  an engine-version table, a capability+speed matrix (green = ok + timing,
  red = failed + hover for the error), and a visual gallery with every
  engine's render of every sample side by side.
- `native/novasvg_native_bench.cpp` pays novasvg's one-time process-wide
  font-cache warm-up (see below) with a throwaway 1×1 text render before
  timing anything, so that cost doesn't land arbitrarily on whichever
  sample happens to contain text first.

## What this found in novasvg itself

Profiling *why* novasvg was slower than expected (via a small ad-hoc
instrumented build, not part of this benchmark) turned up two distinct
costs that were getting conflated:

1. **One-time, not per-file**: the first bit of text rendered anywhere in
   a process — system font or embedded, doesn't matter — lazily triggers
   novasvg's process-wide `fontFaceCache()` singleton, which scans every
   installed system font. Measured cost here: a few ms once warm-cached by
   the OS, up to ~300ms cold. `novasvg_native_bench` now pays it up front
   (see above) instead of it landing on whichever sample renders text first.
2. **Real, per-render, in novasvg's filter pipeline**: `gradient-filter`
   and `filter-primitives` (SVG `<filter>` with blur/offset/composite)
   consistently cost novasvg several ms more than lunasvg/thorvg/resvg pay
   for the same files, even fully warmed up. Genuine algorithmic cost in
   novasvg's filter code (likely candidate: the blur convolution), not a
   benchmark artifact — worth profiling upstream if novasvg's maintainers
   want to chase it. Not attempted here: changing that code blind, without
   novasvg's own test suite to check against, isn't something to do inside
   a benchmark PR.

## Tunables

```
cmake -S . -B build -DNOVASVG_BENCH_RUNS=10 -DNOVASVG_BENCH_WIDTH=512 -DNOVASVG_BENCH_HEIGHT=512
# build different checkouts (novasvg itself uses the local tree by default -- see above):
cmake -S . -B build -DNOVASVG_BENCH_LUNASVG_GIT_TAG=v3.5.0     # lunasvg
cmake -S . -B build -DNOVASVG_BENCH_THORVG_GIT_TAG=v1.0.1      # thorvg
cmake -S . -B build -DNOVASVG_BENCH_RESVG_GIT_TAG=v0.45.1      # resvg (rust-version bound, see above)
# relocate either output (both default to sensible build/ vs. source-tree locations, see above):
cmake -S . -B build -DNOVASVG_BENCH_RESULTS_DIR=/tmp/novasvg-bench-results -DNOVASVG_BENCH_OUTPUT_DIR=/tmp/novasvg-bench-outputs
```

## ponytail-scoped (not done here, upgrade path if it matters later)

- **Timing isolation**: 5 engines run in-process (one per native binary),
  cairosvg in-process in Python, back to back, no warmup-run discard, no
  OS-level isolation between samples. Fine for relative comparison, not
  rigorous enough to publish as absolute numbers. Upgrade path: separate
  process per sample for all 6, discard the first run of each.
- lunasvg/thorvg/resvg `*_GIT_TAG` default to that project's own default
  branch (`master` or `main`), not a pinned release, except resvg which is
  pinned to v0.45.1 for the rust-version reason above — matches the
  pygixml/benchmarks convention (fetch a real published checkout) for
  those three, since they're genuinely separate dependencies, not the
  thing this benchmark ships alongside.
- **resvg's rust-version ceiling**: pinned at v0.45.1 because this
  environment's `cargo`/`rustc` (via `apt install cargo rustc`) is 1.75;
  resvg's HEAD needs 1.85+. A newer toolchain (rustup, or a newer distro
  package) would let `NOVASVG_BENCH_RESVG_GIT_TAG` track HEAD instead.
- thorvg is built with only the SVG loader and software (sw) engine to
  keep the build fast — no font/text loaders (`-Dloaders=svg` excludes
  `ttf`), no GL/WG engines. Doesn't change results here since thorvg
  already doesn't render `<text>` at all (a known, longstanding limitation,
  not something this minimal build config caused).
