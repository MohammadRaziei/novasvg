# ADR-0015: a gradient whose stops differ in alpha is a luminosity soft mask in PDF

**Status:** Accepted and implemented (2026-11-22).
**Date:** 2026-11-22
**Deciders:** Mohammad Raziei (maintainer)

## Context
ADR-0014 left one gap in the PDF backend: a gradient whose stops have different alpha (`stop-opacity` going from 1 to
0, the usual "fade") was painted opaque, with a warning. A PDF shading has colour only; alpha cannot be in its function.
What a gradient can have is one alpha shared by all its stops (that is what `fill-opacity` becomes), which is an
ordinary constant alpha in the graphics state.

## Options Considered
1. **Rasterize the shape** and place it as an image with an alpha channel. Always right, but it loses the vector
   output that the PDF mode exists for, and the resolution is a guess.
2. **Split the gradient into many thin constant-alpha strips.** Vector, but file size grows with the smoothness wanted,
   and radial, reflect and repeat make it a project of its own.
3. **A soft mask** (`/SMask`, subtype `/Luminosity`) over the same shading. The mask is a transparency group that paints
   the same gradient geometry in grey levels equal to the stop alphas (luminosity of `(a, a, a)` is `a`); the colour
   shading stays as it was. Vector, small (one more shading and one small form per fade), and handles linear, radial,
   pad, reflect and repeat with the code that already builds the shading.

## Decision
Option 3. `shading_pattern` takes the stops to paint, so `fade_mask` reuses it with grey stops; the mask is a Form
XObject whose content is one rectangle over the page painted with that pattern, named by the ExtGState that the paint
uses (`alpha_state(alpha, blend, soft_mask)`). Every ExtGState now names its mask, `/SMask /None` when it has none,
because a graphics state parameter stays in force until another one replaces it.

## Two things the first attempt got wrong, kept here because they are easy to get wrong again
- **A pattern's matrix is relative to the default space of the content stream that uses it.** On the page that is point
  space with y up, so the matrix carries the pixel-to-point flip. Inside a Form XObject it is the form's own space, which
  is the pixel space the page content already works in, so the same flip applied again scaled the mask by 0.75 and
  mirrored it (the fade ended at 130 px instead of 190 px and a radial fade left its form's BBox and vanished).
  `shading_pattern(..., in_form)` leaves the flip out for the mask.
- The soft mask is read in the space the `gs` that names it runs in, so its BBox and rectangle are in pixels.

## Consequences
- Checked on a page with a linear fade, a radial fade with three stops, a reflect fade and a stroke fade; and under a
  group `opacity`, a `rotate`/`scale` transform, `fill-opacity` and a radial gradient with a focal point. Against the
  raster render the mean difference is under 1.2 (pdfium) and 0.5 (poppler), with at most 0.03% of the pixels more than 20 off, and
  `qpdf --check` finds no error in any of the 68 sample PDFs.
- PDF interpolates colour and alpha separately (non-premultiplied, as browsers do for SVG gradients). The raster backend
  appears to interpolate premultiplied, so a fade between two *different* colours looks lighter there than in the PDF.
  A fade between the same colour, the common case, is identical in both. Not resolved: which of the two the raster
  backend should do is a libstb question.
- The stop alphas are the *only* alpha the mask carries: a constant alpha from `opacity` on the element is still the
  ExtGState `/ca` and multiplies with it, as it should.
- Not covered: the SVG `<mask>` element and clip masks in vector form, font embedding and more than one page. They are
  separate changes.
