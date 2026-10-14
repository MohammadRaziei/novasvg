#!/usr/bin/env python3
"""Gathers the per-engine results into one render_benchmark.json.

Each engine has its own folder, <out_dir>/renders/<engine>/, written by ../python/run_benchmark.py:
the PNGs it rendered and a result.json about that engine alone. This script only merges them:

    <out_dir>/renders/<engine>/result.json  (x N)   ->   <out_dir>/render_benchmark.json

plus the one thing that can't be known by any single engine: each render's RMSE against the
Chromium (ground_truth) render of the same sample, computed from the PNGs on disk, and the
per-engine summary the report's "At a glance" is drawn from. It never runs a renderer.

Usage: gather.py <out_dir>
"""
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "python"))
import charts  # noqa: E402
from compare import compute_rmse  # noqa: E402


def load_engine_results(renders_dir):
    """{engine key: parsed result.json} for every <renders_dir>/<key>/result.json, with a warning for a
    folder that has none (a run that died half way) or one that can't be parsed."""
    found = {}
    for d in sorted(p for p in Path(renders_dir).glob("*") if p.is_dir()):
        path = d / "result.json"
        if not path.is_file():
            print(f"gather: {d.name}: no result.json, skipped (did its run finish?)", file=sys.stderr)
            continue
        try:
            found[d.name] = json.loads(path.read_text(encoding="utf-8"))
        except ValueError as exc:
            print(f"gather: {d.name}: result.json is not valid JSON ({exc}), skipped", file=sys.stderr)
    return found


def _order(keys):
    known = [k for k in charts.ENGINE_ORDER if k in keys]
    return known + sorted(k for k in keys if k not in known)


def gather(out_dir):
    out_dir = Path(out_dir)
    found = load_engine_results(out_dir / "renders")
    errors = {k: r["unavailable"] for k, r in found.items() if "unavailable" in r}
    engines = _order([k for k, r in found.items() if "unavailable" not in r])
    if not engines:
        raise SystemExit(f"gather: no engine results under {out_dir / 'renders'} -- run run_benchmark.py first")

    # the sample list is the union of what the engines ran, in the order first seen
    files, seen = [], set()
    for k in engines:
        for s in found[k]["samples"]:
            if s["name"] not in seen:
                seen.add(s["name"])
                files.append({f: s[f] for f in ("name", "filename", "desc", "render_w", "render_h") if f in s})

    matrix = {f["name"]: {} for f in files}
    for k in engines:
        for s in found[k]["samples"]:
            matrix[s["name"]][k] = {
                "ok": s["ok"], "seconds": s["seconds"], "rmse": None, "error": s.get("error"),
                "png": f"renders/{k}/{s['png']}" if s.get("png") else None,
            }

    # RMSE against the ground-truth render of the same sample
    ref = charts.REFERENCE
    for name, row in matrix.items():
        gt = row.get(ref)
        if not (gt and gt["png"]):
            continue
        gt_png = (out_dir / gt["png"]).read_bytes()
        for k, cell in row.items():
            if k == ref or not cell["png"]:
                continue
            try:
                cell["rmse"] = compute_rmse(gt_png, (out_dir / cell["png"]).read_bytes())
            except Exception as exc:  # noqa: BLE001 - recorded, never blocks the gather
                cell["rmse_error"] = str(exc)

    summary = {}
    for k in engines:
        cells = [row[k] for row in matrix.values() if k in row and row[k]["ok"]]
        t = [c["seconds"] for c in cells]
        r = [c["rmse"] for c in cells if c["rmse"] is not None]
        summary[k] = {
            "label": found[k]["label"], "version": found[k]["version"],
            "ok": len(cells), "total": len(files),
            "avg_ms": sum(t) / len(t) * 1000 if t else None,
            "total_ms": sum(t) * 1000 if t else None,
            "rmse": sum(r) / len(r) if r else None,
        }

    # one run configuration for the page; engines run separately may differ, so say so
    for field in ("runs", "width", "height"):
        values = {found[k][field] for k in engines}
        if len(values) > 1:
            print(f"gather: engines were run with different {field} {sorted(values)}; "
                  f"the report will show the first", file=sys.stderr)
    first = found[engines[0]]
    data = {
        "schema": 1,
        "generated": max(found[k].get("generated") or "" for k in engines)
                     or datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        "width": first["width"], "height": first["height"], "runs": first["runs"],
        "engine_load_errors": errors,
        "summary": summary,
        "files": files,
        "matrix": matrix,
    }
    path = out_dir / "render_benchmark.json"
    path.write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    return path


def main():
    if len(sys.argv) != 2:
        print("usage: gather.py <out_dir>", file=sys.stderr)
        sys.exit(1)
    path = gather(sys.argv[1])
    print(f"Wrote {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
