"""Chromium (via Playwright) as visual ground truth -- not a "competing"
renderer, the reference every other engine's output gets checked against.
Same approach as ../../playwright_render.py (this repo's own SVG-to-PNG
script): navigate straight to the SVG as a document and screenshot it,
Chrome's own SVG engine doing the rendering. Forces each render to the
exact width/height corpus.py's fit_box() computed, matching every other
engine's output geometry so the gallery is a fair side-by-side.

Timing is recorded but not meant to be compared against the other engines:
this pays full browser page-navigation overhead per sample, not a
library call, and one Chromium instance covers the whole corpus so even
that overhead is shared, not the point of this column.
"""
import time
from pathlib import Path


def render_ground_truth(jobs, timeout_ms=30000):
    """jobs: list of (name, svg_path, out_png_path, width, height).
    Returns (version_string, {name: {"ok": bool, "seconds": float|None,
    "error": str|None}})."""
    from playwright.sync_api import sync_playwright

    results = {}
    version = "unknown"
    with sync_playwright() as p:
        browser = p.chromium.launch()
        version = f"Chromium {browser.version}"
        page = browser.new_page()
        for name, svg_path, out_path, w, h in jobs:
            try:
                t0 = time.perf_counter()
                page.set_viewport_size({"width": w, "height": h})
                page.goto(Path(svg_path).resolve().as_uri())
                # A raw .svg navigation has no <body> -- the <svg> root IS
                # documentElement. Force it to our target size so every
                # engine's output lands at the same pixel dimensions.
                page.evaluate(
                    """([w, h]) => {
                        const el = document.documentElement;
                        if (el && el.tagName.toLowerCase() === 'svg') {
                            el.style.width = w + 'px';
                            el.style.height = h + 'px';
                        }
                    }""",
                    [w, h],
                )
                page.screenshot(path=out_path, omit_background=True, timeout=timeout_ms)
                results[name] = {"ok": True, "seconds": time.perf_counter() - t0, "error": None}
            except Exception as exc:  # noqa: BLE001 - recorded per-cell, run must continue
                results[name] = {"ok": False, "seconds": None, "error": f"{type(exc).__name__}: {exc}"}
        browser.close()
    return version, results
