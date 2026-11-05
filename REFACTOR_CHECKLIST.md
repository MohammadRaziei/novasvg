# novasvg: refactor checklist (compiled library, libstb, stl-like naming)

> Separate from the project's existing `checklist.md`, which is untouched.
> Goal: stop being header-only, use `libstb` wherever it fits, adopt stl-like naming, and tidy the design.
> This is a breaking change. The reasoning behind each decision is in [`docs/adr/`](docs/adr/README.md).

## Working rule

A step is done only when **all** of these hold:

1. The project builds.
2. The tests match the baseline or better (numbers below).
3. `refactor/golden.py diff` over the 62 golden renders shows zero differences (or each difference is explained here).
4. This file is updated: boxes ticked, findings written under the step.
5. A zip of the project is delivered, one per step, to be pushed on a new branch.

Steps stay small so that a failure has one suspect. See [ADR-0007](docs/adr/0007-refactor-in-verified-steps.md).

## How to verify

```sh
# Build (on a one-core machine each translation unit takes minutes; ninja resumes where it stopped)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DNOVA_BUILD_PYTHON=OFF -DNOVA_BUILD_TESTS=ON -DNOVA_BUILD_EXAMPLES=OFF
cmake --build build

# Tests
./build/tests/cpp/test_novasvg_cpp
./build/tests/cpp/test_novasvg_embedded_font
./build/tests/cpp/test_novasvg_libstb

# Pixel comparison against the baseline
python3 refactor/golden.py render ./build/novasvg /tmp/golden-new
python3 refactor/golden.py diff refactor/golden-baseline /tmp/golden-new
```

`golden.py` decodes the PNGs before comparing, so swapping the PNG encoder does not count as a difference; only
pixels do. The tool was tested both ways: two renders of one binary differ by zero, and a one-level change in a
single pixel (delta 7) is reported.

## Baseline (version 0.11.4, commit `85ebfc7`)

The golden renders live in `refactor/golden-baseline/`, which is in `.gitignore` (they travel in the zip, never in
git). If they are lost, regenerate them from the baseline commit:

```sh
git stash -u && git checkout 85ebfc7          # or use a separate worktree
cmake -S . -B /tmp/b0 -G Ninja -DCMAKE_BUILD_TYPE=Release -DNOVA_BUILD_PYTHON=OFF \
      -DNOVA_BUILD_TESTS=OFF -DNOVA_BUILD_EXAMPLES=OFF && cmake --build /tmp/b0
python3 refactor/golden.py render /tmp/b0/novasvg refactor/golden-baseline
```

| Item | Value |
|---|---|
| `test_novasvg_cpp` | 64 cases, 377 assertions, all pass |
| `test_novasvg_embedded_font` | 5 cases, 71 assertions, all pass |
| golden renders | 62 images (3.2 MB) in `refactor/golden-baseline/` |
| Python tests | not run yet (the Python bindings were off in the baseline build) |
| After step 2a | `test_novasvg_cpp` 70 cases / 443 assertions (`test_image_io` added); the rest unchanged |

## Decisions

Reasons, options considered and costs are in the ADRs; this is only the summary.

| ADR | Decision | Status |
|---|---|---|
| [0001](docs/adr/0001-compile-novasvg-into-a-library.md) | Compile novasvg into a library, thin public headers | Accepted (step 4) |
| [0002](docs/adr/0002-depend-on-libstb-via-fetchcontent-dynamic.md) | libstb via FetchContent from git (pinned `765f48c`), linked dynamically | Implemented (step 1) |
| [0003](docs/adr/0003-public-image-type-is-stb-image.md) | Public pixel type is `stb::image`; premultiplied render surface stays internal | Accepted (step 2b; C++ side done, Python side open) |
| [0004](docs/adr/0004-stl-like-naming-and-namespace.md) | stl-like `snake_case` naming; namespace `novasvg` | Accepted (step 6) |
| [0005](docs/adr/0005-replace-vendored-stb-with-libstb.md) | Replace vendored stb_image, image writer and inflater with libstb | Implemented (step 2a) |
| [0006](docs/adr/0006-fonts-on-libstb.md) | Build fonts on libstb; add a glyph-outline API to libstb | Proposed (step 3) |
| [0007](docs/adr/0007-refactor-in-verified-steps.md) | Verified steps gated by tests and a pixel-level golden diff | Accepted |

Order change from the first proposal: replacing vendored code with libstb comes *before* moving code into
`.cpp` files, because it removes about a fifth of the header text first and makes the rest lighter.

Open questions:

- [x] How libstb is obtained: FetchContent from git, for now. To be revisited.
- [x] libstb is linked **dynamically**; CMake enforces it. Still open: libstb's shared build has had Windows (DLL)
      and musllinux (segfault) trouble, which now applies to novasvg; check it in step 7 (wheels).
- [x] Public render result: `stb::image`. A libstb issue asks for SIMD premultiplied/straight conversion kernels;
      it does not change the decision (ADR-0003).
- [ ] Is novasvg itself shipped shared, static, or both? (step 4)
- [ ] Ship the optional `namespace nova = novasvg;` alias? (step 6)

## Naming rules (taken from libstb)

| Thing | Rule | Example |
|---|---|---|
| Types (class/struct/enum) | `snake_case` | `document`, `text_node`, `blend_mode` |
| Functions and methods | `snake_case` | `load_file`, `has_attribute` |
| Getters | no `get_` prefix | `width()`, `bounding_box()` |
| Setters | `set_` prefix | `set_attribute(...)` |
| Booleans | `is_` / `has_` | `is_null()`, `has_attribute()` |
| Factories / conversions | `from_*` / `to_*` | `matrix::from_rotation(...)`, `to_string()` |
| Private data members | trailing `_` | `node_` |
| Enums | `enum class`, `snake_case` values | `text_align::center` |
| Exceptions | `*_error`, derived from `std::` types | `invalid_color_error` |
| Macros | `NOVA_*` | `NOVA_API` |
| Namespaces | lowercase | `novasvg`, `novasvg::detail` |

The exact old-to-new name table for the public API is written separately in step 6 and approved before use; the
examples above only show the direction.

## Steps

## Temporary refactor-only artifacts (gitignored, deleted when the refactor ends)
Nothing that exists only to prove the refactor is an official test. All of it lives in `/refactor/` (in `.gitignore`):
- `refactor/golden.py`, `refactor/golden-svg/`, `refactor/golden-baseline/`: before/after render diff
- `refactor/temp-tests/test_*.cpp`: verification tests (old scalar loops vs libstb, layer route vs direct drawing);
  compiled into `test_novasvg_cpp` only while the folder exists, through a marked block in `tests/cpp/CMakeLists.txt`
  (remove that block at the end)
Official tests (`tests/`) only describe the behaviour of the library as it is meant to be.


### Step 0: baseline and verification tooling
- [x] Clone novasvg and libstb
- [x] libstb builds (static, standalone)
- [x] novasvg baseline builds; tests green (numbers above)
- [x] `refactor/golden.py` written and tested; 62 golden renders made (in `refactor/`, git-ignored)
- [x] This checklist
- [ ] Run the Python tests (needs nanobind; revisit in step 7)

### Step 1: CMake plumbing for libstb ✅
No change to code or names.
- [x] libstb via `FetchContent` (repository and pin are overridable: `NOVA_LIBSTB_GIT_REPOSITORY` /
      `NOVA_LIBSTB_GIT_TAG`; local checkout: `-DFETCHCONTENT_SOURCE_DIR_LIBSTB=/path/to/libstb`;
      offline: `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`)
- [x] `novasvg_noflag` links `libstb::core` (`novasvg::novasvg` inherits it)
- [x] Shared linking is enforced (`STB_BUILD_SHARED=ON` forced, target type checked)
- [x] Smoke test `test_novasvg_libstb`: links only through `novasvg::noflag`, round-trips colour, zlib and PNG,
      and (on Linux/macOS, with `dladdr`) proves libstb is a separate shared object
- [x] Install and `find_package(novasvg)` verified with a separate consumer project
- [x] Tests and golden unchanged

Findings:
- **CMake module clash.** Both projects have `Startup`/`Optimize`/`DynamicVersion` modules and libstb appends its
  directory to `CMAKE_MODULE_PATH` *after* novasvg's, so libstb silently used novasvg's `Optimize.cmake` and
  lost `-fstrict-aliasing`. novasvg clears the module path while libstb configures. Suggested fix in libstb:
  `list(PREPEND CMAKE_MODULE_PATH ...)` instead of `APPEND`.
- **Install.** The package config must `find_dependency(libstb)` (added). Installing novasvg also installs libstb
  into the same prefix.
- **Installed RPATH.** The installed CLI gets `$ORIGIN/../lib` (`@loader_path/../lib` on macOS); without it the
  CLI cannot find libstb after install. On Windows `stb.dll` installs next to `novasvg.exe`.
- Minimum CMake rises from 3.19 to **3.21** (libstb requires it).
- At this step `ldd novasvg` did not list libstb: the CLI called no libstb symbol yet and the linker
  (`--as-needed`) dropped it. It appears from step 2a on.

### Step 2: images and zlib

#### 2a: remove the vendored code ✅
- [x] `stbi_load` / `stbi_load_from_memory` replaced by `stb::image::open` / `decode` with `format = rgba` (EXIF is
      not applied, as before); failure still yields `nullptr`
- [x] `stbi_zlib_decode_malloc` (embedded fonts) replaced by `stb::zlib_decompress`
- [x] `BitmapCodec` (about 1,400 lines, a port of stb_image_write) removed; `writeTo{Png,Bmp,Tga,Jpg}` run on `stb::image`
- [x] stb_image (about 7,700 lines) removed from `render/surface.h`; 9,152 lines gone in total
- [x] Test targets link `libstb::core`
- [x] Tests: `test_novasvg_cpp` 70 cases / 443 assertions (64/377 before plus 6/66 new),
      `test_novasvg_embedded_font` 5/71, libstb link check OK; golden 62 images identical; no `stbi_*` symbols in the binaries
- [x] `tests/cpp/test_image_io.cpp` added: lossless PNG/TGA with straight alpha, BMP/JPEG size and colour, file
      against callback output, padded stride, failure paths, JPEG quality clamping. (The golden set only checked
      PNG output and decoding of three SVGs, so BMP/TGA/JPEG had no coverage.)
- Result: CLI 1.4 to 1.3 MB, main test binary 2.4 to 1.8 MB; `ldd novasvg` now lists libstb.
- **Latent bug found and reproduced.** Writing a `Bitmap` whose stride exceeds `width * 4` (a bitmap over caller
  memory) triggers an AddressSanitizer `heap-buffer-overflow` on baseline `85ebfc7` (a 60-byte buffer written 4
  bytes past its end). The new code converts row by row and is clean under ASan and UBSan on the same test.
- Known differences: the encoded bytes of PNG/JPEG/BMP/TGA change (libstb's encoder; the pixels are identical);
  Softimage PIC, which stb_image read, is gone; decoding is capped at 512 MiB by default; JPEG quality is still
  clamped to 1..100.
- [ ] README, docs and licence notices still mention stb_image (cleaned up in step 7)

#### 2b: `Bitmap` leaves the public API  (C++ and Python bindings done; Bitmap not yet moved to `detail`)
libstb floats on `master` for now (temporary, by the maintainer's decision); fetched `3a6269f` while this step was built.
Target is `libstb::libstb` (renamed from `libstb::core` in libstb 0.9).
- [x] `renderToImage()` returns `stb::image` (conversion in place + `image::wrap`, keep_alive on the surface; no copy
      for a tightly packed bitmap, padded stride falls back to one copy). Old `render(Bitmap&)` / `renderToBitmap` kept
      as `@internal`
- [x] Rendering onto an existing image (`render(stb::image&)`): transparent layer + `stb::image::blend`. Normal mode
      differs from direct drawing by at most 2 per channel; `mix-blend-mode` differs (delta 162 in the test): it now
      blends against the layer, not against the image underneath. A semi-transparent root `background-color` is composited
      over the image instead of replacing it. Both recorded in `test_render_image.cpp`
- [x] `writeTo*` / `write` removed from `Bitmap`; CLI picks the encoder by extension (unknown -> PNG, JPEG quality 80,
      as before) because `stb::image::write` throws on unknown extensions
- [x] The two conversion call sites (plus `Bitmap::convertToRGBA`) use libstb's `premultiplied_argb_to_rgba` /
      `rgba_to_premultiplied_argb`; `test_premultiply_exact.cpp` compares all 65,536 (alpha, colour) pairs, including
      colour > alpha (mod 256), with the old scalar loops
- [x] Golden set grown 62 -> 67 (`refactor/golden-svg/`: RTL text, JPEG/GIF/PNG-alpha images, image pattern, 8 blend modes,
      filters); baseline rendered with `85ebfc7`
- [x] CLI, C++ tests and `sample01` updated
- [x] Python bindings: the ten encoder bindings became one path (`encode_to_bytes` / `write_to_file` over
      `bitmap_to_rgba_image` + libstb encoders; JPEG quality still clamped to 1..100). The Python `Bitmap` class stays
      over the internal type (its documented premultiplied zero-copy export would break); moving it to libstb's image
      type belongs with packaging. `render()` is bound explicitly to its `Bitmap&` overload. Extension built with
      nanobind 3.1 / Python 3.12 (SKBUILD=2); 433 Python tests pass
- [x] Found by the Python tests: with libstb a shared library, `import novasvg` failed unless `libstb` was imported
      first (no runpath). Fixed with `INSTALL_RPATH $ORIGIN/../libstb/lib` (`@loader_path` on macOS) on `novasvg_py`;
      the wheel layout (`site-packages/libstb/lib`) still needs checking in step 7
- [x] Found by the Python tests: libstb 0.9's Python `imread` takes `format=`, not `channels=`; `benchmarks/python/compare.py`
      ported, `test` extra now needs `libstb>=0.9.0`
- [x] `novasvg_write_func_t` typedef removed (no users left)
- [x] Gate (this machine, Release, 1 core, libstb master `3a6269f`): `test_novasvg_cpp` 79 cases / 552 assertions;
      `test_novasvg_embedded_font` 5 / 71; libstb link check OK; Python 433 tests; golden 67 images, 0 differ, tol 0;
      `sample01` (14 SVGs -> PNG, 0 failed) and `sample02` run
- [ ] `Bitmap` still `nova::vg::Bitmap` (marked internal); moving it to `detail` waits for step 4 (public/internal split)

### Step 3: fonts, remove stb_truetype (ADR-0006; D3 done 2026-11-04, final gate pending)
novasvg calls 13 stb_truetype functions. `stb::font` renders text to bitmaps, but SVG needs **vector glyph
outlines** (`stbtt_GetGlyphShape`), so libstb must grow an API first.
- [ ] Re-read libstb's `font.hpp` and write the libstb issue: glyph index, glyph outline (move/line/quad/cubic),
      per-glyph metrics and box, font bounding box, face count in a collection, kerning (kern and GPOS)
- [ ] Add that API in libstb (its own repository, tests and zip)
- [x] Rebuild `FontFace` on `stb::font` (`src/gfx/font_face.cpp`)
- [ ] Persian/Arabic shaping (bidi, joining, GSUB) through `stb::shape_*`
- [ ] Extend the golden set first with RTL text and embedded-font SVGs
- [x] Remove stb_truetype (`gfx/font_impl.h`, 6266 lines)
- [ ] Tests and golden; watch: RTL text, kerning, embedded fonts, WOFF, `.ttc`

### Step 4: from header-only to compiled (ADR-0001)
- [ ] Public/internal boundary: thin public headers (`document`, `element`, `node`, `box`, `matrix`)
- [ ] Move `detail/` into `src/`, one module per small change
- [ ] Split `svgelement.h` (about 7,600 lines, about 90 types): DOM, properties, filters, gradients, text, foreignObject
- [ ] Make the CSS/HTML engine (`Rule`, `SimpleSelector`, `RuleIndex`, `HtmlNode`, `StyleNode`) its own module
- [ ] `NOVA_API` and symbol visibility
- [ ] CMake targets: `novasvg::novasvg` becomes the real library; decide what `noflag` means
- [ ] Decide the fate of the dist single header and `NOVA_TEST_INCLUDE_DIR`
- [ ] Measure build time before and after and record it here
  - Before (header-only, Release, 1 core, GCC; from `build/.ninja_log`): every translation unit that includes
    `novasvg.h` costs 14-55 s (CLI 23 s, `sample02` 14 s, the doctest files 23-48 s, `check_embedded_font` 55 s);
    the 15 such units in a full tests + examples build add up to roughly 9 minutes. Header lines: 34,469


#### Step 4 progress (module 1: the `render/` layer)  -- IN PROGRESS
- [x] Compiled core: `novasvg_core` (STATIC, PIC, LTO) in `CMakeLists.txt`; `novasvg::noflag` / `novasvg::novasvg` link it;
      tests link `novasvg::noflag`; macros moved to `detail/config.h`
- [x] Moved to `src/gfx/*.cpp`: matrix, utils, path, paint, rasterize, surface, blend, canvas (265 symbols in
      `libnovasvg_core.a`) with `refactor/split_header.py` (temporary tool)
- [x] Gate: 79 tests, font test 5 / 71, libstb link check, golden 67 images 0 differ (tol 0)
- Lesson: no function may be left `NOVA_INLINE` in a header once `render.h` declares it `NOVA_API`
  (undefined reference in TUs that only see the declaration); everything moved, inlining relies on LTO
- Build time so far: barely better (test_basics 43 -> 32 s, check_embedded_font 55 -> ~42 s, CLI 23 -> 27 s; rough,
  from `.ninja_log`), because `svgelement.h`, `font.h` and the rest of `detail/` are still header-only
- [x] One definition of the export macros: `detail/config.h` (the duplicate block in `render/render.h` removed).
      `NOVA_API` has the same scheme as libstb's `STB_API` (`NOVA_BUILDING_LIBRARY` / `NOVA_STATIC_DEFINE`,
      set by CMake on `novasvg_core`); `NOVA_MAX/MIN` stay as they are (maintainer's decision)
- [ ] Benchmark the hot rasterize/blend path (now cross-TU, inlined only by LTO)
- [ ] vendor `ft_raster` / `ft_stroker` / `ft_math` (different pattern, by hand)
- [x] Removed duplicate declarations (`render.h` vs per-module headers): 192 duplicates dropped from matrix/path/paint/canvas/surface/blend/rasterize `.h`; 13 internal-only functions (`parse_matrix_parameters`, `path_add_command`, `split_bezier`, the path traverse/parse helpers, `state_*`, `span_buffer_update_extents`) moved into their `.cpp` as `static`; `detail/canvas.h`, `bitmap.h`, `font.h` now include only `render/render.h`; `svgparser.hpp` includes `render/surface.h` explicitly for `base64_decode`
- [ ] `render/font.h` waits for step 3; then `color`, `canvas`/`bitmap`, split `svgelement.h`, CSS/HTML engine, close `detail/`

### Step 5: remove duplicates
Today each concept is defined in several layers (public, `detail/canvas.h`, `gfx/`).
- [ ] `Color` becomes `stb::color` (it was derived from this code)
- [ ] One `matrix` instead of `Matrix` / `Transform` / `matrix`
- [ ] One `rect` / `point` / `size` / `box`
- [ ] One `path`
- [ ] Merge `Font` / `FontFace` / `FontFaceCache` with `font_face` / `font_face_cache` / `glyph`
- [ ] Settle `Canvas` / `canvas` and `Bitmap` / `surface`
- [x] `render/vendor/ft_*` (FreeType/plutovg code) moved to `src/gfx/vendor/`, included only from `src/gfx/rasterize.cpp`; FT helpers (`ft_outline_*`, `spans_generation_callback`) are file-local; `rasterize.h` shrank to `span_buffer_*` + `rasterize()`

### Step 6: stl-like naming (ADR-0004)
- [ ] Write the old-to-new name table for the public API and get it approved (`FillRule::NonZero` -> `fill_rule::non_zero`, `getRF`, `addRect`, `lineWidth`... are still the old names; new code of 2026-11-04 (`pdf_canvas`, its tests) uses them too)
- [ ] Rename by script, one module per commit
- [ ] Update the CLI, tests, examples, docs and the Python bindings (which may stay Pythonic)
- [x] Namespace decided: `nova::svg` and `nova::vg` ([ADR-0009](docs/adr/0009-namespaces-nova-svg-and-nova-vg.md)); do it in the same scripted step as the rename

### Step 6.5: design / OOP review
- [ ] Class hierarchy (`SVGElement` and subclasses) and the role of the public handles (`element`, `node`)
- [ ] Error handling: exceptions (as libstb does) or return codes?
- [ ] Ownership and lifetime in the public API
- [ ] Thread safety (as `stb::font`: immutable and cheap to copy)
- [ ] Findings from step 2b to settle here (seen in the code, not yet changed):
  - `Bitmap` copies alias the same mutable pixels (`surface_reference`), yet `convertToRGBA()` converts in place and
    `bitmap_release_to_image` relies on unique ownership. Make it move-only with an explicit `clone()`, or split the
    states into two types (premultiplied surface / straight image) so a double conversion cannot compile. The Python
    docs already warn about the double-convert corruption of `convert_to_rgba()`
  - `render` / `renderToBitmap` / `render(stb::image&)` / `renderToImage` are written twice (`Element` and `Document`);
    `Document` should delegate to its root element, and layer + blend lives in one place
  - `convert_argb_to_rgba` / `convert_rgba_to_argb` in `render.h` are now one-line wrappers over libstb: remove them
  - Two pixel types are still public through Python until the Python `Bitmap` moves to libstb's image type

### Step 7: packaging and release
- [ ] Python bindings and Python tests
- [ ] Wheels and CI (Windows, macOS, manylinux, musllinux), including shipping libstb
- [ ] `CHANGELOG`, README, docs, version (breaking)
- [ ] `find_package(novasvg)` and `novasvgConfig.cmake`
- [ ] Update README and licence notices that still name stb_image

### Verification gate, new session (2026-11-02)
No code changed. Fresh unzip, full rebuild, gate run:
- C++: 79 cases / 552 assertions pass; font test 71 assertions pass; libstb link check OK.
- Golden: 67 images, 0 differ (tol=0, worst delta=0).
- libstb fetched hash: `e655ce9`.
- Python module rebuilt on top of `novasvg_core` (SKBUILD path): 433 tests passed (closes the "not rebuilt after step 4" item).
- Still open: install/export consumer test with the compiled core, hot-path benchmark (7.1.1), dist single header.

### Step 4, module 2: vendored rasterizer moved to src/ (2026-11-02)
`include/novasvg/gfx/vendor/` -> `src/gfx/vendor/` (git mv). `rasterize.h` no longer includes FreeType headers or declares any `ft_*`; consumers of `canvas.h` stop parsing ~4,800 vendored lines.
Gate: C++ 79 / 552, font 71, link check OK, golden 67 images 0 differ, Python 433 passed, libstb `e655ce9`.
Finding: libstb `draw.hpp` (rect/ellipse/line, int coordinates) has no path, bezier, fill rule, join/cap/dash stroker or span output, so the FreeType rasterizer/stroker has no libstb replacement yet. Design question (public vector-drawing layer, `detail/` removal, issue for libstb) is open, see the chat.

### Step 4, module 3: render/ headers cleaned (2026-11-03)
`render.h` is now the single declaration point for the public render API. Per-module headers keep only inline helpers, macros and the few cross-TU internals (`base64_decode`). The 3 consumers outside `render/` no longer pull blend/SSE internals.
Gate: C++ 79 / 552, font 71, link check OK, golden 67 images 0 differ, Python 433 passed, libstb `e655ce9` (not updated to `955870f` yet).
Open: per-module headers still carry `static inline` helpers (paint.h hex/hsl parsing, blend.h SSE) that belong in the `.cpp` if no other TU uses them; `private.h` duplicates the `span_buffer_*` family.

### Architecture reset, step A: layout (2026-11-03, ADR-0008)
`detail/` is gone. `include/novasvg/{config.h, gfx/, svg/}`, private headers + cpp in `src/gfx/` (was `src/render/`), `namespace render` -> `gfx`.
- `gfx/`: color, bitmap, canvas, font, default_font, render (C-style API, now also declares `base64_decode`), plus temporary-public `internal.h`, `utils.h`, `font_impl.h` (the inline stb_truetype stack that `gfx/font.h` still needs; goes away with step 3 / libstb fonts).
- `svg/`: svgelement, svgparser, svgparserutils. `gfx/font.h` still includes `svg/svgparserutils.h` (one inverted dependency, goes with the `Font` rewrite).
- Paused: the planned `paint.h` / `blend.h` / `private.h` cleanup (those files get rewritten in steps B and C).
- Single-header dist (quom) is cancelled by the maintainer: removed from root and Python CMake and from CI. Not verified: `install()` of the new header tree; docs/Doxyfile paths were sed-updated only.
- Gate: C++ 79 / 552, font 71, link check OK, golden 67 images 0 differ, Python 433 passed, libstb `e655ce9`.

### Step 6 started: public method names (2026-11-03)
38 camelCase methods renamed to snake_case outside string literals (see `docs/naming-table.md`); Python and CLI names unchanged. Types (`Element` -> `element` etc.) pending. Gate: C++ 79 / 552, font 71, link OK, golden 67 / 0 differ, Python 433.

### Step 6 continued: DOM types (2026-11-03)
`Node` `TextNode` `Element` `Document` -> snake_case; every `SVG*` identifier -> `svg_*`; `isEmpty` -> `empty()` (critical review of ADR-0004: its `is_` rule for booleans contradicts the STL / libstb `empty()` it claims to follow). Colliding variables renamed (`element`->`el`, `node`->`nd`, `document`->`dom`). `Box` / `Matrix` wait for step 5. Gate: C++ 79 / 552, font 71, link OK, golden 67 / 0 differ, Python 433.
Still camelCase in the svg layer: non-`SVG*` internal types and members (e.g. `Length`, `Paint`, `Color`, `Bitmap`, `Canvas`, `Font*`), to be done per module.

### Step C: abstract `canvas` + `raster_canvas` (2026-11-03, ADR-0008)
`include/novasvg/gfx/canvas.h`: new abstract `nova::vg::canvas` (set_color, set_linear/radial_gradient, fill_path, stroke_path, fill_text, stroke_text, clip_path, clip_rect, draw_image, save, restore); `Canvas` is now `raster_canvas final : canvas` with snake_case methods (`Canvas::canvas()` -> `handle()`). `svg_render_state::operator->/*` return `canvas`; `raster()` returns the `raster_canvas`. C struct `gfx::canvas` renamed `canvas_context` (`canvas_t` unchanged) to free the name.
**Remaining raster-only leaks in the SVG renderer (the exact C2 list):**
1. Groups/layers: `svg_render_state::beginGroup/endGroup` create a `raster_canvas` and `blend_canvas` it (needs `begin_layer(bounds, opacity, blend_mode)` / `end_layer()`).
2. Pattern fill: `set_texture(raster_canvas)` (needs a recorded/offscreen picture as paint source).
3. Masks and clip masks: render into a canvas, `convert_to_luminance_mask`, `blend_canvas(Dst_In)` (needs a mask/soft-mask layer API).
4. Filters: whole `svg_filter_context` works on `raster_canvas` pixels (raster-only by nature; a PDF backend rasterizes filtered groups).
Also open: the interface classes live in `novasvg`, the C API in `nova::vg` (namespace consolidation), and `Path/Transform/Rect/Color/Font` are still local types (step 5 swaps in `stb::`).
Gate: C++ 79 / 552, font 71, link OK, golden 67 / 0 differ, Python 433.

### libstb version note (2026-11-03)
`NOVA_LIBSTB_GIT_TAG` floats on `master` (by design for now). The `e655ce9` recorded above was only true up to the step-A configure; a later reconfigure silently moved the build to `955870f` (`stb::path`, `stb::matrix`, glyph `outline`), and now `a0726ba` (MSVC fix, `STB_MIN`/`STB_MAX`). Gate on `a0726ba`: C++ 79 / 552, font 71, link OK, golden 67 / 0 differ. Python not rerun for this bump (C++ side only changed). libstb still has no rasterizer / stroker (`stb/raster.hpp`), so `src/gfx/vendor/` stays.

### NOVA_MIN/MAX -> STB_MIN/STB_MAX (2026-11-03)
Macros removed from `config.h` and `gfx/color.h`; all uses switched to libstb's `STB_MIN/STB_MAX` (`<stb/export.hpp>`, included by `config.h` and `color.h`). Note the tie semantics differ (STB follows `std::min/max`: returns the first argument on equality; the old macros returned the second), which only matters for `+0/-0` and NaN. Golden 67 images / 0 differ says it does not matter in any rendered case. Gate on libstb `a0726ba`: C++ 79 / 552, font 71, link OK, golden 0 differ, Python 433.
Build note: the Python build (`/tmp/pybuild`) fetches its own libstb; after a libstb bump it needs `cmake /tmp/pybuild` once.

### Step D1: the library is compiled, `novasvg.h` is declarations only (2026-11-03)
- `include/novasvg/novasvg.h` (1180 -> 647 lines) now only declares; its former inline tail is `src/svg/novasvg.cpp` (compiled into `novasvg_core`).
- `svgelement.h` and `svgparser.hpp` moved to `src/svg/` (private). Definitions of public-class members that lived in them (3, e.g. `document::query_selector_all`) moved to `novasvg.cpp`; everything else in them stays `inline` for now.
- Consumers of `novasvg.h` no longer parse the SVG layer. `gfx/canvas.h`, `gfx/font.h` (+ `font_impl.h` = stb_truetype, 6266 lines), `internal.h`, `utils.h`, `default_font.h` are STILL public and header-only (tests and the Python binding use `Transform` / `Font` from them): this is the next thing to move.
- Debt: the Python binding and `test_foreignobject_style.cpp` include the private `svg/svgelement.h` (CMake adds `src/` to their include path). Needed: a public text-measure API (`Font`, `utf8ToU32`, `measure_foreign_object`).
- Remaining header-only code, honestly: `svgelement.h` ~7.7k lines (236 `NOVA_INLINE`), `font_impl.h` 6.3k, `canvas.h` 1.4k, `svgparser.hpp` 1.8k, `color.h`, `bitmap.h`, `font.h`. `NOVA_INLINE` itself still exists. Next: D2 = move bodies of `svgelement.h` / `svgparser.hpp` / `canvas.h` into `.cpp` files (split by element family), D3 = fonts onto libstb and delete `font_impl.h`.
- Gate (libstb a0726ba): C++ 79 / 552, font 71, link OK, golden 67 / 0 differ, Python 433.

### Step D2: bodies moved out of headers (2026-11-03)
Scripted (not by hand): every namespace-scope `NOVA_INLINE` definition (non-template, no default args, not `static`) moved into a `.cpp`; members vanish from the header (declared in-class already), free functions keep a declaration.
- `svgelement.h` 7682 -> 3764 lines + `src/svg/svgelement.cpp` (189 members, 40 free functions); `svgparser.hpp` -> `svgparser.cpp`.
- Public: `gfx/canvas.h` 1403 -> 780 + `src/gfx/raster_canvas.cpp`; `gfx/bitmap.h` + `src/gfx/bitmap.cpp`; `gfx/font.h` + `src/gfx/font.cpp`.
- `novasvg_destroy_func_t` moved to `config.h` (font.h needs it).
- The tool misclassified free functions whose return type is qualified (`stb::image f()`); fixed by hand for `bitmap_to_rgba_image` / `bitmap_release_to_image`. Anyone re-running `/tmp/outline.py`-style tooling must classify by the last token before `(`.
- Still header-only, honestly: `gfx/font_impl.h` 6266 lines (stb_truetype, goes in D3 with libstb fonts), `svgelement.h` / `svgparser.hpp` in-class bodies and 13 `NOVA_INLINE` the tool skipped (templates, defaults, `static`), `canvas.h` 5 skipped, `color.h` (1036 lines of inline color math), `default_font.h` (embedded font bytes), `render.h` 2510 lines (declarations + docs, fine).
- Not done yet: `NOVA_INLINE` macro removal, private gfx headers (`canvas.h`, `font.h`, `font_impl.h`, `internal.h`, `utils.h`) still in `include/`.
- Gate (libstb a0726ba): C++ 79 / 552, font 71, link OK, golden 67 / 0 differ, Python 433.


## Step 2 (private gfx headers out of include/) -- done 2026-11-04, final gate pending

- Moved `canvas.h`, `font.h`, `internal.h`, `utils.h`, `default_font.h` from `include/novasvg/gfx/` to `src/gfx/`; sources include them as `"gfx/x.h"` (`src/` is a private include dir of `novasvg_core`).
- Public headers now: `novasvg.h`, `config.h`, `gfx/color.h`, `gfx/bitmap.h`, `gfx/render.h` (bitmap.h still needs render.h), `svg/svgparserutils.h`.
- NOT done: the small public text-measuring API. It needs `Font`/`ForeignObjectFonts` to become public first (design decision), so the Python binding and `test_foreignobject_style.cpp` still include private headers through `src/`.
- Not tested yet (bold mode): compile and link only.

## libstb sync (2026-11-04)

- libstb is now `b5bfe90`. `stb/path` was renamed `stb/outline` there (`stb::path_command` -> `stb::outline_command`, `path_op` -> `outline_op`);
  `src/gfx/font_face.cpp` follows. The libstb tag is still `master`, so any rename upstream breaks the build until it is followed here:
  pinning a commit (`NOVA_LIBSTB_GIT_TAG`) would avoid that.
- libstb now has the rasterizer and stroker (`stb/raster.hpp`: `fill`, `stroke`, `rasterize_fill`, `rasterize_stroke`, `span_sink`), which is
  what Step 5 (remove `src/gfx/vendor/`) was waiting for.

## Step 5a (rasterizer and stroker from libstb) -- done 2026-11-04, final gate pending

- `rasterize()` in `src/gfx/rasterize.cpp` now converts the `path_t` to a `stb::outline` and calls `stb::rasterize_fill` / `stb::rasterize_stroke`
  (`stb/raster.hpp`); the spans go into the same `span_buffer_t` as before. Nothing else in novasvg changed.
- `src/gfx/vendor/` (FreeType port: `ft_raster.h`, `ft_stroker.h`, `ft_math.h`, `ft_types.h`, about 5,500 lines) and its `VENDOR.md` are deleted. The FreeType
  license notice now comes with libstb (`NOTICE.md` there).
- Things the final gate must confirm (golden tolerance 0):
  - stroke width scale: novasvg used `hypot(sx, sy) / sqrt(2)`; libstb says "mean scale of the two axes".
  - dashing: novasvg's own `path_clone_dashed` before; libstb's `dash.cpp` now. `path_clone_dashed` stays in the public C API, no longer used by `rasterize()`.
  - points are mapped by libstb's `matrix::map` (compiled without FMA) instead of novasvg's `matrix_map_points`.
  - no clip: libstb needs a clip rect, so +-1,000,000 is passed (it limits coordinates there anyway).
- Step 5b (still open): replace novasvg's own `Path`, `Transform`, `Rect`, `Color`, `Bitmap` with `stb::` types. Bigger, touches the whole svg layer and the public API.

## Step C2/F part 1 (pdf_canvas) -- 2026-11-04

- [x] `pdf_canvas` (`src/gfx/pdf_canvas.{h,cpp}`) for the `canvas` interface as it is: paints, gradients, fill/stroke, clip, images, text as outlines.
- [x] `tests/cpp/test_pdf_canvas.cpp` (structure checks; `NOVA_PDF_OUT=x.pdf` writes the page). Smoke-checked once: `qpdf --check` clean, `pdftoppm` renders it right.
- [~] Layers: done for `pdf_canvas` (`begin_group/end_group`, `draw_raster_group`) but NOT as virtuals of `canvas` (see ADR-0008, part 2); promote them when a third backend needs it.
- [x] Pattern: tile bitmap as PDF tiling pattern. Mask, clip mask, filter: rasterized group placed as an image (a vector `SMask` luminosity mask is an upgrade).
- [x] `svg_render_state` runs on any `canvas` (`raster()` is null on a PDF page); beginGroup/endGroup and the pattern fill know `pdf_canvas`.
- [x] Public entry point: `document::render_to_pdf` / `save_pdf` with `pdf_mode::vector | raster` and `dpi`; `tests/cpp/test_pdf_render.cpp`.
- [ ] Still open: gradient reflect/repeat and per-stop alpha in PDF, vector `SMask` for masks, element-level `render_to_pdf`, font embedding (selectable text), multi-page, PDF metadata (title), CLI flag, Python binding (off for now).
- Gate note: `svg_render_state::beginGroup/endGroup` and the pattern code were edited; the pixel path is meant to be identical, the final golden gate must confirm.

## Next, in this order (written 2026-11-05)

1. [ ] **Full gate first** (nothing but compile + link and the PDF smoke tests has run since D3): C++ cases, font test, libstb link check, golden
       67 images at tolerance 0. Expected suspects, if golden moves: stroke width scale, dashing, `matrix::map` without FMA (all step 5a);
       outline parity, `close` skipped, ligatures through `shape_glyphs`, GPOS kerning, `units_per_em` (D3); `beginGroup/endGroup` and the
       pattern fill (PDF work). Revert one piece at a time to find the cause. Record the libstb hash.
2. [ ] Step 5b: `Path`, `Transform`/`Matrix`, `Rect`/`Box`, `Color`, `Bitmap` -> `stb::` types.
3. [ ] Split `svgelement.{h,cpp}` (about 4,200 lines) by element family; drop `NOVA_INLINE` (13 skipped); public text-measure API
       (`Font`/`ForeignObjectFonts` public first).
4. [ ] ADR-0009 step: namespaces + directories + `snake_case` in one script; CMake target `nova::vg`.
5. [ ] Promote layers/pattern/mask to virtuals of `canvas`; PDF: vector `SMask` for masks, gradient reflect/repeat, per-stop alpha, font embedding,
       multi-page, metadata; element-level `render_to_pdf`; CLI `-o x.pdf|png|jpg`.
6. [ ] Python binding back on (switched off for now by the maintainer): update for `render_to_pdf`, fonts, private includes; rerun the 433 tests.
7. [ ] Step 7 (README, Doxygen, CI, wheels, install tree) and delete the temporary `refactor/` material and the hook in `tests/cpp/CMakeLists.txt`.

## Logging on stb::logging (session 3, after the full gate)

- no wrapper header: each .cpp gets its logger with `stb::logging::get_logger` (names `novasvg`, `novasvg.svg`, `novasvg.font`, `novasvg.pdf`. The library attaches no handler;
  without one, stb::logging's last resort prints warnings and worse to stderr.
- Logged: cannot open / cannot parse a document (error), font load failures (warning), PDF without size (error),
  PDF gradient reflect/repeat and per-stop alpha written differently (warning, once per process), render settings (debug).
- CLI: `--log-level` and `--log-file`; its "Error:" lines go through logger `novasvg.cli` (format `LEVEL: message`).
- Not done: the Python binding should hand `novasvg` records to Python's `logging` with `callback_handler` (Python is OFF);
  a public way to name the loggers is not documented yet; `assert(false)` sites are unchanged.
- Gate after this change (libstb 279716c): C++ 84 cases / 583 assertions, font 71, link OK, golden 67 images 0 differ at tolerance 0.

## Namespaces nova::svg / nova::vg and NOVA_ macros (session 3)

- `novasvg` and `novasvg::gfx` are now `nova::vg` (canvas, backends, path, Rect/Box/Matrix/Transform, Color, Bitmap, fonts, C API,
  `color_literals`, parser utils) and `nova::svg` (document, element, node, `version()`, DOM and CSS code). `nova::svg` has
  `using namespace vg;`. The CMake target and the project name stay `novasvg` (`novasvg::novasvg`).
- Every `NOVASVG_*` identifier (macros, C enum constants, CMake options, env vars such as `NOVA_PDF_DIR`) is `NOVA_*`.
  Build with `-DNOVA_BUILD_PYTHON=OFF` now.
- Dependency direction fixed so `vg` no longer needs `svg`: Box/Matrix/Transform definitions moved to `src/gfx/geometry.cpp`,
  `embeddedFallbackFont` to `src/gfx/embedded_font.cpp`, `svgparserutils.h` to `include/novasvg/gfx/parserutils.h`.
- NOT done (still ADR-0009): `include/nova/{svg,vg}/` and `src/vg/`, target `nova::vg`, `snake_case`, a build check that `vg` cannot
  include `svg`. `svgelement.h` still holds some code that is really vg (check when it is split). Python binding, README/docs
  snippets and CI yml were changed by the same script but are NOT built or checked. ADRs and CHANGELOG keep the old names (history).
