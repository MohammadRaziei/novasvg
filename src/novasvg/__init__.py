"""
NovaSVG - A header-only SVG library with Python bindings
"""

from ._novasvg import (
    # Version functions
    version,
    version_string,
    
    # Font functions
    add_font_face_from_file,
    add_font_face_from_data,
    
    # Classes
    Bitmap,
    Box,
    Matrix,
    Node,
    TextNode,
    Element,
    Document,
)

from .__about__ import __version__

import numpy as np
from typing import Optional, Union, List, Tuple

__all__ = [
    "version",
    "version_string",
    "add_font_face_from_file",
    "add_font_face_from_data",
    "Bitmap",
    "Box",
    "Matrix",
    "Node",
    "TextNode",
    "Element",
    "Document",
    "load",
    "render_svg",
    "svg_to_array",
    "__version__",
]


def load(filename: str) -> Document:
    """
    Load an SVG document from a file.
    
    Parameters
    ----------
    filename : str
        Path to the SVG file
        
    Returns
    -------
    Document
        Loaded SVG document
    """
    return Document.load_from_file(filename)


def render_svg(
    svg_source: Union[str, Document],
    width: int = -1,
    height: int = -1,
    background_color: int = 0x00000000,
) -> np.ndarray:
    """
    Render SVG to a numpy array.
    
    Parameters
    ----------
    svg_source : str or Document
        SVG string or loaded Document
    width : int, optional
        Output width in pixels (-1 for auto)
    height : int, optional
        Output height in pixels (-1 for auto)
    background_color : int, optional
        Background color in 0xRRGGBBAA format
        
    Returns
    -------
    np.ndarray
        RGBA image array with shape (height, width, 4)
    """
    if isinstance(svg_source, str):
        # Check if it's a filename or SVG string
        if svg_source.strip().startswith("<"):
            doc = Document.load_from_data(svg_source)
        else:
            doc = Document.load_from_file(svg_source)
    else:
        doc = svg_source
    
    bitmap = doc.render_to_bitmap(width, height, background_color)
    return bitmap.to_numpy()


def svg_to_array(
    svg_source: Union[str, Document],
    width: int = -1,
    height: int = -1,
    background_color: int = 0x00000000,
) -> np.ndarray:
    """
    Alias for render_svg.
    
    Parameters
    ----------
    svg_source : str or Document
        SVG string or loaded Document
    width : int, optional
        Output width in pixels (-1 for auto)
    height : int, optional
        Output height in pixels (-1 for auto)
    background_color : int, optional
        Background color in 0xRRGGBBAA format
        
    Returns
    -------
    np.ndarray
        RGBA image array with shape (height, width, 4)
    """
    return render_svg(svg_source, width, height, background_color)


# Convenience functions for common operations
def create_matrix(
    a: float = 1.0,
    b: float = 0.0,
    c: float = 0.0,
    d: float = 1.0,
    e: float = 0.0,
    f: float = 0.0,
) -> Matrix:
    """
    Create a transformation matrix.
    
    Parameters
    ----------
    a, b, c, d, e, f : float
        Matrix components
        
    Returns
    -------
    Matrix
        Transformation matrix
    """
    return Matrix(a, b, c, d, e, f)


def create_bitmap_from_array(arr: np.ndarray) -> Bitmap:
    """
    Create a Bitmap from a numpy array.
    
    Parameters
    ----------
    arr : np.ndarray
        RGBA array with shape (height, width, 4)
        
    Returns
    -------
    Bitmap
        NovaSVG Bitmap object
    """
    return Bitmap.from_numpy(arr)


# Add numpy integration to existing classes
def _bitmap_to_array(self) -> np.ndarray:
    """Convert Bitmap to numpy array."""
    return self.to_numpy()


def _element_render_to_array(
    self,
    width: int = -1,
    height: int = -1,
    background_color: int = 0x00000000,
) -> np.ndarray:
    """Render element to numpy array."""
    bitmap = self.render_to_bitmap(width, height, background_color)
    return bitmap.to_numpy()


def _document_render_to_array(
    self,
    width: int = -1,
    height: int = -1,
    background_color: int = 0x00000000,
) -> np.ndarray:
    """Render document to numpy array."""
    bitmap = self.render_to_bitmap(width, height, background_color)
    return bitmap.to_numpy()


# Monkey patch the classes with numpy methods
Bitmap.to_array = _bitmap_to_array
Element.render_to_array = _element_render_to_array
Document.render_to_array = _document_render_to_array

# Add __version__ to module
__version__ = __version__