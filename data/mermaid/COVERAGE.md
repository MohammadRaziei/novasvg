# novasvg coverage vs mmdc (mermaid-cli) reference

Pipeline: mermaidx issue source -> .mmd -> mmdc renders ground-truth .svg/.png -> novasvg-cli renders that same .svg -> .png -> visual diff.

| # | Sample (mermaidx issue) | novasvg vs mmdc |
|---|---|---|
| 01 | venn (#35) — plain `<text>`/shapes | Pixel-equivalent match |
| 02 | flowchart (#17) — subgraph, classDef, `<br/>` inside foreignObject/div labels | Shapes/edges correct, but: line breaks inside foreignObject text collapse ("Two line<br/>edge comment" -> one squished line), long labels aren't wrapped/centered, and one label ("Inner / circle and some odd special characters") is dropped entirely (empty circle) |
| 03 | block (#23) — native SVG shapes | Pixel-equivalent match |
| 04 | zenuml (#20) — heavy nested HTML/CSS inside foreignObject (Vue-rendered) | Fails: only raw concatenated text is drawn, all layout/boxes/icons/lifelines lost |

Verdict: novasvg's native SVG path (shapes, gradients handled elsewhere, plain text) is solid.
The gap is HTML-in-SVG (`<foreignObject>`): partial support for simple divs with `<br>`,
and no support once nesting/CSS layout gets non-trivial (zenuml).
