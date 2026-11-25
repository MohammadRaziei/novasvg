# ADR-0016: `<mask>` and clip masks are soft masks in PDF, not bitmaps

**Status:** Accepted and implemented (2026-11-25).
**Date:** 2026-11-25
**Deciders:** Mohammad Raziei (maintainer)

## Context
ADR-0014 made every group that has a mask, a clip mask or a filter a bitmap at `raster_scale()`, run through
`layer_effects` and placed on the PDF page as an image. For a filter that is the only option. A mask is different: the
SVG `<mask>` is a luminance (or alpha) mask over a group, and PDF has exactly that, the soft mask of a graphics state
(`/SMask`, subtype `/Luminosity` or `/Alpha`, ADR-0015 already uses it for fades). Drawn as a bitmap a masked shape
loses its vectors, its text can no longer be selected or zoomed, and the file grows with the page size.

## Options Considered
1. **Keep the bitmap.** Right at one resolution only.
2. **Convert the mask into clip paths or shapes** (a gradient mask into strips). Only works for the easy cases.
3. **Hand the mask to the canvas as a drawing** and let the PDF backend make a soft mask of it.

## Decision
Option 3. `layer_params` gets an optional `layer_mask { std::function<void()> draw; bool luminance; }` and `canvas` gets
`vector_masks()` (false by default, true for `pdf_canvas`). `svg_render_state::begin_group` sets the mask instead of
`needs_pixels` when the canvas keeps masks as vectors and the group has *one* mask or *one* clip mask that needs
masking (several shapes, text, `<use>`), no filter, and nothing nested: the mask element has no `clip-path` or `mask`
of its own and there is no reference cycle. `draw` is `svg_mask_element::paint_content` or
`svg_clip_path_element::paint_content`, the code that `apply_mask` and `apply_clip_mask` already used to fill their
bitmap, now shared (`apply_*` call it with a `raster_canvas`). A clip mask is the *alpha* of its shapes, so it is an
`/Alpha` soft mask and the union of the shapes comes out right where a PDF clip path would not (the clip rule is per
shape).

`pdf_canvas::end_layer` paints the mask after the group's content, as a group of its own (`mask_group`: `begin_group`,
`draw`, `finish_group`), and composites the group through it (`alpha_state(opacity, blend, soft_mask, mask_alpha)`, the
ExtGState names the mask and says whether it is read as luminosity or alpha). Opacity and blend mode of the group go in
the same ExtGState, so mask, opacity and blend mode compose as they do in SVG. What is outside the mask region is not
painted in the mask group, which is a mask value of 0 for both kinds: the group is hidden there, as in SVG.

Everything else stays a bitmap, as before: a mask and a clip mask on the same element, a filter anywhere on the
group, a mask or clip path that itself has a mask or clip path. They could be nested groups; none of them is common.

## One bug found on the way, in the existing code
A pattern's matrix is relative to the default space of the content stream that uses it (ADR-0015): on the page that is
point space with y up, inside a Form XObject it is the form's pixel space. `fade_mask` knew that, but `begin_paint`
(the ordinary gradient and tiling-pattern paint) never did: every gradient or tiling pattern painted *inside a group*
(an `opacity`, `mix-blend-mode` or mask group) got the page matrix and was drawn at three quarters of its size, mirrored
in y. pdfium and poppler agree on this. `begin_paint` now passes `in_form = !m_groups.empty()` to `shading_pattern` and
`tiling_pattern`. It shows in the sample PDFs as correct gradients under group opacity; the 68 sample PDFs stay valid
(`qpdf --check`).

## Consequences
- Checked against the raster render of the same SVG with pdfium at 96 dpi (tests/python/test_pdf_vector_mask.py): mean
  difference under 2 for luminance and alpha masks, a union of two circles as a clip, a mask region, `objectBoundingBox`
  content units, a mask under group opacity, and gradients inside opacity and blend groups; a few edge pixels differ
  (anti-aliasing). tests/cpp/test_pdf_vector_mask.cpp checks the structure (soft mask present, no image, and the fallback
  cases still an image).
- **A PDF luminosity mask weighs R, G, B as 0.30 / 0.59 / 0.11; an SVG luminance mask (and our raster backend) as
  0.2125 / 0.7154 / 0.0721.** Grey, black, white and every gradient between them agree. A saturated colour in a mask
  differs: a black shape seen through a pure green mask comes out 32 levels (of 255) lighter in the PDF than in the
  raster render, through a pure red mask 22 levels darker. PDF has no way to set the weights, so this stays; the raster
  path (`PdfMode.RASTER`) is exact if a mask in vivid colours matters.
- `color-interpolation: linearRGB` on a mask is not honoured, in the raster backend either.
- The PDF has no font embedding and one page; unchanged.
