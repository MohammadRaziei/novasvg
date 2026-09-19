#!/usr/bin/env python3
"""Turns results.json (from ../python/run_benchmark.py) into a single
self-contained report.html: capability+speed matrix, per-engine summary,
and a base64-embedded render gallery. No template engine -- plain f-strings
are enough for one page (ponytail: pulling in jinja2 for this would just be
YAGNI).

Usage: generate_report.py <results.json> <out.html>
"""
import html
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ENGINE_ORDER = ["novasvg", "resvg", "lunasvg", "cairosvg", "thorvg", "nanosvg"]


def fmt_ms(seconds):
    return f"{seconds * 1000:.2f} ms" if seconds is not None else "-"


def build_matrix_table(report):
    engines = [k for k in ENGINE_ORDER if k in report["engines"]]
    head = "".join(f"<th>{html.escape(report['engines'][k]['label'])}</th>" for k in engines)
    rows = []
    for f in report["files"]:
        name = f["name"]
        cells = []
        for k in engines:
            cell = report["matrix"][name].get(k, {"ok": False, "error": "not run"})
            if cell["ok"]:
                cells.append(f'<td class="ok">{fmt_ms(cell["seconds"])}</td>')
            else:
                err = html.escape((cell.get("error") or "fail")[:80])
                cells.append(f'<td class="fail" title="{err}">FAIL</td>')
        rows.append(
            f'<tr><td class="filename">{html.escape(name)}'
            f'<div class="desc">{html.escape(f["desc"])}</div></td>{"".join(cells)}</tr>'
        )
    return f"""
    <table class="matrix">
      <thead><tr><th>Sample</th>{head}</tr></thead>
      <tbody>{"".join(rows)}</tbody>
    </table>
    """


def build_summary_table(report):
    engines = [k for k in ENGINE_ORDER if k in report["engines"]]
    rows = []
    n_files = len(report["files"])
    for k in engines:
        times = [
            report["matrix"][f["name"]][k]["seconds"]
            for f in report["files"]
            if report["matrix"][f["name"]].get(k, {}).get("ok")
        ]
        n_ok = len(times)
        avg = sum(times) / n_ok if n_ok else None
        total = sum(times) if n_ok else None
        rows.append(
            f"<tr><td>{html.escape(report['engines'][k]['label'])}</td>"
            f"<td>{html.escape(report['engines'][k]['version'])}</td>"
            f"<td>{n_ok}/{n_files}</td>"
            f"<td>{fmt_ms(avg)}</td>"
            f"<td>{fmt_ms(total)}</td></tr>"
        )
    return f"""
    <table class="summary">
      <thead><tr><th>Engine</th><th>Version</th><th>Rendered OK</th><th>Avg time / sample</th><th>Total time</th></tr></thead>
      <tbody>{"".join(rows)}</tbody>
    </table>
    """


def build_gallery(report):
    engines = [k for k in ENGINE_ORDER if k in report["engines"]]
    blocks = []
    for f in report["files"]:
        name = f["name"]
        thumbs = []
        for k in engines:
            cell = report["matrix"][name].get(k, {})
            label = html.escape(report["engines"][k]["label"])
            if cell.get("ok") and cell.get("png_b64"):
                img = (
                    f'<img src="data:image/png;base64,{cell["png_b64"]}" '
                    f'alt="{label} render of {html.escape(name)}" loading="lazy">'
                )
                caption = fmt_ms(cell["seconds"])
            else:
                img = '<div class="missing">no render</div>'
                caption = "FAIL"
            thumbs.append(f'<figure><div class="thumb">{img}</div><figcaption>{label}<br>{caption}</figcaption></figure>')
        dims = f' <span class="dims">({f["render_w"]}&times;{f["render_h"]}px)</span>' if "render_w" in f else ""
        blocks.append(
            f'<section class="sample"><h3>{html.escape(name)}{dims}</h3>'
            f'<p class="desc">{html.escape(f["desc"])}</p>'
            f'<div class="thumbs">{"".join(thumbs)}</div></section>'
        )
    return "".join(blocks)


CSS = """
:root {
  --bg: #ffffff; --fg: #1a1a1a; --muted: #666; --border: #e2e2e2;
  --ok-bg: #e8f7ee; --ok-fg: #1a7f3c; --fail-bg: #fdecec; --fail-fg: #b3261e;
  --card-bg: #fafafa;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #14161a; --fg: #eaeaea; --muted: #9a9a9a; --border: #2c2f36;
    --ok-bg: #123522; --ok-fg: #63d68f; --fail-bg: #3a1616; --fail-fg: #ff8a80;
    --card-bg: #1c1f26;
  }
}
:root[data-theme="dark"] {
  --bg: #14161a; --fg: #eaeaea; --muted: #9a9a9a; --border: #2c2f36;
  --ok-bg: #123522; --ok-fg: #63d68f; --fail-bg: #3a1616; --fail-fg: #ff8a80;
  --card-bg: #1c1f26;
}
body {
  background: var(--bg); color: var(--fg); margin: 0; padding: 2rem 1.25rem 4rem;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
}
.wrap { max-width: 980px; margin: 0 auto; }
h1 { font-size: 1.6rem; margin-bottom: 0.25rem; }
.subtitle { color: var(--muted); margin-top: 0; margin-bottom: 2rem; font-size: 0.9rem; }
h2 { font-size: 1.15rem; margin-top: 2.5rem; border-bottom: 1px solid var(--border); padding-bottom: 0.4rem; }
table { border-collapse: collapse; width: 100%; margin-top: 1rem; font-size: 0.88rem; }
th, td { padding: 0.5rem 0.6rem; border: 1px solid var(--border); text-align: left; }
th { background: var(--card-bg); }
td.filename { font-weight: 600; }
td.filename .desc { font-weight: 400; color: var(--muted); font-size: 0.78rem; }
td.ok { background: var(--ok-bg); color: var(--ok-fg); text-align: right; font-variant-numeric: tabular-nums; }
td.fail { background: var(--fail-bg); color: var(--fail-fg); text-align: center; cursor: help; }
.matrix { overflow-x: auto; display: block; }
.sample { margin-top: 2rem; padding: 1rem; border: 1px solid var(--border); border-radius: 10px; background: var(--card-bg); }
.sample h3 { margin: 0 0 0.15rem; }
.sample h3 .dims { font-weight: 400; color: var(--muted); font-size: 0.8rem; }
.sample .desc { color: var(--muted); margin-top: 0; font-size: 0.85rem; }
.thumbs { display: flex; flex-wrap: wrap; gap: 1rem; margin-top: 0.75rem; overflow-x: auto; }
figure { margin: 0; text-align: center; width: 140px; flex: 0 0 auto; }
.thumb { width: 128px; height: 128px; display: flex; align-items: center; justify-content: center;
  background:
    linear-gradient(45deg, #80808022 25%, transparent 25%), linear-gradient(-45deg, #80808022 25%, transparent 25%),
    linear-gradient(45deg, transparent 75%, #80808022 75%), linear-gradient(-45deg, transparent 75%, #80808022 75%);
  background-size: 16px 16px; background-position: 0 0, 0 8px, 8px -8px, -8px 0px;
  border: 1px solid var(--border); border-radius: 6px; overflow: hidden; }
.thumb img { max-width: 100%; max-height: 100%; }
.missing { color: var(--muted); font-size: 0.75rem; }
figcaption { font-size: 0.75rem; color: var(--muted); margin-top: 0.35rem; line-height: 1.3; }
.meta { color: var(--muted); font-size: 0.85rem; }
footer { margin-top: 3rem; color: var(--muted); font-size: 0.78rem; border-top: 1px solid var(--border); padding-top: 1rem; }
"""


def build_html(report):
    engines = [k for k in ENGINE_ORDER if k in report["engines"]]
    engine_list = ", ".join(html.escape(report["engines"][k]["label"]) for k in engines)
    unavailable = report.get("engine_load_errors") or {}
    unavailable_note = (
        f'<p class="meta">Not available in this build environment: {html.escape(", ".join(unavailable))}.</p>'
        if unavailable
        else ""
    )
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>novasvg vs. resvg, lunasvg, cairosvg, thorvg, nanosvg — SVG renderer benchmark</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>{CSS}</style>
</head>
<body>
<div class="wrap">
  <h1>SVG renderer benchmark</h1>
  <p class="subtitle">novasvg vs. {engine_list} — {len(report['files'])} samples,
     each rendered at its own aspect ratio (fit within {report['width']}&times;{report['height']}px),
     median of {report['runs']} runs per cell. Generated {now}.</p>
  {unavailable_note}
  <p class="meta">nanosvg is included as a lightweight baseline, not a fair fight with the other 5 — it's a
     minimal path/gradient rasterizer with no CSS, no filters, and no text layout, so its FAILs and blank
     renders on filter- or text-heavy samples below are expected scope, not bugs.</p>

  <h2>Engine versions</h2>
  {build_summary_table(report)}

  <h2>Capability &amp; speed matrix</h2>
  <p class="meta">Green = rendered successfully (median render time shown). Red = the engine failed on that file; hover for the error.</p>
  {build_matrix_table(report)}

  <h2>Rendered output</h2>
  <p class="meta">Same size (that sample's own aspect ratio, fit within {report['width']}&times;{report['height']}px) from every engine, side by side, so visual differences (missing text, wrong fills, unapplied filters, ...) are easy to spot.</p>
  {build_gallery(report)}

  <footer>
    Built entirely by <code>benchmarks/CMakeLists.txt</code> (<code>cmake --build build --target novasvg_benchmark</code>)
    from novasvg's own <code>data/</code> sample corpus. novasvg, resvg, lunasvg, thorvg and nanosvg are each driven
    directly through their own C/C++ API in one process (<code>native/*_native_bench.cpp</code>) — no CLI, no
    per-render subprocess, no Python bindings. cairosvg is the one exception (it has no native library of its
    own to link against) and runs through its Python binding instead. This report is fully self-contained —
    every image is a base64 data URI, no external files or network access required to view it.
  </footer>
</div>
</body>
</html>
"""


def main():
    if len(sys.argv) != 3:
        print("usage: generate_report.py <results.json> <out.html>", file=sys.stderr)
        sys.exit(1)
    results_path, out_path = Path(sys.argv[1]), Path(sys.argv[2])
    report = json.loads(results_path.read_text())
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(build_html(report))
    print(f"Wrote {out_path} ({out_path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
