"""
novasvg.fonts -- font loading and text/HTML-label measurement.

Wraps FontFace/Font (see novasvg/font.h in the C++ library) and the free
functions built around them (see binding.cpp), all backed by novasvg's own
vendored stb_truetype font stack (novasvg/detail/render/font.h) -- the same
code that later paints text and <foreignObject> HTML content, so anything
measured here can never disagree with what novasvg itself goes on to
render.

Typical usage::

    import novasvg.fonts as fonts

    fonts.add_font_face_from_file("DejaVu Sans", False, False, "DejaVuSans.ttf")
    face = fonts.get_font_face("DejaVu Sans", bold=False, italic=False)
    font = fonts.Font(face, size=16.0)
    font.measure_text("Hello, world!")  # -> width in pixels
"""

from .novasvg_py import FontFace
from .novasvg_py import Font
from .novasvg_py import add_font_face_from_file
from .novasvg_py import add_font_face_from_data
from .novasvg_py import get_font_face
from .novasvg_py import get_font_face_for_family_stack
from .novasvg_py import measure_foreign_object

__all__ = [
    "FontFace",
    "Font",
    "add_font_face_from_file",
    "add_font_face_from_data",
    "get_font_face",
    "get_font_face_for_family_stack",
    "measure_foreign_object",
]
