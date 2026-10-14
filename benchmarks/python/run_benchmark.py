#!/usr/bin/env python3
"""Runs every engine against every corpus SVG, timing each render and saving the PNG.

Every engine gets a folder of its own, <out_dir>/renders/<engine>/, holding its PNGs and a
result.json that describes that engine's run alone (version, timings, which samples failed).
Nothing here knows about the other engines' results, so one engine's folder can be redone
without touching the rest. ../report/gather.py merges the folders into render_benchmark.json
(and scores every render against Chromium's).

5 of 6 engines (novasvg, resvg, lunasvg, thorvg, nanosvg) are driven by
native_*_bench binaries -- one subprocess call each, covering the whole
corpus in-process (see native_bench.py, ../native/*.cpp). cairosvg has no
native counterpart and is called per-file through its Python binding in
engines.py. Chromium (ground_truth) is driven through Playwright.

Usage: run_benchmark.py <out_dir> [--runs N] [--width W] [--height H] [--<engine>-native-bench PATH ...]
"""
import argparse
import json
import shutil
import statistics
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from corpus import corpus_files, intrinsic_size, fit_box  # noqa: E402
from engines import load_engines  # noqa: E402
from native_bench import run_native_bench  # noqa: E402
from ground_truth import render_ground_truth  # noqa: E402

# Every native engine this driver knows how to call, in report display
# order. Each maps to a --<key>-native-bench CLI flag and a
# <out_dir>/renders/<key>/ output directory.
NATIVE_ENGINES = ["novasvg", "resvg", "lunasvg", "thorvg", "nanosvg"]
GROUND_TRUTH = "ground_truth"


def time_render(render_fn, svg_path, w, h, runs):
    """Median wall-clock render time over `runs` attempts, plus the PNG
    from the last successful run. ponytail: no warmup discard -- fine for
    relative comparison, not for publishable microbenchmark numbers."""
    times = []
    png = None
    for _ in range(runs):
        t0 = time.perf_counter()
        png = render_fn(str(svg_path), w, h)
        times.append(time.perf_counter() - t0)
        if not png:
            raise RuntimeError("render() returned no bytes")
    return statistics.median(times), png


def fresh_dir(out_dir, key):
    """<out_dir>/renders/<key>/, emptied first so a rerun can't leave renders of samples that no longer exist."""
    d = Path(out_dir) / "renders" / key
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    return d


def write_unavailable(out_dir, key, reason):
    """The engine could not run: leave a folder that says why (gather reports it as unavailable)."""
    d = fresh_dir(out_dir, key)
    (d / "result.json").write_text(json.dumps({"engine": key, "unavailable": str(reason)}, indent=1) + "\n", encoding="utf-8")
    print(f"  {key}: unavailable ({str(reason)[:80]})")


def write_result(d, key, label, version, args, resolved, results):
    """d/result.json for one engine. results: {sample name: {"ok", "seconds", "error"}}; the PNGs are already in d."""
    samples = []
    for name, path, desc, w, h in resolved:
        res = dict(results.get(name) or {"ok": False, "seconds": None, "error": "not run"})
        if res["ok"] and not (d / f"{name}.png").is_file():
            res = {"ok": False, "seconds": None, "error": "engine reported OK but wrote no PNG"}
        samples.append({
            "name": name, "filename": path.name, "desc": desc, "render_w": w, "render_h": h,
            "ok": res["ok"], "seconds": res["seconds"], "error": res["error"],
            "png": f"{name}.png" if res["ok"] else None,
        })
        status = f"{res['seconds'] * 1000:.2f} ms" if res["ok"] else f"FAIL ({(res['error'] or '')[:60]})"
        print(f"  {name:20s} {key:12s} {status}")
    doc = {
        "engine": key, "label": label, "version": version,
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        "runs": args.runs, "width": args.width, "height": args.height,
        "samples": samples,
    }
    (d / "result.json").write_text(json.dumps(doc, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out_dir")
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--width", type=int, default=320)
    ap.add_argument("--height", type=int, default=320)
    for key in NATIVE_ENGINES:
        ap.add_argument(f"--{key}-native-bench", default=None,
                         help=f"path to {key}_native_bench, built by cmake/Fetch{key.capitalize()}.cmake")
    args = ap.parse_args()
    out_dir = Path(args.out_dir)

    py_engines, load_errors = load_engines()
    print(f"Loaded {len(py_engines)} Python-bound engine(s): {', '.join(py_engines)}")
    files = list(corpus_files())
    print(f"Corpus: {len(files)} file(s)")

    # Resolve every sample's aspect-fit render size up front -- native
    # engines need the whole job list before they run once each; cairosvg
    # uses the same per-file sizes right below.
    resolved = []
    for name, path, desc in files:
        iw, ih = intrinsic_size(path)
        render_w, render_h = fit_box(iw, ih, args.width, args.height)
        resolved.append((name, path, desc, render_w, render_h))

    # --- native engines: one process each, whole corpus, via their own C/C++ API ---
    for key in NATIVE_ENGINES:
        bin_path = getattr(args, f"{key}_native_bench")
        if not bin_path:
            write_unavailable(out_dir, key, f"{key}_native_bench not built/provided")
            continue
        d = fresh_dir(out_dir, key)
        jobs = [(name, str(path), str(d / f"{name}.png"), w, h) for name, path, desc, w, h in resolved]
        version, results = run_native_bench(bin_path, key, jobs, args.runs)
        write_result(d, key, key, version, args, resolved, results)

    # --- cairosvg: per-file, in-process, via its Python binding ---
    for key, reason in load_errors.items():
        write_unavailable(out_dir, key, reason)
    for key, eng in py_engines.items():
        d = fresh_dir(out_dir, key)
        results = {}
        for name, path, desc, w, h in resolved:
            try:
                seconds, png = time_render(eng.render, path, w, h, args.runs)
                (d / f"{name}.png").write_bytes(png)
                results[name] = {"ok": True, "seconds": seconds, "error": None}
            except Exception as exc:  # noqa: BLE001 - recorded per-sample, run must continue
                results[name] = {"ok": False, "seconds": None, "error": f"{type(exc).__name__}: {exc}"}
        write_result(d, key, eng.label, safe_version(eng), args, resolved, results)

    # --- ground truth: Chromium via Playwright, one browser, whole corpus. Best-effort, never blocks the run ---
    try:
        d = fresh_dir(out_dir, GROUND_TRUTH)
        jobs = [(name, str(path), str(d / f"{name}.png"), w, h) for name, path, desc, w, h in resolved]
        version, results = render_ground_truth(jobs)
        write_result(d, GROUND_TRUTH, "Chromium (ground truth)", version, args, resolved, results)
    except Exception as exc:  # noqa: BLE001
        write_unavailable(out_dir, GROUND_TRUTH, f"Chromium/Playwright unavailable: {exc}")

    print(f"\nWrote per-engine results under {out_dir / 'renders'}")


def safe_version(engine):
    try:
        return engine.version()
    except Exception:  # noqa: BLE001
        return "unknown"


if __name__ == "__main__":
    main()
