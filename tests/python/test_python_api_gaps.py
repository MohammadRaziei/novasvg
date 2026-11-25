"""measure_text, is_valid_selector, add_font_file and render_to_pdf / save_pdf: what the C++ API and the CLI had
and the Python module did not."""

from pathlib import Path

import pytest

import novasvg
import novasvg.fonts as fonts

FONT_DIR = Path(__file__).resolve().parents[2] / "data" / "fonts"
SVG = (
    '<svg xmlns="http://www.w3.org/2000/svg" width="80" height="40">'
    '<rect width="80" height="40" fill="#fc0"/><circle cx="40" cy="20" r="12" fill="#06c"/></svg>'
)


class TestAddFontFile:
    def test_registers_the_faces_of_a_file(self):
        assert fonts.add_font_file(str(FONT_DIR / "DejaVuSans.ttf")) >= 1
        face = fonts.get_font_face("DejaVu Sans", False, False)
        assert fonts.Font(face, 16.0).measure_text("Hello") > 0

    def test_missing_file_adds_nothing(self, tmp_path):
        assert fonts.add_font_file(str(tmp_path / "nope.ttf")) == 0


class TestMeasureText:
    @pytest.fixture(autouse=True)
    def _font(self):
        fonts.add_font_file(str(FONT_DIR / "DejaVuSans.ttf"))
        fonts.add_font_file(str(FONT_DIR / "DejaVuSans-Bold.ttf"))

    def test_metrics_of_a_run(self):
        m = fonts.measure_text("Hello, world!", "DejaVu Sans", 16.0)
        assert m.width > 0 and m.ascent > 0 and m.descent >= 0 and m.line_gap >= 0

    def test_agrees_with_font_measure_text(self):
        font = fonts.Font(fonts.get_font_face("DejaVu Sans", False, False), 16.0)
        assert fonts.measure_text("Hello, world!", "DejaVu Sans", 16.0).width == pytest.approx(
            font.measure_text("Hello, world!"), abs=0.01)

    def test_scales_with_the_size(self):
        small = fonts.measure_text("Hello", "DejaVu Sans", 10.0)
        big = fonts.measure_text("Hello", "DejaVu Sans", 20.0)
        assert big.width == pytest.approx(2 * small.width, rel=0.02)
        assert big.ascent == pytest.approx(2 * small.ascent, rel=0.02)

    def test_bold_is_wider_and_true_is_not_weight_one(self):
        regular = fonts.measure_text("Hello", "DejaVu Sans", 16.0)
        bold = fonts.measure_text("Hello", "DejaVu Sans", 16.0, True)
        assert bold.width > regular.width
        # `True` is also an int in Python: it has to mean bold, and 700 has to give the same thing.
        assert fonts.measure_text("Hello", "DejaVu Sans", 16.0, 700).width == pytest.approx(bold.width)
        assert fonts.measure_text("Hello", "DejaVu Sans", 16.0, 400).width == pytest.approx(regular.width)

    def test_css_family_list_and_unknown_family(self):
        listed = fonts.measure_text("Hello", '"No Such Family", DejaVu Sans', 16.0)
        assert listed.width == pytest.approx(fonts.measure_text("Hello", "DejaVu Sans", 16.0).width)

    def test_is_exported_with_its_type(self):
        assert "measure_text" in fonts.__all__ and "TextMetrics" in fonts.__all__
        assert "width" in repr(fonts.measure_text("a", "DejaVu Sans", 12.0))


class TestIsValidSelector:
    @pytest.mark.parametrize("selector", ["rect", "#id", ".cls", "g > rect", "rect, circle", "[data-x]", "rect:first-child"])
    def test_valid(self, selector):
        assert novasvg.is_valid_selector(selector) is True

    @pytest.mark.parametrize("selector", ["", "rect[", "###", "rect >", "<<"])
    def test_invalid(self, selector):
        assert novasvg.is_valid_selector(selector) is False

    def test_tells_no_match_from_wrong(self):
        doc = novasvg.Document.load_from_data(SVG)
        assert doc.query_selector_all("ellipse") == []      # valid, nothing matches
        assert novasvg.is_valid_selector("ellipse") is True
        assert novasvg.is_valid_selector("ellipse[") is False


class TestRenderToPdf:
    @pytest.fixture
    def doc(self):
        return novasvg.Document.load_from_data(SVG)

    def test_vector_is_a_pdf(self, doc):
        data = doc.render_to_pdf()
        assert isinstance(data, bytes) and data.startswith(b"%PDF-") and b"%%EOF" in data[-32:]

    def test_both_modes_and_dpi(self, doc):
        vector = doc.render_to_pdf(novasvg.PdfMode.VECTOR)
        raster = doc.render_to_pdf(novasvg.PdfMode.RASTER, 96.0)
        assert vector.startswith(b"%PDF-") and raster.startswith(b"%PDF-")
        assert vector != raster
        assert len(doc.render_to_pdf(novasvg.PdfMode.RASTER, 300.0)) > len(raster)

    def test_default_is_vector_192(self, doc):
        assert doc.render_to_pdf() == doc.render_to_pdf(novasvg.PdfMode.VECTOR, 192.0)

    def test_save_pdf_writes_the_same_bytes(self, doc, tmp_path):
        out = tmp_path / "a.pdf"
        assert doc.save_pdf(str(out)) is True
        assert out.read_bytes() == doc.render_to_pdf()

    def test_save_pdf_to_a_bad_path_is_false(self, doc, tmp_path):
        assert doc.save_pdf(str(tmp_path / "no" / "such" / "dir" / "a.pdf")) is False

    def test_a_document_without_a_size_gives_empty_bytes(self):
        doc = novasvg.Document.load_from_data('<svg xmlns="http://www.w3.org/2000/svg"/>')
        assert doc is None or doc.render_to_pdf() == b""
