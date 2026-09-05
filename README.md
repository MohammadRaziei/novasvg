# NovaSVG

<div align="center">
<img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/refs/heads/master/data/nova.svg" width="160"
     onerror="this.onerror=null; this.src='data/nova.svg';" alt="NovaSVG Logo">
</div>

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.txt)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)
[![codecov](https://codecov.io/gh/MohammadRaziei/novasvg/branch/master/graph/badge.svg)](https://codecov.io/gh/MohammadRaziei/novasvg)
[![single header file-novasvg.h](https://img.shields.io/badge/single_header_file-novasvg.h-blue.svg)](https://github.com/MohammadRaziei/novasvg/releases/download/__beta__/novasvg.h)

</div>

<div align="center">
    <a href="https://github.com/MohammadRaziei/novasvg/releases/download/__beta__/novasvg.h">
        <img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/refs/heads/master/data/download-novasvg.svg" width="120"
     onerror="this.onerror=null; this.src='data/download-novasvg.svg';" alt="Download NovaSVG">
     </a>
</div>

NovaSVG is a lightweight, **header-only C++17 library** for parsing, manipulating, and rasterizing SVG files. It provides a clean, modern API for loading SVG documents, querying elements, applying CSS styles, and rendering to bitmaps or PNG files—all from a single include.

The library also offers a **command-line interface** for batch processing and automation.

## ✨ Features

### Core Library
- **Header-only design** – Single include file for easy integration
- **Modern C++17 API** – Clean, type-safe interface with RAII semantics
- **SVG parsing** – Load from files, strings, or memory buffers
- **Rasterization** – Render to bitmaps with customizable dimensions and backgrounds
- **Element querying** – CSS selector support for finding elements
- **CSS application** – Apply stylesheets to modify SVG appearance
- **Transformation support** – Matrix operations for scaling, rotation, translation
- **Font management** – Add custom fonts for text rendering
- **Cross-platform** – Works on Windows, Linux, and macOS

### Command-Line Interface
- **SVG to PNG conversion** – Batch convert with customizable dimensions
- **SVG information** – Extract metadata, bounding boxes, and element counts
- **CSS querying** – Find elements using CSS selectors
- **Style application** – Apply CSS stylesheets to SVG documents
- **Font management** – Add and manage fonts for rendering

### SVG Filters
- **Full filter-primitive pipeline** – `feGaussianBlur`, `feOffset`, `feFlood`,
  `feComposite` (Over/In/Out/Atop/Xor), `feMerge`/`feMergeNode`, and the
  `feDropShadow` shorthand, with real `in`/`in2`/`result` chaining
  (`SourceGraphic`/`SourceAlpha` included) — chain primitives together to
  build custom effects, not just apply one at a time
- **CSS `transform:` property** – both the SVG `transform=` attribute and
  the CSS `transform:` property (set via `style=`/a stylesheet class,
  including `deg`/`rad`/`grad`/`turn` units) are supported

### `<foreignObject>` / HTML-in-SVG text
- **Plain-text extraction** – renders the text content of HTML embedded in
  `<foreignObject>` (tags stripped, CSS `color`/`background-color` from
  the HTML's own `style=`/`class=` respected) rather than skipping it
  entirely. This is what diagram tools like Mermaid.js rely on for every
  node/edge label; most other lightweight SVG renderers (resvg, lunasvg,
  cairosvg) currently render these as blank boxes — see `COMPARISON.md`
  for a from-scratch, empirical comparison. Full HTML/CSS layout (line
  wrapping, nested block layout) is out of scope; see `checklist.md`.

## 🚀 Quick Start

### C++ Usage

```cpp
#define NOVASVG_IMPLEMENTATION
#include <novasvg/novasvg.h>

int main() {
    // Load SVG from file
    auto doc = novasvg::Document::loadFromFile("artwork.svg");
    if (doc) {
        // Render to bitmap (auto-sized)
        auto bitmap = doc->renderToBitmap();
        
        // Save as PNG
        bitmap.writeToPng("artwork.png");
        
        // Query elements
        auto rectangles = doc->querySelectorAll("rect");
        std::cout << "Found " << rectangles.size() << " rectangles\n";
        
        // Get document dimensions
        std::cout << "Size: " << doc->width() << "x" << doc->height() << "\n";
    }
    return 0;
}
```

### Command-Line Usage

```bash
cmake -B build && cmake --build build
./build/novasvg input.svg              # convert is implied, no need to type it
./build/novasvg input.svg -o output.png -w 800 -H 600
```

See the "CLI Reference" section below for the full option list.

## 📦 Installation

### C++ Library

#### As Header-Only Library
Simply copy `include/novasvg/novasvg.h` and `include/novasvg/detail/novasvg_impl.h` to your project.

#### Building from Source
```bash
git clone https://github.com/MohammadRaziei/novasvg.git
cd novasvg
mkdir build && cd build
cmake .. -DNOVASVG_BUILD_EXAMPLES=ON
make -j4
```

### Command-Line Tool
The `novasvg` target builds automatically as part of the project (no
separate flag needed) — after the build above, the binary is at
`build/novasvg`:
```bash
sudo make install  # Optional: install system-wide
```

## 📚 API Overview

### Core Classes

#### `Document`
- `loadFromFile()` / `loadFromData()` – Load SVG documents
- `renderToBitmap()` – Render to bitmap with optional dimensions
- `querySelectorAll()` – Find elements using CSS selectors
- `applyStyleSheet()` – Apply CSS styles
- `width()` / `height()` – Get document dimensions
- `boundingBox()` – Get document bounding box

#### `Bitmap`
- `writeToPng()` – Save as PNG file or stream
- `data()` – Access raw pixel data (ARGB32 Premultiplied)
- `width()` / `height()` / `stride()` – Get bitmap properties
- `convertToRGBA()` – Convert to RGBA format

#### `Element`
- `hasAttribute()` / `getAttribute()` / `setAttribute()` – Manage element attributes
- `render()` – Render element to bitmap
- `getLocalBoundingBox()` / `getGlobalBoundingBox()` – Get element bounds
- `getLocalMatrix()` / `getGlobalMatrix()` – Get transformation matrices

#### `Matrix`
- `translate()` / `scale()` / `rotate()` / `shear()` – Transformation operations
- `inverse()` – Matrix inversion
- Operator `*` – Matrix multiplication

## 🖥️ CLI Reference

```bash
# Convert -- the default action, no subcommand keyword needed
novasvg input.svg
novasvg input.svg -o output.png
novasvg input.svg -w 800 -H 600         # resize
novasvg input.svg -s 2.0                # scale factor
novasvg input.svg -b FFFFFF             # background color (hex)
novasvg input.svg --style "rect { fill: red; }"
novasvg input.svg --css-file custom.css

# Info / query / batch -- real subcommands for the other actions
novasvg info document.svg
novasvg info document.svg --json
novasvg query "circle" shapes.svg
novasvg query "rect[fill='red']" shapes.svg --json
novasvg batch svg_dir/ png_dir/
```

| Option | Description |
|---|---|
| `-o, --output <file>` | Output PNG file (default: input name with `.png`) |
| `-w, --width <px>` / `-H, --height <px>` | Output dimensions |
| `-s, --scale <factor>` | Scale factor (e.g. `2.0`) |
| `-b, --background-color <hex>` | `RRGGBB` or `RRGGBBAA` (default: transparent) |
| `--style <css>` / `--css-file <file>` | Apply CSS before rendering |

Run `novasvg --help` or `novasvg <subcommand> --help` for the complete,
always-up-to-date option list.

## 🔧 Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `NOVASVG_BUILD_EXAMPLES` | Build C++ examples | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_TESTS` | Build unit tests | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_PYTHON` | Build Python bindings | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_DOCS` | Build documentation with Doxygen | `OFF` |
| `NOVASVG_DIST_DIR` | Generate single-header distribution | Not defined |

The `novasvg` command-line tool itself has no build flag — it's built
unconditionally alongside the library. Converting is the implied default
action (`novasvg input.svg` works with no subcommand needed).

## 🧪 Examples

### C++ Examples
- `examples/cpp/sample01_convert_svg_to_png.cpp` – Batch convert SVGs to PNGs
- `examples/cpp/sample02_query_svg_size.cpp` – Query document properties


## 🎯 Use Cases

1. **Graphics Applications** – Embed SVG rendering in C++ applications
2. **Web Development** – Server-side SVG processing and optimization
3. **Game Development** – Load vector graphics for UI elements
4. **Desktop Applications** – Display vector graphics in GUI applications
5. **Automation** – Batch convert SVG assets with CLI tool

## 🔍 Performance Considerations

- **Memory efficient** – Minimal overhead for header-only design
- **Fast rasterization** – Optimized rendering pipeline
- **Reusable bitmaps** – Create once, render multiple times
- **Appropriate sizing** – Render at required resolution, not larger
- **Batch operations** – Process multiple SVGs efficiently

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

NovaSVG is distributed under the **MIT License**. See [LICENSE.txt](LICENSE.txt) for details.

## 📖 Documentation

- **API Reference**: Built with Doxygen (enable with `-DNOVASVG_BUILD_DOCS=ON`)
- **Online Documentation**: https://mohammadraziei.github.io/novasvg
- **Examples**: See `examples/` directory
- **CLI Documentation**: See the "CLI Reference" section above
- **Renderer comparison**: `COMPARISON.md` — an empirical, same-input
  comparison against resvg/lunasvg/cairosvg/thorvg (filters, CSS
  transforms, foreignObject text handling)
- **Known gaps / roadmap**: `checklist.md` — what's fixed, what's
  intentionally out of scope and why, and optimization notes for
  anything that's currently correct-but-not-fast

## 🙏 Acknowledgments

NovaSVG builds upon ideas from existing SVG libraries while maintaining a distinct architectural philosophy focused on minimalism and header-only design.

## 📞 Support

- **GitHub Issues**: https://github.com/MohammadRaziei/novasvg/issues
- **Documentation**: https://mohammadraziei.github.io/novasvg

---

<div align="center">
<em>NovaSVG – A new star in lightweight SVG processing</em>
</div>
