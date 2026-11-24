"""A group with `opacity` that holds only a line running left or up must still be drawn (it is mermaid's gantt grid:
`<g class="tick" opacity="..." transform="translate(50.5,0)"><line y2="-111"/></g>`)."""
import numpy as np
import pytest

import novasvg


def _alpha_pixels(inner):
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="80" height="80" viewBox="0 0 80 80">{inner}</svg>'
    arr = novasvg.Document.load_from_data(svg).render_to_bitmap(backgroundColor=0).numpy()
    return int((arr[:, :, 3] > 0).sum())


LINE = 'stroke="black"'


@pytest.mark.parametrize("inner", [
    f'<g transform="translate(30.5,60)" opacity="0.8"><line y2="-55" {LINE}/></g>',        # runs up
    f'<g transform="translate(60,30.5)" opacity="0.8"><line x2="-55" {LINE}/></g>',        # runs left
    f'<g opacity="0.8"><line x1="30.5" x2="30.5" y1="60" y2="5" {LINE}/></g>',              # y1 > y2, no transform
    f'<g transform="translate(30.5,60)" opacity="0.8"><line y2="-55" {LINE} stroke-width="2"/></g>',
])
def test_group_opacity_draws_a_line_that_runs_up_or_left(inner):
    assert _alpha_pixels(inner) > 0


def test_a_line_running_down_is_unchanged():
    assert _alpha_pixels(f'<g transform="translate(30.5,5)" opacity="0.8"><line y2="55" {LINE}/></g>') == 55
