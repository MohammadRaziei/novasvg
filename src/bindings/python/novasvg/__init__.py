"""
NovaSVG - A header-only SVG library with Python bindings
"""

# Import the C++ extension module
from .novasvg_py import Document
from .novasvg_py import Element
from .novasvg_py import TextNode
from .novasvg_py import Node
from .novasvg_py import Matrix
from .novasvg_py import Box
from .novasvg_py import Bitmap
from .novasvg_py import Color

# Import utility functions
from .novasvg_py import version_string
from .novasvg_py import add_font_face_from_file
from .novasvg_py import add_font_face_from_data

# Set version directly from the C++ library
__version__ = version_string()
__author__ = "Mohammad Raziei"

import os
import logging
from io import IOBase
from pathlib import Path
from typing import Union, Optional

# Configure logger
logger = logging.getLogger(__name__)




class _ColorUtilities:
    """
    Represents a color in ARGB format.
    Provides static factory methods to create color instances.
    """

    def __init__(self, r: int, g: int, b: int, a: int = 255):
        """
        Initialize a Color instance.

        :param r: Red component (0-255).
        :param g: Green component (0-255).
        :param b: Blue component (0-255).
        :param a: Alpha component (0-255).
        """
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    def to_int(self) -> int:
        """
        Convert the color to a 32-bit integer in 0xAARRGGBB format.

        :return: Integer representation of the color.
        """
        return (self.r << 24) | (self.g << 16) | (self.b << 8) | self.a

    def __repr__(self) -> str:
        return f"Color(r={self.r}, g={self.g}, b={self.b}, a={self.a})"

    # --- Static Factory Methods ---

    @staticmethod
    def rgba(r: int, g: int, b: int, a: int = 255) -> 'Color':
        """
        Create a color from Red, Green, Blue, and Alpha components.

        :param r: Red component (0-255).
        :param g: Green component (0-255).
        :param b: Blue component (0-255).
        :param a: Alpha component (0-255), defaults to 255 (opaque).
        :return: A Color instance.
        """
        return Color(r, g, b, a)

    @staticmethod
    def from_hash(hex_code: str) -> 'Color':
        """
        Create a color from a hexadecimal string.
        Supports formats: #RRGGBB, #RGB, #RRGGBBAA, #RGBA.

        :param hex_code: Hexadecimal string (e.g., "#FF0000", "#F00", "#FF000080").
        :return: A Color instance.
        :raises ValueError: If the hex string format is invalid.
        """
        if not hex_code.startswith("#"):
            raise ValueError("Hex color string must start with '#'")

        hex_body = hex_code[1:]

        if len(hex_body) not in (3,4,6,8):
            raise ValueError(f"Invalid hex color format: '{hex_code}'")

        
        # Handle shorthand (e.g., #FFF -> #FFFFFF)
        if len(hex_body) == 3:
            hex_body = f"{hex_body[0]}{hex_body[0]}{hex_body[1]}{hex_body[1]}{hex_body[2]}{hex_body[2]}"
        
        # Handle shorthand with alpha (e.g., #FFFA -> #FFFFFFAA)
        elif len(hex_body) == 4:
            hex_body = f"{hex_body[0]}{hex_body[0]}{hex_body[1]}{hex_body[1]}{hex_body[2]}{hex_body[2]}{hex_body[3]}{hex_body[3]}"


        r = int(hex_body[0:2], 16)
        g = int(hex_body[2:4], 16)
        b = int(hex_body[4:6], 16)
        a = 255
        
        if len(hex_body) == 8:
            # #RRGGBBAA
            a = int(hex_body[6:8], 16)
        return Color(r, g, b, a)

    @staticmethod
    def from_name(name: str) -> 'Color':
        """
        Create a color from a standard CSS color name.
        This method dynamically looks up the corresponding static method.

        :param name: The name of the color (e.g., "red", "skyblue").
        :return: A Color instance.
        :raises ValueError: If the color name is not recognized.
        """
        name_lower = name.lower()
        
        # Check if a static method with this name exists
        if hasattr(Color, name_lower):
            attr = getattr(Color, name_lower)
            # Ensure it is callable (a method) and not a property/variable
            if callable(attr):
                try:
                    return attr()
                except Exception:
                    pass # Fall through to error if call fails
        
        raise ValueError(f"Unknown color name: '{name}'")

    # --- Predefined Colors (Static Methods) ---

    @staticmethod
    def black() -> 'Color': return Color(0, 0, 0)
    @staticmethod
    def white() -> 'Color': return Color(255, 255, 255)
    @staticmethod
    def red() -> 'Color': return Color(255, 0, 0)
    @staticmethod
    def green() -> 'Color': return Color(0, 128, 0)
    @staticmethod
    def lime() -> 'Color': return Color(0, 255, 0)
    @staticmethod
    def blue() -> 'Color': return Color(0, 0, 255)
    @staticmethod
    def yellow() -> 'Color': return Color(255, 255, 0)
    @staticmethod
    def cyan() -> 'Color': return Color(0, 255, 255)
    @staticmethod
    def magenta() -> 'Color': return Color(255, 0, 255)
    @staticmethod
    def silver() -> 'Color': return Color(192, 192, 192)
    @staticmethod
    def gray() -> 'Color': return Color(128, 128, 128)
    @staticmethod
    def maroon() -> 'Color': return Color(128, 0, 0)
    @staticmethod
    def olive() -> 'Color': return Color(128, 128, 0)
    @staticmethod
    def purple() -> 'Color': return Color(128, 0, 128)
    @staticmethod
    def teal() -> 'Color': return Color(0, 128, 128)
    @staticmethod
    def navy() -> 'Color': return Color(0, 0, 128)
    @staticmethod
    def orange() -> 'Color': return Color(255, 165, 0)
    @staticmethod
    def pink() -> 'Color': return Color(255, 192, 203)
    @staticmethod
    def brown() -> 'Color': return Color(165, 42, 42)
    @staticmethod
    def transparent() -> 'Color': return Color(0, 0, 0, 0)

def _handle_input_data(
    input_data: Union[str, bytes, Path, IOBase]
) -> Optional[Document]:
    """
    Parses input data and returns a loaded :class:`Document` object.

    This internal function handles various input types including file paths,
    raw strings/bytes, and file-like objects.

    :param input_data: The SVG source. Can be a file path (str/Path),
                       raw data (str/bytes), or a file-like object.
    :type input_data: Union[str, bytes, Path, IOBase]
    :return: A loaded Document object if successful, None otherwise.
    :rtype: Optional[Document]
    """
    doc = None

    # 1. Handle File-like objects (IOBase)
    if isinstance(input_data, IOBase):
        try:
            doc = Document.load_from_data(input_data.read())
        except Exception as e:
            logger.error(f"Failed to read from file-like object: {e}")
            return None

    # 2. Handle Path objects (pathlib)
    elif isinstance(input_data, Path):
        if input_data.is_file():
            doc = Document.load_from_file(input_data.as_posix())
        else:
            logger.error(f"File not found at path: {input_data}")
            return None

    # 3. Handle String or Bytes data
    elif isinstance(input_data, (str, bytes)):
        # Check if it is a valid file path (only for strings)
        is_file_path = False
        if isinstance(input_data, str):
            is_file_path = os.path.isfile(input_data)

        if is_file_path:
            doc = Document.load_from_file(input_data)
        else:
            doc = Document.load_from_data(input_data)
            
    if doc is None:
        logger.error("Failed to parse SVG content.")
        return None

    return doc

"""
Core utility functions for SVG rendering.
"""

def _handle_input_data(
    input_data: Union[str, bytes, Path, IOBase]
) -> Document:
    """
    Parses input data and returns a loaded :class:`Document` object.
    Raises exceptions if parsing fails.

    :param input_data: The SVG source. Can be a file path (str/Path),
                       raw data (str/bytes), or a file-like object.
    :type input_data: Union[str, bytes, Path, IOBase]
    :return: A loaded Document object.
    :rtype: Document
    :raises FileNotFoundError: If input_data is a path that does not exist.
    :raises ValueError: If input_data is invalid or cannot be parsed.
    """
    doc = None
    content_str = None

    # 1. Handle File-like objects (IOBase)
    if isinstance(input_data, IOBase):
        try:
            content_str = input_data.read()
        except Exception as e:
            raise IOError(f"Failed to read from file-like object: {e}")

    # 2. Handle Path objects (pathlib)
    elif isinstance(input_data, Path):
        if not input_data.is_file():
            raise FileNotFoundError(f"SVG file not found at path: {input_data}")
        doc = Document.load_from_file(input_data.as_posix())

    # 3. Handle String or Bytes data
    elif isinstance(input_data, (str, bytes)):
        # Check if it is a valid file path
        is_file_path = len(input_data) < 1000 and not input_data.lstrip().startswith("<")
        if is_file_path:
            if not os.path.isfile(input_data):
                raise FileNotFoundError(f"SVG file not found at path: {input_data}")
            doc = Document.load_from_file(input_data)
        else:
            content_str = input_data

    # 4. Load Document from raw string if not already loaded
    if doc is None:
        if content_str is not None:
            doc = Document.load_from_data(content_str)
        else:
            raise ValueError("Invalid input data provided.")

    if doc is None:
        raise ValueError("Failed to parse SVG content.")

    return doc


def svg2png(
    input_data: Union[str, bytes, Path, IOBase],
    output_path: Union[str, Path],
    width: int = -1,
    height: int = -1,
    background_color: Union[int, Color] = 0x00000000,
    css: Optional[str] = None
) -> None:
    """
    Renders an SVG source to a PNG file.

    This function accepts a file path, a raw string/bytes containing SVG data,
    or a file-like object. It renders the content and saves it as a PNG image.
    Optionally, CSS styles can be applied to the SVG before rendering.

    :param input_data: The SVG source. Can be a file path (str/Path),
                       raw data (str/bytes), or a file-like object.
    :param output_path: The file system path where the PNG image will be saved.
    :param width: The desired width of the output image in pixels.
                  Defaults to -1 (auto-scale based on intrinsic size).
    :param height: The desired height of the output image in pixels.
                   Defaults to -1 (auto-scale based on intrinsic size).
    :param background_color: The background color. Can be an integer (0xAARRGGBB)
                             or a :class:`Color` object.
                             Defaults to 0x00000000 (fully transparent).
    :param css: A string containing CSS rules to apply to the SVG document.
                Defaults to None.
    :type input_data: Union[str, bytes, Path, IOBase]
    :type output_path: Union[str, Path]
    :type width: int
    :type height: int
    :type background_color: Union[int, Color]
    :type css: Optional[str]
    :raises FileNotFoundError: If the input file does not exist.
    :raises ValueError: If the SVG content is invalid or cannot be rendered.
    :raises IOError: If the output file cannot be written.
    """
    if isinstance(background_color, int):
        background_color = Color.from_value(background_color)

    # Load the document using the helper function
    doc = _handle_input_data(input_data)

    # Apply CSS if provided
    if css:
        try:
            doc.apply_stylesheet(css)
        except Exception as e:
            logger.warning(f"Failed to apply CSS stylesheet: {e}")
            # We don't raise here, as rendering might still work without CSS

    # Render to bitmap
    bitmap = doc.render_to_bitmap(width, height, background_color)

    if bitmap.is_null():
        raise ValueError("Failed to render bitmap (result is null).")

    # Convert output_path to string if it is a Path object
    output_str = str(output_path)

    # Write to PNG file
    success = bitmap.write_to_png(output_str)

    if not success:
        msg = f"Failed to write PNG to '{output_str}'"
        logger.error(msg)
        raise IOError(msg)
    
    logger.info(f"Successfully saved PNG to '{output_str}'")



__all__ = [
    "add_font_face_from_file", "add_font_face_from_data",
    "Bitmap", "Box", "Matrix", "Node", "TextNode", "Element", "Document",
    "svg2png",
    "__version__", "__author__"
]
