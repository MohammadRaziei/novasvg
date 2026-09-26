"""
novasvg.utils -- high-level convenience helpers built on top of Document.

For anything more involved than "render this SVG to a PNG file" (partial
renders, custom background handling, reusing a parsed Document across
multiple renders, ...), use novasvg.Document directly instead.
"""

import logging
import os
from io import IOBase
from pathlib import Path
from typing import Optional, Union

from .novasvg_py import Color, Document

logger = logging.getLogger(__name__)

__all__ = ["svg2png"]


def _handle_input_data(input_data: Union[str, bytes, Path, IOBase]) -> Document:
    """Parses input data and returns a loaded :class:`Document`.

    Accepts a file path (str/Path), raw SVG data (str/bytes), or a
    file-like object.

    :raises FileNotFoundError: If input_data is a path that does not exist.
    :raises ValueError: If input_data is invalid or cannot be parsed.
    :raises IOError: If a file-like object cannot be read.
    """
    doc = None
    content_str = None

    if isinstance(input_data, IOBase):
        try:
            content_str = input_data.read()
        except Exception as e:
            raise IOError(f"Failed to read from file-like object: {e}") from e

    elif isinstance(input_data, Path):
        if not input_data.is_file():
            raise FileNotFoundError(f"SVG file not found at path: {input_data}")
        doc = Document.load_from_file(input_data.as_posix())

    elif isinstance(input_data, (str, bytes)):
        # Heuristic: short strings that don't start with "<" are treated as
        # a file path rather than inline SVG markup.
        is_file_path = len(input_data) < 1000 and not input_data.lstrip().startswith("<")
        if is_file_path:
            if not os.path.isfile(input_data):
                raise FileNotFoundError(f"SVG file not found at path: {input_data}")
            doc = Document.load_from_file(input_data)
        else:
            content_str = input_data

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
    css: Optional[str] = None,
) -> None:
    """Renders an SVG source to a PNG file.

    :param input_data: The SVG source: a file path (str/Path), raw
        data (str/bytes), or a file-like object.
    :param output_path: Where to save the PNG image.
    :param width: Output width in pixels. -1 (default) auto-scales from
        the SVG's intrinsic size.
    :param height: Output height in pixels. -1 (default) auto-scales from
        the SVG's intrinsic size.
    :param background_color: A packed 0xAARRGGBB integer or a
        :class:`novasvg.Color`. Defaults to fully transparent.
    :param css: Optional CSS rules to apply to the document before
        rendering.
    :raises FileNotFoundError: If the input file does not exist.
    :raises ValueError: If the SVG content is invalid or cannot be rendered.
    :raises IOError: If the output file cannot be written.
    """
    if isinstance(background_color, int):
        background_color = Color.from_value(background_color)

    doc = _handle_input_data(input_data)

    if css:
        try:
            doc.apply_stylesheet(css)
        except Exception as e:
            # Rendering may still succeed without the requested styling, so
            # this is a warning rather than a raised exception.
            logger.warning(f"Failed to apply CSS stylesheet: {e}")

    bitmap = doc.render_to_bitmap(width, height, background_color)
    if bitmap.is_null():
        raise ValueError("Failed to render bitmap (result is null).")

    output_str = str(output_path)
    if not bitmap.write_to_png(output_str):
        raise IOError(f"Failed to write PNG to '{output_str}'")

    logger.info(f"Successfully saved PNG to '{output_str}'")
