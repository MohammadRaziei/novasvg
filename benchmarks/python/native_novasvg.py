"""Builds the manifest for, and parses the output of,
../native/novasvg_native_bench.cpp -- the one place novasvg is measured
differently from the other 4 engines (a native binary linking novasvg
directly, instead of a Python binding), because that's how you time a C++
library in-process without either writing a Python C-extension for it or
paying subprocess-per-render cost. See that file's own header comment for
the full rationale.
"""
import subprocess
import tempfile
from pathlib import Path


def run_native_novasvg(native_bin_path, jobs, runs, timeout=120):
    """jobs: list of (name, svg_path, out_png_path, width, height).
    Returns (version_string, {name: {"ok": bool, "seconds": float|None,
    "error": str|None}}). One subprocess call covers the *entire* corpus --
    novasvg pays process-start/library-init cost exactly once here, same as
    every other engine pays it exactly once (on first Python import)."""
    if not native_bin_path or not Path(native_bin_path).is_file():
        raise RuntimeError(f"novasvg_native_bench not found at {native_bin_path!r}")

    with tempfile.NamedTemporaryFile(mode="w", suffix=".tsv", delete=False) as manifest:
        for name, svg_path, out_path, w, h in jobs:
            manifest.write(f"{name}\t{svg_path}\t{out_path}\t{w}\t{h}\n")
        manifest_path = manifest.name

    try:
        proc = subprocess.run(
            [native_bin_path, manifest_path, str(runs)],
            capture_output=True, text=True, timeout=timeout,
        )
    finally:
        Path(manifest_path).unlink(missing_ok=True)

    if proc.returncode != 0:
        raise RuntimeError(f"novasvg_native_bench exit {proc.returncode}: {proc.stderr.strip()[:300]}")

    version = "unknown"
    results = {}
    for line in proc.stdout.splitlines():
        parts = line.split("\t")
        if not parts:
            continue
        if parts[0] == "@@VERSION" and len(parts) >= 2:
            version = parts[1]
            continue
        if len(parts) < 3:
            continue
        name, status = parts[0], parts[1]
        if status == "OK":
            results[name] = {"ok": True, "seconds": float(parts[2]), "error": None}
        else:
            results[name] = {"ok": False, "seconds": None, "error": parts[2] if len(parts) > 2 else "FAIL"}
    return version, results
