"""<switch>: render only the first direct child whose conditional-processing attributes pass."""
import novasvg

RED, BLUE = (255, 0, 0), (0, 0, 255)
SVG_OPEN = '<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20">'


def _render(inner):
    svg = SVG_OPEN + inner + "</svg>"
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)  # 0 = transparent background
    bitmap.convert_to_rgba()  # numpy() is BGRA until converted
    return bitmap.numpy()


def _colors_at_center(inner):
    r, g, b, a = (int(v) for v in _render(inner)[10, 10])
    return (r, g, b) if a else None


def _attr(name, value):
    return f'{name}="{value}"'


def _switch(*children):
    return "<switch>" + "".join(children) + "</switch>"


def rect(color, extra="", size=20):
    return f'<rect width="{size}" height="{size}" fill="rgb{color}" {extra}/>'


def test_first_child_without_conditions_wins():
    assert _colors_at_center(_switch(rect(RED), rect(BLUE))) == RED


def test_children_after_the_selected_one_are_not_drawn():
    # a later child that would paint over the first must NOT be painted
    assert _colors_at_center(_switch(rect(RED), rect(BLUE))) != BLUE


def test_required_extensions_xhtml_is_supported():
    ext = _attr("requiredExtensions", "http://www.w3.org/1999/xhtml")
    assert _colors_at_center(_switch(rect(RED, ext), rect(BLUE))) == RED


def test_unknown_required_extension_falls_through():
    ext = _attr("requiredExtensions", "urn:unsupported")
    assert _colors_at_center(_switch(rect(RED, ext), rect(BLUE))) == BLUE


def test_empty_required_extensions_is_false():
    ext = _attr("requiredExtensions", "")
    assert _colors_at_center(_switch(rect(RED, ext), rect(BLUE))) == BLUE


def test_system_language_matches_en_and_subtags():
    ok = _attr("systemLanguage", "fa, en-US")
    no = _attr("systemLanguage", "fa")
    assert _colors_at_center(_switch(rect(RED, ok), rect(BLUE))) == RED
    assert _colors_at_center(_switch(rect(RED, no), rect(BLUE))) == BLUE


def test_non_graphics_children_are_skipped():
    inner = _switch("<title>t</title>", "<desc>d</desc>", rect(RED))
    assert _colors_at_center(inner) == RED


def test_no_passing_child_draws_nothing():
    no = _attr("systemLanguage", "fa")
    assert _colors_at_center(_switch(rect(RED, no))) is None


def test_switch_inside_group_with_transform():
    inner = '<g transform="translate(5 5)">' + _switch(rect(RED, size=5)) + "</g>"
    arr = _render(inner)
    assert tuple(int(v) for v in arr[7, 7][:3]) == RED and arr[2, 2][3] == 0
