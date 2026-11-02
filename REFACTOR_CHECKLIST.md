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
      -DNOVASVG_BUILD_PYTHON=OFF -DNOVASVG_BUILD_TESTS=ON -DNOVASVG_BUILD_EXAMPLES=OFF
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
cmake -S . -B /tmp/b0 -G Ninja -DCMAKE_BUILD_TYPE=Release -DNOVASVG_BUILD_PYTHON=OFF \
      -DNOVASVG_BUILD_TESTS=OFF -DNOVASVG_BUILD_EXAMPLES=OFF && cmake --build /tmp/b0
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
| Macros | `NOVASVG_*` | `NOVASVG_API` |
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
- [x] libstb via `FetchContent` (repository and pin are overridable: `NOVASVG_LIBSTB_GIT_REPOSITORY` /
      `NOVASVG_LIBSTB_GIT_TAG`; local checkout: `-DFETCHCONTENT_SOURCE_DIR_LIBSTB=/path/to/libstb`;
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
- [ ] `Bitmap` still `novasvg::Bitmap` (marked internal); moving it to `detail` waits for step 4 (public/internal split)

### Step 3: fonts, remove stb_truetype (see ADR-0006, proposed)
novasvg calls 13 stb_truetype functions. `stb::font` renders text to bitmaps, but SVG needs **vector glyph
outlines** (`stbtt_GetGlyphShape`), so libstb must grow an API first.
- [ ] Re-read libstb's `font.hpp` and write the libstb issue: glyph index, glyph outline (move/line/quad/cubic),
      per-glyph metrics and box, font bounding box, face count in a collection, kerning (kern and GPOS)
- [ ] Add that API in libstb (its own repository, tests and zip)
- [ ] Rebuild `FontFace` on `stb::font`
- [ ] Persian/Arabic shaping (bidi, joining, GSUB) through `stb::shape_*`
- [ ] Extend the golden set first with RTL text and embedded-font SVGs
- [ ] Remove stb_truetype and stb_rect_pack (about 5,000 lines) from `render/font.h`
- [ ] Tests and golden; watch: RTL text, kerning, embedded fonts, WOFF, `.ttc`

### Step 4: from header-only to compiled (ADR-0001)
- [ ] Public/internal boundary: thin public headers (`document`, `element`, `node`, `box`, `matrix`)
- [ ] Move `detail/` into `src/`, one module per small change
- [ ] Split `svgelement.h` (about 7,600 lines, about 90 types): DOM, properties, filters, gradients, text, foreignObject
- [ ] Make the CSS/HTML engine (`Rule`, `SimpleSelector`, `RuleIndex`, `HtmlNode`, `StyleNode`) its own module
- [ ] `NOVASVG_API` and symbol visibility
- [ ] CMake targets: `novasvg::novasvg` becomes the real library; decide what `noflag` means
- [ ] Decide the fate of the dist single header and `NOVASVG_TEST_INCLUDE_DIR`
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
- Lesson: no function may be left `NOVASVG_INLINE` in a header once `render.h` declares it `NOVASVG_API`
  (undefined reference in TUs that only see the declaration); everything moved, inlining relies on LTO
- Build time so far: barely better (test_basics 43 -> 32 s, check_embedded_font 55 -> ~42 s, CLI 23 -> 27 s; rough,
  from `.ninja_log`), because `svgelement.h`, `font.h` and the rest of `detail/` are still header-only
- [x] One definition of the export macros: `detail/config.h` (the duplicate block in `render/render.h` removed).
      `NOVASVG_API` has the same scheme as libstb's `STB_API` (`NOVASVG_BUILDING_LIBRARY` / `NOVASVG_STATIC_DEFINE`,
      set by CMake on `novasvg_core`); `NOVASVG_MAX/MIN` stay as they are (maintainer's decision)
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
- [ ] Write the old-to-new name table for the public API and get it approved
- [ ] Rename by script, one module per commit
- [ ] Update the CLI, tests, examples, docs and the Python bindings (which may stay Pythonic)
- [ ] Decide on the `nova` alias

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
