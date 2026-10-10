#!/usr/bin/env python3
"""Generate docs/index.html from docs/index.html.in.

Reads docs/supported_langs.json (the language registry) plus, per language,
  <folder>/index-docs.json     structured API-reference card fields
  <folder>/index-install.txt   raw HTML for the install panel
  <folder>/index-example.txt   raw HTML for the quick-example panel
and substitutes the @DOCS_TABS@ / @DOCS_CONTENT@ / @INSTALL_TABS@ /
@INSTALL_CONTENT@ / @EXAMPLE_TABS@ / @EXAMPLE_CONTENT@ placeholders (plus
@PROJECT_VERSION@ / @LOGO_CONTENT@ / @FAVICON@) in
index.html.in. The language lists in the hero and footer are derived from the
same registry (each entry's optional "role", see supported_langs.json):
@LANG_EYEBROW@ ("C++ · Python"), @LANG_LIST@ ("C++ and Python"),
@BINDING_LIST@ (same, without the C++ core) and @BINDING_COUNT@.

Invoked by docs/CreateDocs.cmake (which also copies novasvg-docs.css and the
logo/favicon into place) — see that file for the exact command line. Can
also be run directly for local iteration, e.g.:

    python3 generate_index.py \
        --index-in index.html.in --output /tmp/preview/index.html \
        --langs-file supported_langs.json --langs-dir . \
        --project-version 0.8.0 --logo-svg images/novasvg-sq.svg
"""
import argparse
import json
import sys
from pathlib import Path

PANELS = ("docs", "install", "example")

# Named SVG glyphs for languages without a suitable Font Awesome icon
# (see supported_langs.json's icon.kind == "svg").
ICON_SVG = {
    "layers": (
        '<svg class="tab-svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" '
        'stroke-width="2" stroke-linecap="round" stroke-linejoin="round">'
        '<path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg>'
    ),
}

DOC_CARD_TEMPLATE = """<div class="doc-card-inner">
    <div class="doc-card-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg>
    </div>
    <div class="doc-card-body">
        <div class="doc-card-title">{title} <span class="card-badge">{badge}</span></div>
        <p class="doc-card-desc">{description}</p>
        <div class="doc-card-meta"><span class="doc-meta-item"><i class="{meta_icon}"></i> {meta_label}</span></div>
        <a href="{link_href}" class="doc-card-btn">{link_label} <i class="fa-solid fa-arrow-right"></i></a>
    </div>
</div>
"""

ROLES = ("tool", "core", "binding")

DOC_FIELDS = ("title", "badge", "description", "meta_icon", "meta_label", "link_href", "link_label")


def oxford_join(items: list) -> str:
    if len(items) <= 2:
        return " and ".join(items)
    return ", ".join(items[:-1]) + ", and " + items[-1]


def language_placeholders(languages: list) -> dict:
    """Hero/footer language text, derived from the registry (single source of truth)."""
    for lang in languages:
        role = lang.get("role", "binding")
        if role not in ROLES:
            sys.exit(f"generate_index.py: unknown role {role!r} for language {lang['folder']!r} (expected one of {ROLES})")
    names = [l["name"] for l in languages if l.get("role", "binding") != "tool"]
    bindings = [l["name"] for l in languages if l.get("role", "binding") == "binding"]
    return {
        "@LANG_EYEBROW@": " · ".join(names),
        "@LANG_LIST@": oxford_join(names),
        "@BINDING_LIST@": oxford_join(bindings),
        "@BINDING_COUNT@": str(len(bindings)),
    }


def icon_html(icon: dict, folder: str) -> str:
    kind, value = icon.get("kind"), icon.get("value")
    if kind == "fa":
        return f'<i class="{value}"></i>'
    if kind == "svg":
        try:
            return ICON_SVG[value]
        except KeyError:
            sys.exit(f"generate_index.py: unknown svg icon {value!r} for language {folder!r}")
    sys.exit(f"generate_index.py: unknown icon kind {kind!r} for language {folder!r} (expected 'fa' or 'svg')")


def tab_button(panel: str, folder: str, name: str, icon: dict, active: bool) -> str:
    cls = "tab-btn active" if active else "tab-btn"
    return (
        f'            <button class="{cls}" data-tab="{panel}-{folder}" '
        f"onclick=\"switchTab('{panel}','{folder}')\">\n"
        f"                {icon_html(icon, folder)} {name}\n"
        f"            </button>\n"
    )


def tab_content(panel: str, folder: str, inner_html: str, active: bool) -> str:
    cls = "tab-content active" if active else "tab-content"
    return f'        <div class="{cls}" id="{panel}-{folder}">\n{inner_html}        </div>\n\n'


def load_docs_card(lang_dir: Path, folder: str) -> str:
    path = lang_dir / "index-docs.json"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        sys.exit(f"generate_index.py: failed to read {path}: {exc}")
    missing = [f for f in DOC_FIELDS if f not in data]
    if missing:
        sys.exit(f"generate_index.py: {path} is missing field(s): {', '.join(missing)}")
    return DOC_CARD_TEMPLATE.format(**{f: data[f] for f in DOC_FIELDS})


def load_raw_fragment(lang_dir: Path, panel: str) -> str:
    path = lang_dir / f"index-{panel}.txt"
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        sys.exit(f"generate_index.py: failed to read {path}: {exc}")


def highlight_svg(source: str) -> str:
    """Tiny SVG syntax highlighter for the landing page's code card (no JS needed)."""
    import html as _html, re as _re

    def tag(m: "re.Match") -> str:
        chunk = m.group(0)
        name = _re.match(r"</?[\w:-]+", chunk).group(0)
        closing = name.startswith("</")
        rest = chunk[len(name):]
        selfclose = rest.rstrip().endswith("/>")
        out = '<span class="tk-p">&lt;' + ("/" if closing else "") + "</span>"
        out += f'<span class="tk-t">{_html.escape(name.lstrip("<").lstrip("/"))}</span>'
        pos = 0
        for a in _re.finditer(r'([\w:-]+)=("[^"]*")', rest):
            out += _html.escape(rest[pos:a.start()])
            out += (f'<span class="tk-a">{_html.escape(a.group(1))}</span><span class="tk-p">=</span>'
                    f'<span class="tk-s">{_html.escape(a.group(2))}</span>')
            pos = a.end()
        out += _html.escape(rest[pos:].rstrip().rstrip(">").rstrip("/").rstrip())
        out += '<span class="tk-p">' + ("/&gt;" if selfclose else "&gt;") + "</span>"
        return out

    parts, last = [], 0
    for m in _re.finditer(r"</?[\w:-]+[^>]*>", source):
        parts.append(_html.escape(source[last:m.start()]))
        parts.append(tag(m))
        last = m.end()
    parts.append(_html.escape(source[last:]))
    return "".join(parts).rstrip("\n")


def bench_section(report_html: str, charts_py: str) -> str:
    """The landing page's benchmark teaser, drawn from the committed report. '' if unavailable."""
    import html as _html, importlib.util
    if not (report_html and charts_py and Path(report_html).is_file() and Path(charts_py).is_file()):
        return ""
    spec = importlib.util.spec_from_file_location("novasvg_bench_charts", charts_py)
    charts = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(charts)
    data = charts.parse_report_html(report_html)
    if not data:
        return ""
    eng = {k: v for k, v in data["engines"].items() if k != charts.REFERENCE}
    nova = eng.get("novasvg")
    if not nova:
        return ""
    meta = data["meta"]
    n = meta.get("samples") or "all"
    by_err = sorted((v["rmse"], k) for k, v in eng.items() if v["rmse"] is not None)
    by_time = sorted((v["avg_ms"], k) for k, v in eng.items() if v["avg_ms"] is not None)
    closest = bool(by_err) and by_err[0][1] == "novasvg"
    fastest = bool(by_time) and by_time[0][1] == "novasvg"
    title = "Closest to Chromium" if closest else "How it compares"
    lede = (f"Across {n} sample SVGs, NovaSVG's output has the lowest mean error against Chromium. " if closest
            else f"Across {n} sample SVGs, each renderer is compared with Chromium on accuracy and speed. ")
    lede += ("It is also the fastest." if fastest else "It is not the fastest renderer, and the full report shows where it wins and where it loses.")
    versions = ", ".join(f"{_html.escape(v['label'])} {_html.escape(v['version'].split(' (')[0])}" for v in eng.values())
    foot = f"{n} samples, median of {meta.get('runs') or '?'} runs" + (f", generated {_html.escape(meta['generated'])}" if meta.get("generated") else "") + f". Measured with {versions}."
    return f'''<!-- Benchmarks -->
<section class="lp lp-sec" id="benchmarks">
    <h2>{title}</h2>
    <p class="lede">{lede}</p>
    <div class="charts">
        <div class="chart"><h3>Accuracy</h3><p>Mean RMSE against a Chromium render. Lower is closer.</p>{charts.fidelity_chart(data)}</div>
        <div class="chart"><h3>Speed</h3><p>Average render time per sample. Lower is faster.</p>{charts.speed_chart(data)}</div>
    </div>
    <div class="bench-foot">
        <a class="btn btn-ghost" href="benchmarks/index.html">Open the full report <i class="fa-solid fa-arrow-right"></i></a>
        <small>{foot}</small>
    </div>
</section>
'''


def build(args: argparse.Namespace) -> str:
    langs_file = Path(args.langs_file)
    langs_dir = Path(args.langs_dir)
    try:
        registry = json.loads(langs_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        sys.exit(f"generate_index.py: failed to read {langs_file}: {exc}")

    languages = registry["languages"]
    default_lang = registry.get("default", languages[0]["folder"])
    tabs = {panel: [] for panel in PANELS}
    content = {panel: [] for panel in PANELS}

    for i, lang in enumerate(languages):
        folder, name, icon = lang["folder"], lang["name"], lang["icon"]
        lang_dir = langs_dir / folder
        active = folder == default_lang  # the registry's "default" language opens active

        for panel in PANELS:
            tabs[panel].append(tab_button(panel, folder, name, icon, active))

        docs_html = load_docs_card(lang_dir, folder)
        content["docs"].append(tab_content("docs", folder, docs_html, active))
        for panel in ("install", "example"):
            frag = load_raw_fragment(lang_dir, panel)
            content[panel].append(tab_content(panel, folder, frag, active))

    index_content = Path(args.index_in).read_text(encoding="utf-8")
    for panel in PANELS:
        index_content = index_content.replace(f"@{panel.upper()}_TABS@", "".join(tabs[panel]))
        index_content = index_content.replace(f"@{panel.upper()}_CONTENT@", "".join(content[panel]))

    for placeholder, value in language_placeholders(languages).items():
        index_content = index_content.replace(placeholder, value)

    index_content = index_content.replace("@PROJECT_VERSION@", args.project_version)
    logo_content = Path(args.logo_svg).read_text(encoding="utf-8") if args.logo_svg else ""
    index_content = index_content.replace("@LOGO_CONTENT@", logo_content)
    index_content = index_content.replace("@FAVICON@", args.favicon)
    index_content = index_content.replace("@DEFAULT_LANG@", default_lang)

    hero_svg = Path(args.hero_svg).read_text(encoding="utf-8") if args.hero_svg else ""
    index_content = index_content.replace("@HERO_SVG_CODE@", highlight_svg(hero_svg))
    index_content = index_content.replace("@BENCH_SECTION@", bench_section(args.bench_report, args.charts_py))
    bars_css = ""
    if args.charts_py and Path(args.charts_py).is_file():
        import importlib.util
        spec = importlib.util.spec_from_file_location("novasvg_bench_charts_css", args.charts_py)
        mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
        bars_css = mod.BARS_CSS
    index_content = index_content.replace("/*@BARS_CSS@*/", bars_css)
    return index_content


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--index-in", required=True, help="path to index.html.in")
    ap.add_argument("--output", required=True, help="path to write the generated index.html")
    ap.add_argument("--langs-file", required=True, help="path to supported_langs.json")
    ap.add_argument("--langs-dir", required=True, help="directory containing each <folder>/ subdirectory")
    ap.add_argument("--project-version", required=True)
    ap.add_argument("--logo-svg", default="", help="path to the inline logo SVG (embedded verbatim)")
    ap.add_argument("--hero-svg", default="", help="SVG source shown (highlighted) in the hero code card")
    ap.add_argument("--bench-report", default="", help="committed benchmarks/outputs/report.html, for the landing teaser charts")
    ap.add_argument("--charts-py", default="", help="benchmarks/report/charts.py")
    ap.add_argument("--favicon", default="novasvg-sq.svg")
    args = ap.parse_args()

    output = build(args)
    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(output, encoding="utf-8")
    print(f"-- Route index created at: {out_path}")


if __name__ == "__main__":
    main()
