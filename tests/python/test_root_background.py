"""A root `background-color` in the <svg>'s own style versus render_to_bitmap's `background_color` argument.

Mermaid writes `style="...; background-color: transparent;"` on its root <svg>. That used to clear the bitmap to
transparent *after* the caller's background was applied, so `background_color=white` was silently ignored.
"""
import novasvg

WHITE, RED = (novasvg.Color.from_name(n).to_int() for n in ("white", "red"))


def _corner(root_style, background):
    style = f' style="{root_style}"' if root_style else ""
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="40" height="40"{style}><circle cx="20" cy="20" r="8"/></svg>'
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, background)
    bitmap.convert_to_rgba()
    return tuple(int(v) for v in bitmap.numpy()[0, 0])


def test_transparent_root_background_keeps_the_callers_background():
    assert _corner("background-color: transparent", WHITE) == (255, 255, 255, 255)


def test_transparent_root_background_without_a_caller_background_stays_transparent():
    assert _corner("background-color: transparent", 0)[3] == 0


def test_opaque_root_background_still_wins():
    assert _corner("background-color: red", WHITE) == (255, 0, 0, 255)


def test_no_root_background_uses_the_callers_background():
    assert _corner("", RED) == (255, 0, 0, 255)
