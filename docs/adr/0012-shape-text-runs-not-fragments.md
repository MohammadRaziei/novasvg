# ADR-0012: shape text runs, split the result into fragments

**Status:** Accepted and implemented (2026-11-10, step 36), except point 5 (`glyph_kind`), see below.
**Deciders:** Mohammad Raziei (maintainer)

## Context
`svg_text.cpp` splits the text of a `<text>` element into *fragments* at every character that has its own position
(`x`, `y`, `dx`, `dy`, `rotate`, a new `<tspan>`) and measures and draws each fragment alone
(`font.measure_text` per fragment). libstb's `font::shape(utf8, begin, end)` (since `6af6df8`) shapes a range of a
longer text *with the context of the whole run*: bidirectional order, joined Arabic letters, ligatures, mirrored
brackets, kerning and GPOS mark placement are decided on the full run, and the glyphs come back with the pen already
placed and with a `glyph_kind` (line break or missing glyph). Shaping every fragment alone throws that context away
exactly where it matters: a kerning pair, a ligature or an Arabic join that crosses a fragment boundary.

## Decision (proposal)
1. A **run** is the text of one text chunk that is drawn with one font (size, family, style, weight): it ends where
   the font changes or where an absolute `x`/`y` starts a new text chunk (SVG 2, 11.5.1). Text inside `<tspan>`s that
   only change the fill, `dx`/`dy`, or `rotate` stays in the same run.
2. Each run is shaped **once**, with `shape(utf8, begin, end)` on the run's UTF-8 text.
3. The glyphs are then **split into fragments** at the cluster boundaries that the positional attributes ask for.
   A position attribute that falls *inside* a cluster (a ligature, a joined Arabic group) leaves the cluster whole
   and applies to its first character, which is what browsers do.
4. A fragment's width is the sum of the pen advances of its glyphs in the shaped run, so a kerning pair that crosses
   a fragment boundary is kept (today it is lost).
5. `glyph_kind` is used for what it says: a line-break glyph is not drawn and not measured; a missing glyph keeps
   the current notdef behaviour of `font_face_data::shape`.

## Consequences
- Latin text with several `<tspan>`s on one font may move by sub-pixel kerning amounts, and Arabic text split across
  `<tspan>`s now joins across the boundary. Expect some of the golden images that contain tspans to change; each
  difference must be explained (kerning, join) before it is re-baselined.
- `svg_text_fragment` keeps its fields (x, y, width, height, length); only how width and glyphs are obtained changes,
  so `textLength`, `text-anchor`, `lengthAdjust` and per-character `rotate` code is not touched.
- Vertical text (`writing-mode`) shapes per character as it does now; it is out of scope.

## Open points (needs the maintainer or a first spike)
- How `letter-spacing` and `word-spacing` are applied to a shaped run (libstb `shape_options`?). Today they are added per fragment.
- Whether the PDF canvas and the raster canvas should share the glyph list of a run instead of shaping again at draw time.
- A test set first: tspans with kerning pairs (`AV`), a ligature (`fi`) split by `dx`, and Arabic split by a `<tspan>`.

## Outcome (step 36)
- A **run** is found once per `<text>` (`svg_text_fragments_builder::find_run_starts`): it starts at the first character, at a
  change of font (face or size) and at every character with an absolute `x` or `y`. Vertical text is shaped per fragment as before.
- Each fragment keeps the run it belongs to (`run_offset`, `run_length_`) and is measured and drawn with
  `shaping_context{run, ligatures}`: `sized_font::measure_text`, `fill_text`, `stroke_text` (raster and PDF) and
  `font_face_data::shape` take it and call libstb's `shape(utf8, begin, end)`. Raster and PDF therefore get the same glyphs.
  The run is shaped again for every fragment instead of once (the cost is one shaping of the run per fragment; fragments are
  few unless `rotate`, `letter-spacing` or per-character positions make one per letter).
- `letter-spacing` is the open point that is decided: a non-zero value sets `ligatures = false` (`shape_options::ligatures`),
  kerning stays.
- Tests: `tests/cpp/test_text_runs.cpp` (kerning `AV` across a `<tspan>`: 52.17 px with the run, 54.73 px without; the `fi` ligature
  stays whole in the fragment of its first letter and is dropped with `ligatures = false`).
- **Golden images: 0 differ.** None of the 67 contains a pair that kerns or a ligature across a fragment boundary.
- Not done: point 5. `font_face_data::shape` still ignores `glyph_kind` (a line-break glyph and a missing glyph are both glyph 0);
  no test needs it yet. Arabic across `<tspan>`s has no test either (libstb joins it by the same call; not verified here).
