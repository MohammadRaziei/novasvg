# novasvg: refactor checklist (compiled library, libstb, stl-like naming)

> Separate from the project's existing `checklist.md`, which is untouched.
> Goal: stop being header-only, use `libstb` wherever it fits, adopt stl-like naming, and tidy the design.
> This is a breaking change. The reasoning behind each decision is in [`docs/adr/`](docs/adr/README.md).

## Working rule

A step is done only when **all** of these hold:

1. The project builds.
2. The tests match the baseline or better (numbers below).
3. `tools/golden.py diff` over the 62 golden renders shows zero differences (or each difference is explained here).
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
python3 tools/golden.py render ./build/novasvg /tmp/golden-new
python3 tools/golden.py diff refactor/golden-baseline /tmp/golden-new
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
python3 tools/golden.py render /tmp/b0/novasvg refactor/golden-baseline
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
| [0003](docs/adr/0003-public-image-type-is-stb-image.md) | Public pixel type is `stb::image`; premultiplied render surface stays internal | Accepted (step 2b) |
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

### Step 0: baseline and verification tooling
- [x] Clone novasvg and libstb
- [x] libstb builds (static, standalone)
- [x] novasvg baseline builds; tests green (numbers above)
- [x] `tools/golden.py` written and tested; 62 golden renders made (in `refactor/`, git-ignored)
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

#### 2b: `Bitmap` leaves the public API
- [ ] `render()` returns `stb::image` (in-place conversion plus `image::wrap` with `keep_alive` on the surface; no copy)
- [ ] Rendering onto an existing image: render a transparent layer and composite it with `stb::image::blend`
      (instead of drawing straight into straight-alpha pixels); test `mix-blend-mode`
- [ ] `Bitmap` becomes an internal (`detail`) type; `writeTo*` removed (`stb::image` encoders take over)
- [ ] CLI, tests, examples and the Python bindings (numpy interop) updated
- [ ] Tests and golden; watch: premultiply/unpremultiply, rendering onto an existing image, stride
- [ ] When libstb's conversion-kernel issue lands, swap the two call sites (final hand-off and `<image>` decode)

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

### Step 5: remove duplicates
Today each concept is defined in several layers (public, `detail/canvas.h`, `detail/render/`).
- [ ] `Color` becomes `stb::color` (it was derived from this code)
- [ ] One `matrix` instead of `Matrix` / `Transform` / `matrix`
- [ ] One `rect` / `point` / `size` / `box`
- [ ] One `path`
- [ ] Merge `Font` / `FontFace` / `FontFaceCache` with `font_face` / `font_face_cache` / `glyph`
- [ ] Settle `Canvas` / `canvas` and `Bitmap` / `surface`
- [ ] `render/vendor/ft_*` (FreeType/plutovg code) stays; just document where it lives

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

### Step 7: packaging and release
- [ ] Python bindings and Python tests
- [ ] Wheels and CI (Windows, macOS, manylinux, musllinux), including shipping libstb
- [ ] `CHANGELOG`, README, docs, version (breaking)
- [ ] `find_package(novasvg)` and `novasvgConfig.cmake`
- [ ] Update README and licence notices that still name stb_image
