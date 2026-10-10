#!/usr/bin/env python3
"""Regenerates docs/showcase/*: the images used on the docs landing page.

Every image here is produced by NovaSVG itself (the Python binding), so the
landing page shows real renderer output instead of mock-ups.

Usage: make_showcase.py [--data ../data] [--out showcase]
Needs: the novasvg Python module on PYTHONPATH, plus Pillow and numpy.
"""
import argparse
import math
from pathlib import Path

import novasvg
from PIL import Image

# ── the hero star -- the exact source shown in the landing page's code card ────
def star_path(cx=120, cy=120, outer=84, inner=34, points=8):
    pts = []
    for i in range(points * 2):
        r = outer if i % 2 == 0 else inner
        a = -math.pi / 2 + i * math.pi / points
        pts.append((round(cx + r * math.cos(a)), round(cy + r * math.sin(a))))
    return "M" + " L".join(f"{x} {y}" for x, y in pts) + "Z"


HERO_SVG = f'''<svg viewBox="0 0 240 240" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="nova" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#1867b2"/>
      <stop offset="1" stop-color="#b6489b"/>
    </linearGradient>
    <filter id="glow"><feGaussianBlur stdDeviation="9"/></filter>
  </defs>
  <circle class="orbit" cx="120" cy="120" r="108" fill="none" stroke="url(#nova)"
          stroke-width="2" stroke-dasharray="2 7" stroke-linecap="round"/>
  <circle cx="120" cy="120" r="58" fill="url(#nova)" opacity=".55" filter="url(#glow)"/>
  <path class="star" fill="url(#nova)" d="{star_path()}"/>
  <circle cx="120" cy="120" r="9" fill="#fff" opacity=".92"/>
</svg>
'''
HERO_STYLESHEET = ".star { fill: #ffb02e } .orbit { stroke: #ffb02e }"

# (output name, source svg relative to data/, render width)
GALLERY = [
    ("tiger",          "tiger.svg",                         760),
    ("gradient-filter", "feature-gradient-filter.svg",      600),
    ("sankey",         "mermaid/25-sankey.mmdc.svg",        900),
    ("venn",           "mermaid/01-venn.mmdc.svg",          800),
    ("architecture",   "mermaid/30-architecture.mmdc.svg",  640),
    ("pie",            "mermaid/15-pie-chart.mmdc.svg",     720),
]


def save(bitmap, path):
    bitmap.convert_to_rgba()  # NovaSVG's native ARGB32 premultiplied -> straight RGBA
    img = Image.fromarray(bitmap.numpy())
    img.save(path, "WEBP", quality=90, method=6)
    print(f"  {path.name:28s} {img.width}x{img.height}  {path.stat().st_size // 1024} KB")


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default=str(here.parent / "data"))
    ap.add_argument("--out", default=str(here / "showcase"))
    a = ap.parse_args()
    data, out = Path(a.data), Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    print(f"NovaSVG {novasvg.__version__} -> {out}")

    (out / "hero-star.svg").write_text(HERO_SVG)
    doc = novasvg.Document.load_from_data(HERO_SVG)
    save(doc.render_to_bitmap(720, 720, 0), out / "hero-star.webp")
    doc.apply_stylesheet(HERO_STYLESHEET)
    save(doc.render_to_bitmap(720, 720, 0), out / "hero-star-styled.webp")

    for name, rel, width in GALLERY:
        d = novasvg.Document.load_from_file(str(data / rel))
        h = max(1, round(width * d.height / d.width))
        save(d.render_to_bitmap(width, h, 0), out / f"{name}.webp")


if __name__ == "__main__":
    main()
