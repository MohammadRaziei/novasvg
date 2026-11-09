# ADR-0011: `vg` builds on libstb instead of duplicating it

**Status:** Accepted (2026-11-08). The file name keeps the first draft's title.
**Deciders:** Mohammad Raziei (maintainer)

## Context
`src/sources/vg` began as a vendored plutovg (a C library) and kept its C-shaped layer in `render.h`: types with
a `_t` suffix, `function(handle, ...)` functions, its own matrix, point, enums and span structs. libstb now
ships most of what that layer implemented. The maintainer's rule for this refactor is only two things: names in
snake_case, and nothing that libstb already has is written a second time here. A `_t` suffix is fine (it is the
STL style too); it is not a goal either way.

The first draft of this ADR was about "retiring the C style". That was a wrong reading of the goal: it renamed
and re-shaped things without removing any duplicate. The steps below are what actually removes duplicates.

## Decision: use libstb's type, delete ours
Done (each with tests, golden 0 diff, ASAN):
- `matrix_t` and `point_t` -> `stb::matrix`, `stb::point`; the 17 `matrix_*` functions are gone (the arithmetic is
  identical, checked line by line). Three helpers libstb lacks stay (`map_points`, `mapped_bounds`, `parse_matrix`).
- `fill_rule`, `line_cap`, `line_join` -> `stb::fill_rule`, `stb::line_cap`, `stb::line_join` (`using` in `vg`).
- `stroke_style_t`, `stroke_dash_t`, `stroke_data_t` -> `stb::stroke_style` (width, cap, join, miter limit, dashes,
  dash offset are exactly its fields); the drawing state owns it by value.
- `span_t` -> `stb::span` (the rasterizer's sink copies spans with `std::copy` now).
- The eight C enums became `enum class` earlier; those not in libstb (`blend_mode`, `spread_method`, `texture_type`,
  `text_encoding`, `path_command`) stay ours. `font_face_t` became the class `font_face_data` (a style change, kept).

## Still duplicated, in order of size
1. `blend.{h,cpp}` (1006 lines, Porter-Duff operators): waits for the libstb issue (`composite_op` and `exclusion`).
2. `surface.cpp` / `pixmap.cpp` (337 lines): `stb::image` with a `pixel_format`.
3. ~~`color_t`~~ DONE: gradient stops were already 8-bit (`with_alpha`), so `stb::color` is exact. `color_parse` (CSS syntax,
   148 names) stays until libstb accepts CSS syntax and all keywords: see `refactor/libstb-issues.md` (issues 2 to 5).
4. `font_face_data`: what it adds over `stb::font` is the glyph cache and the size scale; the rest (metrics, outline,
   kerning, shaping) should call `stb::font` directly. The updated libstb (6af6df8: GPOS marks, `glyph_kind`, shaping
   of a text range) makes this possible for text drawn in pieces.
5. `font_face_cache`: check what libstb's font discovery covers before rewriting the malloc'd entry array.

## Not duplicates (stay)
Paints (gradients, patterns, masks as spans), the canvas interface (`canvas`, `raster_canvas`, `pdf_canvas`), SVG
parsing and layout, and the span buffer (storage and intersection of spans): libstb leaves these to the caller.
