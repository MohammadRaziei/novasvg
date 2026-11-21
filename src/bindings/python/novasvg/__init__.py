"""
novasvg -- a header-only SVG library, with Python bindings.

Core document/rendering classes (Document, Element, Node, Matrix, Box,
Bitmap, Color) live at the top level, alongside svg2png() for the common
"render this SVG to a PNG file" case (see novasvg.utils for its
implementation).

Font loading and text/HTML-label measurement (FontFace, Font,
add_font_face_from_file, get_font_face, measure_foreign_object, ...) live
under novasvg.fonts -- import that submodule directly rather than reaching
for these through the top-level namespace:

    import novasvg.fonts as fonts
    fonts.add_font_face_from_file("DejaVu Sans", False, False, "DejaVuSans.ttf")
"""

from .novasvg_py import Bitmap
from .novasvg_py import Box
from .novasvg_py import Color
from .novasvg_py import Document
from .novasvg_py import Element
from .novasvg_py import InvalidColorError
from .novasvg_py import Matrix
from .novasvg_py import Node
from .novasvg_py import TextNode
from .novasvg_py import version_string

from . import fonts
from . import interop as _interop  # adds Bitmap.to_pil / Bitmap.from_pil
from .utils import svg2png

__version__ = version_string()
__author__ = "Mohammad Raziei"


def get_include_dir() -> str:
    """Directory to put on the include path: `#include <novasvg/novasvg.h>` resolves inside it."""
    from pathlib import Path
    return str(Path(__file__).resolve().parent / "include")


def get_cmake_dir() -> str:
    """Directory of novasvgConfig.cmake, for `find_package(novasvg CONFIG)` / CMAKE_PREFIX_PATH."""
    from pathlib import Path
    return str(Path(__file__).resolve().parent / "lib" / "cmake" / "novasvg")

__all__ = [
    "Bitmap", "Box", "Color", "Document", "Element", "InvalidColorError", "Matrix", "Node", "TextNode",
    "fonts",
    "svg2png",
    "__version__", "__author__", "get_include_dir", "get_cmake_dir",
]
