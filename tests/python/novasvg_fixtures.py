import pytest
from pathlib import Path

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
