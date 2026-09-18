#!/usr/bin/env python3
"""Runs every engine against every corpus SVG, timing each render and saving
the resulting PNG. Writes results.json consumed by ../report/generate_report.py.

Usage: run_benchmark.py <results_dir> [--runs N] [--width W] [--height H]
"""
import argparse
import base64
import json
import statistics
import sys
import time
import traceback
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from corpus import corpus_files, intrinsic_size, fit_box  # noqa: E402
from engines import load_engines  # noqa: E402


def time_render(render_fn, svg_path, w, h, runs):
    """Median wall-clock render time over `runs` attempts, plus the PNG
    from the last successful run. ponytail: no warmup discard, no process
    isolation between engines -- fine for relative comparison, not for
    publishable microbenchmark numbers."""
    times = []
    png = None
    for _ in range(runs):
        t0 = time.perf_counter()
        png = render_fn(str(svg_path), w, h)
        times.append(time.perf_counter() - t0)
        if not png:
            raise RuntimeError("render() returned no bytes")
    return statistics.median(times), png


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("results_dir")
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--width", type=int, default=320)
    ap.add_argument("--height", type=int, default=320)
    ap.add_argument("--novasvg-cli", default=None, help="path to the novasvg_cli binary built by cmake/FetchNovasvg.cmake")
    args = ap.parse_args()

    results_dir = Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    engines, load_errors = load_engines(args.novasvg_cli)
    if not engines:
        print("No engines available -- nothing to benchmark.", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded {len(engines)} engine(s): {', '.join(engines)}")
    if load_errors:
        print(f"Unavailable: {', '.join(load_errors)}")

    files = list(corpus_files())
    print(f"Corpus: {len(files)} file(s)")

    report = {
        "width": args.width,
        "height": args.height,
        "runs": args.runs,
        "engines": {k: {"label": e.label, "version": safe_version(e)} for k, e in engines.items()},
        "engine_load_errors": load_errors,
        "files": [],
        "matrix": {},  # matrix[file_name][engine_key] = {ok, seconds, error, png_b64}
    }

    for name, path, desc in files:
        iw, ih = intrinsic_size(path)
        render_w, render_h = fit_box(iw, ih, args.width, args.height)
        report["files"].append({
            "name": name, "filename": path.name, "desc": desc,
            "render_w": render_w, "render_h": render_h,
        })
        report["matrix"][name] = {}
        for key, eng in engines.items():
            cell = {"ok": False, "seconds": None, "error": None}
            try:
                seconds, png = time_render(eng.render, path, render_w, render_h, args.runs)
                cell["ok"] = True
                cell["seconds"] = seconds
                cell["png_b64"] = base64.b64encode(png).decode("ascii")
                status = f"{seconds*1000:.2f} ms"
            except Exception as exc:  # noqa: BLE001 - recorded per-cell, run must continue
                cell["error"] = f"{type(exc).__name__}: {exc}"
                status = f"FAIL ({cell['error'][:60]})"
            report["matrix"][name][key] = cell
            print(f"  {name:20s} {key:10s} {status}")

    out_path = results_dir / "results.json"
    out_path.write_text(json.dumps(report))
    print(f"\nWrote {out_path} ({out_path.stat().st_size} bytes)")


def safe_version(engine):
    try:
        return engine.version()
    except Exception:  # noqa: BLE001
        return "unknown"


if __name__ == "__main__":
    main()
