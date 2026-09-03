# novasvg vs resvg vs lunasvg vs cairosvg — empirical, same 8 files

Engines actually run (not guessed): novasvg-cli (this repo, built from source),
resvg 0.46-class via `resvg_py` (Rust, what mermaidx itself uses), lunasvg 3.5
(built from source, official `svg2png` example), CairoSVG 2.9 (Python/Cairo).
Ground truth for the 4 mermaid samples is mmdc (real Chrome, mermaid.js).

| Test | novasvg | resvg | lunasvg | cairosvg |
|---|---|---|---|---|
| venn — plain shapes/text | match | match | match | match |
| block — plain shapes/text | match | match | match | match |
| flowchart — `<br/>`/text inside `<foreignObject>` | partial: text present but line-breaks/wrapping broken, 1 label dropped | **blank** (foreignObject text not rendered at all) | **blank** | **blank** |
| zenuml — nested HTML+CSS inside `<foreignObject>` (Vue-rendered) | only raw concatenated text, no layout | blank canvas, nothing drawn | **fails to parse the file at all** (hard error) | **hard crash**: `ValueError: could not convert string to float: 'calc(100'` |
| base64 `<image>` (href/xlink:href) | ok | ok | ok | ok |
| linear/radial gradient | ok | ok | ok | ok |
| `feDropShadow` / `feGaussianBlur` filter | **not applied** | **applied correctly** | not applied | not applied |
| `clip-path` | ok | ok | ok | ok |
| `mask` (luminance) | ok | ok | ok | ok |
| CSS class fill/stroke + `<use>`/`<symbol>` | ok | ok | ok | ok |
| CSS `transform:` property (not the `transform=` attribute) | not applied | not applied | not applied | not applied |

## Takeaway

Not a strict "better/worse" — it's a different trade-off per engine:

- **Plain SVG (shapes, gradients, clip, mask, use/symbol, raster embeds):**
  all four are equivalent. novasvg's rasterizer lineage is a from-scratch
  restructure of lunasvg's (same plutovg/FreeType-derived rasterizer under
  the hood — see `docs/about.md`), so this parity is expected, not a surprise.
- **Filter effects (blur, drop-shadow):** resvg is the only one of the four
  that actually implements them. novasvg, lunasvg and cairosvg all skip them
  silently.
- **HTML-in-SVG (`<foreignObject>`), which is what mermaid.js leans on for
  every text label:** this is where novasvg actually pulls ahead of the
  other three — it's the only engine of the four that extracts *any* text
  from foreignObject content (imperfectly: no wrapping, occasional dropped
  label). resvg and lunasvg render it as empty boxes; cairosvg crashes
  outright on the CSS `calc()` zenuml uses, and lunasvg's parser rejects
  that file entirely.
- None of the four apply the CSS `transform:` *property* (as opposed to the
  SVG `transform=` attribute) — so that's not a novasvg gap specifically,
  it's shared across this whole class of "SVG-only, no HTML/CSS box model"
  renderers.

So: for pure vector art, no difference. For anything that (like mermaid)
relies on foreignObject text, novasvg is the strongest of these four —
still incomplete, but the only one that tries. For filter-heavy SVG, resvg
is the one to reach for.
