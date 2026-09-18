"""Uniform wrapper around resvg, lunasvg, cairosvg and thorvg.

Every engine exposes the same shape: `render(svg_path, w, h) -> bytes` (raw
PNG bytes) or raises. That's the only contract run_benchmark.py depends on
for these 4 -- ponytail: adding a 6th Python-bound engine later means
adding one more Engine() entry below, not touching the driver.

novasvg itself is NOT here: it's driven directly through its C++ API by
native/novasvg_native_bench.cpp (see native_novasvg.py), not a Python
binding, so its whole corpus pass happens as one subprocess call handled
separately in run_benchmark.py.
"""
from dataclasses import dataclass
from typing import Callable
import io


@dataclass
class Engine:
    key: str
    label: str
    render: Callable[[str, int, int], bytes]
    version: Callable[[], str]


def _resvg():
    import resvg_py

    def render(svg_path, w, h):
        return bytes(resvg_py.svg_to_bytes(svg_path=svg_path, width=w, height=h))

    return Engine("resvg", "resvg", render, lambda: resvg_py.__resvg_version__)


def _lunasvg():
    import pylunasvg

    def render(svg_path, w, h):
        doc = pylunasvg.Document.loadFromFile(str(svg_path))
        if doc is None:
            raise RuntimeError("pylunasvg: failed to parse document")
        bmp = doc.renderToBitmap(w, h)
        # pylunasvg's Bitmap only writes to a path; use a temp file.
        import tempfile, os
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            tmp_path = tmp.name
        try:
            bmp.writeToPng(tmp_path)
            with open(tmp_path, "rb") as f:
                return f.read()
        finally:
            os.unlink(tmp_path)

    return Engine("lunasvg", "lunasvg", render, lambda: pylunasvg.lunasvg_version_string())


def _cairosvg():
    import cairosvg

    def render(svg_path, w, h):
        return cairosvg.svg2png(url=str(svg_path), output_width=w, output_height=h)

    return Engine("cairosvg", "cairosvg", render, lambda: cairosvg.__version__)


def _thorvg():
    import thorvg_python as thorvg

    def render(svg_path, w, h):
        engine = thorvg.Engine()
        canvas = thorvg.SwCanvas(engine)
        canvas.set_target(w, h)
        picture = thorvg.Picture(engine)
        res = picture.load(str(svg_path))
        if res != thorvg.Result.SUCCESS:
            raise RuntimeError(f"thorvg: load failed ({res})")
        picture.set_size(w, h)
        canvas.add(picture)
        canvas.draw(True)
        canvas.sync()
        img = canvas.get_pillow()
        buf = io.BytesIO()
        img.save(buf, format="PNG")
        return buf.getvalue()

    return Engine("thorvg", "thorvg", render, lambda: "1.1.3")


# Built lazily / defensively: an engine whose import fails is dropped from
# the run entirely (recorded as unavailable in the report) rather than
# aborting the whole benchmark.
_PY_BUILDERS = [_resvg, _lunasvg, _cairosvg, _thorvg]


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
