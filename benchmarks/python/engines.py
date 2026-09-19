"""Wrapper for cairosvg -- the one engine in this benchmark with no native
C/C++ library to link against (cairosvg IS Python: it wraps libcairo for
drawing primitives, but the SVG parsing/CSS/layout logic is pure Python).
Every other engine (novasvg, resvg, lunasvg, thorvg, nanosvg) is driven
directly through its C/C++ API by a native_*_bench binary -- see
native_bench.py and ../native/*.cpp.
"""
from dataclasses import dataclass
from typing import Callable


@dataclass
class Engine:
    key: str
    label: str
    render: Callable[[str, int, int], bytes]
    version: Callable[[], str]


def _cairosvg():
    import cairosvg

    def render(svg_path, w, h):
        return cairosvg.svg2png(url=str(svg_path), output_width=w, output_height=h)

    return Engine("cairosvg", "cairosvg", render, lambda: cairosvg.__version__)


_PY_BUILDERS = [_cairosvg]


def load_engines():
    engines = {}
    errors = {}
    for build in _PY_BUILDERS:
        try:
            eng = build()
            engines[eng.key] = eng
        except Exception as exc:  # noqa: BLE001 - intentionally broad, recorded not raised
            errors[build.__name__.strip("_")] = str(exc)
    return engines, errors
