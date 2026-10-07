"""CSS font-family names that are not installed resolve through novasvg's own substitution table.

An SVG that asks for `arial` or `"trebuchet ms", verdana, sans-serif` (what Mermaid emits) must land on a
metric-compatible font when the machine has one, without any OS font-matching library. Every test registers
its own recognisable fonts, so the outcome never depends on what is installed: a copy of the bundled
DejaVu Sans whose advance for "A" is changed tells us exactly which face was picked.

The font cache is process-wide and the newest registration wins a tie, so each test uses a family of its own.
"""
import io
from pathlib import Path

import numpy as np
import pytest

import novasvg
import novasvg.fonts as fonts

fontTools = pytest.importorskip("fontTools.ttLib")

_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"


def register(family, a_advance):
    """Register a DejaVu Sans copy under `family` whose "A" advances `a_advance` font units."""
    font = fontTools.TTFont(str(_FONT_PATH))
    _, lsb = font["hmtx"].metrics["A"]
    font["hmtx"].metrics["A"] = (a_advance, lsb)
    out = io.BytesIO()
    font.save(out)
    assert fonts.add_font_face_from_data(family, False, False, out.getvalue())


def a_advance(stack):
    face = fonts.get_font_face_for_family_stack(stack, False, False)
    return face.advance_width_units(ord("A")) if face else None


def test_alias_reaches_a_metric_compatible_family():
    register("Liberation Mono", 1111)
    assert a_advance("Courier New") == 1111
    assert a_advance("courier new") == 1111  # CSS family names are case-insensitive


def test_installed_name_beats_its_alias_whatever_the_case():
    register("Liberation Serif", 1111)
    register("Times New Roman", 2222)
    assert a_advance("Times New Roman") == 2222
    assert a_advance("times new roman") == 2222
    assert a_advance("TIMES NEW ROMAN") == 2222


def test_stack_is_walked_in_order_and_skips_missing_names():
    register("Carlito", 3333)
    assert a_advance('"No Such Family", Calibri, monospace') == 3333


def test_earlier_name_wins_over_later_one():
    register("Caladea", 4444)
    register("Liberation Sans", 5555)
    assert a_advance("Cambria, Arial") == 4444
    assert a_advance("Arial, Cambria") == 5555


def test_unknown_names_resolve_to_nothing():
    assert a_advance("No Such Family") is None
    assert a_advance("") is None


def test_generic_keywords_resolve_through_the_generic_table():
    expected = fonts.get_font_face("sans-serif", False, False).advance_width_units(ord("A"))
    assert a_advance("No Such Family, sans-serif") == expected
    assert a_advance("No Such Family, SANS-SERIF") == expected


def test_a_name_without_a_metric_compatible_stand_in_is_skipped_not_guessed():
    assert a_advance("Verdana") is None
    assert a_advance('"Trebuchet MS", Verdana') is None


def test_text_in_an_svg_uses_the_substituted_face():
    fonts.add_font_face_from_file("DejaVu Sans", False, False, str(_FONT_PATH))
    register("Caladea", 4444)  # "Cambria" -> "Caladea"; the huge advance is visible in the layout

    def render(family):
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="60">'
               '<rect width="100%" height="100%" fill="white"/>'
               f'<text x="5" y="40" font-size="32" font-family="{family}">AAA</text></svg>')
        bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)
        bitmap.convert_to_rgba()
        return np.asarray(bitmap.numpy())

    assert np.array_equal(render("Cambria"), render("Caladea"))
    assert not np.array_equal(render("Cambria"), render("DejaVu Sans"))
