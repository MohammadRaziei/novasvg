#!/usr/bin/env python3
"""Gathers the per-engine results into one render_benchmark.json.

Each engine has its own folder, <out_dir>/renders/<engine>/, written by ../python/run_benchmark.py:
the PNGs it rendered and a result.json about that engine alone. This script only merges them:

    <out_dir>/renders/<engine>/result.json  (x N)   ->   <out_dir>/render_benchmark.json

plus the one thing that can't be known by any single engine: each render's RMSE against the
Chromium (ground_truth) render of the same sample, computed from the PNGs on disk, and the
per-engine summary the report's "At a glance" is drawn from. It never runs a renderer.

It also merges the hand-kept ../ratings.json (the Human defect grade, HDG: 0 accurate, 1 defective,
2 broken) into every cell as `hdg` / `hdg_note`, and counts the grades per engine in the summary.
RMSE is a poor judge of what a person sees (a missing label barely moves it), so the grade is the
headline accuracy measure; a render with no entry in ratings.json is left unrated (`hdg` is null).

Usage: gather.py <out_dir> [ratings.json]
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


HDG_GRADES = (0, 1, 2)
DEFAULT_RATINGS = HERE.parent / "ratings.json"


def load_ratings(path):
    """(metric description, {sample: {engine: {"hdg": grade, "note": text}}}) from ratings.json, or (None, {})
    when there is no such file (the report then simply has no grades)."""
    path = Path(path)
    if not path.is_file():
        return None, {}
    doc = json.loads(path.read_text(encoding="utf-8"))
    metric = {k: doc[k] for k in ("metric", "name", "abbr", "scale", "reference", "rated_by") if k in doc}
    return metric, doc.get("ratings", {})


def apply_ratings(matrix, ratings, engines):
    """Copies each valid grade into its matrix cell. A grade for a sample or engine that is not in this run, or
    one that is not 0/1/2, is reported on stderr and ignored rather than silently shown."""
    for sample, row in ratings.items():
        for engine, entry in row.items():
            cell = matrix.get(sample, {}).get(engine)
            grade = entry.get("hdg") if isinstance(entry, dict) else None
            if cell is None:
                print(f"gather: ratings.json rates {sample}/{engine}, which is not in this run; ignored", file=sys.stderr)
            elif grade not in HDG_GRADES:
                print(f"gather: ratings.json: {sample}/{engine}: hdg must be 0, 1 or 2, got {grade!r}; ignored", file=sys.stderr)
            else:
                cell["hdg"] = grade
                if entry.get("note"):
                    cell["hdg_note"] = entry["note"]


def hdg_counts(matrix, engine):
    """{"0": n, "1": n, "2": n, "rated": n, "total": n} for one engine; unrated renders are only in `total`."""
    cells = [row[engine] for row in matrix.values() if engine in row]
    counts = {str(g): sum(1 for c in cells if c.get("hdg") == g) for g in HDG_GRADES}
    counts["rated"] = sum(counts[str(g)] for g in HDG_GRADES)
    counts["total"] = len(cells)
    return counts


def _order(keys):
    known = [k for k in charts.ENGINE_ORDER if k in keys]
    return known + sorted(k for k in keys if k not in known)


def gather(out_dir, ratings_path=None):
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

    metric, ratings = load_ratings(ratings_path) if ratings_path else (None, {})
    apply_ratings(matrix, ratings, engines)

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
        if metric and k != charts.REFERENCE:
            summary[k]["hdg"] = hdg_counts(matrix, k)

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
        "metrics": {metric.get("metric", "hdg"): metric} if metric else {},
        "summary": summary,
        "files": files,
        "matrix": matrix,
    }
    path = out_dir / "render_benchmark.json"
    path.write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    return path


def main():
    if len(sys.argv) not in (2, 3):
        print("usage: gather.py <out_dir> [ratings.json]", file=sys.stderr)
        sys.exit(1)
    path = gather(sys.argv[1], sys.argv[2] if len(sys.argv) == 3 else DEFAULT_RATINGS)
    print(f"Wrote {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
