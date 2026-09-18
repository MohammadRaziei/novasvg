#!/usr/bin/env python3
"""Runs every engine against every corpus SVG, timing each render and saving
the resulting PNG. Writes results.json consumed by ../report/generate_report.py.

novasvg is rendered by native_novasvg.py (one subprocess call, in-process
C++ over the whole corpus); resvg/lunasvg/cairosvg/thorvg are called
per-file through their Python bindings in engines.py. Both paths land in
the same results.json matrix.

Usage: run_benchmark.py <results_dir> [--runs N] [--width W] [--height H]
"""
import argparse
import base64
import json
import statistics
import sys
import time
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from corpus import corpus_files, intrinsic_size, fit_box  # noqa: E402
from engines import load_engines  # noqa: E402
from native_novasvg import run_native_novasvg  # noqa: E402


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("results_dir")
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--width", type=int, default=320)
    ap.add_argument("--height", type=int, default=320)
    ap.add_argument("--novasvg-native-bench", default=None,
                     help="path to novasvg_native_bench, built by cmake/FetchNovasvg.cmake + native/CMakeLists.txt")
    args = ap.parse_args()

    results_dir = Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)
    renders_dir = results_dir / "renders" / "novasvg"
    renders_dir.mkdir(parents=True, exist_ok=True)

    engines, load_errors = load_engines()
    print(f"Loaded {len(engines)} Python-bound engine(s): {', '.join(engines)}")
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

    # Resolve every sample's aspect-fit render size up front -- native
    # novasvg needs the whole job list before it runs once; the Python
    # engines use the same per-file sizes right below.
    resolved = []
    for name, path, desc in files:
        iw, ih = intrinsic_size(path)
        render_w, render_h = fit_box(iw, ih, args.width, args.height)
        resolved.append((name, path, desc, render_w, render_h))
        report["files"].append({
            "name": name, "filename": path.name, "desc": desc,
            "render_w": render_w, "render_h": render_h,
        })
        report["matrix"][name] = {}

    # --- novasvg: one process, whole corpus, via its own C++ API ---
    if args.novasvg_native_bench:
        jobs = [
            (name, str(path), str(renders_dir / f"{name}.png"), w, h)
            for name, path, desc, w, h in resolved
        ]
        version, native_results = run_native_novasvg(args.novasvg_native_bench, jobs, args.runs)
        report["engines"]["novasvg"] = {"label": "novasvg", "version": version}
        for name, path, desc, w, h in resolved:
            res = native_results.get(name, {"ok": False, "seconds": None, "error": "not run"})
            cell = {"ok": res["ok"], "seconds": res["seconds"], "error": res["error"]}
            if res["ok"]:
                png_path = renders_dir / f"{name}.png"
                cell["png_b64"] = base64.b64encode(png_path.read_bytes()).decode("ascii")
                status = f"{res['seconds']*1000:.2f} ms"
            else:
                status = f"FAIL ({(res['error'] or '')[:60]})"
            report["matrix"][name]["novasvg"] = cell
            print(f"  {name:20s} {'novasvg':10s} {status}")
    else:
        print("No --novasvg-native-bench given -- skipping novasvg entirely.")
        report["engine_load_errors"]["novasvg"] = "novasvg_native_bench not built/provided"

    # --- the other 4: per-file, in-process, via their Python bindings ---
    for name, path, desc, render_w, render_h in resolved:
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
