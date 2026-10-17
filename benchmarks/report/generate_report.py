#!/usr/bin/env python3
"""Builds index.html from render_benchmark.json (the file ../report/gather.py writes).

    outputs/
      index.html             <- this script; loads every render from renders/ by relative path
      render_benchmark.json   <- gather.py; every number on the page, "At a glance" included, comes from it
      renders/<engine>/       <- run_benchmark.py; one folder per engine: its PNGs and its own result.json

The page holds no data of its own and embeds nothing: edit the JSON (or re-gather) and re-run this to
change it, no benchmark run needed.

Usage: generate_report.py <render_benchmark.json>      (writes index.html next to it)

The page shares its look with the docs site: the brand tokens are read from
docs/novasvg-docs.css and the colourful charts come from charts.py, which the
docs landing page also uses.
"""
import html
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parent))
import charts  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]


def fmt_ms(ms):
    return f"{ms:.2f} ms" if ms is not None else "-"


# Human defect grade (HDG) icons. Shape differs per grade (circle+check, triangle, octagon+cross), so the grade
# never depends on colour alone.
_SVG = '<svg viewBox="0 0 16 16" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'
HDG_ICONS = {
    0: _SVG + '<circle cx="8" cy="8" r="6.4"/><path d="M5 8.2l2.2 2.2L11.2 6"/></svg>',
    1: _SVG + '<path d="M8 2.2L14.2 13H1.8z"/><path d="M8 6.4v3.2M8 11.6v.1"/></svg>',
    2: _SVG + '<path d="M5.2 1.6h5.6l3.6 3.6v5.6l-3.6 3.6H5.2L1.6 10.8V5.2z"/><path d="M5.6 5.6l4.8 4.8M10.4 5.6l-4.8 4.8"/></svg>',
}
HDG_NONE_ICON = _SVG + '<circle cx="8" cy="8" r="6.4" stroke-dasharray="2 2.4"/></svg>'
HDG_SHORT = {0: "accurate", 1: "defective", 2: "broken"}


def hdg_metric(data):
    return (data.get("metrics") or {}).get("hdg")


def fmt_hdg(cell, abbr="HDG"):
    """The grade icon shown under a render's caption; its tooltip / aria-label carries the grade and the note."""
    grade = cell.get("hdg")
    if grade not in HDG_ICONS:
        return f'<span class="hdg g-none" title="{html.escape(abbr)}: not rated" aria-label="{html.escape(abbr)}: not rated" role="img" tabindex="0">{HDG_NONE_ICON}<span>not rated</span></span>'
    note = cell.get("hdg_note")
    text = f"{abbr} {grade}: {HDG_SHORT[grade]}" + (f" - {note}" if note else "")
    return (f'<span class="hdg g{grade}" title="{html.escape(text, quote=True)}" aria-label="{html.escape(text, quote=True)}" '
            f'role="img" tabindex="0">{HDG_ICONS[grade]}<span>{abbr} {grade}</span></span>')


def fmt_hdg_summary(counts):
    """'59 / 1 / 0' with a stacked bar, for the engine table; '-' when the engine has no grades."""
    if not counts or not counts.get("rated"):
        return "-"
    n = counts["rated"]
    segs = "".join(f'<i class="g{g}" style="width:{counts[str(g)] / n * 100:.2f}%"></i>' for g in (0, 1, 2))
    nums = " / ".join(f'<span class="g{g}">{counts[str(g)]}</span>' for g in (0, 1, 2))
    unrated = f' <small>({counts["total"] - n} unrated)</small>' if counts["total"] > n else ""
    return f'<span class="hdgbar" aria-hidden="true">{segs}</span><span class="hdgnums">{nums}</span>{unrated}'


def build_summary_table(data):
    # engine order = the JSON's own `summary` order (gather.py already put it in display order)
    summary = data["summary"]
    metric = hdg_metric(data)
    rows = []
    for k in list(summary):
        e = summary[k]
        hdg_cell = f"<td>{fmt_hdg_summary(e.get('hdg'))}</td>" if metric else ""
        row_cls = ' class="is-nova"' if k == "novasvg" else ""  # not inline: no backslashes in f-string expressions before 3.12
        rows.append(
            f"<tr{row_cls}><td>{html.escape(e['label'])}</td>"
            f"<td>{html.escape(e['version'])}</td>"
            f"<td>{e['ok']}/{e['total']}</td>"
            f"{hdg_cell}"
            f"<td>{fmt_ms(e['avg_ms'])}</td>"
            f"<td>{fmt_ms(e['total_ms'])}</td></tr>"
        )
    hdg_head = (f'<th title="Human defect grade: samples graded 0 accurate / 1 defective / 2 broken">'
                f'{html.escape(metric.get("abbr", "HDG"))} 0 / 1 / 2</th>') if metric else ""
    return f"""
    <div class="tablewrap"><table class="summary">
      <thead><tr><th>Engine</th><th>Version</th><th>Rendered OK</th>{hdg_head}<th>Avg time / sample</th><th>Total time</th></tr></thead>
      <tbody>{"".join(rows)}</tbody>
    </table></div>
    """


def fmt_rmse(cell, is_ground_truth):
    if is_ground_truth:
        return '<span class="rmse-ref">reference</span>'
    rmse = cell.get("rmse")
    if rmse is None:
        return ""
    cls = "good" if rmse < 5 else "mid" if rmse < 25 else "far"
    return f'<span class="rmse {cls}">RMSE {rmse:.2f}</span>'


# GitHub-style "link to this example" icon, shown next to a sample's name on hover
ANCHOR_ICON = (
    '<svg viewBox="0 0 16 16" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.6" '
    'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'
    '<path d="M6.5 9.5a3 3 0 0 0 4.2 0l2.3-2.3a3 3 0 0 0-4.2-4.2l-.8.8"/>'
    '<path d="M9.5 6.5a3 3 0 0 0-4.2 0L3 8.8a3 3 0 0 0 4.2 4.2l.8-.8"/></svg>'
)


def build_gallery(data):
    summary = data["summary"]
    metric = hdg_metric(data)
    engines = list(summary)
    blocks = []
    for f in data["files"]:
        name = f["name"]
        thumbs = []
        for k in engines:
            cell = data["matrix"][name].get(k, {})
            label = html.escape(summary[k]["label"])
            if cell.get("ok") and cell.get("png"):
                img = (
                    f'<img src="{html.escape(cell["png"], quote=True)}" '
                    f'alt="{label} render of {html.escape(name)}" loading="lazy" '
                    f'class="zoomable" onclick="openLightbox(this)">'
                )
                caption = f'{fmt_ms(cell["seconds"] * 1000)}<br>{fmt_rmse(cell, k == "ground_truth")}'
                if metric and k != "ground_truth":
                    caption += f'<br>{fmt_hdg(cell, metric.get("abbr", "HDG"))}'
            else:
                img = '<div class="missing">no render</div>'
                caption = "FAIL"
            fig_cls = ' class="is-nova"' if k == "novasvg" else ""
            thumbs.append(
                f'<figure{fig_cls}><div class="thumb">{img}</div>'
                f'<figcaption><b>{label}</b>{caption}</figcaption></figure>'
            )
        dims = f' <span class="dims">({f["render_w"]}&times;{f["render_h"]}px)</span>' if "render_w" in f else ""
        blocks.append(
            f'<section class="sample" id="{html.escape(name, quote=True)}"><h3>{html.escape(name)}{dims}'
            f'<a class="anchor" href="#{quote(name, safe="")}" title="Link to this example" '
            f'aria-label="Link to {html.escape(name, quote=True)}">{ANCHOR_ICON}</a></h3>'
            f'<p class="desc">{html.escape(f["desc"])}</p>'
            f'<div class="thumbs">{"".join(thumbs)}</div></section>'
        )
    return "".join(blocks)


FALLBACK_TOKENS = """
:root { --bg:#fbfbfe; --bg2:#f3f3fa; --bg3:#ebebf5; --fg:#14142b; --fg2:#55556e; --fg3:#8a8aa3; --border:#e4e4f0;
  --border2:#d3d3e6; --accent:#1867b2; --accent2:#884d9e; --accent3:#b6489b; --accent-glow:rgba(24,103,178,.16);
  --accent-glow2:rgba(24,103,178,.07); --card:#fff; --code-bg:#f4f4fb; --code-fg:#1d1d3a;
  --font-display:'Outfit',system-ui,sans-serif; --font-body:'Figtree',system-ui,sans-serif; --font-mono:ui-monospace,Menlo,monospace;
  --brand-grad:linear-gradient(100deg,#19469b,#1867b2 28%,#884d9e 64%,#b6489b);
  --icon-grad:linear-gradient(135deg,#1867b2,#b6489b);
  --chart-nova:linear-gradient(90deg,#19469b,#1867b2 35%,#884d9e 70%,#b6489b);
  --shadow-sm:0 1px 3px rgba(20,20,60,.08); --shadow-lg:0 16px 40px rgba(40,30,90,.14); }
[data-theme="dark"] { --bg:#0a0a13; --bg2:#11111d; --bg3:#191929; --fg:#ececf8; --fg2:#a0a0bd; --fg3:#6a6a88;
  --border:#23233a; --border2:#30304c; --accent:#5aa7f0; --accent2:#b07dd0; --accent3:#e06bbd;
  --accent-glow:rgba(90,167,240,.18); --accent-glow2:rgba(90,167,240,.07); --card:#11111d; --code-bg:#0d0d19;
  --code-fg:#e4e4f6; --brand-grad:linear-gradient(100deg,#3f8fe0,#5aa7f0 28%,#a468c0 64%,#e06bbd);
  --icon-grad:linear-gradient(135deg,#3f8fe0,#e06bbd);
  --chart-nova:linear-gradient(90deg,#2c6fc0,#3f8fe0 35%,#a468c0 70%,#e06bbd);
  --shadow-sm:0 1px 3px rgba(0,0,0,.5); --shadow-lg:0 18px 44px rgba(0,0,0,.65); }
"""


def brand_tokens():
    """Token blocks from the docs stylesheet (single source of truth), with a baked-in fallback."""
    css = ROOT / "docs" / "novasvg-docs.css"
    try:
        text = css.read_text()
        start = text.index("/* ── NovaSVG brand tokens")
        end = text.index("\n* { margin: 0", start)
        return text[start:end]
    except (OSError, ValueError):
        return FALLBACK_TOKENS


def logo_svg():
    try:
        return (ROOT / "data" / "nova.svg").read_text()
    except OSError:
        return ""


CSS = r"""
* { box-sizing: border-box; margin: 0; padding: 0; }
html { scroll-behavior: smooth; scrollbar-color: var(--border2) transparent; }
body { background: var(--bg); color: var(--fg); font-family: var(--font-body); line-height: 1.6; -webkit-font-smoothing: antialiased; }
body::before { content: ""; position: fixed; inset: 0; z-index: -1; pointer-events: none;
  background:
    radial-gradient(60rem 36rem at 10% -8%, color-mix(in srgb, var(--accent) 15%, transparent), transparent 70%),
    radial-gradient(50rem 32rem at 98% 2%, color-mix(in srgb, var(--accent3) 13%, transparent), transparent 70%); }
::selection { background: var(--accent-glow); }
a { color: var(--accent); }
code { font-family: var(--font-mono); font-size: .86em; background: var(--bg3); padding: .1em .38em; border-radius: 5px; }
h1, h2, h3 { font-family: var(--font-display); letter-spacing: -0.03em; }

/* topbar, same as the docs site */
.topbar { position: sticky; top: 0; z-index: 50; height: 56px; display: flex; align-items: center; gap: .75rem; padding: 0 1.5rem;
  background: color-mix(in srgb, var(--bg) 82%, transparent); -webkit-backdrop-filter: blur(14px); backdrop-filter: blur(14px);
  border-bottom: 1px solid var(--border); }
.topbar-brand { display: flex; align-items: center; gap: .75rem; text-decoration: none; color: var(--fg); }
.topbar-logo { height: 32px; aspect-ratio: 2 / 1; } .topbar-logo svg { width: 100%; height: 100%; display: block; }
.topbar-name { position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0 0 0 0); }
.topbar-sep { width: 1px; height: 18px; background: var(--border2); }
.topbar-sub { font-size: .82rem; color: var(--fg2); }
.topbar-nav { display: flex; margin-left: 1.25rem; }
.topbar-link { font-size: .8rem; font-weight: 500; color: var(--fg2); text-decoration: none; padding: .3rem .7rem; border-bottom: 2px solid transparent; transition: all .18s; }
.topbar-link:hover, .topbar-link.current { color: var(--fg); border-bottom-color: var(--accent); }
.topbar-actions { margin-left: auto; display: flex; gap: .6rem; align-items: center; }
.topbar-gh, .theme-toggle { font: 500 .8rem var(--font-body); color: var(--fg2); text-decoration: none; border: 1px solid var(--border2);
  background: transparent; padding: .45rem .8rem; border-radius: 8px; cursor: pointer; transition: all .18s; display: inline-flex; align-items: center; }
.topbar-gh:hover, .theme-toggle:hover { color: var(--fg); border-color: var(--accent); background: var(--accent-glow2); }
.theme-toggle svg { width: 16px; height: 16px; }
.theme-toggle .icon-moon { display: none; } [data-theme="dark"] .theme-toggle .icon-moon { display: block; } [data-theme="dark"] .theme-toggle .icon-sun { display: none; }
a:focus-visible, button:focus-visible { outline: 2px solid var(--accent); outline-offset: 3px; }

.wrap { max-width: 1120px; margin: 0 auto; padding: 0 1.5rem 5rem; }
.intro { padding: 4rem 0 1rem; }
.intro h1 { font-size: clamp(2.4rem, 5vw, 4rem); font-weight: 800; line-height: 1; letter-spacing: -0.04em; }
.intro h1 .ink { position: relative; display: inline-block; }
.intro h1 .ink::after { content: ""; position: absolute; left: 0; right: 3%; bottom: .02em; height: .1em; border-radius: 99px; background: var(--brand-grad); }
.lede { margin-top: 1.2rem; max-width: 46rem; color: var(--fg2); font-size: 1.05rem; }
.facts { margin-top: 1.4rem; display: flex; flex-wrap: wrap; gap: .5rem; }
.facts span { font: 500 .78rem var(--font-mono); color: var(--fg2); border: 1px solid var(--border2); background: var(--card); padding: .35rem .7rem; border-radius: 99px; }

h2 { font-size: clamp(1.5rem, 2.6vw, 2rem); font-weight: 700; margin-top: 4rem; }
h2 + .meta { margin-top: .6rem; }
.meta { color: var(--fg2); font-size: .95rem; max-width: 52rem; margin-top: .8rem; }
.meta strong { color: var(--fg); }

.charts { margin-top: 1.6rem; display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 1.25rem; }
.chart { border: 1px solid var(--border); border-radius: 20px; background: var(--card); padding: 1.4rem 1.5rem 1.5rem; }
.chart h3 { font-size: 1.05rem; font-weight: 700; } .chart p { margin: .2rem 0 1.1rem; font-size: .82rem; color: var(--fg2); }
.charts + .meta { margin-top: 1.4rem; }
/*@BARS_CSS@*/

.tablewrap { margin-top: 1.6rem; overflow-x: auto; border: 1px solid var(--border); border-radius: 16px; background: var(--card); }
table { border-collapse: collapse; width: 100%; font-size: .9rem; }
th, td { padding: .75rem 1rem; text-align: left; border-bottom: 1px solid var(--border); white-space: nowrap; }
th { font: 600 .72rem var(--font-body); letter-spacing: .08em; text-transform: uppercase; color: var(--fg3); background: var(--bg2); }
tbody tr:last-child td { border-bottom: 0; }
td:nth-child(n+3), th:nth-child(n+3) { text-align: right; font-variant-numeric: tabular-nums; }
tr.is-nova td { background: var(--accent-glow2); font-weight: 600; }
tr.is-nova td:first-child { box-shadow: inset 3px 0 0 var(--accent2); }

.sample { margin-top: 1.25rem; padding: 1.2rem 1.3rem 1.3rem; border: 1px solid var(--border); border-radius: 18px; background: var(--card); }
.sample h3 { font-size: 1.05rem; font-weight: 700; }
.sample h3 .dims { font: 400 .76rem var(--font-mono); color: var(--fg3); margin-left: .4rem; }
.sample { scroll-margin-top: 4.5rem; }  /* the sticky 56px topbar must not cover the heading after a #fragment jump */
.sample h3 .anchor { display: inline-flex; vertical-align: -2px; margin-left: .45rem; padding: 2px; border-radius: 6px;
  color: var(--fg3); text-decoration: none; opacity: 0; transition: opacity .15s, color .15s; }
.sample h3:hover .anchor, .sample h3 .anchor:focus-visible, .sample:target h3 .anchor { opacity: 1; }
.sample h3 .anchor:hover { color: var(--accent2); }
@media (hover: none) { .sample h3 .anchor { opacity: .6; } }  /* touch screens have no hover: keep it visible */
.sample:target { border-color: var(--accent2); box-shadow: 0 0 0 1px var(--accent2), 0 8px 24px color-mix(in srgb, var(--accent2) 18%, transparent); }
.sample .desc { color: var(--fg2); font-size: .85rem; margin-top: .1rem; }
.thumbs { display: flex; gap: .75rem; margin-top: 1rem; overflow-x: auto; padding-bottom: .4rem; }
figure { margin: 0; text-align: center; width: 128px; flex: 0 0 auto; }
.thumb { width: 128px; height: 128px; display: flex; align-items: center; justify-content: center; border-radius: 12px; overflow: hidden;
  border: 1px solid var(--border2);
  background: linear-gradient(45deg,#8080801f 25%,transparent 25%), linear-gradient(-45deg,#8080801f 25%,transparent 25%),
    linear-gradient(45deg,transparent 75%,#8080801f 75%), linear-gradient(-45deg,transparent 75%,#8080801f 75%);
  background-size: 16px 16px; background-position: 0 0, 0 8px, 8px -8px, -8px 0; background-color: #fff; }
.thumb img { max-width: 100%; max-height: 100%; cursor: zoom-in; transition: transform .25s; } .thumb img:hover { transform: scale(1.04); }
.missing { color: #666; font-size: .75rem; }
figcaption { font-size: .75rem; color: var(--fg2); margin-top: .45rem; line-height: 1.45; }
figcaption b { display: block; color: var(--fg); font-weight: 600; }
figcaption .rmse { font-weight: 600; } figcaption .rmse.good { color: var(--accent); } figcaption .rmse.mid { color: var(--accent2); } figcaption .rmse.far { color: var(--accent3); }
figcaption .rmse-ref { font-style: italic; color: var(--fg3); }
/* HDG colours live here, not in the brand tokens (those are read from docs/novasvg-docs.css). Green / amber / red. */
:root { --ok:#1f8a4c; --warn:#b97a00; --bad:#d03a3a; }
[data-theme="dark"] { --ok:#4cd68a; --warn:#f2c14e; --bad:#ff6b6b; }
figcaption .hdg { display: inline-flex; align-items: center; gap: .3rem; font-weight: 600; margin-top: .15rem; cursor: help; border-radius: 6px; }
figcaption .hdg svg { flex: none; }
.hdg.g0, .hdgnums .g0 { color: var(--ok); } .hdg.g1, .hdgnums .g1 { color: var(--warn); } .hdg.g2, .hdgnums .g2 { color: var(--bad); }
.hdg.g0, .hdg.g1, .hdg.g2 { padding: .1rem .45rem .1rem .3rem; border: 1px solid currentColor; background: color-mix(in srgb, currentColor 13%, transparent); }
.hdg.g-none { color: var(--fg3); font-weight: 500; font-style: italic; }
.hdg:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.hdgbar { display: inline-flex; width: 7rem; height: .6rem; border-radius: 3px; overflow: hidden; vertical-align: middle; margin-right: .6rem; background: var(--bg3); }
.hdgbar i { display: block; height: 100%; } .hdgbar .g0 { background: var(--ok); } .hdgbar .g1 { background: var(--warn); } .hdgbar .g2 { background: var(--bad); }
.hdgnums { font-weight: 600; white-space: nowrap; } .hdgnums small, td small { font-weight: 400; color: var(--fg3); }
.hdglegend { list-style: none; padding: 0; margin: .4rem 0 1rem; display: grid; gap: .35rem; }
.hdglegend li { display: flex; gap: .55rem; align-items: flex-start; color: var(--fg2); font-size: .85rem; }
.hdglegend .hdg { flex: none; min-width: 5.2rem; }
figure.is-nova .thumb { border-color: var(--accent2); box-shadow: 0 0 0 1px var(--accent2), 0 8px 24px color-mix(in srgb, var(--accent2) 28%, transparent); }

footer { margin-top: 4rem; padding-top: 1.2rem; border-top: 1px solid var(--border); color: var(--fg3); font-size: .8rem; max-width: 60rem; }
#lightbox { position: fixed; inset: 0; background: rgba(5,5,12,.86); display: none; align-items: center; justify-content: center; z-index: 1000; padding: 3vh 3vw; cursor: zoom-out; }
#lightbox.open { display: flex; }
#lightbox img { max-width: 100%; max-height: 100%; border-radius: 10px; box-shadow: 0 8px 40px rgba(0,0,0,.5); background: #fff; }
#lightbox-caption { position: fixed; bottom: 2vh; left: 0; right: 0; text-align: center; color: #eee; font-size: .85rem; }
@media (prefers-reduced-motion: reduce) { html { scroll-behavior: auto; } .thumb img { transition: none; } }
@media (max-width: 860px) { .charts { grid-template-columns: 1fr; } .topbar-sub, .topbar-gh { display: none; } }
"""

TOPBAR = """<header class="topbar">
  <a class="topbar-brand" href="../index.html">
    <div class="topbar-logo">{logo}</div><span class="topbar-name">NovaSVG</span>
    <span class="topbar-sep"></span><span class="topbar-sub">Documentation</span>
  </a>
  <nav class="topbar-nav"><a class="topbar-link current" href="index.html" aria-current="page">Benchmarks</a></nav>
  <div class="topbar-actions">
    <a class="topbar-gh" href="https://github.com/MohammadRaziei/novasvg" target="_blank" rel="noopener">GitHub</a>
    <button class="theme-toggle" onclick="toggleTheme()" aria-label="Toggle theme">
      <svg class="icon-sun" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.8" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M12 3v2.25m6.364.386-1.591 1.591M21 12h-2.25m-.386 6.364-1.591-1.591M12 18.75V21m-4.773-4.227-1.591 1.591M5.25 12H3m4.227-4.773L5.636 5.636M15.75 12a3.75 3.75 0 1 1-7.5 0 3.75 3.75 0 0 1 7.5 0Z"/></svg>
      <svg class="icon-moon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.8" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M21.752 15.002A9.72 9.72 0 0 1 18 15.75c-5.385 0-9.75-4.365-9.75-9.75 0-1.33.266-2.597.748-3.752A9.753 9.753 0 0 0 3 11.25C3 16.635 7.365 21 12.75 21a9.753 9.753 0 0 0 9.002-5.998Z"/></svg>
    </button>
  </div>
</header>"""

THEME_JS = """
(function () {
  var t = 'dark';
  try { var s = JSON.parse(localStorage.getItem('novasvg-docs') || '{}');
        if (s.theme) t = s.theme; else if (matchMedia('(prefers-color-scheme: light)').matches) t = 'light'; } catch (e) {}
  document.documentElement.setAttribute('data-theme', t);
  document.documentElement.classList.add('js');
})();
function toggleTheme() {
  var t = document.documentElement.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', t);
  try { var s = JSON.parse(localStorage.getItem('novasvg-docs') || '{}'); s.theme = t; localStorage.setItem('novasvg-docs', JSON.stringify(s)); } catch (e) {}
}
"""


def build_html(data):
    """data: the dict loaded from render_benchmark.json. Every number on the page comes from it."""
    summary = data["summary"]
    engines = list(summary)
    engine_list = ", ".join(html.escape(summary[k]["label"]) for k in engines if k not in ("ground_truth", "novasvg"))
    unavailable = data.get("engine_load_errors") or {}
    unavailable_note = (
        f'<p class="meta">Not available in this build environment: {html.escape(", ".join(unavailable))}.</p>'
        if unavailable
        else ""
    )
    now = data.get("generated") or datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

    # "At a glance": the charts are drawn from the JSON's per-engine summary
    chart_data = {"engines": summary}
    chart_block = (
        '<div class="charts">'
        f'<div class="chart"><h3>Accuracy</h3><p>Mean RMSE against the Chromium render. Lower is closer.</p>{charts.fidelity_chart(chart_data)}</div>'
        f'<div class="chart"><h3>Speed</h3><p>Average render time per sample. Lower is faster.</p>{charts.speed_chart(chart_data)}</div>'
        "</div>"
    )
    metric = hdg_metric(data)
    if metric:
        abbr = html.escape(metric.get("abbr", "HDG"))
        scale = metric.get("scale", {})
        legend = "".join(
            f'<li><span class="hdg g{g}">{HDG_ICONS[g]}<span>{abbr} {g}</span></span><span>{html.escape(scale.get(str(g), HDG_SHORT[g]))}</span></li>'
            for g in (0, 1, 2)
        )
        hdg_block = (
            f'<h3>{html.escape(metric.get("name", "Human defect grade"))} ({abbr})</h3>'
            f'<p class="meta">RMSE says how many pixels differ, not how bad it looks: a missing label barely moves it, a few pixels of '
            f'font difference move it a lot. So every render is also graded by a person against the Chromium render, on a three-step '
            f'scale where <strong>lower is better</strong>. The grade shows under each render below (hover or focus it for the reason), '
            f'and the table counts them per engine. Source: <code>benchmarks/ratings.json</code>, merged into <a href="render_benchmark.json"><code>render_benchmark.json</code></a>'
            f'{" (" + html.escape(metric["rated_by"]) + ")" if metric.get("rated_by") else ""}.</p>'
            f'<ul class="hdglegend">{legend}</ul>'
        )
    else:
        hdg_block = ""
    css = CSS.replace("/*@BARS_CSS@*/", charts.BARS_CSS)
    return f"""<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<title>novasvg vs. resvg, lunasvg, cairosvg, thorvg, nanosvg: SVG renderer benchmark</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<script>{THEME_JS}</script>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Figtree:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&family=Outfit:wght@500;600;700;800&display=swap" rel="stylesheet">
<style>{brand_tokens()}
{css}</style>
</head>
<body>
{TOPBAR.format(logo=logo_svg())}
<div class="wrap">
  <section class="intro">
    <h1>SVG renderer <span class="ink">benchmark</span></h1>
    <p class="lede">novasvg against {engine_list}. Each of the {len(data['files'])} samples is rendered at its own aspect ratio,
      fit within {data['width']}&times;{data['height']}px, and timed as the median of {data['runs']} runs.</p>
    <div class="facts"><span>{len(data['files'])} samples</span><span>median of {data['runs']} runs</span><span>generated {now}</span></div>
  </section>
  {unavailable_note}

  <h2>At a glance</h2>
  <p class="meta"><strong>Chromium (ground truth)</strong> is the reference every other engine is checked
     against, not a competitor. Its render time includes full browser page-navigation overhead (one shared
     instance for the whole corpus, one render each, no median-of-N), so it is left out of the charts.
     <strong>RMSE</strong> is the root mean squared error against the ground-truth render, at the same pixel
     dimensions, over all 4 RGBA channels on the 0-255 scale. Squaring before the square root weights a handful
     of badly wrong pixels (a missing filter, a wrong fill, a shifted shape) far more than the routine
     anti-aliasing noise along every edge, so low single digits is essentially imperceptible and anything in
     the tens or higher usually means something structural differs.</p>
  {hdg_block}
  {chart_block}
  <p class="meta">These charts and the table below are drawn from <a href="render_benchmark.json"><code>render_benchmark.json</code></a>, which sits next to this page.</p>
  <p class="meta">nanosvg is included as a lightweight baseline, not a fair fight with the other 5. It is a
     minimal path and gradient rasterizer with no CSS, no filters and no text layout, so its FAILs and blank
     renders on filter or text heavy samples below are expected scope, not bugs.</p>

  <h2>Engine versions</h2>
  {build_summary_table(data)}

  <h2>Rendered output</h2>
  <p class="meta">Same size (that sample's own aspect ratio, fit within {data['width']}&times;{data['height']}px) from every engine, side by side, so visual differences (missing text, wrong fills, unapplied filters, ...) are easy to spot. Click any render to zoom in.</p>
  {build_gallery(data)}

  <footer>
    Built entirely by <code>benchmarks/CMakeLists.txt</code> (<code>cmake --build build --target novasvg_benchmark</code>)
    from novasvg's own <code>data/</code> sample corpus. novasvg, resvg, lunasvg, thorvg and nanosvg are each driven
    directly through their own C/C++ API in one process (<code>native/*_native_bench.cpp</code>), with no CLI, no
    per-render subprocess and no Python bindings. cairosvg is the one exception (it has no native library of its
    own to link against) and runs through its Python binding instead. Chromium (ground truth) is driven through
    Playwright. Every render is a plain PNG under <code>renders/</code> next to this page and the numbers are in
    <code>render_benchmark.json</code>; keep the three together, no server or network is needed to view them, apart
    from the web fonts, which fall back to system fonts offline.
  </footer>
</div>

<div id="lightbox" onclick="closeLightbox()">
  <img id="lightbox-img" src="" alt="">
  <div id="lightbox-caption"></div>
</div>
<script>
function openLightbox(imgEl) {{
  const lb = document.getElementById('lightbox');
  document.getElementById('lightbox-img').src = imgEl.src;
  document.getElementById('lightbox-img').alt = imgEl.alt;
  document.getElementById('lightbox-caption').textContent = imgEl.alt;
  lb.classList.add('open');
}}
function closeLightbox() {{ document.getElementById('lightbox').classList.remove('open'); }}
document.addEventListener('keydown', (e) => {{ if (e.key === 'Escape') closeLightbox(); }});
(function () {{
  const bars = document.querySelectorAll('.bars');
  if (!('IntersectionObserver' in window)) {{ bars.forEach(c => c.classList.add('in')); return; }}
  const io = new IntersectionObserver((es) => es.forEach(e => {{ if (e.isIntersecting) {{ e.target.classList.add('in'); io.unobserve(e.target); }} }}), {{ threshold: 0.25 }});
  bars.forEach(c => io.observe(c));
}})();
</script>
</body>
</html>
"""


def write_report(json_path):
    """render_benchmark.json -> index.html next to it, built only from what the JSON says."""
    json_path = Path(json_path)
    data = json.loads(json_path.read_text(encoding="utf-8"))
    html_path = json_path.with_name("index.html")
    html_path.write_text(build_html(data), encoding="utf-8")
    return html_path


def main():
    if len(sys.argv) != 2:
        print("usage: generate_report.py <render_benchmark.json>", file=sys.stderr)
        sys.exit(1)
    html_path = write_report(sys.argv[1])
    print(f"Wrote {html_path} ({html_path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
