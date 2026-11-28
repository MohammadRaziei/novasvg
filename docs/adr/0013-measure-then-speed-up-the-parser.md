# ADR-0013: measure the parser first, then borrow pugixml's techniques instead of its DOM

**Status:** Measured (2026-11-22). Step 2 is not done: for ordinary SVGs parsing is a small share of a render. Nothing in the parser has changed.
**Date:** 2026-11-18, measured 2026-11-22
**Deciders:** Mohammad Raziei (maintainer)

## Context
novasvg reads an SVG in one pass: `document::parse` (novasvg.cpp, about 230 lines) tokenizes the XML by hand and
builds the typed `svg_element` tree directly, and `svg_style.cpp` is a hand-written CSS reader. The question was whether
to replace the tokenizer with pugixml, or build on its DOM, to gain speed and use less memory. novasvg only reads
documents; it does not write them.

What the code shows (read, not measured): attributes are stored per element in a `std::forward_list<attribute_entry>`
with a `std::string` for each value; every `svg_element` is its own heap allocation behind a `unique_ptr`; values are
copied out of the input; `decode_text` and `read_identifier` work on `std::string`.

What is **not** known: how much of the total time and memory is parsing at all. Rasterizing and text shaping are
usually the larger part of a render.

## Options Considered
1. **Build on pugixml's DOM.** The typed elements (virtual layout and render) and the cascade (specificity, `var()`,
   inline `style`) cannot live inside `xml_node`, which holds strings only. So the real shape is two trees: pugixml's
   as the document, ours derived from it. Every attribute would be held twice unless the typed layer reads straight
   from the XML nodes, which is a large redesign. Cost: more memory and a second pass, for a library that does not
   edit documents.
2. **Replace only the tokenizer with pugixml.** About 150-200 lines saved, a new dependency, a stricter parser, no
   DTD `<!ENTITY>` expansion, and the same double storage as option 1.
3. **myhtml / lexbor** for the foreignObject HTML and CSS. myhtml is superseded by lexbor (from memory, not
   re-checked). Its selector engine runs on its own DOM, while ours match `svg_element` and `property_id`, so we would
   mirror the SVG tree into it. Heavy C dependency. Rejected.
4. **Keep our parser; borrow the techniques that make pugixml fast.** Chosen, once the numbers say it is worth it.

## Decision
1. **Measure first**, with no code change: `perf` for time and `valgrind --tool=massif` (or heaptrack) for memory, on a
   few large inputs (a big mermaid diagram, a map, an icon sheet), through the existing `benchmarks/` harness. Record
   parse time and parse allocations as a share of the whole render.
2. **Only if parsing and allocation are a significant share**, apply these in `document::parse`, one at a time, each
   behind the golden gate:
   - values as `string_view` into the input, which the document copies once and owns; entities are decoded in place,
     since the decoded text is never longer than the source;
   - an arena for elements and attributes instead of one allocation each;
   - a 256-entry character-class table (space, name start, name char) instead of chains of comparisons.
3. **Do not** vendor or copy pugixml code, and do not adopt its DOM. The techniques are ideas; the code is
   template- and macro-heavy and does not come apart cleanly.

## Measurements (step 1, 2026-11-22)
`perf` and `valgrind` are not installed on the machine used, so the numbers come from a small harness (not kept in
the repository) linked against the Release build: `document::load_data` and `render_to_image()` timed with
`steady_clock`, the median of 14 runs after a warm-up (4 for the two large files), and `operator new` replaced by a
counter for the allocation count and bytes. One core, so timings are noisy to a few percent; allocation counts are exact.

| file | bytes | parse | render | parse share | allocs parse / render | bytes allocated parse / render |
|---|---|---|---|---|---|---|
| tiger.svg | 68,630 | 0.70 ms | 14.25 ms | 4.7% | 5,545 / 5,747 | 593 KB / 12,751 KB |
| mermaid 21-flowchart-shapes-styles | 77,144 | 0.75 ms | 4.33 ms | 14.7% | 4,492 / 4,196 | 577 KB / 4,863 KB |
| mermaid 11-flowchart-multiline-entities | 63,844 | 0.53 ms | 2.89 ms | 15.4% | 2,975 / 2,424 | 426 KB / 3,315 KB |
| mermaid 40-erd-relations | 62,254 | 0.73 ms | 4.44 ms | 14.1% | 5,404 / 5,750 | 595 KB / 6,289 KB |
| mermaid 05-data-flow | 55,796 | 0.37 ms | 1.46 ms | 20.2% | 2,116 / 907 | 336 KB / 1,016 KB |
| feature-embedded-font.svg | 548,137 | 2.02 ms | 0.44 ms | 82% | 75 / 532 | 2,018 KB / 439 KB |
| download-novasvg.svg | 1,619,284 | 21.20 ms | 0.39 ms | 98% | 355 / 236 | 7,284 KB / 461 KB |

Reading it:
- For a drawing (tiger, the mermaid diagrams) parsing is 5% to 20% of the render and under 1 ms. Even doubling the
  tokenizer's speed would shorten the whole render by 2% to 10%, on files that take a few milliseconds. The allocation
  counts of parse and render are about equal, but the bytes are an order of magnitude smaller in parse.
- The two large files are not a measure of the tokenizer. `download-novasvg.svg` is 23 elements and one 1.6 MB
  `data:image/png;base64` href; `feature-embedded-font.svg` is one big embedded font. Their "parse" time includes the
  base64 and image or font decode that happens while the document is built, and their render is near zero because the
  work is already done. What they do show is that an embedded blob is copied several times (7.3 MB allocated for a
  1.6 MB input).
- Not measured: peak resident memory under `massif`, a map or an icon sheet with tens of thousands of elements, and
  the split inside parse between tokenizing, the CSS cascade and decoding.

## Outcome
Step 2 (string views, an arena, a character-class table) is **not** worth doing for drawings: closed as "measured, not
worth it". The one thing the numbers point at is the copying of large embedded attribute values (base64 data URIs); if
that matters in practice, the fix is narrow (read the value as a view, decode it once) and does not need the
attribute-ownership redesign this record worried about. No new dependency, as before.

## Trade-off Analysis
Ownership gets more subtle: the public `set_attribute(name, std::string)` and `set_data` need an owned `std::string`,
while parsed values would be views into the document's buffer, so an attribute needs to hold either. This is the main
design cost of step 2 and has to be settled before the code is written.

## Consequences
- If parsing is a small share of a render, step 2 is not done and this record is closed as "measured, not worth it".
- A new case to add before any parser change: an SVG with an internal DTD `<!ENTITY>`, to see what the current
  tokenizer does with it.
- No new dependency in any outcome.

## Action Items
- [x] Measure (step 1) and write the numbers into this record.
- [x] Attribute ownership model: not needed, step 2 is not going ahead.
- [x] Add the DTD `<!ENTITY>` test (`tests/cpp/test_xml_prolog.cpp`): the prolog, DOCTYPE, comment, predefined entities, character references and CDATA all load and decode; an entity declared in the DTD is not expanded (`fill="&c;"` is not `red`).
