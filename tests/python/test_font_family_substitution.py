"""CSS font-family names that are not installed resolve through novasvg's own substitution table.

An SVG that asks for `helvetica neue` or `"trebuchet ms", verdana, sans-serif` (what Mermaid emits) must land
on a metric-compatible font when the machine has one, without any OS font-matching library.

The tests register recognisable fonts of their own: a copy of the bundled DejaVu Sans whose advance for "A"
is changed tells us exactly which face was picked. Real machines already have some of the names in the
alias table (macOS has Arial and Courier New, Windows has Calibri and Georgia, ...), and an installed font
rightly beats its alias, so every test works with alias keys that are NOT installed on this machine,
determined once at import, before any test registers anything.

The font cache is process-wide and the newest registration wins a tie, so a test that registers a font under
an alias key itself gets a key of its own.
"""
import io
from pathlib import Path

import numpy as np
import pytest
from fontTools.ttLib import TTFont

import novasvg
import novasvg.fonts as fonts

_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"

# (alias key, the metric-compatible family the table substitutes for it)
_ALIASES = [
    ("Calibri", "Carlito"),
    ("Cambria", "Caladea"),
    ("Helvetica Neue", "Liberation Sans"),
    ("Times", "Liberation Serif"),
    ("Courier", "Liberation Mono"),
    ("Helvetica", "Liberation Sans"),
    ("Georgia", "Gelasio"),
]
_FREE = [pair for pair in _ALIASES if not fonts.get_font_face(pair[0], False, False)]  # a null FontFace is falsy


def free_alias(index):
    if len(_FREE) < 2:
        pytest.skip("fewer than two alias keys are uninstalled on this machine")
    return _FREE[index]


def register(family, a_advance):
    """Register a DejaVu Sans copy under `family` whose "A" advances `a_advance` font units."""
    font = TTFont(str(_FONT_PATH))
    _, lsb = font["hmtx"].metrics["A"]
    font["hmtx"].metrics["A"] = (a_advance, lsb)
    out = io.BytesIO()
    font.save(out)
    assert fonts.add_font_face_from_data(family, False, False, out.getvalue())


def a_advance(stack):
    face = fonts.get_font_face_for_family_stack(stack, False, False)
    return face.advance_width_units(ord("A")) if face else None


def test_alias_reaches_a_metric_compatible_family():
    key, target = free_alias(0)
    register(target, 1111)
    assert a_advance(key) == 1111
    assert a_advance(key.lower()) == 1111  # CSS family names are case-insensitive
    assert a_advance(key.upper()) == 1111


def test_installed_name_beats_its_alias_whatever_the_case():
    key, target = free_alias(-1)
    register(target, 1111)
    register(key, 2222)
    assert a_advance(key) == 2222
    assert a_advance(key.lower()) == 2222
    assert a_advance(key.upper()) == 2222


def test_stack_is_walked_in_order_and_skips_missing_names():
    key, target = free_alias(0)
    register(target, 3333)
    assert a_advance(f'"No Such Family", {key}, monospace') == 3333


def test_earlier_name_wins_over_later_one():
    key, target = free_alias(0)
    register(target, 4444)
    register("Ordering Probe", 5555)
    assert a_advance(f"{key}, Ordering Probe") == 4444
    assert a_advance(f"Ordering Probe, {key}") == 5555


def test_unknown_names_resolve_to_nothing():
    assert a_advance("No Such Family") is None
    assert a_advance('"Another Missing One", No Such Family') is None
    assert a_advance("") is None


def test_generic_keywords_resolve_through_the_generic_table():
    expected = fonts.get_font_face("sans-serif", False, False).advance_width_units(ord("A"))
    assert a_advance("No Such Family, sans-serif") == expected
    assert a_advance("No Such Family, SANS-SERIF") == expected


def test_text_in_an_svg_uses_the_substituted_face():
    key, target = free_alias(0)
    fonts.add_font_face_from_file("DejaVu Sans", False, False, str(_FONT_PATH))
    register(target, 4444)  # the wide "A" is visible in the layout

    def render(family):
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="60">'
               '<rect width="100%" height="100%" fill="white"/>'
               f'<text x="5" y="40" font-size="32" font-family="{family}">AAA</text></svg>')
        bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)
        bitmap.convert_to_rgba()
        return np.asarray(bitmap.numpy())

    assert np.array_equal(render(key), render(target))
    assert not np.array_equal(render(key), render("DejaVu Sans"))
