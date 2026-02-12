# NovaSVG Documentation

NovaSVG is a header-only C++ library for parsing, rendering, and manipulating SVG (Scalable Vector Graphics) files.

## Features

- **Header-only**: Easy integration, no linking required
- **Fast rendering**: High-performance SVG rendering
- **Complete SVG support**: Supports most SVG 1.1 features
- **Modern C++**: Written in C++17 with clean API
- **Cross-platform**: Works on Windows, Linux, macOS

## Quick Start

```cpp
#include <novasvg/novasvg.h>

int main() {
    // Load SVG from file
    auto document = novasvg::Document::loadFromFile("image.svg");
    
    // Create bitmap for rendering
    novasvg::Bitmap bitmap(800, 600);
    
    // Render SVG to bitmap
    document.render(bitmap);
    
    // Save to PNG
    bitmap.writeToPng("output.png");
    
    return 0;
}
```

## API Overview

### Core Classes

- **`Document`**: Main class for SVG documents
- **`Bitmap`**: Image buffer for rendering
- **`Matrix`**: 2D transformation matrix
- **`Box`**: Bounding box representation
- **`Element`**: SVG element manipulation
- **`Color`**: Color representation and parsing

### Key Functions

- `Document::loadFromFile()` - Load SVG from file
- `Document::loadFromData()` - Load SVG from string
- `Bitmap::writeToPng()` - Save bitmap as PNG
- `Element::render()` - Render individual elements
- `Matrix` operations - Transformations and compositions

## Examples

Check the `examples/` directory for complete examples:

1. **SVG to PNG conversion** (`sample01_convert_svg_to_png.cpp`)
2. **SVG size query** (`sample02_query_svg_size.cpp`)

## Building

```bash
mkdir build
cd build
cmake .. -DNOVASVG_BUILD_EXAMPLES=ON -DNOVASVG_BUILD_TESTS=ON
make
```

## Documentation

This documentation is generated using Doxygen with doxygen-awesome-css theme.

### Navigation

- **Classes**: Browse all C++ classes
- **Files**: View header files
- **Examples**: See example code
- **Graphs**: Visualize class relationships

## License

MIT License - see LICENSE file for details.

## Links

- GitHub: https://github.com/mohammadraziei/novasvg
- Issues: https://github.com/mohammadraziei/novasvg/issues