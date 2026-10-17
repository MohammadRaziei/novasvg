## [Unreleased]

## [0.11.3] - 2026-10-17

### Added

- Benchmarks: a **Human defect grade (HDG)** next to RMSE, which counts pixels rather than what a person
  sees. Every render is graded against the Chromium one on an ordinal scale where lower is better:
  `0` accurate, `1` defective, `2` broken. The grades are kept by hand in `benchmarks/ratings.json`
  (`{"hdg": 0|1|2, "note": ...}` per sample and engine; a first draft covers the 60 samples x 6 engines),
  merged by `report/gather.py` into `render_benchmark.json` (`hdg` / `hdg_note` per cell, per-engine counts
  in `summary`, the metric's description under `metrics.hdg`) and shown in the report as a check / warning /
  stop icon under each render, a legend, and an HDG column in the engine table. An unrated render shows as
  "not rated"; a grade that is not 0, 1 or 2 is reported and ignored.

### Changed

- Benchmarks: the RMSE comparison against the Chromium ground truth (`benchmarks/python/compare.py`)
  decodes the PNGs with [libstb](https://github.com/MohammadRaziei/libstb) instead of Pillow. It is
  lighter and about 5x faster to decode, and the pixels it reads are identical, so the RMSE values do
  not change. `benchmarks/python/requirements.txt` lists `libstb>=0.7.0` in place of `Pillow`. Pillow stays an
  optional extra for `Bitmap.to_pil()` / `Bitmap.from_pil()`.

### Fixed

- A root `<svg style="background-color: transparent">` wiped the `background_color` passed to
  `render_to_bitmap()` (and the `background` of `svg2png()` and mermaidx's `.png(background=...)`).
  `Document::render` clears the bitmap to the root's own `background-color` after the caller's background has
  been applied, and it did so even when that color was fully transparent, so the PNG came out transparent
  instead of white. Mermaid writes exactly this `transparent` on every diagram it generates. A fully
  transparent root `background-color` now paints nothing and leaves the caller's background in place; an
  opaque one still wins, as before. A semi-transparent root color still replaces the caller's background
  rather than being composited over it.

### Tests

- `test_root_background.py` covers a transparent, an opaque and a missing root `background-color` against the
  `background_color` argument.
- `test_benchmark_report.py` covers the HDG merge, the per-engine counts, the "not rated" and invalid-grade
  cases and the icons in the page.
- The benchmark tests (`test_benchmark_corpus.py`, `test_benchmark_report.py`) build their PNGs with
  libstb instead of Pillow. `libstb>=0.7.0` joins the `test` extra and `requirements-dev.txt`; Pillow is kept
  there for the Bitmap interop tests.

## [0.11.2] - 2026-10-17

### Added

- CSS: the structural pseudo-classes `:nth-child()`, `:nth-last-child()`, `:nth-of-type()` and
  `:nth-last-of-type()`, with the full `An+B` syntax (`odd`, `even`, `3`, `2n`, `2n+1`, `-n+3`,
  `2n + 1`...). They are matched against the element's siblings, counted from the first or the last one,
  all siblings or only those of the same element type.

### Fixed

- A single rule the CSS parser could not handle discarded **every following rule** of the `<style>`
  sheet. An unsupported pseudo-class with an argument, such as `:nth-of-type(2)` (which was not
  recognised until now), made `parseStyleSheet` stop at that rule, so rules that came after it, such
  as `#id .node rect{fill:#fff}`, were never applied. Shapes then fell back to an inherited value, e.g.
  the dark root `fill`. This is how SVGs written by merman (mermaid-rs) came out as solid dark boxes.
  An invalid or unsupported rule is now skipped on its own, up to its closing `}`, and parsing carries
  on with the next rule, as CSS error recovery prescribes.

### Tests

- `tests/python/test_css_nth_and_recovery.py`: `An+B` formulas, `nth-last-*`, `nth-of-type` counting
  only same-type siblings, the merman regression (rules after an `:nth-of-type()` rule still apply),
  and dropping an invalid rule without losing the rules before or after it.

## [0.11.1] - 2026-10-17

### Added

- `novasvg::InvalidColorError` (C++, derives from `std::invalid_argument`) and `novasvg.InvalidColorError`
  (Python, a `ValueError` subclass), raised for every color parse failure: empty string, missing `#`,
  wrong length, non-hex digit and unknown color name. Existing `except ValueError` /
  `catch (const std::invalid_argument&)` code keeps working.

### Fixed

- `Color("#gggggg")` raised a bare `ValueError: stoi`. The message now names the problem, e.g.
  `Invalid hex color '#gggggg': 'g' is not a hex digit (0-9, a-f, A-F)`, and a wrong length lists the
  accepted forms.
- `Color::fromHash` silently accepted a bad trailing digit: `#12345g` parsed as a valid color because
  `std::stoi` stops at the first invalid character. Every digit is now checked.

## [0.11.0] - 2026-10-17

### Added

- **Python: `Bitmap` interoperability with the array ecosystem.** NumPy, Pillow and
  [libstb](https://github.com/MohammadRaziei/libstb) are optional extras
  (`novasvg[numpy]`, `novasvg[pillow]`, `novasvg[stb]`); `import novasvg` never loads them.
  - Zero-copy export of the Bitmap's own memory: the buffer protocol (`memoryview(bitmap)`,
    a writable 3-D `(height, width, 4)` uint8 view, no NumPy needed), `bitmap.numpy()` /
    `np.asarray(bitmap)`, DLPack (`__dlpack__`, `__dlpack_device__`: `np.from_dlpack`,
    `torch.from_dlpack`, `jax.numpy.from_dlpack`, `cupy.from_dlpack`) and
    `__array_interface__`, plus `bitmap.shape`.
  - Conversions to ordinary straight-alpha RGBA, as copies: `bitmap.tobytes()`,
    `bitmap.numpy(rgba=True)`, `bitmap.to_pil()` (Pillow) and `bitmap.to_stb()` (libstb).
  - Import, always as a copy: `Bitmap(array)`, `Bitmap.from_array(x, raw=False)`,
    `Bitmap.from_dlpack(x, raw=False)`, `Bitmap.from_pil(img)`, `Bitmap.from_stb(img)`.
    Accepts uint8 `(H, W)` or `(H, W, 1..4)` (L, LA, RGB, RGBA, straight alpha) from any
    buffer, DLPack object or `__array_interface__` object. `raw=True` takes `(H, W, 4)` already
    in the Bitmap's own layout.
  - `Bitmap.copy()`, `copy.copy` and `copy.deepcopy` give an independent deep copy
    (the C++ copy constructor shares pixels). `Bitmap.__repr__`.
- CI: native `ubuntu-24.04-arm` runner for ARM64 wheels, replacing emulation.
- Tests for all of the above, including a run with NumPy, Pillow and libstb blocked.

### Changed

- `Bitmap.numpy()` now takes the keyword-only arguments `rgba`, `dtype` and `copy`. The default is
  unchanged: a zero-copy view of the raw pixels. For a null bitmap it returns an empty
  `(0, 0, 4)` array instead of `None`.
- NumPy is optional: `numpy()` and `np.asarray(bitmap)` raise a clear `ImportError` when it is
  missing, and everything else keeps working.
- `README_PYTHON.md` documents the interop and the pixel format.

### Notes

- A Bitmap always stores premultiplied ARGB32 (BGRA bytes on little-endian). Zero-copy views show
  that memory as it is, so pass a Bitmap to Pillow or libstb through `to_pil()` / `to_stb()`,
  not `PIL.Image.fromarray(bitmap)` or `libstb.Image(bitmap)`, which would read the colours in the
  wrong order.
- `convert_to_rgba()` rewrites the buffer in place; do not combine it with `numpy(rgba=True)` or
  `tobytes()`, which would convert a second time.