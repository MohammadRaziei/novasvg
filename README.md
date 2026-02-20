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
# Convert SVG to PNG
novasvg convert input.svg output.png

# Convert with specific dimensions
novasvg convert -w 800 -H 600 input.svg output.png

# Get SVG information
novasvg info document.svg

# Find all circles
novasvg query "circle" shapes.svg

# Apply CSS styles
novasvg apply-css styles.css input.svg styled.png
```

## 📦 Installation

### C++ Library

#### As Header-Only Library
Simply copy `include/novasvg/novasvg.h` and `include/novasvg/detail/novasvg_impl.h` to your project.

#### Building from Source
```bash
git clone https://github.com/MohammadRaziei/novasvg.git
cd novasvg
mkdir build && cd build
cmake .. -DNOVASVG_BUILD_EXAMPLES=ON -DNOVASVG_BUILD_CLI=ON
make -j4
```

### Command-Line Tool
Build with CLI enabled:
```bash
cmake .. -DNOVASVG_BUILD_CLI=ON
make -j4
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

## 🔧 Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `NOVASVG_BUILD_EXAMPLES` | Build C++ examples | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_TESTS` | Build unit tests | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_CLI` | Build command-line interface | `PROJECT_IS_TOP_LEVEL` |
| `NOVASVG_BUILD_DOCS` | Build documentation with Doxygen | `OFF` |
| `NOVASVG_DIST_DIR` | Generate single-header distribution | Not defined |

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
- **CLI Documentation**: See `README_CLI.md`

## � Acknowledgments

NovaSVG builds upon ideas from existing SVG libraries while maintaining a distinct architectural philosophy focused on minimalism and header-only design.

## �📞 Support

- **GitHub Issues**: https://github.com/MohammadRaziei/novasvg/issues
- **Documentation**: https://mohammadraziei.github.io/novasvg

---

<div align="center">
<em>NovaSVG – A new star in lightweight SVG processing</em>
</div>
