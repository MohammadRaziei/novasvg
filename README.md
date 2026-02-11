# NovaSVG

<div align="center">
<img src="https://raw.githubusercontent.com/MohammadRaziei/novasvg/refs/heads/master/data/nova.svg" width="160">
</div>

NovaSVG is a lightweight, header-only C++17/20 library for parsing and rasterizing SVG files. It provides a compact API for loading SVG documents, querying elements, and rendering to a bitmap using a single include.

## Features

- Header-only public API under `include/novasvg/`
- Parse SVG content from files or memory
- Render documents to bitmaps or PNG output streams
- Simple matrix and geometry helpers

## Usage

Include the public header and define `NOVASVG_IMPLEMENTATION` in exactly one translation unit:

```cpp
#define NOVASVG_IMPLEMENTATION
#include <novasvg/novasvg.h>
```

### Loading and rendering

```cpp
auto doc = novasvg::Document::loadFromFile("artwork.svg");
if(doc)
{
    auto bitmap = doc->renderToBitmap();
    bitmap.writeToPng("artwork.png");
}
```

### Querying the document

```cpp
auto doc = novasvg::Document::loadFromData(svg_text);
auto root = doc->documentElement();
auto matches = doc->querySelectorAll("rect");
```

## Examples

See the `examples/` directory for complete examples that locate the `data/` directory using `std::filesystem::path(__FILE__)`:

- `sample01_convert_svg_to_png.cpp`
- `sample02_query_svg_size.cpp`

## License

NovaSVG is distributed under the terms of the MIT license. See [LICENSE](LICENSE).
