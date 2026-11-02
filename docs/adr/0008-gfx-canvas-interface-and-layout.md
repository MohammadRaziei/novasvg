# ADR-0008: A drawing layer (`gfx`) behind an abstract canvas; no `detail/`

**Status:** Accepted (direction); step A (layout) done, steps B-F open
**Date:** 2026-11-03
**Deciders:** Mohammad Raziei (maintainer)

## Context
The SVG renderer drew straight into a raster surface through C-style plutovg functions (`path_t*`, `matrix_t*`),
and the drawing code was hidden under `detail/`. The maintainer wants libstb plus novasvg to double as a general
vector-drawing/plot library, and wants room for SVG-to-PDF later. Hiding the drawing layer and tying it to pixels
rules both out.

## Decision
1. Three layers: **libstb** (`image`, `color`, `font`, `path`, `matrix`, later the rasterizer) -> **`novasvg::gfx`**
   (public drawing layer) -> **svg** (DOM, parser, CSS, layout, filters).
2. `gfx` exposes an abstract `canvas` (save/restore, transform, clip, fill, stroke, draw_image, draw_glyphs,
   begin/end layer with opacity and blend mode) over `stb::path`, a `paint` (solid, gradient, pattern) and a
   `stroke_style`. Backends: `raster_canvas` (stb::image) now, `pdf_canvas` later. SVG rendering targets the
   interface only. SVG filters stay raster-only (a PDF backend rasterizes them).
3. No `detail/`. Public headers live in `include/novasvg/{gfx,svg}/`; private headers sit beside their `.cpp` in
   `src/gfx/` and `src/svg/`. Headers that must stay public only because the font stack is still inline
   (`gfx/internal.h`, `gfx/utils.h`, `gfx/font_impl.h`) are marked temporary.
4. Naming follows ADR-0004. The namespace is `novasvg::gfx` for now; a rename to a standalone name is a one-line `sed`.

## Order
A layout move (done) -> B naming by script (ADR-0004) -> C abstract `canvas` + `raster_canvas`, golden must stay
identical -> D split `svgelement.h`, compile the svg layer -> E swap in `stb::path` / `stb::matrix` / `stb::color`
and libstb's rasterizer when it lands (then drop `src/gfx/vendor/`) -> F `pdf_canvas`.

## Consequences
`render::` is now `gfx::`. The C-style handle API stays until C and E replace it. A header-only single-file dist is
no longer a goal (ADR-0001).
