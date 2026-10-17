## [Unreleased]

### Added

- `novasvg::InvalidColorError` (C++, derives from `std::invalid_argument`) and `novasvg.InvalidColorError`
  (Python, a `ValueError` subclass), raised for every color parse failure: empty string, missing `#`,
  wrong length, non-hex digit and unknown color name. Existing `except ValueError` /
  `catch (const std::invalid_argument&)` code keeps working.

### Fixed

- `Color("#gggggg")` raised a bare `ValueError: stoi`. The message now names the problem, e.g.
  `Invalid hex color '#gggggg': 'g' is not a hex digit (0-9, a-f, A-F)`, and a wrong length lists the
  accepted forms.
- `Color::fromHash` silently accepted a bad trailing digit: `#12345g` parsed as a valid color because
  `std::stoi` stops at the first invalid character. Every digit is now checked.

## [0.11.0] - 2026-10-17

### Added

- **Python: `Bitmap` interoperability with the array ecosystem.** NumPy, Pillow and
  [libstb](https://github.com/MohammadRaziei/libstb) are optional extras
  (`novasvg[numpy]`, `novasvg[pillow]`, `novasvg[stb]`); `import novasvg` never loads them.
  - Zero-copy export of the Bitmap's own memory: the buffer protocol (`memoryview(bitmap)`,
    a writable 3-D `(height, width, 4)` uint8 view, no NumPy needed), `bitmap.numpy()` /
    `np.asarray(bitmap)`, DLPack (`__dlpack__`, `__dlpack_device__`: `np.from_dlpack`,
    `torch.from_dlpack`, `jax.numpy.from_dlpack`, `cupy.from_dlpack`) and
    `__array_interface__`, plus `bitmap.shape`.
  - Conversions to ordinary straight-alpha RGBA, as copies: `bitmap.tobytes()`,
    `bitmap.numpy(rgba=True)`, `bitmap.to_pil()` (Pillow) and `bitmap.to_stb()` (libstb).
  - Import, always as a copy: `Bitmap(array)`, `Bitmap.from_array(x, raw=False)`,
    `Bitmap.from_dlpack(x, raw=False)`, `Bitmap.from_pil(img)`, `Bitmap.from_stb(img)`.
    Accepts uint8 `(H, W)` or `(H, W, 1..4)` (L, LA, RGB, RGBA, straight alpha) from any
    buffer, DLPack object or `__array_interface__` object. `raw=True` takes `(H, W, 4)` already
    in the Bitmap's own layout.
  - `Bitmap.copy()`, `copy.copy` and `copy.deepcopy` give an independent deep copy
    (the C++ copy constructor shares pixels). `Bitmap.__repr__`.
- CI: native `ubuntu-24.04-arm` runner for ARM64 wheels, replacing emulation.
- Tests for all of the above, including a run with NumPy, Pillow and libstb blocked.

### Changed

- `Bitmap.numpy()` now takes the keyword-only arguments `rgba`, `dtype` and `copy`. The default is
  unchanged: a zero-copy view of the raw pixels. For a null bitmap it returns an empty
  `(0, 0, 4)` array instead of `None`.
- NumPy is optional: `numpy()` and `np.asarray(bitmap)` raise a clear `ImportError` when it is
  missing, and everything else keeps working.
- `README_PYTHON.md` documents the interop and the pixel format.

### Notes

- A Bitmap always stores premultiplied ARGB32 (BGRA bytes on little-endian). Zero-copy views show
  that memory as it is, so pass a Bitmap to Pillow or libstb through `to_pil()` / `to_stb()`,
  not `PIL.Image.fromarray(bitmap)` or `libstb.Image(bitmap)`, which would read the colours in the
  wrong order.
- `convert_to_rgba()` rewrites the buffer in place; do not combine it with `numpy(rgba=True)` or
  `tobytes()`, which would convert a second time.