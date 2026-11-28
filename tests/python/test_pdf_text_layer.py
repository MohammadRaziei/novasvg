"""The text of a PDF made by render_to_pdf() can be selected, searched and extracted: an invisible text layer (render mode
3, a font without glyphs and a /ToUnicode map) sits on the outlines that draw it. Read back with pdfium (pypdfium2)."""

import pytest

import novasvg
import novasvg.fonts as fonts

pdfium = pytest.importorskip("pypdfium2")

W, H = 300, 120


def _svg(body, w=W, h=H):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}">{body}</svg>'


def _text(body):
    data = novasvg.Document.load_from_data(_svg(body)).render_to_pdf()
    return pdfium.PdfDocument(data)[0].get_textpage().get_text_range()


class TestExtraction:
    def test_plain_and_non_ascii(self):
        assert _text('<text x="8" y="40" font-size="20" font-family="sans-serif">Hello, world! \u00dcn\u00ef \u2713</text>') == \
            "Hello, world! \u00dcn\u00ef \u2713"

    def test_beyond_the_basic_multilingual_plane(self):
        assert _text('<text x="8" y="40" font-size="20">a\U0001F600b</text>') == "a\U0001F600b"

    def test_several_texts_and_tspans(self):
        got = _text('<text x="8" y="30" font-size="16">First line</text><text x="8" y="70" font-size="16">Second line</text>')
        assert got.split() == ["First", "line", "Second", "line"]
        assert _text('<text x="8" y="40" font-size="16">a <tspan fill="red">b</tspan> c</text>') == "a b c"

    def test_rotated_text_and_a_group_with_opacity(self):
        assert _text('<text transform="rotate(-30 60 90)" x="60" y="90" font-size="18">Rotated</text>') == "Rotated"
        assert _text('<g opacity=".5"><text x="8" y="40" font-size="20">In a group</text></g>') == "In a group"

    def test_text_under_a_gradient_fill(self):
        assert _text('<defs><linearGradient id="g"><stop offset="0" stop-color="#f00"/><stop offset="1" stop-color="#00f"/>'
                     '</linearGradient></defs><text x="8" y="40" font-size="30" fill="url(#g)">Gradient</text>') == "Gradient"


class TestOnce:
    @pytest.mark.parametrize("attrs", [
        'fill="#036" stroke="#000" stroke-width="1"',
        'fill="#036" stroke="#000" style="paint-order:stroke"',
        'fill="none" stroke="#000"',
    ])
    def test_filled_and_or_stroked_text_is_there_once(self, attrs):
        assert _text(f'<text x="8" y="40" font-size="24" {attrs}>Both</text>') == "Both"


class TestNotThere:
    def test_hidden_text(self):
        assert _text('<text x="8" y="40" font-size="20" visibility="hidden">gone</text>'
                     '<text x="8" y="80" font-size="20" display="none">gone too</text>') == ""

    def test_text_that_only_makes_a_mask(self):
        assert _text('<defs><mask id="m"><text x="8" y="60" font-size="40" fill="#fff">MASKTEXT</text></mask></defs>'
                     '<rect width="300" height="120" fill="#06c" mask="url(#m)"/>') == ""

    def test_raster_mode_is_one_bitmap(self):
        doc = novasvg.Document.load_from_data(_svg('<text x="8" y="40" font-size="20">Hello</text>'))
        data = doc.render_to_pdf(novasvg.PdfMode.RASTER)
        assert pdfium.PdfDocument(data)[0].get_textpage().get_text_range() == ""


class TestPlacement:
    def test_the_characters_sit_on_the_outlines(self):
        # <text x=8 y=40 font-size=20>: the first character starts at 8 px = 6 pt, the baseline is (120 - 40) px = 60 pt
        # above the bottom edge of the page... in points: (120 - 40) * 0.75.
        text = "Hello, world"
        data = novasvg.Document.load_from_data(
            _svg(f'<text x="8" y="40" font-size="20" font-family="sans-serif">{text}</text>')).render_to_pdf()
        tp = pdfium.PdfDocument(data)[0].get_textpage()
        assert tp.get_text_range() == text
        advance = fonts.measure_text(text, "sans-serif", 20.0).width
        first, last = tp.get_charbox(0), tp.get_charbox(len(text) - 1)
        assert first[0] == pytest.approx(8 * 0.75, abs=2.0)
        assert last[2] == pytest.approx((8 + advance) * 0.75, abs=2.0)   # the run was stretched to the measured advance
        assert first[1] == pytest.approx((H - 40) * 0.75, abs=2.0)       # its baseline

    def test_translated_text_moves_with_its_transform(self):
        data = novasvg.Document.load_from_data(
            _svg('<g transform="translate(100 20)"><text x="8" y="40" font-size="20">Moved</text></g>')).render_to_pdf()
        tp = pdfium.PdfDocument(data)[0].get_textpage()
        first = tp.get_charbox(0)
        assert first[0] == pytest.approx((100 + 8) * 0.75, abs=2.0)
        assert first[1] == pytest.approx((H - 60) * 0.75, abs=2.0)
