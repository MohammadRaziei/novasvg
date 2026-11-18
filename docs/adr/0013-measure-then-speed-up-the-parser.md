# ADR-0013: measure the parser first, then borrow pugixml's techniques instead of its DOM

**Status:** Proposed, not implemented. Nothing in the code has changed for this record.
**Date:** 2026-11-18
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
- [ ] Measure (step 1) and write the numbers into this record.
- [ ] Decide the attribute ownership model (view or owned) if step 2 goes ahead.
- [ ] Add the DTD `<!ENTITY>` test.
