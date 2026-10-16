# NovaSVG for Python

Python bindings for [NovaSVG](README.md), built with nanobind and scikit-build-core.

```bash
pip install novasvg
```

**No runtime dependencies.** NovaSVG renders and encodes PNG, JPEG, BMP and TGA on its own, with no Pillow and no NumPy. NumPy is only needed for `Bitmap.numpy()` and Pillow only for `to_pil()` / `from_pil()`; if one is missing, that call raises an `ImportError` that says so, and everything else keeps working.

Requires Python 3.9 or newer. Building from source also needs a C++17 compiler and CMake:

```bash
git clone https://github.com/mohammadraziei/novasvg
cd novasvg
pip install .
```

## Quick start

```python
import novasvg
from novasvg import Color

# One call: SVG file, string, bytes or file object -> PNG
novasvg.svg2png("artwork.svg", "artwork.png", width=800, background_color=Color.white())

# Or keep the parsed document and render it as many times as you like
doc = novasvg.Document.load_from_file("artwork.svg")
print(doc.width, doc.height)

bitmap = doc.render_to_bitmap(width=800)      # height follows the aspect ratio
bitmap.write_to_png("artwork.png")
png_bytes = bitmap.to_png()                   # or in memory, no temp file
```

## API

### `Document`

```python
doc = novasvg.Document.load_from_file("image.svg")
doc = novasvg.Document.load_from_data(svg_string)     # str or bytes

doc.width, doc.height                  # floats
doc.bounding_box()                     # Box

bitmap = doc.render_to_bitmap(width=400, height=300)
bitmap = doc.render_to_bitmap(400, 300, Color.white())    # with a background
bitmap = doc.render_to_bitmap(400, 300, 0xFFFFFFFF)       # packed 0xRRGGBBAA also works

root     = doc.document_element()
element  = doc.get_element_by_id("my-element")
elements = doc.query_selector_all("rect, circle")         # CSS selectors
doc.apply_stylesheet("circle { fill: purple; }")
```

### `Bitmap`

```python
bitmap = novasvg.Bitmap(800, 600)
bitmap.clear(Color.white())
bitmap.width, bitmap.height, bitmap.stride

# Encode in memory
png  = bitmap.to_png()
bmp  = bitmap.to_bmp()
tga  = bitmap.to_tga()
jpg  = bitmap.to_jpg(quality=90)                 # 1-100, default 80
data = bitmap.to_bytes("jpeg", quality=90)       # "png" (default), "bmp", "tga", "jpeg"

# Or write to a file; each returns True on success
bitmap.write_to_png("out.png")
bitmap.write_to_jpg("out.jpg", quality=90)
bitmap.write("out.jpg")                          # format picked from the extension
```

Notes:

- The `to_*` methods raise `ValueError` for a null bitmap and `RuntimeError` if encoding fails. They never return empty `bytes`.
- **Don't call `convert_to_rgba()` before encoding.** The encoders already convert from premultiplied ARGB to straight RGBA themselves, on a scratch copy. Converting the bitmap in place first makes them convert a second time, which swaps red and blue and corrupts semi-transparent pixels. `convert_to_rgba()` is only for handing the raw buffer to something else.
- JPEG has no alpha channel. Render onto an opaque background (`doc.render_to_bitmap(w, h, 0xFFFFFFFF)`) before `to_jpg()`, otherwise transparent areas come out black.

#### Interop: memoryview, NumPy, DLPack, Pillow

NumPy, Pillow and [libstb](https://github.com/MohammadRaziei/libstb) are optional extras (`pip install "novasvg[numpy]"`, `"novasvg[pillow]"`, `"novasvg[stb]"`); `import novasvg` never loads them.

```python
import numpy as np
import novasvg

bm = novasvg.Document.load_from_file("art.svg").render_to_bitmap(400)

# Zero-copy views of the Bitmap's own memory (raw, see "Pixel format" below)
bm.shape                       # (height, width, 4)
memoryview(bm)                 # writable 3-D uint8 view, no NumPy needed
bm.numpy()                     # ndarray view; np.asarray(bm) is the same
np.from_dlpack(bm)             # DLPack: also torch / jax / cupy.from_dlpack(bm)
bm.__array_interface__         # NumPy's array interface

# Copies in ordinary straight-alpha RGBA
bm.tobytes()                   # bytes, (height, width, 4), tightly packed, no NumPy needed
bm.numpy(rgba=True)            # ndarray
pil = bm.to_pil()              # Pillow RGBA image
img = bm.to_stb()              # libstb.Image, 4 channels: resize, convert, write jpg / png / bmp / tga

# Import (always a copy): uint8 (H, W) or (H, W, 1..4), straight alpha
bm = novasvg.Bitmap.from_array(arr)       # any buffer, DLPack object or __array_interface__ object
bm = novasvg.Bitmap(arr)                  # same thing
bm = novasvg.Bitmap.from_dlpack(tensor)   # numpy / torch / jax / cupy (CPU) -> Bitmap
bm = novasvg.Bitmap.from_pil(pil_image)   # L, LA, RGB, RGBA, 1, P, ... (see the docstring)
bm = novasvg.Bitmap.from_stb(stb_image)   # libstb.Image of 1 to 4 channels
bm = novasvg.Bitmap.from_array(raw, raw=True)   # (H, W, 4) in the Bitmap's own layout, as the views give it
bm.copy()                                 # an independent Bitmap (a deep copy)
```

**Pixel format.** A `Bitmap` always stores *premultiplied* ARGB32, because that is what the renderer draws in: on little-endian machines the bytes are B, G, R, A. Everything zero-copy (`memoryview`, `numpy()`, DLPack, `__array_interface__`) shows exactly that memory, so the colours are in BGR order and scaled by alpha. Everything that gives ordinary RGBA converts and therefore copies: `tobytes()`, `numpy(rgba=True)`, `to_pil()`. Do not pass the Bitmap straight to `PIL.Image.fromarray()` or `libstb.Image()`; use `to_pil()` / `to_stb()`. `convert_to_rgba()` rewrites the buffer in place to straight RGBA, after which the views are RGBA too; do not combine it with `numpy(rgba=True)` or `tobytes()`, which would convert a second time.

**Import copies.** The Bitmap never views Python memory, so nothing has to be kept alive and writing to the array afterwards does not change it. Both `from_array` and `from_pil` take straight alpha and premultiply while copying. Sizes must stay below 32768 pixels per side.

If NumPy is missing, `numpy()` and `np.asarray(bitmap)` raise an `ImportError` that says so; everything else keeps working.

### `Element`

```python
rect = doc.get_element_by_id("rect1")

if rect.has_attribute("fill"):
    color = rect.get_attribute("fill")
rect.set_attribute("fill", "#00FF00")

rect.get_local_bounding_box()      # Box
rect.get_global_bounding_box()     # Box
rect.get_local_matrix()            # Matrix
rect.children()

rect.render_to_bitmap(width=200, height=200)       # render just this element
rect.render(bitmap, matrix)                        # or onto an existing bitmap
```

### `Matrix`, `Box`, `Color`

```python
from novasvg import Matrix, Color

m = Matrix.translated(100, 50).multiply(Matrix.scaled(2.0, 2.0))
r = Matrix.rotated(45, 100, 100)          # angle, cx, cy
m.inverse()

Color(255, 0, 0, 255)
Color.from_name("orange")
Color.from_hash("#336699")
Color.white(), Color.transparent()
```

### Fonts

Font loading and text measurement live in `novasvg.fonts`:

```python
import novasvg.fonts as fonts

fonts.add_font_face_from_file("Inter", False, False, "Inter-Regular.ttf")   # family, bold, italic, path
face = fonts.get_font_face("Inter", False, False)
```

## Example: edit, then render

```python
import novasvg

svg = """
<svg width="300" height="200" xmlns="http://www.w3.org/2000/svg">
    <rect id="rect1" x="50" y="50" width="100" height="100" fill="blue"/>
    <circle id="circle1" cx="150" cy="100" r="40" fill="red"/>
</svg>
"""

doc = novasvg.Document.load_from_data(svg)

rect = doc.get_element_by_id("rect1")
rect.set_attribute("fill", "#00FF00")
rect.set_attribute("stroke", "black")
rect.set_attribute("stroke-width", "3")

doc.render_to_bitmap(600).write_to_png("edited.png")
```

## Command line

The native `novasvg` command-line tool (convert, info, query, batch) is documented in the [main README](README.md#command-line).

## Troubleshooting

**Build fails.** You need a C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+), CMake, and the Python development headers (`python3-dev` on Ubuntu).

**`ImportError` from `numpy()` or `to_pil()`.** Install NumPy (`pip install numpy`) or Pillow (`pip install pillow`). Nothing else needs them.

**Text looks wrong.** Install a metric-compatible font family such as `fonts-liberation`, or register your own with `fonts.add_font_face_from_file`. See the [Fonts section](README.md#fonts) of the main README.

## License

MIT, see [LICENSE.txt](LICENSE.txt). Issues: https://github.com/mohammadraziei/novasvg/issues. Docs: https://mohammadraziei.github.io/novasvg
