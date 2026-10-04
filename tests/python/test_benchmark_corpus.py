"""The benchmark's inputs and metric: which SVGs it renders, at what size, and how RMSE is scored.

These guard the *data* side of benchmarks/ (corpus discovery, intrinsic sizing, the RMSE function) so a
sample that is silently dropped or measured at the wrong size can't make the report look better or worse
than novasvg really is.
"""
import re
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "benchmarks" / "python"))

import corpus  # noqa: E402  (benchmarks/python/corpus.py)

MERMAID_DIR = ROOT / "data" / "mermaid"
MERMAID_SVGS = sorted(MERMAID_DIR.rglob("*.mmdc.svg"))


def _write(tmp_path, text):
    path = tmp_path / "t.svg"
    path.write_text(text, encoding="utf-8")
    return path


# --- mermaid data ----------------------------------------------------------------------------------

SAMPLE_NAME = re.compile(r"^(\d{2})-[a-z0-9]+(?:-[a-z0-9]+)*\.(mmd|mmdc\.svg)$")


def test_there_are_at_least_forty_mermaid_samples():
    assert len(MERMAID_SVGS) >= 40


def test_samples_are_numbered_in_order_without_gaps_or_leftovers():
    files = sorted(p.name for p in MERMAID_DIR.glob("*") if p.name != "COVERAGE.md")
    bad = [name for name in files if not SAMPLE_NAME.match(name)]
    assert not bad, f"not named NN-some-name.mmd / .mmdc.svg: {bad}"
    numbers = sorted({int(SAMPLE_NAME.match(name).group(1)) for name in files})
    assert numbers == list(range(1, len(numbers) + 1)), f"numbering has gaps or does not start at 01: {numbers}"
    for n in numbers:  # exactly one source and one render per number
        assert len(list(MERMAID_DIR.glob(f"{n:02d}-*.mmd"))) == 1, n
        assert len(list(MERMAID_DIR.glob(f"{n:02d}-*.mmdc.svg"))) == 1, n


def test_no_issue_numbers_or_other_project_names_in_the_samples_and_their_docs():
    banned = re.compile(r"mermaidx|\bissues?\s*#?\d+|\(#\d+\)", re.I)
    texts = [*MERMAID_DIR.glob("*"), ROOT / "benchmarks" / "README.md", ROOT / "COMPARISON.md", ROOT / "checklist.md"]
    offenders = [f"{p.name}: {m.group(0)}" for p in texts if p.is_file()
                 for m in [banned.search(p.read_text(encoding="utf-8"))] if m]
    assert not offenders, offenders


@pytest.mark.parametrize("svg", MERMAID_SVGS, ids=lambda p: p.relative_to(MERMAID_DIR).as_posix())
def test_every_mermaid_svg_keeps_its_source(svg):
    source = svg.with_name(svg.name.replace(".mmdc.svg", ".mmd"))
    assert source.is_file(), f"{svg.name} has no {source.name} next to it"
    assert source.read_text(encoding="utf-8").strip(), f"{source.name} is empty"


@pytest.mark.parametrize("mmd", sorted(MERMAID_DIR.rglob("*.mmd")), ids=lambda p: p.relative_to(MERMAID_DIR).as_posix())
def test_every_mermaid_source_has_its_mmdc_render(mmd):
    assert mmd.with_name(mmd.stem + ".mmdc.svg").is_file(), f"{mmd.name} has no .mmdc.svg next to it"


@pytest.mark.parametrize("svg", MERMAID_SVGS, ids=lambda p: p.relative_to(MERMAID_DIR).as_posix())
def test_every_mermaid_svg_is_an_actual_diagram_not_mermaids_error_graphic(svg):
    head = svg.read_text(encoding="utf-8")[:2000]
    assert "<svg" in head
    assert 'aria-roledescription="error"' not in head, "mmdc rendered a syntax-error diagram"


def test_coverage_notes_only_describe_samples_that_exist():
    # data/mermaid/COVERAGE.md has one table row per sample, `| 02 | flowchart ...`; none may outlive its files
    rows = re.findall(r"^\|\s*(\d+)\s*\|", (MERMAID_DIR / "COVERAGE.md").read_text(encoding="utf-8"), re.M)
    assert rows, "COVERAGE.md has no sample rows"
    missing = [n for n in rows if not list(MERMAID_DIR.glob(f"{n}-*.mmdc.svg"))]
    assert not missing, f"COVERAGE.md describes samples with no files: {missing}"


# --- the corpus ------------------------------------------------------------------------------------

def test_corpus_contains_every_mermaid_svg_without_a_hand_kept_list():
    in_corpus = {path.resolve() for _, path, _ in corpus.corpus_files()}
    missing = [p.name for p in MERMAID_SVGS if p.resolve() not in in_corpus]
    assert not missing, f"not in the benchmark corpus: {missing}"


def test_corpus_names_are_unique_and_described():
    entries = list(corpus.corpus_files())
    names = [name for name, _, _ in entries]
    assert len(names) == len(set(names)), "two corpus entries share a name"
    assert all(desc.strip() for _, _, desc in entries)


def test_corpus_keeps_the_non_mermaid_samples():
    names = {name for name, _, _ in corpus.corpus_files()}
    assert {"circle", "tiger", "clip-mask", "filter-primitives", "base64-image"} <= names


def test_mermaid_entries_say_which_diagram_type_they_are():
    descs = {path.name: desc.lower() for _, path, desc in corpus.corpus_files()}

    def descriptions_of(suffix):
        found = [d for name, d in descs.items() if name.endswith(suffix)]
        assert found, f"no sample ends with {suffix}"
        return found

    assert all("venn" in d for d in descriptions_of("-venn.mmdc.svg"))      # there are two venn samples
    assert all("gitgraph" in d for d in descriptions_of("-gitgraph.mmdc.svg"))
    assert all("sankey" in d for d in descriptions_of("-sankey.mmdc.svg"))


def test_corpus_entry_names_are_the_sample_names_with_a_mermaid_prefix():
    names = {name for name, path, _ in corpus.corpus_files() if path.is_relative_to(MERMAID_DIR)}
    assert names == {"mermaid-" + p.name[: -len(".mmdc.svg")] for p in MERMAID_SVGS}


# --- intrinsic size --------------------------------------------------------------------------------

@pytest.mark.parametrize("svg", MERMAID_SVGS, ids=lambda p: p.relative_to(MERMAID_DIR).as_posix())
def test_mermaid_intrinsic_size_is_a_real_size_never_the_1x1_fallback(svg):
    # the ROOT tag only: diagrams can nest other <svg viewBox=...> elements (icons) further down
    head = re.search(r"<svg\b[^>]*>", svg.read_text(encoding="utf-8")).group(0)
    view_box = re.search(r'viewBox="\s*[-\d.e]+\s+[-\d.e]+\s+([-\d.e]+)\s+([-\d.e]+)', head)
    assert view_box, f"{svg.name} has no viewBox on its root"  # mmdc writes width="100%" and a viewBox
    expected = (float(view_box.group(1)), float(view_box.group(2)))
    assert expected != (1.0, 1.0)
    assert corpus.intrinsic_size(svg) == expected


def test_intrinsic_size_reads_pixel_width_and_height(tmp_path):
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" width="120" height="80"/>')
    assert corpus.intrinsic_size(svg) == (120.0, 80.0)


def test_intrinsic_size_ignores_percentages_and_uses_the_viewbox(tmp_path):
    # mermaid's journey diagram: width="100%" next to a pixel height -- 100 is NOT a width in px
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="565" viewBox="0 0 640 565"/>')
    assert corpus.intrinsic_size(svg) == (640.0, 565.0)
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="0 0 30 10"/>')
    assert corpus.intrinsic_size(svg) == (30.0, 10.0)


def test_intrinsic_size_uses_the_css_width_and_height_of_the_style_attribute(tmp_path):
    # a root sized only by inline CSS (no viewBox, no height) -- and a max-width must not be mistaken for width
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" width="100%" '
                           'style="max-width: 999px; width: 784px; height: 235px; background-color: white;"/>')
    assert corpus.intrinsic_size(svg) == (784.0, 235.0)
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" style="max-width: 999px;" viewBox="0 0 50 20"/>')
    assert corpus.intrinsic_size(svg) == (50.0, 20.0)


def test_css_size_beats_the_width_and_height_attributes_like_in_a_browser(tmp_path):
    svg = _write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10" style="width:40px;height:20px"/>')
    assert corpus.intrinsic_size(svg) == (40.0, 20.0)


def test_intrinsic_size_falls_back_to_a_square_when_nothing_is_declared(tmp_path):
    assert corpus.intrinsic_size(_write(tmp_path, '<svg xmlns="http://www.w3.org/2000/svg"/>')) == (1.0, 1.0)


def test_fit_box_keeps_the_aspect_ratio_inside_the_box():
    assert corpus.fit_box(640, 320, 320, 320) == (320, 160)
    assert corpus.fit_box(100, 400, 320, 320) == (80, 320)
    assert corpus.fit_box(1, 1, 320, 320) == (320, 320)
    w, h = corpus.fit_box(1000, 1, 320, 320)
    assert (w, h) == (320, 1)  # never collapses to zero pixels


# --- the RMSE metric -------------------------------------------------------------------------------

def _png(color, size=(4, 3)):
    import io

    from PIL import Image

    buf = io.BytesIO()
    Image.new("RGBA", size, color).save(buf, format="PNG")
    return buf.getvalue()


@pytest.fixture(scope="module")
def compute_rmse():
    pytest.importorskip("numpy")
    pytest.importorskip("PIL")
    import compare

    return compare.compute_rmse


def test_rmse_of_identical_images_is_zero(compute_rmse):
    assert compute_rmse(_png((10, 20, 30, 255)), _png((10, 20, 30, 255))) == 0.0


def test_rmse_is_on_the_0_255_scale_over_all_four_channels(compute_rmse):
    # black vs white, both opaque: R, G, B differ by 255 and A by 0 -> sqrt(3 * 255^2 / 4)
    assert compute_rmse(_png((0, 0, 0, 255)), _png((255, 255, 255, 255))) == pytest.approx(255 * 0.75 ** 0.5)
    # a single channel off by 10 across the image -> sqrt(10^2 / 4) = 5
    assert compute_rmse(_png((0, 0, 0, 255)), _png((10, 0, 0, 255))) == pytest.approx(5.0)


def test_rmse_is_symmetric(compute_rmse):
    a, b = _png((5, 50, 200, 255)), _png((90, 10, 0, 128))
    assert compute_rmse(a, b) == pytest.approx(compute_rmse(b, a))


def test_rmse_weights_a_few_badly_wrong_pixels_more_than_widespread_noise(compute_rmse):
    import io

    from PIL import Image

    def with_pixels(changes):
        img = Image.new("RGBA", (10, 10), (0, 0, 0, 255))
        for xy, value in changes.items():
            img.putpixel(xy, value)
        buf = io.BytesIO()
        img.save(buf, format="PNG")
        return buf.getvalue()

    base = with_pixels({})
    one_bad_pixel = with_pixels({(0, 0): (255, 255, 255, 255)})  # 1 pixel completely wrong
    all_slightly_off = with_pixels({(x, y): (3, 3, 3, 255) for x in range(10) for y in range(10)})
    assert compute_rmse(base, one_bad_pixel) > compute_rmse(base, all_slightly_off)


def test_rmse_refuses_images_of_different_sizes(compute_rmse):
    with pytest.raises(ValueError, match="size mismatch"):
        compute_rmse(_png((0, 0, 0, 255), (4, 3)), _png((0, 0, 0, 255), (3, 4)))


# --- ground truth: which Chromium Playwright launches ----------------------------------------------

def test_chromium_launches_playwrights_own_build_unless_told_otherwise():
    import ground_truth

    assert ground_truth.chromium_launch_options({}) == {}


def test_chromium_executable_and_flags_can_be_pinned_from_the_environment():
    import ground_truth

    env = {"NOVASVG_BENCH_CHROMIUM": "/opt/chrome/chrome", "NOVASVG_BENCH_CHROMIUM_ARGS": "--no-sandbox  --disable-gpu"}
    assert ground_truth.chromium_launch_options(env) == {
        "executable_path": "/opt/chrome/chrome",
        "args": ["--no-sandbox", "--disable-gpu"],
    }


def test_chromium_args_alone_do_not_invent_an_executable():
    import ground_truth

    assert ground_truth.chromium_launch_options({"NOVASVG_BENCH_CHROMIUM_ARGS": "--no-sandbox"}) == {"args": ["--no-sandbox"]}
    assert ground_truth.chromium_launch_options({"NOVASVG_BENCH_CHROMIUM": ""}) == {}
