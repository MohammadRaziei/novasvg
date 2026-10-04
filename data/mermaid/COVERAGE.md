# novasvg coverage vs mmdc (mermaid-cli) reference

Pipeline: .mmd source -> mmdc renders the ground-truth .svg -> novasvg-cli renders that same .svg -> .png -> visual diff.

| # | Sample | novasvg vs mmdc |
|---|---|---|
| 01 | venn — plain `<text>`/shapes | Pixel-equivalent match |
| 02 | flowchart — subgraph, classDef, `<br/>` inside foreignObject/div labels | Shapes/edges correct, but: line breaks inside foreignObject text collapse ("Two line<br/>edge comment" -> one squished line), long labels aren't wrapped/centered, and one label ("Inner / circle and some odd special characters") is dropped entirely (empty circle) |
| 03 | block — native SVG shapes | Pixel-equivalent match |

Verdict: novasvg's native SVG path (shapes, gradients handled elsewhere, plain text) is solid.
The gap is HTML-in-SVG (`<foreignObject>`): partial support for simple divs with `<br>`,
and no support once nesting/CSS layout gets non-trivial.
