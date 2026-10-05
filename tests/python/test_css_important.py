"""`!important` in a `style="..."` attribute, and where it lands in the cascade.

Mermaid writes a user's `style NodeId fill:#f9f,stroke:#333` as
`style="fill:#f9f !important;stroke:#333 !important;stroke-width:2px !important"`. Reading `#f9f !important`
as one colour value made those shapes come out black.

Cascade order (CSS): inline !important > stylesheet !important > inline > stylesheet.
"""
import novasvg

PINK, RED, GREEN, BLUE = (255, 153, 255), (255, 0, 0), (0, 255, 0), (0, 0, 255)


def _svg(body, style=""):
    sheet = f"<style>{style}</style>" if style else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20">{sheet}{body}</svg>'


def _pixel(svg, x=10, y=10):
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)  # transparent background
    bitmap.convert_to_rgba()
    r, g, b, a = (int(v) for v in bitmap.numpy()[y, x])
    return (r, g, b) if a else None


def rect(extra):
    return f'<rect width="20" height="20" {extra}/>'


def test_important_in_an_inline_style_is_not_part_of_the_value():
    assert _pixel(_svg(rect('style="fill:#f9f !important"'))) == PINK


def test_important_without_a_space_before_the_bang():
    assert _pixel(_svg(rect('style="fill:#f9f!important"'))) == PINK


def test_important_is_case_and_spacing_insensitive():
    assert _pixel(_svg(rect('style="fill:#f9f ! IMPORTANT"'))) == PINK


def test_every_declaration_of_a_style_attribute_keeps_working_when_all_are_important():
    # mermaid's exact shape: three important declarations in a row, the last one without a semicolon
    svg = _svg('<rect x="2" y="2" width="16" height="16" '
               'style="fill:#f9f !important;stroke:#00f !important;stroke-width:2px !important"/>')
    assert _pixel(svg, 10, 10) == PINK     # fill
    assert _pixel(svg, 2, 10) == BLUE      # the 2px stroke is centred on x=2: pixel column 2 is stroke


def test_mixed_important_and_plain_declarations():
    svg = _svg('<rect x="2" y="2" width="16" height="16" style="fill:#0f0;stroke:#00f !important;stroke-width:2px"/>')
    assert _pixel(svg, 10, 10) == GREEN
    assert _pixel(svg, 2, 10) == BLUE


def test_a_plain_inline_style_still_wins_over_a_plain_stylesheet_rule():
    assert _pixel(_svg(rect('style="fill:#f00"'), "rect{fill:#00f}")) == RED


def test_an_important_stylesheet_rule_beats_a_plain_inline_style():
    assert _pixel(_svg(rect('style="fill:#f00"'), "rect{fill:#00f !important}")) == BLUE


def test_an_important_inline_style_beats_an_important_stylesheet_rule():
    assert _pixel(_svg(rect('style="fill:#f00 !important"'), "rect{fill:#00f !important}")) == RED


def test_an_important_inline_style_beats_a_plain_stylesheet_rule():
    assert _pixel(_svg(rect('style="fill:#f00 !important"'), "rect{fill:#00f}")) == RED


def test_important_inline_beats_a_presentation_attribute():
    assert _pixel(_svg(rect('fill="#00f" style="fill:#f00 !important"'))) == RED


def test_a_later_important_declaration_in_the_same_style_wins():
    assert _pixel(_svg(rect('style="fill:#f00 !important;fill:#0f0 !important"'))) == GREEN


def test_setting_an_attribute_through_the_api_still_beats_everything_in_the_document():
    # a program that loads a document and then changes a colour means it, whatever CSS the file carried
    for style, sheet in [('style="fill:#f00 !important"', ""), ('style="fill:#f00"', "rect{fill:#0f0 !important}"),
                         ('style="fill:#f00 !important"', "rect{fill:#0f0 !important}")]:
        doc = novasvg.Document.load_from_data(_svg(rect(f'id="r" {style}'), sheet))
        doc.get_element_by_id("r").set_attribute("fill", "#00f")
        bitmap = doc.render_to_bitmap(-1, -1, 0)
        bitmap.convert_to_rgba()
        assert tuple(int(v) for v in bitmap.numpy()[10, 10][:3]) == BLUE, (style, sheet)
