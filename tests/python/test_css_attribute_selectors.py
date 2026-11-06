"""CSS attribute selectors on attributes novasvg has no property for (`data-*`, `aria-*`, ...)."""
import io

import pytest
from PIL import Image

import novasvg

RED = (255, 0, 0)
WHITE = (255, 255, 255)


def _center_pixel(selector, group_open):
    svg = (
        '<svg xmlns="http://www.w3.org/2000/svg" width="40" height="40">'
        f"<style>{selector}{{fill:#f00}}</style>"
        f'{group_open}<rect width="40" height="40" fill="#fff"/></g></svg>'
    )
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap()
    return Image.open(io.BytesIO(bitmap.to_png())).convert("RGB").getpixel((20, 20))


@pytest.mark.parametrize(
    "selector, group",
    [
        (".a rect", '<g class="a">'),
        ("[data-k] rect", '<g data-k="1">'),
        ("[data-k] rect", '<g data-k="">'),
        ('[data-k="1"] rect', '<g data-k="1">'),
        ('.a[data-k="1"] rect', '<g class="a" data-k="1">'),
        ('[data-k="1"][data-j="2"] rect', '<g data-k="1" data-j="2">'),
        ('[data-k^="ab"] rect', '<g data-k="abc">'),
        ('[data-k$="bc"] rect', '<g data-k="abc">'),
        ('[data-k*="b"] rect', '<g data-k="abc">'),
        ('[data-k~="b"] rect', '<g data-k="a b c">'),
        ('[data-k|="a"] rect', '<g data-k="a-b">'),
        ('rect[data-k="1"]', '<g data-k="1">'),
    ],
)
def test_attribute_selector_matches(selector, group):
    if selector.startswith("rect["):
        # the attribute sits on the rect itself
        svg = (
            '<svg xmlns="http://www.w3.org/2000/svg" width="40" height="40">'
            f"<style>{selector}{{fill:#f00}}</style>"
            '<rect data-k="1" width="40" height="40" fill="#fff"/></svg>'
        )
        bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap()
        pixel = Image.open(io.BytesIO(bitmap.to_png())).convert("RGB").getpixel((20, 20))
    else:
        pixel = _center_pixel(selector, group)
    assert pixel == RED


@pytest.mark.parametrize(
    "selector, group",
    [
        ("[data-k] rect", "<g>"),
        ('[data-k="2"] rect', '<g data-k="1">'),
        ('[data-k="1"][data-j="2"] rect', '<g data-k="1">'),
        ('[data-k^="b"] rect', '<g data-k="abc">'),
    ],
)
def test_attribute_selector_does_not_match(selector, group):
    assert _center_pixel(selector, group) == WHITE


def test_mermaid_style_rule():
    selector = '#s [data-look="neo"][data-color-id="color-0"].node rect'
    svg = (
        '<svg xmlns="http://www.w3.org/2000/svg" id="s" width="40" height="40">'
        f"<style>{selector}{{fill:#f00}}</style>"
        '<g class="node" data-look="neo" data-color-id="color-0">'
        '<rect width="40" height="40" fill="#fff"/></g></svg>'
    )
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap()
    pixel = Image.open(io.BytesIO(bitmap.to_png())).convert("RGB").getpixel((20, 20))
    assert pixel == RED
