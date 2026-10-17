"""The benchmark's output layout and the three steps that make it.

    run_benchmark.py   renders/<engine>/<sample>.png + result.json, one folder per engine
    gather.py          merges the result.json files -> render_benchmark.json (+ RMSE vs Chromium, + summary)
    generate_report.py render_benchmark.json -> index.html, which embeds nothing and holds no data of its own
"""
import json
import re
import sys
from argparse import Namespace
from pathlib import Path

import pytest

pytest.importorskip("numpy")
libstb = pytest.importorskip("libstb")
import numpy as np  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "benchmarks" / "report"))
sys.path.insert(0, str(ROOT / "benchmarks" / "python"))

import charts  # noqa: E402
import gather as gt  # noqa: E402
import generate_report as gr  # noqa: E402
import run_benchmark as rb  # noqa: E402

BLACK, OFF_BY_10 = (0, 0, 0, 255), (10, 0, 0, 255)  # RMSE between them = sqrt(10^2 / 4) = 5


def _solid_png(path, size, colour):
    pixels = np.empty((size[1], size[0], 4), dtype=np.uint8)
    pixels[:] = colour
    libstb.imwrite(str(path), pixels)


def make_engine(out, key, samples, label=None, generated="2026-01-02 03:04 UTC", version="1.0"):
    """Write what run_benchmark.py would: renders/<key>/<name>.png and result.json.
    samples: {name: (seconds, colour)}; colour None = the sample failed."""
    d = Path(out) / "renders" / key
    d.mkdir(parents=True)
    rows = []
    for name, (seconds, colour) in samples.items():
        if colour is not None:
            _solid_png(d / f"{name}.png", (4, 3), colour)
        rows.append({"name": name, "filename": f"{name}.svg", "desc": f"{name} desc", "render_w": 4, "render_h": 3,
                     "ok": colour is not None, "seconds": seconds if colour is not None else None,
                     "error": None if colour is not None else "boom",
                     "png": f"{name}.png" if colour is not None else None})
    (d / "result.json").write_text(json.dumps({
        "engine": key, "label": label or key, "version": version, "generated": generated,
        "runs": 5, "width": 320, "height": 320, "samples": rows}), encoding="utf-8")


@pytest.fixture()
def out(tmp_path):
    make_engine(tmp_path, "ground_truth", {"circle": (0.5, BLACK), "tiger": (0.5, BLACK)}, label="Chromium (ground truth)")
    make_engine(tmp_path, "novasvg", {"circle": (0.002, BLACK), "tiger": (0.004, OFF_BY_10)})
    make_engine(tmp_path, "resvg", {"circle": (0.004, OFF_BY_10), "tiger": (None, None)})
    return tmp_path


# --- gather.py -------------------------------------------------------------------------------------

def test_gather_merges_the_engine_folders_into_one_json(out):
    data = json.loads(gt.gather(out).read_text(encoding="utf-8"))
    assert list(data["summary"]) == ["ground_truth", "novasvg", "resvg"]  # display order, not alphabetical
    assert [f["name"] for f in data["files"]] == ["circle", "tiger"]
    assert data["matrix"]["circle"]["novasvg"]["png"] == "renders/novasvg/circle.png"
    assert data["matrix"]["tiger"]["resvg"]["png"] is None and data["matrix"]["tiger"]["resvg"]["error"] == "boom"
    assert (data["runs"], data["width"], data["height"]) == (5, 320, 320)


def test_gather_scores_every_render_against_the_chromium_render_of_the_same_sample(out):
    m = json.loads(gt.gather(out).read_text(encoding="utf-8"))["matrix"]
    assert m["circle"]["novasvg"]["rmse"] == 0.0
    assert m["tiger"]["novasvg"]["rmse"] == pytest.approx(5.0)
    assert m["circle"]["resvg"]["rmse"] == pytest.approx(5.0)
    assert m["circle"]["ground_truth"]["rmse"] is None  # the reference is not scored against itself
    assert m["tiger"]["resvg"]["rmse"] is None          # nothing rendered, nothing to score


def test_gather_summarises_each_engine(out):
    s = json.loads(gt.gather(out).read_text(encoding="utf-8"))["summary"]
    assert s["novasvg"]["avg_ms"] == pytest.approx(3.0) and s["novasvg"]["total_ms"] == pytest.approx(6.0)
    assert s["novasvg"]["rmse"] == pytest.approx(2.5)
    assert (s["resvg"]["ok"], s["resvg"]["total"]) == (1, 2)
    assert s["resvg"]["rmse"] == pytest.approx(5.0)


def test_an_engine_that_could_not_run_is_reported_but_not_summarised(out):
    d = out / "renders" / "thorvg"
    d.mkdir()
    (d / "result.json").write_text(json.dumps({"engine": "thorvg", "unavailable": "not built"}), encoding="utf-8")
    data = json.loads(gt.gather(out).read_text(encoding="utf-8"))
    assert data["engine_load_errors"] == {"thorvg": "not built"} and "thorvg" not in data["summary"]


def test_a_folder_without_result_json_is_skipped_with_a_warning(out, capsys):
    (out / "renders" / "lunasvg").mkdir()  # a run that died before writing its result
    assert "lunasvg" not in json.loads(gt.gather(out).read_text(encoding="utf-8"))["summary"]
    assert "lunasvg" in capsys.readouterr().err


def test_without_chromium_everything_still_gathers_just_without_rmse(out):
    import shutil
    shutil.rmtree(out / "renders" / "ground_truth")
    data = json.loads(gt.gather(out).read_text(encoding="utf-8"))
    assert data["summary"]["novasvg"]["rmse"] is None and data["summary"]["novasvg"]["ok"] == 2


def test_the_newest_engine_run_dates_the_report_and_mixed_settings_are_flagged(out, capsys):
    make_engine(out, "lunasvg", {"circle": (0.003, BLACK)}, generated="2026-02-03 04:05 UTC")
    assert json.loads(gt.gather(out).read_text(encoding="utf-8"))["generated"] == "2026-02-03 04:05 UTC"
    p = out / "renders" / "lunasvg" / "result.json"
    r = json.loads(p.read_text()); r["runs"] = 9; p.write_text(json.dumps(r))
    gt.gather(out)
    assert "different runs" in capsys.readouterr().err


def test_gather_refuses_to_write_an_empty_benchmark(tmp_path):
    with pytest.raises(SystemExit, match="no engine results"):
        gt.gather(tmp_path)


# --- generate_report.py ----------------------------------------------------------------------------

def test_the_page_embeds_nothing_and_every_image_is_a_file_next_to_it(out):
    gr.write_report(gt.gather(out))
    page = (out / "index.html").read_text(encoding="utf-8")
    assert "data:image" not in page and "base64" not in page
    srcs = re.findall(r'<img src="([^"]+)"', page)
    assert len(srcs) == 5  # 2 + 2 + 1; resvg's failed cell has no image
    assert all(s.startswith("renders/") and (out / s).is_file() for s in srcs)


def test_every_sample_has_an_id_equal_to_its_name_and_a_link_icon_to_it(out):
    gr.write_report(gt.gather(out))
    page = (out / "index.html").read_text(encoding="utf-8")
    ids = re.findall(r'\sid="([^"]+)"', page)
    assert len(ids) == len(set(ids))  # a fragment must hit exactly one element (the lightbox ids included)
    for name in ("circle", "tiger"):
        assert f'<section class="sample" id="{name}">' in page
        assert f'<a class="anchor" href="#{name}"' in page


def test_an_unusual_sample_name_is_escaped_in_the_id_and_encoded_in_the_fragment(tmp_path):
    make_engine(tmp_path, "ground_truth", {"a b&c": (0.5, BLACK)})
    gr.write_report(gt.gather(tmp_path))
    page = (tmp_path / "index.html").read_text(encoding="utf-8")
    assert 'id="a b&amp;c"' in page and 'href="#a%20b%26c"' in page


def test_at_a_glance_is_drawn_from_the_json_not_from_the_run(out):
    path = gt.gather(out)
    data = json.loads(path.read_text(encoding="utf-8"))
    data["summary"]["novasvg"]["avg_ms"] = 123.45  # edit the JSON only ...
    path.write_text(json.dumps(data), encoding="utf-8")
    gr.write_report(path)  # ... and rebuild the page from it
    assert "123.45" in (out / "index.html").read_text(encoding="utf-8")


# --- run_benchmark.py: what one engine's run leaves behind ------------------------------------------

def _args():
    return Namespace(runs=5, width=320, height=320)


def _resolved():
    return [("circle", Path("circle.svg"), "plain shape", 320, 320), ("tiger", Path("tiger.svg"), "art", 320, 160)]


def test_write_result_describes_one_engine_and_links_only_pngs_that_exist(tmp_path):
    d = rb.fresh_dir(tmp_path, "novasvg")
    _solid_png(d / "circle.png", (2, 2), (0, 0, 0, 0))
    results = {"circle": {"ok": True, "seconds": 0.002, "error": None},
               "tiger": {"ok": True, "seconds": 0.003, "error": None}}  # claims OK but wrote no PNG
    rb.write_result(d, "novasvg", "novasvg", "v1", _args(), _resolved(), results)
    doc = json.loads((d / "result.json").read_text(encoding="utf-8"))
    assert (doc["engine"], doc["version"], doc["runs"], doc["width"]) == ("novasvg", "v1", 5, 320)
    circle, tiger = doc["samples"]
    assert circle["png"] == "circle.png" and circle["ok"]
    assert tiger["png"] is None and not tiger["ok"] and "no PNG" in tiger["error"]
    assert (tiger["render_w"], tiger["render_h"], tiger["filename"]) == (320, 160, "tiger.svg")


def test_a_rerun_clears_the_engines_old_renders_and_leaves_other_engines_alone(tmp_path):
    (tmp_path / "renders" / "resvg").mkdir(parents=True)
    (tmp_path / "renders" / "resvg" / "keep.png").write_bytes(b"x")
    d = rb.fresh_dir(tmp_path, "novasvg")
    (d / "stale.png").write_bytes(b"x")
    d = rb.fresh_dir(tmp_path, "novasvg")
    assert list(d.iterdir()) == [] and (tmp_path / "renders" / "resvg" / "keep.png").exists()


def test_an_engine_that_cannot_run_leaves_a_folder_saying_why(tmp_path):
    rb.write_unavailable(tmp_path, "thorvg", "thorvg_native_bench not built/provided")
    doc = json.loads((tmp_path / "renders" / "thorvg" / "result.json").read_text(encoding="utf-8"))
    assert doc == {"engine": "thorvg", "unavailable": "thorvg_native_bench not built/provided"}


# --- docs/bench-teaser.js: the landing page's teaser, averaged and drawn in the browser -----------------
# The averaging exists twice on purpose (gather.py for the static report, the JS for the landing page), so
# this runs the real script in Chromium and checks it against gather.py. Skipped where Playwright/Chromium is absent.

import contextlib  # noqa: E402
import functools  # noqa: E402
import http.server  # noqa: E402
import importlib.util  # noqa: E402
import socketserver  # noqa: E402
import threading  # noqa: E402

_DOCS = ROOT / "docs"
_spec = importlib.util.spec_from_file_location("novasvg_generate_index", _DOCS / "generate_index.py")
gi = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gi)


@pytest.fixture(scope="module")
def browser():
    sync_api = pytest.importorskip("playwright.sync_api")
    with sync_api.sync_playwright() as p:
        try:
            b = p.chromium.launch()
        except Exception as exc:  # noqa: BLE001 - no browser installed
            pytest.skip(f"Chromium not available: {exc}")
        yield b
        b.close()


@contextlib.contextmanager
def _site(tmp_path, bench_json=None):
    """A tiny site: index.html = the real teaser shell + script, plus benchmarks/render_benchmark.json if given."""
    js = (_DOCS / "bench-teaser.js").read_text(encoding="utf-8")
    (tmp_path / "index.html").write_text(
        f'<!doctype html><html class="js"><body>{gi.bench_shell()}<script>{js}</script></body></html>', encoding="utf-8")
    if bench_json is not None:
        (tmp_path / "benchmarks").mkdir()
        (tmp_path / "benchmarks" / "render_benchmark.json").write_text(bench_json, encoding="utf-8")
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(tmp_path))
    handler.log_message = lambda *a, **k: None
    with socketserver.TCPServer(("127.0.0.1", 0), handler) as srv:
        threading.Thread(target=srv.serve_forever, daemon=True).start()
        try:
            yield f"http://127.0.0.1:{srv.server_address[1]}/"
        finally:
            srv.shutdown()


_READ_BARS = """c => [...document.querySelectorAll(`#benchmarks [data-chart="${c}"] .bar-row`)]
    .map(r => [r.querySelector('.bar-label').textContent, r.querySelector('.bar-value').firstChild.textContent])"""


def test_the_teaser_averages_in_the_browser_exactly_as_gather_does(out, tmp_path, browser):
    summary = json.loads(gt.gather(out).read_text(encoding="utf-8"))["summary"]
    site = tmp_path / "site"
    site.mkdir()
    with _site(site, (out / "render_benchmark.json").read_text(encoding="utf-8")) as url:
        page = browser.new_page()
        page.goto(url)
        page.wait_for_function("!document.getElementById('benchmarks').hidden", timeout=5000)
        contenders = {k: v for k, v in summary.items() if k != charts.REFERENCE}
        want_acc = [[v["label"], f"{v['rmse']:.1f}"] for v in sorted(contenders.values(), key=lambda v: v["rmse"])]
        want_spd = [[v["label"], f"{v['avg_ms']:.2f}"] for v in sorted(contenders.values(), key=lambda v: v["avg_ms"])]
        assert page.evaluate(_READ_BARS, "accuracy") == want_acc
        assert page.evaluate(_READ_BARS, "speed") == want_spd
        assert page.inner_text("#benchmarks h2") == "Closest to Chromium"
        assert "also the fastest" in page.inner_text("#benchmarks .lede")
        assert "2 samples, median of 5 runs, generated 2026-01-02 03:04 UTC" in page.inner_text("#benchmarks .bench-foot small")
        # the first bar's unit <small> must stay empty: the footer sentence belongs to .bench-foot only
        assert page.inner_text("#benchmarks [data-chart='accuracy'] .bar-value small") == ""


@pytest.mark.parametrize("bench_json", [None, "{not json", '{"matrix": {"circle": {"resvg": {"ok": true, "seconds": 0.1}}}}'],
                         ids=["file-missing", "invalid-json", "no-novasvg"])
def test_the_teaser_is_skipped_when_there_is_no_usable_data(tmp_path, browser, bench_json):
    with _site(tmp_path, bench_json) as url:
        page = browser.new_page()
        page.goto(url)
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(300)
        assert page.evaluate("document.getElementById('benchmarks').hidden") is True
