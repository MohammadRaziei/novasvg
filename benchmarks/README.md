# novasvg benchmarks

Compares **novasvg** against **resvg**, **lunasvg**, **cairosvg** and **thorvg**
on capability, rendered output, and speed — driven entirely by CMake, ending
in one self-contained `results/report.html`.

## Run it

```
cmake -S . -B build
cmake --build build --target novasvg_benchmark
```

Open `results/report.html` — no server, no network needed, every render is
a base64 `data:` URI embedded directly in the page.

## How it works

- **novasvg** is fetched and built from source (`cmake/FetchNovasvg.cmake`)
  and driven directly through its own C++ API — `novasvg::Document` /
  `novasvg::Bitmap` — by `native/novasvg_native_bench.cpp`, a small
  executable that links `novasvg::novasvg` and renders the *entire* corpus
  in one process. No CLI, no subprocess per render: the same
  in-process-after-first-init model the other 4 engines get for free from
  being Python bindings, so timings are finally apples-to-apples.
- The other 4 engines go through their **Python bindings**:
  `python/CMakeLists.txt` creates an isolated venv and installs `resvg-py`,
  `pylunasvg`, `cairosvg`, `thorvg-python`.
- `python/corpus.py` reuses 10 files from novasvg's own `../data/` (plain
  shapes, the `tiger.svg` torture test, gradients/filters, clip+mask, CSS
  `transform`, an embedded raster `<image>`, and the mermaid flowchart with
  `<foreignObject>` text) instead of inventing a parallel test set.
- `python/run_benchmark.py` renders every (engine × sample) pair, timing
  each one (median of `NOVASVG_BENCH_RUNS`, default 5) and recording
  success/failure + the PNG bytes into `results/results.json`. Each sample
  is rendered at *its own* aspect ratio, fit inside the
  `NOVASVG_BENCH_WIDTH`&times;`NOVASVG_BENCH_HEIGHT` box (`corpus.py`'s
  `fit_box()`, same logic as CSS `object-fit: contain`) — most of the
  corpus isn't square (e.g. `nova.svg` is 146.5&times;73.6, the mermaid
  flowchart is 441&times;517), so forcing every render to a fixed square
  would stretch the artwork itself, not just its thumbnail.
- `report/generate_report.py` turns that JSON into the single `report.html`:
  an engine-version table, a capability+speed matrix (green = ok + timing,
  red = failed + hover for the error), and a visual gallery with every
  engine's render of every sample side by side.
- `native/novasvg_native_bench.cpp` pays novasvg's one-time process-wide
  font-cache warm-up (see below) with a throwaway 1&times;1 text render
  before timing anything, so that cost doesn't land arbitrarily on
  whichever sample happens to contain text first.

## What this found in novasvg itself

Profiling *why* novasvg was slower than expected (via a small ad-hoc
instrumented build, not part of this benchmark) turned up two distinct
costs that were getting conflated:

1. **One-time, not per-file**: the first bit of text rendered anywhere in
   a process — system font or embedded, doesn't matter — lazily triggers
   novasvg's process-wide `fontFaceCache()` singleton, which scans every
   installed system font. Measured cost here: a few ms once warm-cached by
   the OS, up to ~300ms cold. This isn't a per-render cost and was
   inflating whichever sample happened to be the first text-bearing one in
   the corpus — `native_novasvg_bench` now pays it up front instead (see
   above), before any timed job runs.
2. **Real, per-render, in novasvg's filter pipeline**: `gradient-filter`
   and `filter-primitives` (SVG `<filter>` with blur/offset/composite)
   consistently cost novasvg ~9–24ms even fully warmed up, for a ~320px
   canvas — notably more than lunasvg/cairosvg/thorvg pay for the same
   files (single-digit ms). This one's a genuine algorithmic cost in
   novasvg's filter code, not a benchmark artifact — worth profiling
   upstream (likely candidate: the blur convolution) if novasvg's own
   maintainers want to chase it. Not attempted here: changing that code
   blind, without novasvg's own test suite to check against, isn't
   something to do inside a benchmark PR.

## Tunables

```
cmake -S . -B build -DNOVASVG_BENCH_RUNS=10 -DNOVASVG_BENCH_WIDTH=512 -DNOVASVG_BENCH_HEIGHT=512
# build a different novasvg checkout (branch/tag/commit):
cmake -S . -B build -DNOVASVG_BENCH_GIT_TAG=v0.5.0
```

## ponytail-scoped (not done here, upgrade path if it matters later)

- **Timing isolation**: all 5 engines now run in-process (novasvg inside
  `novasvg_native_bench`, the other 4 inside this Python process), back to
  back, no warmup-run discard, no OS-level isolation between samples. Fine
  for relative comparison, not rigorous enough to publish as absolute
  numbers. Upgrade path: separate process per sample for all 5, discard
  the first run of each.
- `NOVASVG_BENCH_GIT_TAG` defaults to `master`, not this exact checkout —
  matches the pygixml/benchmarks convention (fetch a real published
  checkout rather than reuse the local source tree, so this directory stays
  copyable/standalone), but means local uncommitted changes to `../src` or
  `../include` aren't reflected unless pushed. Upgrade path: point it at a
  local `file://` path or a feature branch while iterating.
