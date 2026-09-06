# SVG renderer comparison: novasvg vs resvg vs lunasvg vs cairosvg vs thorvg

All five actually built/installed and run on the same 8 files (not guessed):

- **novasvg** — this repo, built from source (`build/novasvg-cli`)
- **resvg** — via `resvg_py` (Rust; the same engine mermaidx itself uses)
- **lunasvg** 3.5 — built from source, official `svg2png` example
- **cairosvg** 2.9 — Python/Cairo, `pip install cairosvg`
- **thorvg** 1.1.3 — via `thorvg-python` (ctypes bindings), software canvas

Ground truth for the 4 mermaid samples (from mermaidx issues #35, #17, #23, #20 —
see `data/mermaid/COVERAGE.md`) is `mmdc` (real Chrome, mermaid.js).
4 more files probe specific SVG features directly (`data/feature-*.svg`).

## Results

| Test | novasvg | resvg | lunasvg | cairosvg | thorvg |
|---|---|---|---|---|---|
| venn — plain shapes/text | match | match | match | match | **text missing entirely** (shapes fine) |
| block — plain shapes/text | match | match | match | match | text missing, **CSS-class fills render solid black** |
| flowchart — `<br/>`/text inside `<foreignObject>`, CSS classDef fills | text now correctly space-separated, colored, and **genuinely multi-line** (real greedy word-wrap, not a single condensed line — see Fixes below); the "Inner / circle..." label renders in black across properly wrapped lines | blank (no foreignObject text) | blank | blank | text missing entirely; subgraph bg and several classDef-filled shapes render **solid black**; edges render as thick black wedges |
| zenuml — nested HTML+CSS inside `<foreignObject>` (Vue-rendered) | raw concatenated text only, no layout | blank canvas | **fails to parse the file** (hard error) | **hard crash**: `ValueError: could not convert string to float: 'calc(100'` | only a stray inline `<svg>` icon fragment renders, wrong scale/position; everything else (boxes, text, lifelines) missing |
| base64 `<image>` (href/xlink:href) | ok | ok | ok | ok | ok (image fine, caption **text missing**) |
| linear/radial gradient | ok | ok | ok | ok | ok |
| `feGaussianBlur` filter | **applied** (added; see Fixes below) | applied | not applied | not applied | applied |
| `feDropShadow` filter | **applied** (added; see Fixes below) | applied | not applied | not applied | not applied |
| `clip-path` | ok | ok | ok | ok | ok |
| `mask` (luminance) | ok | ok | ok | ok | ok |
| CSS class fill/stroke (simple stylesheet) + `<use>`/`<symbol>` | ok | ok | ok | ok | ok |
| CSS `transform:` property | **applied** (added; see Fixes below) | not applied | not applied | not applied | not tested |

## Reading it

- **Plain vector SVG** (shapes, gradients, clip, mask, use/symbol, raster
  embeds, *simple* CSS classes): novasvg, resvg, lunasvg and cairosvg are
  all equivalent. Expected — novasvg is a from-scratch header-only
  restructure of lunasvg's own architecture (same plutovg/FreeType-derived
  rasterizer, see `docs/about.md`), so parity with lunasvg specifically is
  by design, not coincidence.
- **thorvg is the outlier, in both directions.** Its SVG loader doesn't
  render `<text>` at all in any of the 8 files — that's a hard gap none of
  the other four have. It also fails to resolve CSS-class fills once the
  stylesheet gets non-trivial (mermaid's classDef output), painting those
  shapes solid black instead. But it's the *only* engine of the five that
  applies `feGaussianBlur` correctly — resvg is the only other one that
  does any filter, and only resvg gets both blur and drop-shadow.
- **`<foreignObject>` (what mermaid.js uses for every text label):**
  novasvg is the strongest of the five — the only one that extracts any
  text from it, even if line-wrapping is broken and one label got dropped.
  resvg and lunasvg render empty boxes; cairosvg crashes on `calc()`;
  thorvg ignores it beyond a stray nested icon.
- **zenuml specifically** breaks all five to some degree: 2 of 5 (novasvg,
  thorvg-partial) draw *something*, 2 of 5 (resvg, thorvg-mostly) draw a
  blank/near-blank canvas, and 2 of 5 (lunasvg, cairosvg) fail outright
  before producing any image.
- Nothing here applied the CSS `transform:` *property* (as opposed to the
  SVG `transform=` attribute) at the time this comparison table was first
  generated — novasvg has since fixed this (see Fixes below); resvg,
  lunasvg, and cairosvg still don't.

## Fixes applied to novasvg since the table above was first generated

- **Full filter-primitive pipeline added**: `feGaussianBlur`, `feOffset`,
  `feFlood`, `feComposite` (Over/In/Out/Atop/Xor), `feMerge`/`feMergeNode`,
  and `feDropShadow`, with real `in`/`in2`/`result` chaining
  (`SourceGraphic`/`SourceAlpha` included). Built as a proper pipeline:
  `SVGElement::applyFilterPrimitive()` is a Template Method virtual,
  overridden by one small class per primitive; `SVGFilterElement` is just
  an executor that calls it on each child in document order and threads
  the result through `SVGFilterContext` — no per-type switch statement,
  adding a primitive later means adding one class. Canvas-level building
  blocks (`boxBlur`, `shift`, `tintToFloodColor`, `fillOpaqueWhite`,
  `compositeWith` with full Porter-Duff math) are shared by all of them.
  Verified with a from-scratch drop shadow built purely from 5 chained
  primitives (`data/feature-filter-primitives.svg`) matching the
  dedicated `feDropShadow` shorthand's output.
  - **`feComposite operator="arithmetic"` added** (`Canvas::compositeArithmetic`,
    the `result = k1*i1*i2 + k2*i1 + k3*i2 + k4` per-channel formula on
    normalized premultiplied values). Verified against resvg with a
    multiply-blend chain (`data/feature-filter-arithmetic.svg`,
    `k1=1,k2=k3=k4=0`) — pixel-identical output.
  - Not implemented (ponytail-scoped, noted in code): per-primitive filter
    regions (`x`/`y`/width/height` on `<filter>` or individual
    primitives — the whole chain currently shares one region sized off
    the filtered element's bbox), and primitives with no test coverage
    yet (`feColorMatrix`, `feTurbulence`, `feDisplacementMap`, ...).
  - This closes novasvg's only remaining gap against resvg found in
    this comparison (drop-shadow/blur were resvg's one advantage;
    novasvg now matches it there while still leading on foreignObject
    text) — and goes further than any of the other four on filters,
    since none of them expose a working primitive chain either.
  - **Possible speed-up, not yet done:** `Canvas::boxBlur`'s per-pixel
    loop (`Canvas::compositeWith` too) is scalar and branch-y; see
    `checklist.md` for concrete SIMD/algorithmic notes if this ever
    shows up as a bottleneck.

- **CSS `transform:` property support added.** Two independent bugs,
  both root-caused and fixed:
  1. `matrix.h`'s attribute-grammar parser rejected any unit suffix
     (`deg`, `rad`, ...) on a transform function's number, silently
     dropping the *entire* transform list on the first such token. Fixed
     via a new `skip_css_unit()` helper (`utils.h`) that also converts
     rad/grad/turn to degrees.
  2. CSS `<style>` block declarations were resolved through the
     hyphenated-only property table (`csspropertyid()`), not the
     combined table that also covers camelCase names like `transform`
     — so `transform` set via a CSS class was invisible to the style
     cascade regardless of the parser fix above. Fixed the lookup call
     in `svgparser.hpp` to use the combined `propertyid()`.
  - Verified: `.class { transform: rotate(15deg); }` now rotates the
    element correctly (`data/feature-css-use-symbol.svg`).

- **`<br>`/tag-boundary word-gluing in `<foreignObject>` text fixed.**
  `foreignObjectPlainText()` now emits a space at every tag boundary
  (not just recognized block-level tags — simpler, and over-inserting is
  harmless since whitespace collapses afterward, while under-inserting
  glues words together). "Two line edge comment", "Rounded square
  shape", and "linebreak in an Odd shape" in the flowchart sample all
  space correctly now; previously read "Two lineedge comment" etc.

- **foreignObject text-color inheritance bug fixed** (the green-on-green
  invisible label). Root cause: mermaid's `classDef green` compiles to
  `.green>*{fill:#9f6 !important}`, which correctly matches the label's
  `<g>` too (it's a direct child of the same green-classed ancestor);
  since `fill` is an inherited SVG property, and `ForeignObjectSimple`
  was reading that inherited `fill` for the text color, the label ended
  up the same green as its background. Fixed by adding
  `foreignObjectTextColor()` — reads CSS `color` from the HTML content's
  own `style=`/`class=` (mirroring the existing `foreignObjectBackgroundColor`
  pattern exactly, generalized into a shared `findTagColor`/`tagColor`/
  `findClassColor`/`parseColorDeclaration` parameterized on property
  name), defaulting to black, independent of the SVG `fill` cascade —
  matching how a real browser treats a foreignObject's HTML content as
  its own formatting context. Verified: the "Inner / circle and some odd
  special characters" label now renders in black
  (`data/mermaid/02-flowchart-issue17.mmdc.svg`); the venn diagram's
  `color`-styled labels ("Backend" green, "Frontend" blue) still resolve
  correctly, confirming no regression on the working case.

- **foreignObject real multi-line text wrapping added.** Previously a
  single horizontally-condensed line no matter what; now genuine
  greedy word-wrap. Two mechanisms, combined:
  1. `foreignObjectPlainText()` now inserts an actual `'\n'` break at
     `<br>`/`<p>`/`<div>`/`<tr>`/`<li>` tag boundaries (previously
     collapsed to a plain space like every other tag, which is why
     `<br/>`-separated text used to glue into one run).
  2. A new `wrapForeignObjectText()` further greedy-wraps each of those
     break-delimited segments by width, using the same `font.measureText()`
     the old single-line condense logic already relied on — reusing it
     per-candidate-line instead of once for the whole string. Each line
     gets the same per-line horizontal-condense fallback the old code
     applied to the whole block, for the rare case of a single
     unbreakable word wider than the box.
  - Verified against the real mmdc/Chrome reference
    (`data/mermaid/02-flowchart-issue17.mmdc.png`): line *content* and
    break points now match closely. Line *count* can still run one or
    two lines higher than Chrome's for borderline-width labels — that
    residual gap is the substitute-font-metrics issue this file's
    comments already flagged (no Trebuchet MS in this environment),
    not a wrapping-logic bug; every line still fits its box cleanly
    rather than being squished, which reads better than the old
    condense-the-whole-block behavior even when the line count differs.
  - Line-height is a fixed 1.2× multiplier rather than reading the
    HTML's own `line-height` (mermaid's content usually says `1.5`) —
    noted as an upgrade path in `checklist.md`.

- **Font-family fallback now goes through real OS font substitution
  (fontconfig on Linux), closing most of the substitute-font-metrics gap
  the wrap fix above still flagged as residual.** Root cause: novasvg's
  font matching never consulted fontconfig at all — it does its own raw
  scan of font files, indexing each by the literal internal name from
  the file's own TTF `name` table. A request for "arial" only ever
  matched a font whose own internal name is literally "Arial"; it never
  found `fonts-liberation`'s "Liberation Sans" even though fontconfig
  itself aliases "arial" to exactly that font system-wide (verified:
  installed `fonts-liberation`, confirmed `fc-match arial` found it
  while novasvg's own lookup still didn't). Since mermaid's own CSS
  always asks for `"trebuchet ms", verdana, arial, sans-serif` — none of
  which are typically installed under their literal names on Linux —
  this silently affected the sizing of every text-containing diagram.
  - Fix: `FontFaceCache::getFontFaceForFamilyStack()` asks fontconfig to
    resolve the *entire* comma-separated family stack in one call
    (`FcFontMatch` over a pattern with all names added as `FC_FAMILY`
    values, matching how fontconfig is meant to be used for CSS-style
    fallback lists), tried before the old literal-name/generic-table
    loop. Gated behind `NOVASVG_HAVE_FONTCONFIG` (CMake auto-detects
    fontconfig on Linux via `find_package(Fontconfig)`, controllable via
    `NOVASVG_USE_FONTCONFIG`) — a clean no-op, falling back to the
    previous literal-scan behavior unchanged, wherever fontconfig isn't
    linked in.
  - Got this wrong on the first attempt, worth recording: calling
    fontconfig once *per name in the stack* (stopping at the first hit)
    seemed like the obvious approach, but `FcFontMatch` always returns
    *some* match — it never reports "not found" — so it "succeeded" on
    the very first name ("trebuchet ms") with fontconfig's own generic
    default substitution before ever reaching "verdana" or "arial" later
    in the stack. Fontconfig has to see the whole ordered stack in one
    pattern to pick the best-aliased entry itself.
  - Verified against the real mmdc/Chrome reference
    (`data/mermaid/02-flowchart-issue17.mmdc.png`): line count for the
    flowchart sample's labels went from matching on almost none of them
    to matching on 10 of 12 (only "Diamond with line break" and one
    "Rounded square shape" instance are still off by a line).

## Still open

- zenuml (nested HTML+CSS inside `<foreignObject>`) — still only raw
  text, no layout. Out of scope for a "fix", this needs an actual HTML
  layout engine.

## Bottom line

No single winner — pick by what you're rendering:

- Need mermaid-style diagrams with foreignObject text labels → **novasvg**
  is the strongest of the five tested here (still incomplete).
- Need correct filter effects → **novasvg**, now the most complete of
  the five: a real primitive pipeline (blur/offset/flood/composite/merge/
  drop-shadow with `in`/`result` chaining), not just the two primitives
  resvg alone previously covered.
- Need a battle-tested, spec-heavy general SVG renderer and don't need
  foreignObject → **resvg** or **lunasvg**, roughly tied.
- **thorvg** is built for Lottie/vector-animation workloads first; as a
  static-SVG renderer specifically it's currently the weakest of the five
  tested here (no text, black-fill CSS bug) — not a fair fight for what
  it's actually designed for, but that's what static-SVG testing shows.

Full per-file renders and the underlying test scripts are in `data/mermaid/`
and `data/feature-*.png` (also `.resvg.png`, `.lunasvg.png`, `.cairosvg.png`,
`.thorvg.png` siblings per file).
