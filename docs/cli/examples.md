# Examples

## Basic conversion

```bash
# SVG -> PNG next to the input
novasvg input.svg

# Explicit output file
novasvg input.svg -o output.png
```

## Size, scale and background

```bash
novasvg input.svg -o out.png -w 800 -H 600
novasvg input.svg -o out@2x.png --scale 2.0
novasvg input.svg -o out.png -b FFFFFF
```

## Apply CSS while rendering

```bash
novasvg input.svg --css-file style.css
novasvg input.svg --style "rect { fill: red; }"
```

## Inspect a document

```bash
novasvg info image.svg
novasvg info image.svg --json
```

## Query elements with CSS selectors

```bash
novasvg query "circle" input.svg
novasvg query "rect[fill='red']" input.svg
```

## Convert a whole directory

```bash
novasvg batch ./svgs ./images
```
