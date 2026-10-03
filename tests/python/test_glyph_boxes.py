"""Font.glyph_boxes(): per-glyph (pen_x, ink_left, ink_right), same walk as measure_text()."""
from pathlib import Path

import pytest

import novasvg.fonts as fonts

# Shipped with the repo so the numbers below never depend on fonts installed on the machine.
_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"
FAMILY = "DejaVu Sans"


@pytest.fixture(scope="module")
def font():
    fonts.add_font_face_from_file(FAMILY, False, False, str(_FONT_PATH))
    return fonts.Font(fonts.get_font_face(FAMILY, False, False), 16.0)


def test_empty_text_has_no_boxes(font):
    assert font.glyph_boxes("") == []


def test_known_values_from_the_font_outlines(font):
    # DejaVu Sans, 2048 units/em, 16px: P = 201..1165, K = 201..1386 (advance 1343), P advance 1235.
    (p_pen, p_l, p_r), (k_pen, k_l, k_r) = font.glyph_boxes("PK")
    assert (p_pen, p_l, p_r) == (0.0, pytest.approx(201 * 16 / 2048), pytest.approx(1165 * 16 / 2048))
    assert k_pen == pytest.approx(1235 * 16 / 2048)
    assert (k_l, k_r) == (pytest.approx(201 * 16 / 2048), pytest.approx(1386 * 16 / 2048))


def test_ink_can_stick_out_past_the_advance_box(font):
    # K's outline ends further right than its own advance (1386 > 1343 units) -- the case where a
    # browser's text box grows beyond the advance width.
    _, (_, _, k_right) = font.glyph_boxes("PK")
    assert k_right == pytest.approx(1386 * 16 / 2048)
    assert k_right > 1343 * 16 / 2048


def test_negative_left_side_bearing_is_reported_as_is(font):
    (_, ink_left, _), *_ = font.glyph_boxes("T")
    assert ink_left < 0  # T's outline starts left of its pen position


def test_spaces_have_no_ink_but_still_advance_the_pen(font):
    boxes = font.glyph_boxes("T o")
    assert len(boxes) == 2  # the space produced no box ...
    assert boxes[1][0] == pytest.approx(font.measure_text("T "), abs=1e-3)  # ... but moved the pen


def test_pen_follows_kerning_like_measure_text(font):
    # "To" is a kerned pair, so 'o' sits closer than the plain advance of 'T'.
    _, (o_pen, _, _) = font.glyph_boxes("To")
    assert o_pen == pytest.approx(font.measure_text("To") - font.measure_text("o"), abs=1e-3)
    assert o_pen < font.measure_text("T") - 1


def test_ligature_is_one_glyph_box(font):
    # fi -> U+FB01 when the font's GSUB says so (see test_foreign_object.py); one glyph, one box.
    assert len(font.glyph_boxes("fi")) == len(font.glyph_boxes("\ufb01")) == 1


def test_last_pen_plus_advance_matches_measure_text(font):
    text = "string"
    boxes = font.glyph_boxes(text)
    assert len(boxes) == len(text)
    assert boxes[-1][0] + font.measure_text("g") == pytest.approx(font.measure_text(text), abs=1e-3)
