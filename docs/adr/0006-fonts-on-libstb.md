# ADR-0006: Build fonts on libstb; add a glyph-outline API to libstb

**Status:** Accepted; implemented in step D3 (2026-11-04) for faces, metrics, outlines, kerning and ligatures. libstb now has the outline API
(`font::outline`, `glyph_index`, `glyph_info`, `design_metrics`, `glyph_kerning`, `face_count`), so nothing had to be added there for this step.
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

novasvg embeds stb_truetype and stb_rect_pack, about 5,000 lines at the top of `gfx/font_impl.h`, and builds
its own font loading (`FontFace`, `FontFaceCache`, WOFF handling) and shaping (kerning, GPOS) on them.
Its own code calls **13 stb_truetype functions**:

`stbtt_InitFont`, `GetFontOffsetForIndex`, `GetNumberOfFonts`, `FindGlyphIndex`, `GetGlyphShape` (and
`FreeShape`), `GetGlyphHMetrics`, `GetGlyphBox`, `GetFontVMetrics`, `GetFontBoundingBox`,
`GetCodepointKernAdvance`, `ScaleForMappingEmToPixels` and `ScaleForPixelHeight`.

libstb already has a font module: `font::from_memory` / `open` (with a collection index), `metrics`,
`has_glyph`, `advance`, `kerning`, `measure`, `render_glyph`, and `shape_glyphs`; plus `shaping.hpp` with
`shape_for_display`, `has_rtl` and Arabic-mark helpers. But its rendering is to **bitmaps**. SVG text is drawn
as **vector outlines** (so it can be transformed, filled with gradients, stroked, clipped), and
`GetGlyphShape` is the one call that has no libstb counterpart. Also missing from libstb's public surface, as far
as its headers show: glyph outline, per-glyph metrics and box in font units, the font bounding box, and the
number of faces in a collection.

## Decision (proposed)

Build `FontFace` on `stb::font` and delete the vendored stb_truetype and stb_rect_pack, after adding to libstb
what novasvg cannot do without. The API is requested from libstb as an issue; its shape is the libstb
maintainer's to settle, but novasvg needs at least:

- glyph index for a code point (and a way to see "missing glyph");
- glyph outline: move, line, quadratic and cubic segments, in font units;
- per-glyph advance, left bearing and bounding box, in font units;
- font bounding box, units per em, ascent, descent and line gap in font units;
- face count for a collection (`.ttc`);
- kerning (kern table and GPOS) addressed by glyph index.

## Options Considered

### Option A: Keep the vendored stb_truetype

| Dimension | Assessment |
|---|---|
| Effort | None |
| Duplication | About 5,000 lines stay, next to libstb's own TrueType code |
| Risk | None now |

### Option B: Add an outline API to libstb, build novasvg on it (proposed)

| Dimension | Assessment |
|---|---|
| Effort | Medium: new libstb API plus reworking `FontFace` and the shaper |
| Duplication | Removed |
| Risk | Text output must stay identical; Arabic/Persian shaping must keep working |

### Option C: Outlines through a visitor instead of a returned array

A variant of B: `font::visit_outline(glyph, sink)` calls the sink for each segment, so no per-glyph allocation.
Left to libstb's maintainer to weigh against a returned `outline` value.

## Trade-off Analysis

Option A is only attractive if the font work is too risky to do now, and it can be deferred without blocking
anything else. Option B is the only way to stop carrying a second TrueType implementation. Since libstb's
shaper already covers bidi, Arabic joining and glyph substitution, there is a real chance that novasvg's
Persian/Arabic text support gets simpler, not riskier; but that is unproven.

## Consequences (if accepted)

- **Easier:** about 5,000 fewer vendored lines; one font stack for both libraries; fixes to libstb's shaper reach
  novasvg.
- **Harder:** novasvg's shaping code (kerning, GPOS) must be compared with `stb::font::kerning` and
  `shape_glyphs`; any gap either moves into libstb or stays in novasvg.
- **Open questions** (to settle before implementing):
  1. Does `shape_glyphs` return glyph indices that can be drawn directly, with substitutions applied, and does it
     position marks (the Arabic mark helpers suggest partial handling)?
  2. Is kerning by GPOS available and equal to what novasvg does today?
  3. Cost of outline extraction per glyph, and whether novasvg or libstb should cache it.
  4. Thread-safety guarantees of `stb::font` for concurrent rendering.
  5. WOFF: the inflater is already libstb's (ADR-0005); WOFF container parsing stays in novasvg unless libstb
     adds it.

## Action Items

1. [ ] Re-read `libstb/include/stb/font.hpp` and write the libstb issue (the way the conversion-kernels issue was written).
2. [ ] Settle the shape of the outline API on the libstb side (returned value or visitor, units, caching).
3. [ ] Rebuild `FontFace` on `stb::font` behind the existing tests; compare Persian, Arabic and Latin renders.
4. [ ] Add text-heavy SVGs, including RTL and embedded fonts, to the golden set before changing anything.
5. [x] Deleted the vendored stb_truetype (`gfx/font_impl.h`, 6266 lines); stb_rect_pack was never used by novasvg's own code.

## Implementation notes (D3)

- `src/gfx/font_face.cpp` holds `font_face_t` (wraps `stb::font`, caches per code point the glyph index, metrics, box and outline),
  the face cache and the system-font scanner. Private declarations are in `src/gfx/font_face.h`.
- Outlines: `font::outline` gives move/line/quad/cubic/close. Quads are raised to cubics exactly as before; `close` is skipped, as stb_truetype never
  emitted it, so stroked text stays the same.
- Ligatures (ff, fi, fl, ffi, ffl): a sequence is taken when `font::shape_glyphs` turns it into the one glyph that U+FB00..FB04 maps to, the same
  condition the old GSUB probe checked.
- libstb has no `name`-table or `head.macStyle` access, so the system-font scanner parses those two tables itself (about 60 lines).
- `font_face_enumerate_codepoints` asks `has_glyph` for every scalar value instead of walking cmap formats 4/12: simpler, covers every cmap format,
  and now skips glyph 0 for format 12 too.
- `units_per_em` is exact now (was `1 / scale` in float).
- Shaping/RTL stays in novasvg's own code for now; adopting `stb::shape_for_display` is a later step.
