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
]

# Every mermaid sample under data/mermaid/: `NN-name.mmd` is the source and `NN-name.mmdc.svg` what
# mmdc (mermaid-cli, rendered in Chromium) made of it, numbered 01, 02, ... without gaps. Picked up by
# glob rather than listed here, so adding a pair of files is all it takes to benchmark it.
MERMAID_DIR = DATA_DIR / "mermaid"


def mermaid_diagram_type(mmd_path):
    """First keyword of a .mmd source (`flowchart`, `sequenceDiagram`, `xychart-beta`, ...), skipping
    blank lines, `%%` comments and a leading `---` front-matter block."""
    in_front_matter = False
    for line in Path(mmd_path).read_text(encoding="utf-8", errors="ignore").splitlines():
        line = line.strip()
        if line == "---":
            in_front_matter = not in_front_matter
            continue
        if in_front_matter or not line or line.startswith("%%"):
            continue
        return line.split()[0]
    return "unknown"


def mermaid_corpus():
    for svg in sorted(MERMAID_DIR.rglob("*.mmdc.svg")):
        stem = svg.name[: -len(".mmdc.svg")]
        name = f"mermaid-{stem}"
        source = svg.with_name(stem + ".mmd")
        kind = mermaid_diagram_type(source) if source.exists() else "unknown"
        yield name, svg, f"mermaid {kind} diagram (mmdc render)"


def corpus_files():
    for name, filename, desc in CORPUS:
        path = DATA_DIR / filename
        if path.exists():
            yield name, path, desc
    yield from mermaid_corpus()


_NUM = r"[-+]?[0-9]*\.?[0-9]+"


def _length(text):
    """'120' / '120px' / '3.5mm' -> 120.0 / 120.0 / 3.5 (unit stripped, like the rest of this file);
    None for a percentage or anything that isn't a plain length -- 100% is not 100 pixels."""
    match = re.fullmatch(rf"\s*({_NUM})([a-z]*)\s*", text or "")
    return float(match.group(1)) if match else None


def _pair(w_text, h_text):
    w, h = _length(w_text), _length(h_text)
    return (w, h) if w and h and w > 0 and h > 0 else None


def intrinsic_size(svg_path):
    """(width, height) the way a browser sizes a standalone SVG, most specific first: the root's CSS
    `style="width:..;height:.."`, then its width/height attributes, then its viewBox. Percentages are
    skipped (they mean "fill the container", not a size). Falls back to (1, 1) if nothing usable is
    declared (caller should then just fit a square). Most sample SVGs here aren't square, so forcing a
    fixed square render would distort them.
    """
    head = Path(svg_path).read_text(encoding="utf-8", errors="ignore")[:4000]
    tag_match = re.search(r"<svg\b[^>]*>", head)
    tag = tag_match.group(0) if tag_match else head

    def attr(name):
        m = re.search(rf'(?<![-\w:]){name}="([^"]*)"', tag)
        return m.group(1) if m else None

    def css(prop):  # `max-width` must not be read as `width`
        m = re.search(rf"(?<![-\w]){prop}\s*:\s*([^;]+)", attr("style") or "")
        return m.group(1) if m else None

    size = _pair(css("width"), css("height")) or _pair(attr("width"), attr("height"))
    if size:
        return size

    vb_match = re.search(rf'viewBox="\s*{_NUM}[\s,]+{_NUM}[\s,]+({_NUM})[\s,]+({_NUM})', tag)
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
