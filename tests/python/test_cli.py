"""The native CLI as the Python package ships it: it is found, and its JSON output is JSON."""
import json
import subprocess

import pytest

from novasvg import native_cli

SVG = (
    '<svg xmlns="http://www.w3.org/2000/svg" width="40" height="20">'
    '<path id="a&quot;b" fill="red" d="M0 0H10V10H0Z"/><path d="M20 0H30V10H20Z"/></svg>'
)


@pytest.fixture(scope="module")
def cli():
    return str(native_cli.novasvg_cli_path())


@pytest.fixture()
def svg(tmp_path):
    path = tmp_path / "in.svg"
    path.write_text(SVG, encoding="utf-8")
    return path


def run(cli, *args):
    return subprocess.run([cli, *map(str, args)], capture_output=True, text=True, check=True).stdout


def test_info_json_is_valid(cli, svg):
    info = json.loads(run(cli, "info", svg, "--json"))
    assert info["width"] == 40 and info["height"] == 20
    assert info["file"] == str(svg)


def test_query_json_is_valid_and_escaped(cli, svg):
    result = json.loads(run(cli, "query", "path", svg, "--json"))
    assert result["count"] == 2
    first = result["elements"][0]
    assert first["attributes"]["id"] == 'a"b'
    assert set(first["global_bbox"]) == {"x", "y", "width", "height"}


def test_batch_converts_every_svg(cli, svg, tmp_path):
    out = tmp_path / "out"
    run(cli, "batch", svg.parent, out)
    assert (out / "in.png").exists()
