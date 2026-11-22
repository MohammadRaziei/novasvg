#!/usr/bin/env python3
"""Regenerate data/fonts/embedded/*.ttf.z, the fallback fonts novasvg compiles in.

These are Latin subsets of DejaVu Sans (regular, bold, oblique and bold oblique), of DejaVu Sans Mono and of DejaVu
Serif (regular and bold each), used only when the machine has no font for a request (see font_face_cache), which
picks the face matching the requested weight and style. Each file is the subset zlib-compressed at level 9 (what
stb::zlib_decompress reads); cmake/EmbedFile.cmake turns the files into a header at configure time, so the build
needs no tool. Kerning (GPOS and the legacy kern table) and the ligature substitutions are kept so that text lays
out exactly like the full font; test_novasvg_embedded_font checks that. The output must not depend on the clock or
the machine's zlib, so the .z files are committed, not rebuilt.

The DejaVu Sans faces come from data/fonts (the tests use them too). The Mono and Serif faces are not kept in the
repository (only their 12 to 17 KB subsets are): download DejaVu (the committed subsets are from 2.37,
https://dejavu-fonts.github.io) and give the directory that holds DejaVuSansMono[-Bold].ttf and
DejaVuSerif[-Bold].ttf as the argument.

    python tools/gen_default_font.py /path/to/dejavu-fonts-ttf-2.37/ttf     # needs: pip install fonttools
"""
import io
import sys
import zlib
from pathlib import Path

from fontTools import subset
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
FONTS = ROOT / "data" / "fonts"
# Sans for everything, Mono for `monospace` and Serif for `serif` (regular and bold each; no oblique). Mono and Serif
# are DejaVu 2.37, Sans 2.35.
FACES = ["DejaVuSans.ttf", "DejaVuSans-Bold.ttf", "DejaVuSans-Oblique.ttf", "DejaVuSans-BoldOblique.ttf",
         "DejaVuSansMono.ttf", "DejaVuSansMono-Bold.ttf", "DejaVuSerif.ttf", "DejaVuSerif-Bold.ttf"]
TARGET = FONTS / "embedded"

# Basic Latin, Latin-1 Supplement, the punctuation/symbols text commonly carries, and the f-ligature
# presentation forms U+FB00..FB04 (novasvg applies the font's fi/fl/ff/ffi/ffl ligatures through them).
CODEPOINTS = (
    list(range(0x20, 0x7F))
    + list(range(0xA0, 0x100))
    + [0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x20AC, 0x2122]
    + list(range(0xFB00, 0xFB05))
)
FEATURES = ["kern", "liga", "calt", "ccmp", "locl", "mark", "mkmk", "clig"]


def subset_font(source):
    options = subset.Options()
    options.layout_features = FEATURES
    options.legacy_kern = True
    options.hinting = False
    options.notdef_outline = True
    options.name_IDs = ["*"]
    font = TTFont(str(source), recalcTimestamp=False)  # keep head.modified: the output must not depend on the clock
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=CODEPOINTS)
    subsetter.subset(font)
    out = io.BytesIO()
    font.save(out)
    return out.getvalue()


if __name__ == "__main__":
    extra = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    sources = {name: next((d / name for d in (FONTS, extra) if d and (d / name).is_file()), None) for name in FACES}
    missing = [name for name, path in sources.items() if path is None]
    if missing:
        sys.exit("not found in data/fonts or in the directory given as argument: " + ", ".join(missing))
    TARGET.mkdir(exist_ok=True)
    for name, path in sources.items():
        data = subset_font(path)
        packed = zlib.compress(data, 9)
        (TARGET / (name + ".z")).write_bytes(packed)
        print("wrote %s (%d bytes, from %d of TrueType)" % ((TARGET / (name + ".z")).relative_to(ROOT), len(packed), len(data)))
