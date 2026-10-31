#!/usr/bin/env python3
"""Pixel-level regression check for the novasvg refactor.

  tools/golden.py render <novasvg-binary> <out-dir>   render every data/**/*.svg to PNG
  tools/golden.py diff <dir-a> <dir-b> [--tol N]      compare two render dirs pixel by pixel

PNGs are decoded before comparing, so a different PNG encoder (e.g. libstb's instead of the
vendored stb_image_write) does not count as a difference; only pixels do.
"""
import subprocess, sys, pathlib
from PIL import Image, ImageChops

ROOT = pathlib.Path(__file__).resolve().parent.parent

def render(binary, out):
    out = pathlib.Path(out); out.mkdir(parents=True, exist_ok=True)
    svgs = sorted((ROOT / "data").rglob("*.svg"))
    ok = bad = 0
    for svg in svgs:
        name = svg.relative_to(ROOT / "data").as_posix().replace("/", "__")
        dst = out / (name[:-4] + ".png")
        r = subprocess.run([binary, str(svg), "-o", str(dst)], capture_output=True, text=True)
        if r.returncode == 0 and dst.exists(): ok += 1
        else: bad += 1; print("FAILED", svg.name, r.stderr.strip()[:120])
    print(f"rendered {ok}, failed {bad} -> {out}")
    return bad

def diff(a, b, tol=0):
    a, b = pathlib.Path(a), pathlib.Path(b)
    names = sorted({p.name for p in a.glob("*.png")} | {p.name for p in b.glob("*.png")})
    worst = 0; fails = 0
    for n in names:
        pa, pb = a / n, b / n
        if not pa.exists() or not pb.exists():
            print("MISSING", n, "(only in " + ("a" if pa.exists() else "b") + ")"); fails += 1; continue
        ia, ib = Image.open(pa).convert("RGBA"), Image.open(pb).convert("RGBA")
        if ia.size != ib.size:
            print("SIZE", n, ia.size, ib.size); fails += 1; continue
        d = max(ImageChops.difference(ia, ib).getextrema(), key=lambda e: e[1])[1]
        worst = max(worst, d)
        if d > tol: print(f"DIFF {n}: max channel delta {d}"); fails += 1
    print(f"{len(names)} images, {fails} differ (tol={tol}, worst delta={worst})")
    return 1 if fails else 0

if __name__ == "__main__":
    if len(sys.argv) >= 4 and sys.argv[1] == "render": sys.exit(render(sys.argv[2], sys.argv[3]))
    if len(sys.argv) >= 4 and sys.argv[1] == "diff":
        tol = int(sys.argv[sys.argv.index("--tol") + 1]) if "--tol" in sys.argv else 0
        sys.exit(diff(sys.argv[2], sys.argv[3], tol))
    print(__doc__); sys.exit(2)
