"""CSS custom properties (`--x`) and `var(--x, fallback)`.

Before, any `var()` made the whole property invalid, so `fill="var(--c)"` was painted black. That broke SVGs that
theme themselves with CSS variables -- pele's output, for one, is `--_fg:var(--pele-fg,#1f1f1f)` on the root and
`fill="var(--_fg)"` below it. The C++ side has the full matrix (tests/cpp/test_css_variables.cpp); this checks
that the Python binding sees the same thing.
"""
import novasvg

RED, GREEN, BLUE = (255, 0, 0), (0, 128, 0), (0, 0, 255)


def _svg(root_style, body, sheet=""):
    style = f"<style>{sheet}</style>" if sheet else ""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" style="{root_style}">'
            f"{style}{body}</svg>")


def _pixel(svg, x=10, y=10):
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0)
    bitmap.convert_to_rgba()
    r, g, b, a = (int(v) for v in bitmap.numpy()[y, x])
    return (r, g, b) if a else None


def rect(extra):
    return f'<rect width="20" height="20" {extra}/>'


def test_var_of_a_custom_property_on_the_root():
    assert _pixel(_svg("--c:red", rect('fill="var(--c)"'))) == RED


def test_var_in_a_style_attribute():
    assert _pixel(_svg("--c:red", rect('style="fill:var(--c)"'))) == RED


def test_fallback_when_the_property_does_not_exist():
    assert _pixel(_svg("", rect('fill="var(--nope, blue)"'))) == BLUE


def test_a_theme_chain_like_peles():
    body = rect('fill="var(--_fg)"')
    assert _pixel(_svg("--_fg:var(--pele-fg,#00f)", body)) == BLUE
    assert _pixel(_svg("--_fg:var(--pele-fg,#00f)", body, "svg{--pele-fg:red}")) == RED


def test_custom_properties_inherit_and_can_be_overridden():
    body = '<g style="--c:green">' + rect('fill="var(--c)"') + "</g>"
    assert _pixel(_svg("--c:red", body)) == GREEN


def test_a_stylesheet_applied_after_loading_changes_the_result():
    document = novasvg.Document.load_from_data(_svg("--c:red", rect('fill="var(--c)"')))
    document.apply_stylesheet(":root{--c:blue !important}")
    bitmap = document.render_to_bitmap(-1, -1, 0)
    bitmap.convert_to_rgba()
    r, g, b, _ = (int(v) for v in bitmap.numpy()[10, 10])
    assert (r, g, b) == BLUE
