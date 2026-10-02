"""<switch>: render only the first direct child whose conditional-processing attributes pass."""
import novasvg

RED, BLUE = (255, 0, 0), (0, 0, 255)


def _colors_at_center(inner):
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20">{inner}</svg>'
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)  # 0 = transparent background
    bitmap.convert_to_rgba()  # numpy() is BGRA until converted
    r, g, b, a = (int(v) for v in bitmap.numpy()[10, 10])
    return (r, g, b) if a else None


def rect(color, extra=""):
    return f'<rect width="20" height="20" fill="rgb{color}" {extra}/>'


def test_first_child_without_conditions_wins():
    assert _colors_at_center(f"<switch>{rect(RED)}{rect(BLUE)}</switch>") == RED


def test_children_after_the_selected_one_are_not_drawn():
    # a later child that would paint over the first must NOT be painted
    assert _colors_at_center(f"<switch>{rect(RED)}{rect(BLUE)}</switch>") != BLUE


def test_required_extensions_xhtml_is_supported():
    ext = 'requiredExtensions="http://www.w3.org/1999/xhtml"'
    assert _colors_at_center(f"<switch>{rect(RED, ext)}{rect(BLUE)}</switch>") == RED


def test_unknown_required_extension_falls_through():
    assert _colors_at_center(f'<switch>{rect(RED, "requiredExtensions=\"urn:unsupported\"")}{rect(BLUE)}</switch>') == BLUE


def test_empty_required_extensions_is_false():
    assert _colors_at_center(f'<switch>{rect(RED, "requiredExtensions=\"\"")}{rect(BLUE)}</switch>') == BLUE


def test_system_language_matches_en_and_subtags():
    assert _colors_at_center(f'<switch>{rect(RED, "systemLanguage=\"fa, en-US\"")}{rect(BLUE)}</switch>') == RED
    assert _colors_at_center(f'<switch>{rect(RED, "systemLanguage=\"fa\"")}{rect(BLUE)}</switch>') == BLUE


def test_non_graphics_children_are_skipped():
    assert _colors_at_center(f"<switch><title>t</title><desc>d</desc>{rect(RED)}</switch>") == RED


def test_no_passing_child_draws_nothing():
    assert _colors_at_center(f'<switch>{rect(RED, "systemLanguage=\"fa\"")}</switch>') is None


def test_switch_inside_group_with_transform():
    inner = f'<g transform="translate(5 5)"><switch>{rect(RED).replace("width=\"20\" height=\"20\"", "width=\"5\" height=\"5\"")}</switch></g>'
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20">{inner}</svg>'
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)  # 0 = transparent background
    bitmap.convert_to_rgba()
    arr = bitmap.numpy()
    assert tuple(int(v) for v in arr[7, 7][:3]) == RED and arr[2, 2][3] == 0
