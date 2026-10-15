<div align="center">

<img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/refs/heads/master/data/nova.svg" width="160"
     onerror="this.onerror=null; this.src='data/nova.svg';" alt="NovaSVG logo">

# NovaSVG

**A header-only C++17 SVG renderer: parse, query, restyle and rasterize SVG with one `#include`.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.txt)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.19+-blue.svg)](https://cmake.org/)
[![PyPI](https://img.shields.io/pypi/v/novasvg.svg)](https://pypi.org/project/novasvg/)
[![codecov](https://codecov.io/gh/MohammadRaziei/novasvg/branch/master/graph/badge.svg)](https://codecov.io/gh/MohammadRaziei/novasvg)

[Documentation](https://mohammadraziei.github.io/novasvg) ·
[Benchmarks](https://mohammadraziei.github.io/novasvg/benchmarks) ·
[Single header](https://github.com/MohammadRaziei/novasvg/releases/download/__beta__/novasvg.h) ·
[Python](https://pypi.org/project/novasvg/) ·
[Issues](https://github.com/MohammadRaziei/novasvg/issues)

</div>

---

## Why NovaSVG

- **Nothing to link.** Drop in `novasvg.h` (or the `include/` folder) and you are done. No submodules, no system libraries, no fontconfig.
- **Renders what real diagrams contain.** Gradients, patterns, clip paths, masks, markers, a full SVG filter pipeline, CSS styling, embedded fonts, and `<foreignObject>` text, which is what Mermaid uses for every label.
- **Close to a browser.** Compared against Chromium on a 60-sample corpus, including 49 Mermaid diagrams. See [how it stacks up](#how-it-compares).
- **More than a renderer.** Query elements with CSS selectors, rewrite styles, read bounding boxes and matrices, and render a single element on its own.
- **Three ways in.** C++ library, a `novasvg` command-line tool, and Python bindings with no dependencies.
- **Portable.** Windows, Linux and macOS, with the same text layout on every platform.

## Quick start

### C++

```cpp
#include <novasvg/novasvg.h>
#include <iostream>

int main() {
    auto doc = novasvg::Document::loadFromFile("artwork.svg");
    if (!doc) return 1;

    // 640 px wide, height follows the aspect ratio, white background
    auto bitmap = doc->renderToBitmap(640, -1, novasvg::Color::white());
    bitmap.writeToPng("artwork.png");        // also writeToJpg / writeToBmp / writeToTga

    // Query it like a DOM
    for (auto& rect : doc->querySelectorAll("rect"))
        std::cout << rect.getAttribute("id") << '\n';

    std::cout << doc->width() << " x " << doc->height() << '\n';
}
```

```bash
g++ -std=c++17 -O2 -Iinclude main.cpp -o main
```

### Command line

```bash
novasvg input.svg                                  # -> input.png
novasvg input.svg -o out.png -w 800 -b FFFFFF      # width 800, white background
novasvg input.svg --style "rect { fill: tomato; }" # restyle before rendering
novasvg info input.svg                             # size, bounding box, element count
novasvg query "circle" input.svg --json            # CSS-selector search
novasvg batch svg_dir/ png_dir/                    # convert a whole folder
```

### Python

```bash
pip install novasvg        # no dependencies; NumPy is only needed for Bitmap.numpy()
```

```python
import novasvg

novasvg.svg2png("artwork.svg", "artwork.png", width=800)     # one call

doc = novasvg.Document.load_from_file("artwork.svg")         # or keep the document
bitmap = doc.render_to_bitmap(width=800)
bitmap.write_to_png("artwork.png")
```

More in [README_PYTHON.md](README_PYTHON.md).

## Installation

**Single header.** Download [`novasvg.h`](https://github.com/MohammadRaziei/novasvg/releases/download/__beta__/novasvg.h) and include it. Every function is `inline`, so it is safe to include from any number of translation units.

**Copy the headers.** Put `include/novasvg/` in your project and `#include <novasvg/novasvg.h>`.

**CMake `FetchContent`.**

```cmake
include(FetchContent)
FetchContent_Declare(novasvg
    GIT_REPOSITORY https://github.com/MohammadRaziei/novasvg.git
    GIT_TAG        master)
FetchContent_MakeAvailable(novasvg)

target_link_libraries(my_app PRIVATE novasvg::novasvg)
```

**Installed package.**

```bash
cmake -B build && cmake --build build -j
sudo cmake --install build
```

```cmake
find_package(novasvg REQUIRED)
target_link_libraries(my_app PRIVATE novasvg::novasvg)   # or novasvg::noflag for no extra compile flags
```

Building from source also produces the `novasvg` command-line tool at `build/novasvg`.

## How it compares

The same SVGs rendered by each engine, with Chromium as the reference. These samples were picked because the engines disagree on them.

| Sample | Chromium | novasvg | resvg | lunasvg | cairosvg | thorvg | nanosvg |
|---|---|---|---|---|---|---|---|
| `clip-mask` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/clip-mask.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/clip-mask.png" width="100"> |
| `mermaid-02-flowchart` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-02-flowchart.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-02-flowchart.png" width="100"> |
| `css-use-symbol` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/css-use-symbol.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/css-use-symbol.png" width="100"> |
| `mermaid-47-flowchart-br-bold-label` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-47-flowchart-br-bold-label.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-47-flowchart-br-bold-label.png" width="100"> |
| `mermaid-33-treemap` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-33-treemap.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-33-treemap.png" width="100"> |
| `gradient-filter` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/gradient-filter.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/gradient-filter.png" width="100"> |
| `mermaid-40-erd-relations` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-40-erd-relations.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-40-erd-relations.png" width="100"> |
| `mermaid-01-venn` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-01-venn.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-01-venn.png" width="100"> |
| `filter-primitives` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/filter-primitives.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/filter-primitives.png" width="100"> |
| `mermaid-32-radar` | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/ground_truth/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/novasvg/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/resvg/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/lunasvg/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/cairosvg/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/thorvg/mermaid-32-radar.png" width="100"> | <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/master/benchmarks/outputs/renders/nanosvg/mermaid-32-radar.png" width="100"> |

**Want the numbers behind the pictures?** The full benchmark page lists every one of the 60 samples with its **RMSE against Chromium** and its **render time in ms** for every engine. You can click any render to zoom in.

### 👉 [mohammadraziei.github.io/novasvg/benchmarks](https://mohammadraziei.github.io/novasvg/benchmarks)

You can also rebuild all of it yourself; see [`benchmarks/README.md`](benchmarks/README.md).

## What it renders

| Area | Supported |
|---|---|
| Shapes and paths | `rect`, `circle`, `ellipse`, `line`, `polyline`, `polygon`, `path`, markers |
| Painting | solid colors, linear and radial gradients, patterns, opacity, `mix-blend-mode` |
| Compositing | `clip-path`, `mask` (luminance), nested groups, `<use>` / `<symbol>`, `<switch>` |
| Filters | `feGaussianBlur`, `feOffset`, `feFlood`, `feComposite` (Over, In, Out, Atop, Xor, arithmetic), `feMerge`, `feDropShadow`, with real `in` / `in2` / `result` chaining |
| CSS | `<style>` sheets, class and attribute selectors, `style=""`, `!important`, the CSS `transform:` property |
| Images | `<image>` with base64 data URIs |
| Text | system fonts, built-in fallback font, `@font-face` with embedded TTF/OTF, `<foreignObject>` text with word-wrap |

Not implemented yet: per-primitive filter regions, `feColorMatrix` / `feTurbulence` / `feDisplacementMap`, WOFF/WOFF2 fonts, and full HTML/CSS layout inside `<foreignObject>`. See [`checklist.md`](checklist.md) and [`COMPARISON.md`](COMPARISON.md) for details.

### Fonts

Font lookup works like a browser, with no OS font library involved. A `font-family` stack is walked name by name: an installed or registered font first, then a metric-compatible stand-in (`arial` → Liberation Sans / Arimo, `times new roman` → Liberation Serif / Tinos, `calibri` → Carlito, ...), then the generic keywords. A ~40 KB Latin subset of DejaVu Sans is compiled in as the last resort, so text renders even on a machine with no fonts installed. For the best fidelity, install a metric-compatible family such as `fonts-liberation`.

```cpp
novasvg::addFontFaceFromFile("Inter", /*bold=*/false, /*italic=*/false, "Inter-Regular.ttf");
```

Disable the embedded font with `-DNOVASVG_EMBED_DEFAULT_FONT=OFF` or by defining `NOVASVG_DISABLE_EMBEDDED_FONT`.

## API overview

| Class | Main members |
|---|---|
| `Document` | `loadFromFile()` · `loadFromData()` · `renderToBitmap()` · `querySelectorAll()` · `getElementById()` · `applyStyleSheet()` · `width()` / `height()` · `boundingBox()` |
| `Element` | `getAttribute()` / `setAttribute()` / `hasAttribute()` · `render()` · `getLocalBoundingBox()` / `getGlobalBoundingBox()` · `getLocalMatrix()` / `getGlobalMatrix()` |
| `Bitmap` | `writeToPng()` / `writeToJpg()` / `writeToBmp()` / `writeToTga()` · `data()` (ARGB32 premultiplied) · `width()` / `height()` / `stride()` · `convertToRGBA()` |
| `Matrix` | `translate()` · `scale()` · `rotate()` · `shear()` · `inverse()` · `operator*` |

Full reference: [mohammadraziei.github.io/novasvg](https://mohammadraziei.github.io/novasvg), or build it locally with `-DNOVASVG_BUILD_DOCS=ON`.

## Command-line reference

| Option | Description |
|---|---|
| `-o, --output <file>` | Output PNG (default: input name with `.png`) |
| `-w, --width <px>` / `-H, --height <px>` | Output size |
| `-s, --scale <factor>` | Scale factor, e.g. `2.0` |
| `-b, --background-color <hex>` | `RRGGBB` or `RRGGBBAA` (default: transparent) |
| `--style <css>` / `--css-file <file>` | Apply CSS before rendering |

| Subcommand | Description |
|---|---|
| `convert` | Convert to PNG. This is the default, so `novasvg in.svg` works on its own |
| `info` | Size, bounding box and element count; add `--json` for machine-readable output |
| `query` | Find elements by CSS selector, e.g. `novasvg query "rect[fill='red']" in.svg` |
| `batch` | Convert every SVG in a directory |

`novasvg --help` and `novasvg <subcommand> --help` always list the current options.

## Build options

| Option | Description | Default |
|---|---|---|
| `NOVASVG_BUILD_EXAMPLES` | Build the C++ examples | `ON` when top-level |
| `NOVASVG_BUILD_TESTS` | Build the unit tests | `ON` when top-level |
| `NOVASVG_BUILD_PYTHON` | Build the Python bindings | `ON` when top-level |
| `NOVASVG_BUILD_DOCS` | Build the docs with Doxygen | `OFF` |
| `NOVASVG_INSTALL` | Install the CMake package (`novasvg::novasvg`, `novasvg::noflag`) | `ON` when top-level |
| `NOVASVG_DIST_DIR` | Generate the single-header distribution into this directory | unset |
| `NOVASVG_EMBED_DEFAULT_FONT` | Compile in the fallback font | `ON` |

## Repository layout

```
include/novasvg/    the library (novasvg.h + detail/)
src/                command-line tool and Python bindings
examples/cpp/       small runnable samples
tests/              C++ (doctest) and Python (pytest) tests
benchmarks/         comparison against resvg, lunasvg, thorvg, nanosvg, cairosvg and Chromium
data/               sample SVGs, Mermaid corpus, bundled fonts
docs/               documentation site sources
```

## Contributing

Issues and pull requests are welcome. Please fork, work on a feature branch, add a test when you change behaviour, and open a PR.

```bash
cmake -B build -DNOVASVG_BUILD_TESTS=ON && cmake --build build -j
ctest --test-dir build
```

## License

[MIT](LICENSE.txt)

## Acknowledgments

NovaSVG grew out of the architecture of [lunasvg](https://github.com/sammycage/lunasvg), restructured as a flat, dependency-free, single-header library. The story is in [`docs/about.md`](docs/about.md).

---

<div align="center">
<em>NovaSVG: a new star in lightweight SVG processing</em>
</div>
