"""CSS `mix-blend-mode`: how an element's group is blended into whatever is already painted under it.

Mermaid's sankey diagram paints its labels first and its (half transparent) links afterwards with
`mix-blend-mode: multiply`. Multiply keeps a dark label dark; ordinary alpha blending washes it out.

The expected numbers come from the W3C Compositing and Blending formulas, on 8-bit channels:
    result = (1 - ab) * Cs + ab * B(Cb, Cs)          (then source-over with the source's alpha)
with Cb the backdrop colour, Cs the source colour and B the blend function of the mode.
"""
import pytest

import novasvg

BACKDROP = "#ff8000"   # (255, 128, 0)
SOURCE = "#808080"     # (128, 128, 128)


def svg(body, css="", size=20):
    sheet = f"<style>{css}</style>" if css else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}">{sheet}{body}</svg>'


def pixel(text, x=10, y=10):
    bitmap = novasvg.Document.load_from_data(text).render_to_bitmap(-1, -1, 0)  # transparent background
    bitmap.convert_to_rgba()
    return tuple(int(v) for v in bitmap.numpy()[y, x])


def close(actual, expected, tolerance=2):
    return len(actual) == len(expected) and all(abs(a - e) <= tolerance for a, e in zip(actual, expected))


def over_backdrop(source_attrs, backdrop=BACKDROP):
    return svg(f'<rect width="20" height="20" fill="{backdrop}"/>'
               f'<rect width="20" height="20" fill="{SOURCE}" {source_attrs}/>')


# --- the blend functions ----------------------------------------------------------------------------

# The W3C blend functions on 0..1 channel values (separable modes), written independently of the library.
BLEND_FUNCTIONS = {
    "multiply": lambda cb, cs: cb * cs,
    "screen": lambda cb, cs: cb + cs - cb * cs,
    "darken": lambda cb, cs: min(cb, cs),
    "lighten": lambda cb, cs: max(cb, cs),
    "difference": lambda cb, cs: abs(cb - cs),
    "exclusion": lambda cb, cs: cb + cs - 2 * cb * cs,
}
MODE_BACKDROP, MODE_SOURCE = (255, 128, 0), (192, 96, 64)


def expected_blend(mode):
    return tuple(round(BLEND_FUNCTIONS[mode](b / 255, s / 255) * 255) for b, s in zip(MODE_BACKDROP, MODE_SOURCE))


@pytest.mark.parametrize("mode", sorted(BLEND_FUNCTIONS))
def test_each_blend_mode_over_an_opaque_backdrop(mode):
    expected = expected_blend(mode)
    # the test only means something if the mode's result is visibly not just the source painted over
    assert max(abs(e - s) for e, s in zip(expected, MODE_SOURCE)) > 20, "pick colours that tell the mode apart"
    body = ('<rect width="20" height="20" fill="rgb(255,128,0)"/>'
            f'<rect width="20" height="20" fill="rgb{MODE_SOURCE}" style="mix-blend-mode:{mode}"/>')
    got = pixel(svg(body))
    assert close(got[:3], expected) and got[3] == 255, (mode, got, expected)


def test_normal_is_ordinary_painting():
    assert pixel(over_backdrop('style="mix-blend-mode:normal"')) == (128, 128, 128, 255)


def test_no_mix_blend_mode_is_normal_too():
    assert pixel(over_backdrop("")) == (128, 128, 128, 255)


def test_a_mode_given_in_capitals_is_the_same_mode():
    assert close(pixel(over_backdrop('style="mix-blend-mode: MULTIPLY"'))[:3], (128, 64, 0))


def test_an_unknown_value_falls_back_to_normal():
    assert pixel(over_backdrop('style="mix-blend-mode:no-such-mode"')) == (128, 128, 128, 255)


def test_a_mode_from_a_stylesheet_rule():
    body = (f'<rect width="20" height="20" fill="{BACKDROP}"/>'
            f'<rect class="m" width="20" height="20" fill="{SOURCE}"/>')
    assert close(pixel(svg(body, ".m{mix-blend-mode:multiply}"))[:3], (128, 64, 0))


# --- backdrop alpha and source alpha ----------------------------------------------------------------

def test_over_a_transparent_backdrop_the_source_keeps_its_own_colour():
    # nothing underneath to multiply with, so multiply must not darken (or hide) the source
    body = f'<rect width="20" height="20" fill="{SOURCE}" style="mix-blend-mode:multiply"/>'
    assert pixel(svg(body)) == (128, 128, 128, 255)


def test_a_half_transparent_source_blends_half_way():
    # (1 - as) * Cb + as * multiply(Cb, Cs)   with as = 0.5  ->  (191.5, 96, 0)
    got = pixel(over_backdrop('fill-opacity="0.5" style="mix-blend-mode:multiply"'))
    assert close(got[:3], (192, 96, 0)) and got[3] == 255, got


def test_a_stroke_with_stroke_opacity_blends_the_same_way():
    # mermaid's sankey links: fill none, stroke-opacity 0.5
    body = (f'<rect width="20" height="20" fill="{BACKDROP}"/>'
            f'<path d="M0 10 L20 10" fill="none" stroke="{SOURCE}" stroke-width="20" stroke-opacity="0.5" '
            f'style="mix-blend-mode:multiply"/>')
    assert close(pixel(svg(body))[:3], (192, 96, 0))


def test_dark_text_under_a_half_transparent_multiply_layer_stays_dark():
    # the sankey bug: a black label painted first, an orange link multiplied over it afterwards
    label = '<rect width="20" height="20" fill="#000000"/>'
    link = '<rect width="20" height="20" fill="#f28e2c" fill-opacity="0.5" style="mix-blend-mode:multiply"/>'
    assert pixel(svg(label + link)) == (0, 0, 0, 255)
    # while plain alpha blending lightens it -- which is why the mode matters
    plain = '<rect width="20" height="20" fill="#f28e2c" fill-opacity="0.5"/>'
    assert pixel(svg(label + plain))[0] > 100


def test_the_opacity_of_the_element_scales_the_blended_result():
    got = pixel(over_backdrop('opacity="0.5" style="mix-blend-mode:multiply"'))
    assert close(got[:3], (192, 96, 0)), got


# --- groups -----------------------------------------------------------------------------------------

def test_a_group_is_blended_as_one_unit_not_child_by_child():
    # inside the group the blue square simply covers the red one; only the finished group multiplies
    # with the (white) backdrop -- child by child, red x blue would give black where they overlap
    body = ('<rect width="20" height="20" fill="#ffffff"/>'
            '<g style="mix-blend-mode:multiply">'
            '<rect x="0" y="0" width="14" height="20" fill="#ff0000"/>'
            '<rect x="6" y="0" width="14" height="20" fill="#0000ff"/></g>')
    assert pixel(svg(body), 10, 10) == (0, 0, 255, 255)   # overlap: blue
    assert pixel(svg(body), 2, 10) == (255, 0, 0, 255)    # red only


def test_the_property_is_not_inherited_so_a_group_member_is_not_blended_twice():
    body = (f'<rect width="20" height="20" fill="{BACKDROP}"/>'
            f'<g style="mix-blend-mode:multiply"><rect width="20" height="20" fill="{SOURCE}"/></g>')
    assert close(pixel(svg(body))[:3], (128, 64, 0))  # once: a second multiply would give (64, 32, 0)


def test_blending_only_touches_where_the_element_paints():
    body = (f'<rect width="20" height="20" fill="{BACKDROP}"/>'
            f'<rect x="10" width="10" height="20" fill="{SOURCE}" style="mix-blend-mode:multiply"/>')
    assert pixel(svg(body), 3, 10) == (255, 128, 0, 255)           # left half untouched
    assert close(pixel(svg(body), 15, 10)[:3], (128, 64, 0))       # right half multiplied
