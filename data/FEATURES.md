# novasvg misc SVG feature probe

| File | Feature | Result |
|---|---|---|
| feature-base64-image.svg | `<image>` with base64 data URI, both `href` and `xlink:href` | OK, both render |
| feature-gradient-filter.svg | linearGradient / radialGradient | OK | 
| feature-gradient-filter.svg | `feDropShadow`, `feGaussianBlur` filters | OK — now applied (added `Canvas::boxBlur`/`SVGFilterElement` support) |
| feature-clip-mask.svg | `clip-path` | OK |
| feature-clip-mask.svg | `mask` | OK (initial "not rendered" result was a bug in this test file's coordinates, not novasvg — fixed and re-verified) |
| feature-css-use-symbol.svg | internal `<style>` class (fill/stroke) | OK |
| feature-css-use-symbol.svg | `<symbol>`/`<use>` | OK |
| feature-css-use-symbol.svg | CSS `transform:` property (vs SVG `transform` attribute) | NOT applied (box stays unrotated) |
