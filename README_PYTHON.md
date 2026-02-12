# NovaSVG Python Bindings

Python bindings for NovaSVG library using nanobind and scikit-build-core.

## Installation

```bash
pip install -e .
```

Or install from source:

```bash
git clone https://github.com/mohammadraziei/novasvg
cd novasvg
pip install .
```

## Requirements

- Python >= 3.7
- numpy >= 1.20.0
- C++17 compiler
- CMake >= 3.15

## Quick Start

```python
import novasvg
import numpy as np
import matplotlib.pyplot as plt

# Load SVG from file or string
svg_string = """
<svg width="200" height="200" xmlns="http://www.w3.org/2000/svg">
    <rect x="20" y="20" width="160" height="160" fill="red" stroke="blue" stroke-width="4"/>
    <circle cx="100" cy="100" r="60" fill="green" opacity="0.7"/>
</svg>
"""

# Load document
doc = novasvg.Document.load_from_data(svg_string)

# Render to numpy array
image = doc.render_to_array()  # shape: (200, 200, 4) RGBA

# Display with matplotlib
plt.imshow(image)
plt.axis('off')
plt.show()

# Save to PNG
bitmap = doc.render_to_bitmap()
bitmap.write_to_png("output.png")
```

## API Reference

### Core Classes

#### `Document`
Main class for SVG documents.

```python
# Load from file
doc = novasvg.load("image.svg")

# Load from string
doc = novasvg.Document.load_from_data(svg_string)

# Properties
width = doc.width()      # float
height = doc.height()    # float
bbox = doc.bounding_box() # Box object

# Rendering
bitmap = doc.render_to_bitmap(width=400, height=300)
array = doc.render_to_array(width=400, height=300)

# Element access
root = doc.document_element()
element = doc.get_element_by_id("my-element")
elements = doc.query_selector_all("rect, circle")
```

#### `Bitmap`
Image buffer class with numpy integration.

```python
# Create bitmap
bitmap = novasvg.Bitmap(800, 600)

# Convert to/from numpy
array = bitmap.to_numpy()  # shape: (height, width, 4)
bitmap2 = novasvg.Bitmap.from_numpy(array)

# Save to PNG
bitmap.write_to_png("output.png")

# Properties
width = bitmap.width()
height = bitmap.height()
stride = bitmap.stride()  # bytes per row
data = bitmap.data()      # raw pointer
```

#### `Matrix`
2D transformation matrix.

```python
# Create matrices
identity = novasvg.Matrix()
translated = novasvg.Matrix.translated(100, 50)
scaled = novasvg.Matrix.scaled(2.0, 2.0)
rotated = novasvg.Matrix.rotated(45, 100, 100)

# Combine transformations
transform = translated * scaled * rotated

# Apply to elements
element.render(bitmap, transform)
```

#### `Element`
SVG element with manipulation capabilities.

```python
# Get/set attributes
if element.has_attribute("fill"):
    color = element.get_attribute("fill")
    element.set_attribute("fill", "#FF0000")

# Bounding boxes
local_bbox = element.get_local_bounding_box()
global_bbox = element.get_global_bounding_box()

# Transformations
matrix = element.get_local_matrix()

# Render individual element
element.render_to_bitmap(width=200, height=200)
```

### Utility Functions

```python
# Convenience functions
doc = novasvg.load("image.svg")  # Alias for Document.load_from_file
array = novasvg.render_svg(svg_string, width=400, height=300)
array = novasvg.svg_to_array(svg_string)  # Same as render_svg

# Matrix creation
matrix = novasvg.create_matrix(a=1, b=0, c=0, d=1, e=100, f=50)

# Bitmap from array
bitmap = novasvg.create_bitmap_from_array(numpy_array)
```

## Examples

### Batch Processing

```python
import novasvg
import numpy as np
from pathlib import Path

# Process multiple SVG files
svg_dir = Path("svgs")
output_dir = Path("rendered")

for svg_file in svg_dir.glob("*.svg"):
    doc = novasvg.load(str(svg_file))
    
    # Render at different sizes
    for size in [256, 512, 1024]:
        image = doc.render_to_array(width=size, height=size)
        
        # Convert to RGB (remove alpha if needed)
        rgb = image[..., :3]
        
        # Save or process further
        output_file = output_dir / f"{svg_file.stem}_{size}.png"
        # ... save using PIL or other library
```

### Element Manipulation

```python
import novasvg

svg = """
<svg width="300" height="200" xmlns="http://www.w3.org/2000/svg">
    <rect id="rect1" x="50" y="50" width="100" height="100" fill="blue"/>
    <circle id="circle1" cx="150" cy="100" r="40" fill="red"/>
</svg>
"""

doc = novasvg.Document.load_from_data(svg)

# Modify element attributes
rect = doc.get_element_by_id("rect1")
rect.set_attribute("fill", "#00FF00")
rect.set_attribute("stroke", "black")
rect.set_attribute("stroke-width", "3")

# Render modified SVG
image = doc.render_to_array()
```

### Integration with NumPy and SciPy

```python
import novasvg
import numpy as np
from scipy import ndimage

# Render SVG
doc = novasvg.load("image.svg")
image = doc.render_to_array()

# Process with numpy/scipy
gray = np.mean(image[..., :3], axis=2)  # Convert to grayscale
edges = ndimage.sobel(gray)  # Edge detection
blurred = ndimage.gaussian_filter(image, sigma=2)

# Create new bitmap from processed image
processed_bitmap = novasvg.Bitmap.from_numpy(blurred)
```

## Performance Tips

1. **Reuse Bitmaps**: Create bitmap once and reuse for multiple renders
2. **Batch Operations**: Process multiple SVGs in sequence
3. **Appropriate Sizes**: Render at required size, not larger
4. **Memory Management**: Large bitmaps consume significant memory

## Troubleshooting

### Build Issues
- Ensure CMake >= 3.15 is installed
- C++17 compiler required (GCC >= 7, Clang >= 5, MSVC >= 2017)
- Python development headers needed (`python3-dev` on Ubuntu)

### Runtime Issues
- Check SVG file validity
- Ensure sufficient memory for large renders
- Verify numpy array shapes (height, width, 4)

## License

MIT License - see LICENSE file for details.

## Support

- GitHub Issues: https://github.com/mohammadraziei/novasvg/issues
- Documentation: https://mohammadraziei.github.io/novasvg