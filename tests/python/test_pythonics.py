import os
import pytest
from io import BytesIO
from pathlib import Path

# Import the library components
# Assuming the package is installed or PYTHONPATH is set correctly
import novasvg

# --- Fixtures ---

@pytest.fixture
def sample_svg_content():
    """Returns a simple SVG string."""
    return '<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg"><rect width="100" height="100" fill="red"/></svg>'

@pytest.fixture
def sample_svg_file(tmp_path, sample_svg_content):
    """Creates a temporary SVG file and returns its path."""
    file_path = tmp_path / "test.svg"
    file_path.write_text(sample_svg_content)
    return file_path

@pytest.fixture
def output_png(tmp_path):
    """Returns a path for a temporary PNG file."""
    return tmp_path / "output.png"


# --- Tests for Color Module ---

class TestColor:
    """Test cases for the Color utility class."""

    def test_rgba_creation(self):
        c = novasvg.Color.rgba(255, 0, 0, 128)
        assert c.r == 255
        assert c.g == 0
        assert c.b == 0
        assert c.a == 128
        # Check integer packing (ARGB)
        # 0x80FF0000 -> 2155905152 in decimal
        assert c.to_int() == 0xFF000080

    def test_predefined_colors(self):
        red = novasvg.Color.red()
        assert red.to_int() == 0xFF0000FF
        assert red.r == 255
        
        white = novasvg.Color.white()
        assert white.to_int() == 0xFFFFFFFF
        
        transparent = novasvg.Color.transparent()
        assert transparent.to_int() == 0x00000000

    def test_from_hash(self):
        # Full hex without alpha
        c1 = novasvg.Color.from_hash("#00FF00")
        assert c1.to_int() == 0x00FF00FF
        
        # Short hex
        c2 = novasvg.Color.from_hash("#0F0")
        assert c2.to_int() == 0x00FF00FF
        
        # Hex with alpha
        c3 = novasvg.Color.from_hash("#FF000080") # Red with 50% alpha
        assert c3.to_int() == 0xFF000080

    def test_from_name(self):
        c1 = novasvg.Color.from_name("blue")
        assert c1.to_int() == 0x0000FFFF
        
        c2 = novasvg.Color.from_name("OrAnGe") # Case insensitive
        assert c2.to_int() == 0xFFA500FF

    def test_from_name_invalid(self):
        with pytest.raises(ValueError):
            novasvg.Color.from_name("notacolor")

    def test_invalid_hash(self):
        with pytest.raises(ValueError):
            novasvg.Color.from_hash("ZZZZZZ")
        with pytest.raises(ValueError):
            novasvg.Color.from_hash("#12345") # Invalid length


# --- Tests for Core Module (svg2png) ---

class TestSvg2Png:
    """Test cases for the svg2png utility function."""

    def test_render_from_string(self, sample_svg_content, output_png):
        novasvg.svg2png(sample_svg_content, output_png)
        assert output_png.exists()
        assert output_png.stat().st_size > 0

    def test_render_from_path(self, sample_svg_file, output_png):
        novasvg.svg2png(sample_svg_file, output_png)
        assert output_png.exists()

    def test_render_from_file_object(self, sample_svg_content, output_png):
        # Simulate a file-like object
        file_obj = BytesIO(sample_svg_content.encode('utf-8'))
        novasvg.svg2png(file_obj, output_png)
        assert output_png.exists()

    def test_render_with_color_object(self, sample_svg_content, output_png):
        # Use Color object instead of int
        bg = novasvg.Color.white()
        novasvg.svg2png(sample_svg_content, output_png, background_color=bg)
        assert output_png.exists()

    def test_render_with_css(self, sample_svg_content, output_png):
        # Change rect color to blue via CSS
        css = "rect { fill: blue; }"
        novasvg.svg2png(sample_svg_content, output_png, css=css)
        assert output_png.exists()
        # Note: To verify pixel data, we'd need to open the PNG with Pillow/PIL,
        # but existence check is sufficient for unit testing the binding logic.

    def test_render_with_dimensions(self, sample_svg_content, output_png):
        novasvg.svg2png(sample_svg_content, output_png, width=50, height=50)
        assert output_png.exists()

    def test_file_not_found_error(self, output_png):
        with pytest.raises(FileNotFoundError):
            novasvg.svg2png("nonexistent_file.svg", output_png)

    def test_invalid_svg_data(self, output_png):
        with pytest.raises(FileNotFoundError):
            novasvg.svg2png("this is not a valid svg", output_png)

    def test_invalid_svg_valid_data(self, output_png):
        with pytest.raises(ValueError):
            novasvg.svg2png("<svg> this is not a valid svg", output_png)

