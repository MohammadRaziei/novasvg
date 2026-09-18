"""Uniform wrapper around each SVG renderer under test.

Every engine exposes the same shape: `render(svg_path, w, h) -> bytes` (raw
PNG bytes) or raises. That's the only contract run_benchmark.py depends on --
ponytail: adding a 6th engine later means adding one more Engine() entry
below, not touching the driver.
"""
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional
import io


@dataclass
class Engine:
    key: str
    label: str
    render: Callable[[str, int, int], bytes]
    version: Callable[[], str]


def _novasvg(cli_path):
    """Drives the real novasvg_cli binary (built from source via
    cmake/FetchNovasvg.cmake) as a subprocess, instead of novasvg's PyPI
    Python binding -- so this measures the actual C++ engine."""
    import subprocess

    if not cli_path or not Path(cli_path).is_file():
        raise RuntimeError(f"novasvg_cli not found at {cli_path!r}")

    def render(svg_path, w, h):
        import tempfile, os
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            tmp_path = tmp.name
        try:
            proc = subprocess.run(
                [cli_path, "convert", str(svg_path), "-o", tmp_path, "-w", str(w), "-H", str(h)],
                capture_output=True, text=True, timeout=30,
            )
            if proc.returncode != 0:
                raise RuntimeError(f"novasvg_cli exit {proc.returncode}: {proc.stderr.strip()[:300]}")
            with open(tmp_path, "rb") as f:
                return f.read()
        finally:
            os.unlink(tmp_path)

    def version():
        proc = subprocess.run([cli_path, "--version"], capture_output=True, text=True, timeout=10)
        return proc.stdout.strip() or proc.stderr.strip() or "unknown"

    return Engine("novasvg", "novasvg (CLI, built from source)", render, version)


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
        buf = io.BytesIO()
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


def load_engines(novasvg_cli_path=None):
    engines = {}
    errors = {}
    try:
        eng = _novasvg(novasvg_cli_path)
        engines[eng.key] = eng
    except Exception as exc:  # noqa: BLE001
        errors["novasvg"] = str(exc)
    for build in _PY_BUILDERS:
        try:
            eng = build()
            engines[eng.key] = eng
        except Exception as exc:  # noqa: BLE001 - intentionally broad, recorded not raised
            errors[build.__name__.strip("_")] = str(exc)
    return engines, errors
