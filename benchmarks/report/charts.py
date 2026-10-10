"""Shared benchmark-chart helpers.

Used by generate_report.py (the full report) and by docs/generate_index.py
(the teaser on the docs landing page), so both draw the same colourful charts
from the same numbers.

parse_report_html() reads the numbers back out of a generated report.html,
which is how the landing page gets them without needing results.json.
"""
import html
import re

# Engine key -> (label, colour). Colours are sampled from the NovaSVG logo ramp
# (blue #1867b2 -> violet #884d9e -> magenta #b6489b); novasvg itself is drawn
# with the logo gradient (class "nova") so it always stands out.
ENGINE_COLORS = {
    "novasvg":  "var(--chart-nova)",
    "resvg":    "#3f8fe0",
    "lunasvg":  "#6f7fe0",
    "cairosvg": "#9a6fd6",
    "thorvg":   "#c565c4",
    "nanosvg":  "#e8739f",
}
REFERENCE = "ground_truth"


def _ms(text):
    m = re.search(r"([\d.]+)\s*ms", text)
    return float(m.group(1)) if m else None


def parse_report_html(path):
    """Return {engine_key: {label, version, ok, total, avg_ms, rmse}} + meta, or None."""
    try:
        s = open(path, encoding="utf-8").read()
    except OSError:
        return None
    rows = re.findall(r"<tr><td>(.*?)</td><td>(.*?)</td><td>(\d+)/(\d+)</td><td>(.*?)</td><td>(.*?)</td></tr>", s)
    if not rows:
        return None
    out = {}
    for label, version, ok, total, avg, tot in rows:
        key = REFERENCE if "ground truth" in label else label.lower()
        out[key] = {"label": html.unescape(label), "version": html.unescape(version),
                    "ok": int(ok), "total": int(total), "avg_ms": _ms(avg), "rmse": None}
    # per-engine mean RMSE against the reference, from the gallery captions
    acc = {}
    for fig in re.findall(r"<figure>.*?</figure>", s, flags=re.S):
        m = re.search(r"<figcaption>(.*?)<br>.*?RMSE ([\d.]+)", fig, flags=re.S)
        if m:
            acc.setdefault(html.unescape(m.group(1)).lower(), []).append(float(m.group(2)))
    for key, vals in acc.items():
        if key in out and vals:
            out[key]["rmse"] = sum(vals) / len(vals)
    meta = {}
    m = re.search(r"(\d+) samples", s)
    meta["samples"] = int(m.group(1)) if m else None
    m = re.search(r"median of (\d+) runs", s)
    meta["runs"] = int(m.group(1)) if m else None
    m = re.search(r"Generated ([^.<]+?)\.", s)
    meta["generated"] = m.group(1).strip() if m else None
    return {"engines": out, "meta": meta}


def _bars(rows, unit, fmt, aria):
    """rows: [(key, label, value)] -> horizontal bar chart (pure HTML/CSS, no JS)."""
    rows = [r for r in rows if r[2] is not None]
    if not rows:
        return ""
    rows.sort(key=lambda r: r[2])
    top = max(v for _, _, v in rows) or 1
    items = []
    for i, (key, label, v) in enumerate(rows):
        pct = max(v / top * 100, 1.5)
        cls = "bar nova" if key == "novasvg" else "bar"
        color = ENGINE_COLORS.get(key, "var(--accent)")
        style = f"--w:{pct:.1f}%;--i:{i};" + ("" if key == "novasvg" else f"--c:{color};")
        items.append(
            f'<li class="bar-row{" is-nova" if key == "novasvg" else ""}">'
            f'<span class="bar-label">{html.escape(label)}</span>'
            f'<span class="bar-track"><span class="{cls}" style="{style}"></span></span>'
            f'<span class="bar-value">{fmt(v)}<small>{unit}</small></span></li>'
        )
    return f'<ul class="bars" role="img" aria-label="{html.escape(aria)}">{"".join(items)}</ul>'


def speed_chart(data):
    rows = [(k, v["label"], v["avg_ms"]) for k, v in data["engines"].items() if k != REFERENCE]
    return _bars(rows, " ms", lambda v: f"{v:.2f}", "Average render time per sample in milliseconds, lower is faster")


def fidelity_chart(data):
    rows = [(k, v["label"], v["rmse"]) for k, v in data["engines"].items() if k != REFERENCE]
    return _bars(rows, "", lambda v: f"{v:.1f}", "Mean RMSE against Chromium, lower is closer to the reference")


BARS_CSS = """
.bars { list-style: none; display: grid; gap: .7rem; }
.bar-row { display: grid; grid-template-columns: 5.2rem minmax(0, 1fr) 5.4rem; gap: .8rem; align-items: center; font-size: .85rem; }
.bar-label { color: var(--fg2); font-weight: 500; }
.bar-row.is-nova .bar-label, .bar-row.is-nova .bar-value { color: var(--fg); font-weight: 700; }
.bar-track { height: .9rem; border-radius: 99px; background: var(--bg3); overflow: hidden; }
.bar { display: block; height: 100%; width: var(--w); border-radius: 99px; background: var(--c, var(--accent)); transform-origin: left; }
.bar.nova { background: var(--chart-nova); box-shadow: 0 0 18px color-mix(in srgb, var(--accent3) 45%, transparent); }
.bar-value { text-align: right; font: 500 .82rem var(--font-mono); color: var(--fg2); }
.bar-value small { color: var(--fg3); font-size: .7rem; }
html.js .bars:not(.in) .bar { width: 0; }
html.js .bars .bar { transition: width .9s cubic-bezier(.2,.7,.2,1); transition-delay: calc(var(--i, 0) * 70ms); }
"""
