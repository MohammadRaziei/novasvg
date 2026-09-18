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

- **novasvg** is fetched and built from source (`cmake/FetchNovasvg.cmake`,
  real `novasvg_cli` binary via CMake `FetchContent`) and driven as a
  subprocess — this measures the actual C++ engine, not a PyPI snapshot.
  The other 4 engines go through their **Python bindings**: `python/CMakeLists.txt`
  creates an isolated venv and installs `resvg-py`, `pylunasvg`, `cairosvg`,
  `thorvg-python`.
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

## Tunables

```
cmake -S . -B build -DNOVASVG_BENCH_RUNS=10 -DNOVASVG_BENCH_WIDTH=512 -DNOVASVG_BENCH_HEIGHT=512
# build a different novasvg checkout (branch/tag/commit):
cmake -S . -B build -DNOVASVG_BENCH_GIT_TAG=v0.5.0
```

## ponytail-scoped (not done here, upgrade path if it matters later)

- **Timing isolation**: 4 of 5 engines run in one long-lived Python process,
  back to back, no per-process isolation or warmup-run discard (novasvg
  itself *is* a fresh subprocess per render, so it already pays process
  startup cost every call — not directly comparable to the in-process
  engines on that basis). Fine for relative comparison, not rigorous enough
  to publish as absolute numbers. Upgrade path: subprocess-per-engine for
  all 5, discard first run.
- `NOVASVG_BENCH_GIT_TAG` defaults to `master`, not this exact checkout —
  matches the pygixml/benchmarks convention (fetch a real published
  checkout rather than reuse the local source tree, so this directory stays
  copyable/standalone), but means local uncommitted changes to `../src` or
  `../include` aren't reflected unless pushed. Upgrade path: point it at a
  local `file://` path or a feature branch while iterating.
