# ADR-0018: The text of a PDF is selectable (an invisible text layer), and the document title is its /Title

**Status:** Accepted and implemented (2026-11-26).
**Date:** 2026-11-26
**Deciders:** Mohammad Raziei (maintainer)

## Context
ADR-0014 draws text as glyph outlines: exact on every viewer and no font to embed, but the text can be neither selected
nor searched nor extracted. The PDF also had no `/Title`, because the parser skips every element it does not know and
`<title>` is one.

## Options Considered
1. **Embed the fonts** (a subset of the glyphs used, as a TrueType or CFF font program, a Type 0 font, a `/ToUnicode`,
   an instance of a variable font at the weight in use). Exact text everywhere. libstb has no font subsetter, so this is a
   large piece of work in libstb first, and what it buys over option 2 is only that the viewer may draw the glyphs itself.
2. **An invisible text layer over the outlines** (text render mode 3): the characters again, placed on the outlines, in a
   font that has no glyphs and only says which Unicode character each code is.
3. **Nothing.**

## Decision
Option 2. `pdf_canvas::fill_text` and `stroke_text` write, after the outlines, the run once more as characters:
`BT 3 Tr /FTxt <size> Tf <Tz> Tz <Tm> <codes> Tj ET`. The font is a Type 0 font (`Identity-H`, a `CIDFontType2` with no
font file, `/DW 500`) whose `/ToUnicode` maps each two-byte code (a CID, one per distinct code point, in order of first
use) to its character (UTF-16, surrogate pairs included). Every glyph is 0.5 em wide, so the run is stretched (`Tz`) to the
advance the shaper measured, and the text matrix is the element's transform with the y axis flipped, so the characters
sit where the outlines are, rotated or scaled with them.

- A text that is both filled and stroked, in either order (`paint-order`), is written once (the last run and its place are
  remembered); a text that is only stroked is written too.
- Text painted into a mask or clip-mask group (ADR-0016) is not the page's text and is not written.
- Text that is not drawn (`visibility: hidden`, `display: none`) never reaches the canvas, so it is not written either.
- `<title>`: the parser keeps the text of the root `<svg>`'s first `<title>` child (`document::title()`, `Document.title`)
  and `render_to_pdf()` writes it as the `/Title` (UTF-16 when it is not ASCII).

## Consequences
- Measured: of the 68 sample PDFs, 53 have text; no text could be extracted from any of them before, all of it can now
  (pdfium); they are all valid for `qpdf --check`, and rendered with pdfium they are pixel for pixel what they were (largest
  difference 0). Poppler's `pdftotext` was tried by hand on single runs, several lines, `tspan`s, a group with opacity, and
  stroked text, and gave the same text; on rotated text its `-layout` mode scrambles the order (pdfium does not).
- The text lies in the PDF in the order it is drawn. Right-to-left text is extracted in its visual order (that is the order
  in which it reaches the canvas). Two `<text>` elements with the same text at the same place are one text.
- Inside a run all characters are the same width, so a selection is exact at the ends of a run and proportional in
  between. A run is a `<text>`/`<tspan>` piece as the renderer lays it out.
- No font program is embedded (ADR-0019 adds the choice to embed one), so a viewer that wants a glyph for a character the layer names has none; the layer is never
  drawn, so none is asked for. Not tried in Acrobat or Preview, only in pdfium and poppler.
- `PdfMode.RASTER` is one bitmap and has no text layer.
