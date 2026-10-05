"""CSS for the HTML inside <foreignObject>: which rule colours which text and which box.

Mermaid colours a label with a rule that names an *ancestor in the SVG* -- `.section-0 span{color:black}`
reaches a <span> that sits inside <g class="section-0"><foreignObject>. A browser matches that chain;
reading only the tag's own class/name made every label in a diagram take the same colour (white text on
yellow boxes in the mindmap).
"""
from collections import Counter
from pathlib import Path

import numpy as np
import pytest

import novasvg
import novasvg.fonts as fonts

_FONT_PATH = Path(__file__).resolve().parents[2] / "data" / "fonts" / "DejaVuSans.ttf"
RED, GREEN, BLUE, BLACK, YELLOW = (255, 0, 0), (0, 160, 0), (0, 0, 255), (0, 0, 0), (255, 255, 0)
ROW = 100  # every label lives in its own 100px high row of the test image


@pytest.fixture(scope="module", autouse=True)
def _font():
    fonts.add_font_face_from_file("DejaVu Sans", False, False, str(_FONT_PATH))


def svg(rows, css="", svg_id="root"):
    """rows: [(g_class, html_inner)] -> one foreignObject per row inside <g class=g_class>."""
    sheet = f"<style>{css}</style>" if css else ""
    body = "".join(
        f'<g class="{cls}" transform="translate(0 {i * ROW})">'
        f'<foreignObject x="10" y="10" width="280" height="80" font-family="DejaVu Sans" font-size="48">'
        f'<div xmlns="http://www.w3.org/1999/xhtml">{html}</div></foreignObject></g>'
        for i, (cls, html) in enumerate(rows))
    return (f'<svg xmlns="http://www.w3.org/2000/svg" id="{svg_id}" width="300" height="{ROW * len(rows)}">'
            f'<rect width="100%" height="100%" fill="white"/>{sheet}{body}</svg>')


def render(doc_or_text):
    doc = novasvg.Document.load_from_data(doc_or_text) if isinstance(doc_or_text, str) else doc_or_text
    bitmap = doc.render_to_bitmap(-1, -1, 0)
    bitmap.convert_to_rgba()
    return np.asarray(bitmap.numpy())


def text_color(pixels, row, box=None):
    """The colour most text pixels in that row have (anti-aliased edges are rare compared to the stems).
    `box` is the colour of a box painted behind the text, which would otherwise outnumber the glyphs."""
    region = pixels[row * ROW:(row + 1) * ROW].reshape(-1, 4)
    ignore = {(255, 255, 255), box}
    opaque = [tuple(int(v) for v in px[:3]) for px in region if px[3] == 255 and tuple(px[:3]) not in ignore]
    return Counter(opaque).most_common(1)[0][0] if opaque else None


def colors(rows, css="", **kw):
    pixels = render(svg(rows, css, **kw))
    return [text_color(pixels, i) for i in range(len(rows))]


WORD = "<span><p>HHHH</p></span>"


# --- an ancestor in the SVG decides -----------------------------------------------------------------

def test_each_label_takes_the_colour_of_the_rule_for_its_own_svg_ancestor():
    # the mindmap bug: both rules match *some* span, but only one matches each label's ancestor
    css = ".hot span{color:#f00} .cold span{color:#00f}"
    assert colors([("hot", WORD), ("cold", WORD)], css) == [RED, BLUE]


def test_the_root_id_in_front_of_the_class_is_part_of_the_chain():
    assert colors([("hot", WORD)], "#root .hot span{color:#f00}") == [RED]


def test_a_rule_naming_an_ancestor_the_label_does_not_have_does_not_apply():
    assert colors([("cold", WORD)], ".hot span{color:#f00}") == [BLACK]


def test_a_wrong_root_id_does_not_match():
    assert colors([("hot", WORD)], "#other .hot span{color:#f00}") == [BLACK]


def test_the_child_combinator_between_svg_ancestor_and_html_is_not_a_descendant_match():
    # <g class=hot> > foreignObject > div > span: `.hot > span` has foreignObject and div in between
    assert colors([("hot", WORD)], ".hot > span{color:#f00}") == [BLACK]


# --- the HTML tree inside --------------------------------------------------------------------------

def test_descendant_selector_inside_the_html():
    assert colors([("g", "<div><span><p>HHHH</p></span></div>")], "div span{color:#f00}") == [RED]


def test_child_combinator_inside_the_html_needs_a_direct_parent():
    css = "div > span{color:#f00}"
    assert colors([("g", "<div><span><p>HHHH</p></span></div>")], css) == [RED]
    assert colors([("g", "<div><b><span><p>HHHH</p></span></b></div>")], css) == [BLACK]


def test_tag_and_class_together():
    css = "span.label{color:#f00}"
    assert colors([("g", '<span class="a label"><p>HHHH</p></span>')], css) == [RED]
    assert colors([("g", '<div class="label"><p>HHHH</p></div>')], css) == [BLACK]


def test_colour_is_inherited_by_the_element_that_holds_the_text():
    # the rule hits the <span>; the glyphs are in the <p> inside it
    assert colors([("g", "<span><p>HHHH</p></span>")], "span{color:#f00}") == [RED]


def test_the_innermost_rule_wins_over_an_inherited_colour():
    css = "span{color:#f00} p{color:#00f}"
    assert colors([("g", "<span><p>HHHH</p></span>")], css) == [BLUE]


# --- the cascade ------------------------------------------------------------------------------------

def test_a_more_specific_rule_beats_a_later_less_specific_one():
    assert colors([("hot", WORD)], ".hot span{color:#f00} span{color:#00f}") == [RED]


def test_the_later_rule_wins_at_equal_specificity():
    assert colors([("g", WORD)], "span{color:#f00} span{color:#00f}") == [BLUE]


def test_inline_style_beats_a_stylesheet_rule():
    assert colors([("g", '<span style="color:#00f"><p>HHHH</p></span>')], "span{color:#f00}") == [BLUE]


def test_an_important_rule_beats_a_plain_inline_style():
    assert colors([("g", '<span style="color:#00f"><p>HHHH</p></span>')], "span{color:#f00 !important}") == [RED]


def test_an_important_inline_style_beats_an_important_rule():
    html = '<span style="color:#00f !important"><p>HHHH</p></span>'
    assert colors([("g", html)], "span{color:#f00 !important}") == [BLUE]


def test_the_default_colour_is_black():
    assert colors([("g", WORD)]) == [BLACK]


def test_colour_formats_come_from_the_css_parser_hsl_rgb_names():
    css = ".a span{color:hsl(0, 100%, 50%)} .b span{color:rgb(0,0,255)} .c span{color:lime}"
    got = colors([("a", WORD), ("b", WORD), ("c", WORD)], css)
    assert got[0] == RED and got[1] == BLUE and got[2] == (0, 255, 0)


# --- the box behind the text ------------------------------------------------------------------------

def yellow_pixels(pixels, row):
    region = pixels[row * ROW:(row + 1) * ROW].reshape(-1, 4)
    return int(sum(1 for px in region if tuple(px[:4]) == (255, 255, 0, 255)))


def test_a_background_color_reaches_the_label_through_an_svg_ancestor():
    css = ".hot div{background-color:#ff0}"
    pixels = render(svg([("hot", WORD), ("cold", WORD)], css))
    assert yellow_pixels(pixels, 0) > 500       # a box was painted behind the first label
    assert yellow_pixels(pixels, 1) == 0        # and none behind the second


def test_a_transparent_background_paints_nothing():
    pixels = render(svg([("hot", WORD)], ".hot div{background-color:transparent}"))
    assert yellow_pixels(pixels, 0) == 0
    assert text_color(pixels, 0) == BLACK


# --- stylesheets applied after loading --------------------------------------------------------------

def test_a_stylesheet_applied_later_wins_over_the_one_in_the_file():
    doc = novasvg.Document.load_from_data(svg([("hot", WORD)], ".hot span{color:#00f}"))
    doc.apply_stylesheet(".hot span{color:#f00}")
    assert text_color(render(doc), 0) == RED


def test_the_rules_in_the_file_still_apply_next_to_a_later_stylesheet():
    doc = novasvg.Document.load_from_data(svg([("hot", WORD)], ".hot span{color:#00f}"))
    doc.apply_stylesheet(".hot div{background-color:#ff0}")
    pixels = render(doc)
    assert text_color(pixels, 0, box=YELLOW) == BLUE and yellow_pixels(pixels, 0) > 500
