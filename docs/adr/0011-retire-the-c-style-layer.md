# ADR-0011: Retire the C-style layer in `render.h`

**Status:** Accepted (maintainer: go ahead, order 1 -> 2 -> 3); step 1 done (2026-11-08)
**Date:** 2026-11-08
**Deciders:** Mohammad Raziei (maintainer)

## Context
`src/sources/vg/render.h` (2063 lines) is the old plutovg C API: 174 `NOVA_API` functions and 25 `_t` types
(`canvas_t`, `surface_t`, `paint_t`, `font_face_t`, `matrix_t`, `color_t`, `point_t`, `rect_t`, enums like
`operator_t`). ADR-0008 kept it "until C and E replace it". C (abstract `canvas`) is done, and libstb now ships
the pieces E needs: `stb::matrix` and `stb::outline` (`stb/outline.hpp`), `stb::color`, and `stb::fill` /
`stb::stroke` with coverage spans (`stb/raster.hpp`).

Where it is still used (calls / type uses, outside `render.h`):
- `canvas_*` 294 calls, almost all in `canvas.cpp` and `raster_canvas.cpp`; `canvas_t` 206 uses.
- `matrix_*` 116 calls, `matrix_t` 117 uses (`matrix.cpp`, `pdf_canvas`, `raster_canvas`, `outline_util`).
- `font_face_*` 172 calls and `font_face_cache_*` 53 (`font_face.cpp`, `font.cpp`).
- `surface_*` 102 calls (`raster_canvas.cpp`, `surface.cpp`, `pixmap.cpp`); `paint_*` 51; `color_init*` 30.
- The enums (`operator_t`, `spread_method_t`, `fill_rule_t`, `line_cap_t`, ...) are small: 7-27 uses each.

## Decision (proposed)
Remove the layer in four steps, each one gated (tests, golden 0 diff, ASAN), none changing behaviour:
1. **Enums** `*_t` -> `enum class` in the style of ADR-0004. DONE: 8 enums; `operator_t` merged into `blend_mode`,
   the duplicate enum classes in `canvas.h` removed; golden 0 diff.
2. **Value types** (matrix and point DONE 2026-11-08, golden 0 diff; the arithmetic of `stb::matrix` equals the old C one,
   so no pixel moved). `color_t` is a float RGBA used only for gradient stops (not `stb::color`, which is 8-bit), and `rect_t`
   has no libstb type: both go with step 3 (`paint`, and the existing `vg::rect`). Original plan: `point_t`, `rect_t`, `matrix_t`, `color_t` -> `stb::outline` point, a local `rect`, `stb::matrix`,
   `stb::color`. The golden set decides if `stb::matrix::map` (no fused multiply-add) changes any pixel.
3. **Handles -> classes** with RAII: `surface`, `paint`, `font_face`, `font_face_cache` get member functions
   instead of `surface_*(h, ...)`; `canvas_t` goes away because `raster_canvas` already is the canvas.
4. Delete `render.h`, `NOVA_API` on the C functions, and `src/gfx/vendor` leftovers; rasterizer swap to `stb::fill`
   / `stb::stroke` is a separate step after this one.

## Consequences
No public API change (`render.h` is private). Every step is a rename or a type swap, so the golden set stays the
oracle. Step 2 is the only one that can move pixels.
