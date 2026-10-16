"""Pillow and libstb interop for Bitmap.

Neither is a dependency: each is imported only when one of these runs (like NumPy for
Bitmap.numpy()); install them with ``pip install "novasvg[pillow]"`` or ``pip install libstb``.
``import novasvg`` never loads them.
"""

from __future__ import annotations

from .novasvg_py import Bitmap

# Pillow modes that are not 8-bit L / LA / RGB / RGBA but convert to one without guessing.
_PIL_NATIVE = ("L", "LA", "RGB", "RGBA")
_PIL_CONVERT = {"1": "L", "La": "LA", "PA": "RGBA", "RGBa": "RGBA", "RGBX": "RGB", "CMYK": "RGB", "YCbCr": "RGB"}


def _pillow():
    try:
        from PIL import Image as PILImage
    except ImportError as e:
        raise ImportError('Pillow is needed for this: pip install "novasvg[pillow]"') from e
    return PILImage


def _to_pil(self):
    """A Pillow image of mode RGBA with these pixels (a copy, straight alpha).

    Needs Pillow, imported only here. Use this rather than ``PIL.Image.fromarray(bitmap)``:
    the Bitmap's own memory is premultiplied BGRA, which Pillow would read as RGBA.
    """
    PILImage = _pillow()
    if self.is_null():
        raise ValueError("cannot convert a null Bitmap")
    return PILImage.frombytes("RGBA", (self.width, self.height), self.tobytes())


def _from_pil(cls, pil):
    """A Bitmap from a Pillow image (a copy). Needs Pillow, imported only here.

    L, LA, RGB and RGBA are taken as they are. Modes that hold 8-bit colour in another form
    are converted first: 1 -> L, P -> RGB (RGBA if the palette has transparency), PA / RGBa
    -> RGBA, La -> LA, RGBX / CMYK / YCbCr -> RGB. Other modes (I, I;16, F, LAB, HSV, ...)
    have no 8-bit colour to take and raise TypeError: convert them yourself.
    """
    PILImage = _pillow()
    if not isinstance(pil, PILImage.Image):
        raise TypeError(f"from_pil needs a PIL.Image.Image, not {type(pil).__name__}")
    mode = pil.mode
    if mode == "P":
        pil = pil.convert("RGBA" if "transparency" in pil.info else "RGB")
    elif mode in _PIL_CONVERT:
        pil = pil.convert(_PIL_CONVERT[mode])
    elif mode not in _PIL_NATIVE:
        raise TypeError(
            f"Pillow mode {mode!r} has no 8-bit colour novasvg can take: use L, LA, RGB or RGBA, "
            "or convert it first, e.g. pil.convert('RGBA')"
        )
    channels = len(pil.getbands())
    view = memoryview(pil.tobytes()).cast("B", shape=[pil.height, pil.width, channels])
    return cls.from_array(view)


def _libstb():
    try:
        import libstb
    except ImportError as e:
        raise ImportError('libstb is needed for this: pip install libstb') from e
    return libstb


def _to_stb(self):
    """A libstb.Image with these pixels: 4 channels, straight RGBA, a copy.

    Needs libstb (https://github.com/MohammadRaziei/libstb), imported only here. From there
    you can resize, convert, or write JPEG / PNG / BMP / TGA with stb's encoders.
    """
    libstb = _libstb()
    if self.is_null():
        raise ValueError("cannot convert a null Bitmap")
    view = memoryview(self.tobytes()).cast("B", shape=[self.height, self.width, 4])
    return libstb.Image(view).copy()  # own the pixels, independent of the temporary bytes


def _from_stb(cls, image):
    """A Bitmap from a libstb.Image of 1 to 4 channels (L, LA, RGB, RGBA), a copy.

    Needs libstb, imported only here. The pixels are taken as straight alpha.
    """
    libstb = _libstb()
    if not isinstance(image, libstb.Image):
        raise TypeError(f"from_stb needs a libstb.Image, not {type(image).__name__}")
    return cls.from_array(image)


Bitmap.to_pil = _to_pil
Bitmap.from_pil = classmethod(_from_pil)
Bitmap.to_stb = _to_stb
Bitmap.from_stb = classmethod(_from_stb)

__all__ = []
