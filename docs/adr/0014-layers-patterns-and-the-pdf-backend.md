# ADR-0014: layers and patterns belong to the canvas interface; PDF fidelity comes after

**Status:** Proposed, not implemented. Nothing in the code has changed for this record.
**Date:** 2026-11-19
**Deciders:** Mohammad Raziei (maintainer)

## Context
`canvas` (vg/canvas.h) is the abstract drawing surface: colours, gradients, fill and stroke of paths and text, clips,
images, save and restore. It has two backends, `raster_canvas` and `pdf_canvas` (ADR-0008). Layers, patterns and masks
are not part of it, so the svg layer reaches past it to the concrete backends. Read in the code:

- **Groups** (opacity, mix-blend-mode, clip mask, mask, filter): `svg_render_state::begin_group` / `end_group`
  (svg_paint.cpp) switch on four layer kinds (`none`, `raster`, `pdf_group`, `raster_in_pdf`) and use
  `dynamic_cast<pdf_canvas*>` and `static_cast<raster_canvas&>` to do it. On a PDF page, opacity and blend mode become a
  vector transparency group; anything with a mask, a clip mask or a filter is drawn to a bitmap at `raster_scale()` and
  placed as an image.
- **Masks, clip masks, filters** (svg_effects.cpp) work on `state.raster()` only: `blend_canvas(..., destination_in)`,
  `convert_to_luminance_mask()`, `copy_pixels_from()`.
- **Patterns** (svg_paint.cpp): the tile is drawn into a `raster_canvas`, then either `pdf->set_tiled_image(...)` or
  `raster()->set_texture(...)`, chosen by another `dynamic_cast`.
- In all, about 35 references to a concrete backend in 13 files of the svg layer (counted by grep; 12 in svg_paint.cpp,
  7 in svg_effects.cpp).

What the PDF backend does not do today (from pdf_canvas.h and the warnings in pdf_canvas.cpp): gradient `spreadMethod`
reflect and repeat are written as pad; the alpha of each gradient stop is ignored; masks, clip masks and filters are
bitmaps; text is written as glyph outlines (no font is embedded, so it cannot be selected or searched); the file has
one page and no `/Info` entry. Two small test files cover PDF (test_pdf_canvas.cpp, test_pdf_render.cpp). The output has
no timestamp, so it is deterministic and can be compared byte for byte.

## Options Considered
1. **Leave it.** The svg layer keeps knowing both backends. A third backend (SVG output, a display list) would have to
   be threaded through the same casts. Rejected.
2. **Virtual layer and pattern calls on `canvas`, with the pixel work done by a callback (chosen).** See below.
3. **Make `canvas` always pixel-backed and convert at the end.** Loses the vector output, the reason `pdf_canvas` exists.
   Rejected.

## Decision
Step 1, no change of behaviour:

```cpp
struct layer_params {
    rect bounds;            // user units, with any filter margin already added
    blend_mode blend = blend_mode::normal;
    float opacity = 1.f;
};
// The effects a layer needs that only pixels can give (clip mask, mask, filter). Called by the backend, on a
// raster_canvas holding the finished layer, with the pixels per user unit of that canvas.
using layer_effects = std::function<void(raster_canvas& layer, float scale)>;

struct layer_scope {
    std::shared_ptr<canvas> target;  // what the group's children are drawn on
    transform pre_transform;         // identity, or the scale of the bitmap a PDF page makes for the group
};

virtual layer_scope begin_layer(const layer_params&, bool has_effects);
virtual void end_layer(const layer_params&, const layer_effects&);   // empty: no effects

virtual float raster_scale() const { return 1.f; }                    // pixels per user unit of a tile or layer bitmap
virtual void set_pattern(const pixmap& tile, const transform& tile_to_user, float opacity);
```

`raster_canvas` makes an offscreen raster canvas and blends it back (what `begin_group` / `end_group` do now). `pdf_canvas`
opens a transparency group when `has_effects` is false, and otherwise makes the bitmap itself, calls the effects on it and
places it: the `raster_in_pdf` branch moves from svg_render_state into the backend. `svg_render_state` then has no
`layer_kind`, no `dynamic_cast` and no `static_cast`. Masks and filters keep their raster code; they receive the layer
as an argument instead of fetching it from the state. Patterns call `set_pattern`; `set_texture` and `set_tiled_image`
become the two backends' implementations of it.

Step 2, PDF fidelity, in this order, each behind its own test:
1. Gradients: `spreadMethod` reflect and repeat (a stitching function repeated up to a bound, mirrored for reflect); a
   gradient whose stops all share one alpha uses a constant alpha.
2. `/Info` metadata (producer, and the title from `<title>`).
3. Per-stop alpha through a soft mask, vector `<mask>` and clip masks as soft masks, and embedding fonts as subsets are
   larger pieces of work and are **not** part of this decision; they need their own records once the interface is in.
   Multiple pages are only worth it if a document API for several SVGs is wanted.

## Trade-off Analysis
The callback keeps the svg effects code (about 330 lines in svg_effects.cpp) untouched in substance and lets the PDF
backend vectorize a mask later without the svg layer noticing. The cost is a `std::function` per layer that has
effects (not per layer: `has_effects` false takes no callback) and a `canvas` interface that mentions `raster_canvas`,
which is acceptable because `raster_canvas` is the pixel type of the whole library.

## Consequences
- Step 1 is checked by: the C++ tests; the raster renders unchanged (the golden files, not at tolerance 0: they are to be
  deleted at the end of the refactor); and the PDF of every sample SVG byte-identical before and after (the output is
  deterministic).
- Unknowns that reading did not settle: whether `svg_foreign_object.cpp`, `svg_image.cpp` and `svg_text.cpp` need
  more than a one-line change, and how clipping mode (`svg_render_mode::clipping`) passes through `begin_layer`.
- The size is an estimate from reading: about 150 lines touched for step 1 (svg_paint.cpp loses most of begin / end group,
  pdf_canvas.cpp gains the raster layer path, canvas.h and raster_canvas gain the virtuals).

## Action Items
- [ ] Settle the interface above (names, whether `has_effects` is a flag or the callback's presence).
- [ ] Step 1, then the PDF byte comparison.
- [ ] Step 2, items 1 and 2.
