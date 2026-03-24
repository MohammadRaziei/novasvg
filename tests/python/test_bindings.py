import novasvg
from novasvg_fixtures import *

class TestBindings:
    """Test cases for direct C++ class bindings."""

    def test_document_load_from_string(self, sample_svg_content):
        doc = novasvg.Document.load_from_data(sample_svg_content)
        assert doc is not None
        assert doc.width == 100
        assert doc.height == 100

    def test_document_get_element_by_id(self):
        svg = '<svg id="root" width="10" height="10"><rect id="myRect" x="5" y="5" width="5" height="5"/></svg>'
        doc = novasvg.Document.load_from_data(svg)
        rect = doc.get_element_by_id("myRect")
        assert not rect.is_null()
        assert rect.has_attribute("x")

    def test_matrix_operations(self):
        m = novasvg.Matrix()
        assert m.a == 1.0
        assert m.e == 0.0
        
        m.translate(10, 20)
        assert m.e == 10.0
        assert m.f == 20.0

    def test_bitmap_properties(self):
        # Create a 10x10 bitmap
        bmp = novasvg.Bitmap(10, 10)
        assert not bmp.is_null()
        assert bmp.width == 10
        assert bmp.height == 10
        assert bmp.stride == 40 # 10 * 4 bytes (ARGB)

    def test_bitmap_clear_and_convert(self):
        bmp = novasvg.Bitmap(10, 10)
        # Clear with Red (0xFF0000FF)
        bmp.clear(0xFF0000FF)
        # Just ensure it doesn't crash
        bmp.convert_to_rgba()