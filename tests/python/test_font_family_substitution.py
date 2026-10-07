"""CSS font-family names that are not installed resolve through novasvg's own substitution table.

An SVG that asks for `helvetica neue` or `"trebuchet ms", verdana, sans-serif` (what Mermaid emits) must land
on a metric-compatible font when the machine has one, without any OS font-matching library.

The tests register recognisable fonts of their own: a copy of the bundled DejaVu Sans whose advance for "A"
is changed tells us exactly which face was picked. Real machines already have some of the names in the
substitution chains (macOS has Times, Windows has Arial), and an installed font rightly beats a substitute
further down the chain, so what a test expects is "the first family of the chain that exists on this machine",
computed from the cache itself. On a machine with none of the earlier names (Linux CI) that is the family the
test registered; elsewhere it is the real font.

The font cache is process-wide and the newest registration wins a tie, so tests that register a font under
an alias key itself use a key of their own.
"""
import io
from pathlib import Path

import numpy as np
from fontTools.ttLib import TTFont

import novasvg
import novasvg.fonts as fonts

_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"

# alias key -> the families the table tries for it, in order (the key's own spelling first)
_SANS = ["Helvetica Neue", "Arial", "Liberation Sans"]
_SERIF = ["Times", "Times New Roman", "Liberation Serif"]


def present(family):
    return bool(fonts.get_font_face(family, False, False))  # a null FontFace is falsy


def resolved(chain):
    """The first family of `chain` that exists right now."""
    return next(family for family in chain if present(family))


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


def chain_advance(chain):
    return fonts.get_font_face(resolved(chain), False, False).advance_width_units(ord("A"))


def test_alias_reaches_a_metric_compatible_family():
    register("Liberation Sans", 1111)
    expected = chain_advance(_SANS)
    assert a_advance("Helvetica Neue") == expected
    assert a_advance("helvetica neue") == expected  # CSS family names are case-insensitive
    assert a_advance("HELVETICA NEUE") == expected


def test_installed_name_beats_its_alias_whatever_the_case():
    register("Liberation Serif", 1111)
    register("Times", 2222)  # on a machine that already has a "Times" the newest registration wins
    assert a_advance("Times") == 2222
    assert a_advance("times") == 2222
    assert a_advance("TIMES") == 2222


def test_stack_is_walked_in_order_and_skips_missing_names():
    register("Liberation Sans", 3333)
    assert a_advance('"No Such Family", Helvetica Neue, monospace') == chain_advance(_SANS)


def test_earlier_name_wins_over_later_one():
    register("Liberation Sans", 4444)
    register("Ordering Probe", 5555)
    assert a_advance("Helvetica Neue, Ordering Probe") == chain_advance(_SANS)
    assert a_advance("Ordering Probe, Helvetica Neue") == 5555


def test_unknown_names_resolve_to_nothing():
    assert a_advance("No Such Family") is None
    assert a_advance('"Another Missing One", No Such Family') is None
    assert a_advance("") is None


def test_generic_keywords_resolve_through_the_generic_table():
    expected = fonts.get_font_face("sans-serif", False, False).advance_width_units(ord("A"))
    assert a_advance("No Such Family, sans-serif") == expected
    assert a_advance("No Such Family, SANS-SERIF") == expected


def test_text_in_an_svg_uses_the_substituted_face():
    fonts.add_font_face_from_file("DejaVu Sans", False, False, str(_FONT_PATH))
    register("Liberation Sans", 4444)  # the wide "A" is visible in the layout
    stand_in = resolved(_SANS)

    def render(family):
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="60">'
               '<rect width="100%" height="100%" fill="white"/>'
               f'<text x="5" y="40" font-size="32" font-family="{family}">AAA</text></svg>')
        bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)
        bitmap.convert_to_rgba()
        return np.asarray(bitmap.numpy())

    assert np.array_equal(render("Helvetica Neue"), render(stand_in))
    assert not np.array_equal(render("Helvetica Neue"), render("DejaVu Sans"))
