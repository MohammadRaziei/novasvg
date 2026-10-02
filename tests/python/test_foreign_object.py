"""<foreignObject> HTML text: soft wrapping (measure + paint agree) and GSUB ligatures."""
from pathlib import Path

import numpy as np
import pytest

import novasvg
import novasvg.fonts as fonts

# The font the tests measure with ships in data/fonts/ (unmodified, license alongside; shared with the C++ tests): wrapping and
# ligature results depend on its exact metrics and GSUB table, so they must not depend on whatever
# fonts a machine happens to have installed.
_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"

FAMILY = "DejaVu Sans"
TEXT = "This is a very long label that should wrap across several lines when the maximum text width is reached"


@pytest.fixture(scope="module")
def font():
    fonts.add_font_face_from_file(FAMILY, False, False, str(_FONT_PATH))
    return fonts.Font(fonts.get_font_face(FAMILY, False, False), 16.0)


def _html(style, text=TEXT):
    return f'<div xmlns="http://www.w3.org/1999/xhtml" style="{style}"><span><p>{text}</p></span></div>'


class TestMeasureWrapping:
    def test_nowrap_is_one_line_even_with_max_width(self, font):
        m = fonts.measure_foreign_object(_html("white-space:nowrap;line-height:1.5;max-width:200px"), font)
        assert m["line_count"] == 1
        assert m["width"] > 200  # the line overflows; nothing soft-wraps it

    def test_break_spaces_wraps_at_declared_width(self, font):
        m = fonts.measure_foreign_object(
            _html("display:table;white-space:break-spaces;line-height:1.5;max-width:200px;width:200px"), font)
        assert m["line_count"] > 1
        assert m["width"] <= 200.0 + 1 / 64
        assert m["height"] == pytest.approx(m["line_count"] * 24.0)  # line-height:1.5 * 16px

    def test_explicit_max_width_overrides_declared(self, font):
        narrow = fonts.measure_foreign_object(_html("line-height:1.5"), font, max_width=120)
        wide = fonts.measure_foreign_object(_html("line-height:1.5"), font, max_width=300)
        assert narrow["line_count"] > wide["line_count"] > 1
        assert narrow["width"] <= 120 + 1 / 64

    def test_nowrap_beats_explicit_max_width(self, font):
        m = fonts.measure_foreign_object(_html("white-space:nowrap"), font, max_width=100)
        assert m["line_count"] == 1

    def test_no_width_no_wrap(self, font):
        assert fonts.measure_foreign_object(_html("line-height:1.5"), font)["line_count"] == 1

    def test_long_word_overflows_instead_of_breaking(self, font):
        m = fonts.measure_foreign_object(_html("", text="Supercalifragilisticexpialidocious"), font, max_width=50)
        assert m["line_count"] == 1 and m["width"] > 50

    def test_forced_breaks_are_kept(self, font):
        m = fonts.measure_foreign_object(_html("white-space:nowrap", text="one<br/>two<br/>three"), font)
        assert m["line_count"] == 3


def _ink_height(svg):
    """Height in px of the rows that contain dark (text) pixels."""
    arr = novasvg.Document.load_from_data(svg).render_to_bitmap().numpy()
    dark = (arr[:, :, 3] > 0) & (arr[:, :, :3].sum(axis=2) < 300)
    rows = np.where(dark.any(axis=1))[0]
    return int(rows[-1] - rows[0] + 1) if len(rows) else 0


class TestPaintWrapping:
    def _svg(self, style):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="220" height="220">'
                f'<rect width="100%" height="100%" fill="white"/>'
                f'<foreignObject x="5" y="5" width="200" height="200" font-family="{FAMILY}" font-size="16">'
                f'{_html(style)}</foreignObject></svg>')

    def test_paint_wraps_like_measure(self, font):
        assert _ink_height(self._svg("white-space:break-spaces;line-height:1.5;width:200px")) > 100  # ~6 lines

    def test_paint_keeps_nowrap_on_one_line(self, font):
        assert _ink_height(self._svg("white-space:nowrap;line-height:1.5")) < 24  # one line


class TestLigatures:
    def test_ff_ligature_is_narrower_than_two_fs(self, font):
        assert font.measure_text("ff") < 2 * font.measure_text("f") - 0.1

    def test_ligature_changes_word_width_consistently(self, font):
        # "Off" = O + ff-ligature; a space breaks the sequence so it does not ligate
        assert font.measure_text("Off") < font.measure_text("Of f") - font.measure_text(" ") - 0.1

    def test_text_without_f_sequences_is_unchanged(self, font):
        assert font.measure_text("Design") == pytest.approx(
            sum(font.measure_text(c) for c in "Design"), abs=1.0)
