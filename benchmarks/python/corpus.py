"""SVG corpus for the benchmark. Every file lives in ../../data (novasvg's own
sample/feature set) -- reused as-is rather than inventing a parallel set of
test files. ponytail: if this benchmark is ever copied out of the novasvg
tree to run standalone, point DATA_DIR at a fetched copy of data/ instead.
"""
from pathlib import Path
import re

DATA_DIR = Path(__file__).resolve().parents[2] / "data"

# (name, filename, one-line description of what it stresses)
CORPUS = [
    ("circle", "circle.svg", "plain shape"),
    ("rect", "rect.svg", "plain shape"),
    ("tiger", "tiger.svg", "complex path-heavy vector art"),
    ("nova-logo", "nova.svg", "logo, mixed shapes/gradients"),
    ("clip-mask", "feature-clip-mask.svg", "clip-path + luminance mask"),
    ("gradient-filter", "feature-gradient-filter.svg", "gradients + SVG filter"),
    ("filter-primitives", "feature-filter-primitives.svg", "chained filter primitives (blur/offset/flood/composite)"),
    ("css-use-symbol", "feature-css-use-symbol.svg", "CSS transform + <use>/<symbol>"),
    ("base64-image", "feature-base64-image.svg", "embedded raster <image>"),
    ("mermaid-flowchart", "mmd-flowchart.svg", "<foreignObject> text, CSS classDef fills"),
    # --- text / font stress tests ---
    ("embedded-font", "feature-embedded-font.svg", "plain <text>, @font-face embedded TTF/OTF"),
    ("embedded-font-distinctive", "feature-embedded-font-distinctive.svg", "embedded monospace font vs. an unresolvable family, side by side"),
    ("mermaid-venn", "mermaid/01-venn-issue35.mmdc.svg", "plain shapes/text via <foreignObject> (mermaid venn diagram)"),
    ("mermaid-block", "mermaid/03-block-issue23.mmdc.svg", "<foreignObject> text + CSS classDef fills (mermaid block diagram)"),
]


def corpus_files():
    for name, filename, desc in CORPUS:
        path = DATA_DIR / filename
        if path.exists():
            yield name, path, desc


_NUM = r"[-+]?[0-9]*\.?[0-9]+"


def intrinsic_size(svg_path):
    """(width, height) from the <svg> tag's own width/height (numbers only,
    unit suffix stripped) or, failing that, its viewBox -- most sample SVGs
    here aren't square, so forcing a fixed square render distorts them.
    Falls back to (1, 1) if neither is parseable (caller should then just
    fit a square).
    """
    head = Path(svg_path).read_text(encoding="utf-8", errors="ignore")[:4000]
    tag_match = re.search(r"<svg\b[^>]*>", head)
    tag = tag_match.group(0) if tag_match else head

    w_match = re.search(rf'\bwidth="({_NUM})', tag)
    h_match = re.search(rf'\bheight="({_NUM})', tag)
    if w_match and h_match:
        w, h = float(w_match.group(1)), float(h_match.group(1))
        if w > 0 and h > 0:
            return w, h

    vb_match = re.search(rf'viewBox="\s*{_NUM}\s+{_NUM}\s+({_NUM})\s+({_NUM})', tag)
    if vb_match:
        w, h = float(vb_match.group(1)), float(vb_match.group(2))
        if w > 0 and h > 0:
            return w, h

    return 1.0, 1.0


def fit_box(intrinsic_w, intrinsic_h, max_w, max_h):
    """Largest (int, int) that preserves the intrinsic aspect ratio while
    fitting inside max_w x max_h -- same 'contain' logic as CSS
    object-fit: contain, just computed once so every engine renders the
    correctly-proportioned image instead of a square-stretched one."""
    scale = min(max_w / intrinsic_w, max_h / intrinsic_h)
    w = max(1, round(intrinsic_w * scale))
    h = max(1, round(intrinsic_h * scale))
    return w, h
