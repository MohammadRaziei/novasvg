# ADR-0019: PDF text can be real text in an embedded font subset, chosen per call (`pdf_text`)

**Status:** Accepted and implemented (2026-11-27).
**Date:** 2026-11-27
**Deciders:** Mohammad Raziei (maintainer)

## Context
ADR-0014 draws text as glyph outlines and ADR-0018 makes that text selectable with an invisible layer. That is exact on
every viewer, but the file does not hold text in a font: it is a drawing of letters, the way Illustrator's "create
outlines" leaves it. Some users want the other thing, as Illustrator lets them choose: the font (the glyphs the page
uses) inside the file and the letters shown with it, a smaller file for text-heavy pages, text that is text for a
print workflow. ADR-0018 said the large piece of work was a subsetter in libstb; libstb now has one
(`stb::subsetter` and `stb::font::embedding()`).

## Decision
A parameter of `render_to_pdf` / `save_pdf` (C++ `document::pdf_text`, Python `PdfText`, CLI `--pdf-text`), in the vector
mode only:

| value | text is |
|---|---|
| `outlines` (default) | glyph outlines plus the invisible text layer of ADR-0018. The bytes of every existing output are unchanged. |
| `embed` | real text in a subset of the font. A font that may not be embedded keeps outlines, with a warning. |
| `embed_ignore_license` | the same, without reading the font's license bits. Whoever asks for it answers for the license. |

**What a font must have to be embedded.** `stb::subsetter::supported()`: TrueType outlines (`glyf`) and no variation
applied. A CFF font, and a variable font used at another weight than its default, stay outlines in every mode (a
technical limit, not a license one: logged once per font). For `embed` the OS/2 `fsType` is also read through
`stb::font::embedding()`: `restricted`, `bitmap_only` and "no subsetting" keep the text as outlines (the last one
because libstb cannot hand over the whole font file yet), each with its own warning. `preview_print`, `editable` and
`installable` are embedded. libstb only reports the bits; the policy is novasvg's.

**How a run is written.** `pdf_canvas::emit_embedded_text`, instead of the outlines and the layer:
`/Span << /ActualText <FEFF..> >> BDC q <begin_paint> <matrix> cm BT <Tr> /FE0 <size> Tf` and for each shaped glyph
`1 0 0 -1 <x> <y> Tm <cid> Tj`, then `ET Q EMC`. The text keeps its own space and goes through `cm`, so a stroke's width
scales with the matrix as the outlines' does, and a fill uses the paint (colour, gradient, pattern) the outlines used.
`Tr` is 0 for a fill and 1 for a stroke; a text that is both filled and stroked is two runs, as the engine paints them
(the second one has an empty `/ActualText`, so it is read once: `/Artifact` would say the same, but poppler ignores it).

- **CIDs.** The font is a Type 0 font (`Identity-H`) over a `CIDFontType2`. A glyph's CID is its index in the ORIGINAL
  font, so the content stream does not wait for the subset (the glyphs that will be used are known only at the end);
  `/CIDToGIDMap` is a stream that says where each used glyph went in the subset (libstb numbers the glyphs again so
  `loca` and `hmtx` have no gaps). `/W` gives the advance of each used glyph.
- **Text a viewer extracts.** `/ToUnicode` maps a glyph to the character it is the font's glyph for (from the
  characters of each run); the run's `/ActualText` is the whole logical text, which covers ligatures, joined and
  reordered letters, where a glyph is no single character's. What a viewer does with right-to-left text still differs:
  pdfium reads the words whole (it puts the runs in its own visual order), where the layer of ADR-0018 gave the letters
  reversed; poppler reads both modes the same. The order of right-to-left text is therefore not promised.
- **Fonts.** One per distinct `stb::font` (a face is shared, so one font object per face), named `/FE0`, `/FE1`, ...,
  `/XXXXXX+Family` with a tag taken from the glyph set, so the same drawing always gives the same bytes. Font objects are
  made in `data()`, which subsets once, at the end.
- **Text in a mask group** is written with an empty `/ActualText`, like the second half of a fill and stroke.
- If the subset fails at the end (damaged glyph data, which libstb reports as `decode_error`) the text is already in the
  content, so the font goes without a program and an error is logged: the file is valid and extracts, a viewer substitutes
  a font.

## Consequences
- Verified by programs that know nothing of novasvg: pdfium reads the same text in `embed` as in `outlines` (Latin with
  accents, Persian, characters past the BMP, rotation, a group with opacity, a gradient fill, fill and stroke), pypdf finds
  one embedded font per face, fontTools opens its program and every used glyph has the outline and advance the
  original has under `/CIDToGIDMap`, and the page rendered by pdfium differs from the outline one only along the edges
  (the viewer hints a font's glyphs, not a path's): `tests/python/test_pdf_text_embed.py`,
  `tests/cpp/test_pdf_text_embed.cpp`.
- Glyphs are positioned one by one (`Tm` per glyph), so the layout is exactly the shaper's (kerning, marks, joined
  letters) and no `TJ` adjustment is guessed; the content stream is Flate compressed, so the cost is small.
  ponytail: runs of glyphs at their natural advance could be one `Tj`.
- Not done: CFF and variable-instance subsetting (libstb), embedding a font whole when its license allows only that,
  a base-14-like standard font, more than one page.
- The raster mode has no text and ignores the option.
