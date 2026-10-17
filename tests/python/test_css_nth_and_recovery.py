"""`:nth-child()` / `:nth-of-type()` selectors, and CSS error recovery in a <style> sheet.

merman (mermaid-rs) emits rules such as `.swimlane-title path:nth-of-type(2){stroke:...}` *before* the
rules that paint every node. novasvg used to choke on the unsupported pseudo-class and then throw away
the rest of the stylesheet, so nodes lost their `fill:#fff` and inherited the dark root fill.
"""
import novasvg
import pytest

WHITE, RED, BLACK = (255, 255, 255), (255, 0, 0), (0, 0, 0)


def _pixel(svg, x, y):
    bitmap = novasvg.Document.load_from_data(svg).render_to_bitmap(-1, -1, 0xFFFFFFFF)
    bitmap.convert_to_rgba()
    r, g, b, a = (int(v) for v in bitmap.numpy()[y, x])
    return (r, g, b)


def _row(style, count=4):
    """`count` 10x10 rects side by side, in one <g>; rect i covers x in [10*i, 10*i+10)."""
    rects = "".join(f'<rect x="{10 * i}" y="0" width="10" height="10"/>' for i in range(count))
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{10 * count}" height="10">'
            f'<style>{style}</style><g>{rects}</g></svg>')


def _painted(style, count=4):
    svg = _row(style, count)
    return [_pixel(svg, 10 * i + 5, 5) == RED for i in range(count)]


@pytest.mark.parametrize("formula, expected", [
    ("2", [False, True, False, False]),
    ("odd", [True, False, True, False]),
    ("even", [False, True, False, True]),
    ("2n", [False, True, False, True]),
    ("2n+1", [True, False, True, False]),
    ("2n + 1", [True, False, True, False]),
    ("n+3", [False, False, True, True]),
    ("-n+2", [True, True, False, False]),
    ("n", [True, True, True, True]),
])
def test_nth_child_formulas(formula, expected):
    assert _painted(f"rect{{fill:#000}} rect:nth-child({formula}){{fill:#f00}}") == expected


def test_nth_last_child():
    assert _painted("rect{fill:#000} rect:nth-last-child(1){fill:#f00}") == [False, False, False, True]


def test_nth_of_type_only_counts_siblings_of_the_same_type():
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="30" height="10">'
           '<style>rect{fill:#000} rect:nth-of-type(2){fill:#f00}</style>'
           '<circle cx="5" cy="5" r="0"/>'
           '<rect x="0" y="0" width="10" height="10"/>'
           '<rect x="10" y="0" width="10" height="10"/>'
           '<rect x="20" y="0" width="10" height="10"/></svg>')
    assert [_pixel(svg, 10 * i + 5, 5) == RED for i in range(3)] == [False, True, False]


def test_nth_last_of_type():
    assert _painted("rect{fill:#000} rect:nth-last-of-type(2){fill:#f00}") == [False, False, True, False]


def test_rules_after_a_nth_selector_still_apply():
    """The merman regression: an `:nth-of-type(2)` rule must not drop the rules that follow it."""
    style = "#m{fill:#000}#m path:nth-of-type(2){stroke:#0f0}#m rect{fill:#fff}"
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" id="m" width="10" height="10">'
           f'<style>{style}</style><rect width="10" height="10"/></svg>')
    assert _pixel(svg, 5, 5) == WHITE


@pytest.mark.parametrize("bad_rule", [
    "rect:nth-child(){fill:#f00}",
    "rect:nth-child(foo){fill:#f00}",
    "rect:nth-child(2n+){fill:#f00}",
    "rect:unknown-fn(1,2){fill:#f00}",
    "rect::: {fill:#f00}",
    "rect:hover{fill:#f00}",
])
def test_an_invalid_rule_is_dropped_but_the_following_rules_survive(bad_rule):
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
           f'<style>rect{{fill:#000}} {bad_rule} rect{{fill:#fff}}</style>'
           '<rect width="10" height="10"/></svg>')
    assert _pixel(svg, 5, 5) == WHITE


def test_rules_before_the_invalid_one_are_kept():
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
           '<style>rect{fill:#f00} rect:nth-child(){fill:#000}</style>'
           '<rect width="10" height="10"/></svg>')
    assert _pixel(svg, 5, 5) == RED
