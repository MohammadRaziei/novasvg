## [Unreleased]

### Changed

- **The library is split into `nova::svg` and `nova::vg`.** `nova::vg` is the drawing layer (canvas, `pixmap`, fonts,
  PDF output) and `nova::svg` the SVG engine on top of it; the headers live under `include/nova/`. foreignObject
  rendering is an extension (ADR-0010).
- **STL-like naming.** Types, functions, members and enumerators are `snake_case`; keywords used as names get a
  trailing underscore (`auto_`, `class_`, ...). A variable, member or parameter that has the name of a type gets a
  trailing underscore as well (`fill_rule_`, `selector_()`), and `rect::Empty` is `rect::empty_rect`.
- **`Bitmap` is `vg::pixmap`; the public image type is `stb::image`** (ADR-0003). Fonts and image codecs come from
  libstb (ADR-0005, ADR-0006).

### Removed

- **The C API `path_t`** (`path.cpp`, `path.h`, about 900 lines) and the bridges to it. The canvas builds and the
  rasterizer takes an `stb::outline` directly; SVG path data, arcs and the rounded-rect/ellipse shapes live in
  `outline_util.cpp` (same geometry as before: the 67 golden images are bit-identical).
- The single-header build and the vendored stb sources.

### Internal

- `nova::utils` (`src/sources/utils/text.h`): `is_space`, `trim`, `ltrim`, `rtrim`, `to_lower`, shared by the svg and
  vg layers instead of per-function lambdas.
- Private C struct tags no longer share a name with a class (`font_face_impl`, `font_face_cache_impl`); a clash made
  `delete face` recurse into the class destructor.

## [0.11.5] - 2026-11-05

### Fixed

- **CSS attribute selectors on attributes novasvg has no property for.** `[data-k]`, `[data-k="1"]`,
  `.a[data-k="1"]`, `rect[data-k="1"]` and chains like `[data-look="neo"][data-color-id="color-0"].node` (what
  Mermaid 12.3 emits to color ER entities and other nodes) never matched, because the parser dropped every
  attribute it has no `property_id` for. Such attributes are now kept on the element (and copied by `<use>`) and
  matched by name. Not covered: attribute selectors against the HTML inside a `<foreignObject>`.

## [0.11.4] - 2026-10-30

### Added

- **CSS custom properties and `var()`.** `--name: value` is read from an element's `style=""` and from
  stylesheets (`:root{--c:red}`, `svg{...}`, `.a{...}`, with the usual cascade: inline over sheet, `!important`
  over both), custom properties **inherit** down the tree, and `var(--name)` / `var(--name, fallback)` is
  replaced in every property value, in presentation attributes as well as in `style` and sheets. A custom
  property can use `var()` itself (what a theme chain like `--_fg: var(--pele-fg, #1f1f1f)` does); the
  fallback is used when a property does not exist or cannot be resolved (a cycle); a value that cannot be
  resolved at all is left unparsed, as if it were not there. Names are case-sensitive. Values are resolved
  once the document (and every sheet applied so far, `Document::applyStyleSheet()` included) is known, so a
  sheet that comes after the element still counts, and a `<use>` copy gets the value its original computed.
  Before this, any `var()` made the property invalid -- a `fill="var(--c)"` was painted black -- which broke
  SVGs that theme themselves with CSS variables (e.g. [pele](https://pele.run)'s output).
  Not covered yet: `var()` inside the HTML of a `<foreignObject>`, and `calc()`/`env()`.

- Benchmarks: a **Human defect grade (HDG)** next to RMSE, which counts pixels rather than what a person
  sees. Every render is graded against the Chromium one on an ordinal scale where lower is better:
  `0` accurate, `1` defective, `2` broken. The grades are kept by hand in `benchmarks/ratings.json`
  (`{"hdg": 0|1|2, "note": ...}` per sample and engine; a first draft covers the 60 samples x 6 engines),
  merged by `report/gather.py` into `render_benchmark.json` (`hdg` / `hdg_note` per cell, per-engine counts
  in `summary`, the metric's description under `metrics.hdg`) and shown in the report as a check / warning /
  stop icon under each render, a legend, and an HDG column in the engine table. An unrated render shows as
  "not rated"; a grade that is not 0, 1 or 2 is reported and ignored.

### Changed

- Benchmarks: nanosvg has no version macro and no release tags, so the report now shows the commit the
  bench was built against (`git 239e102`) instead of a placeholder. CMake reads it from the fetched checkout
  and passes it to `nanosvg_native_bench` as `NANOSVG_COMMIT`.
- Benchmarks: the RMSE comparison against the Chromium ground truth (`benchmarks/python/compare.py`)
  decodes the PNGs with [libstb](https://github.com/MohammadRaziei/libstb) instead of Pillow. It is
  lighter and about 5x faster to decode, and the pixels it reads are identical, so the RMSE values do
  not change. `benchmarks/python/requirements.txt` lists `libstb>=0.7.0` in place of `Pillow`. Pillow stays an
  optional extra for `Bitmap.to_pil()` / `Bitmap.from_pil()`.

### Tests

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