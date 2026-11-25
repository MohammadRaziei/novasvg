"""A mask or a clip mask in a PDF is a soft mask (vector), and the page looks like the raster render of the same SVG.

The PDF is rasterized with pdfium (pypdfium2) at 96 dpi, so one CSS px is one pixel, and compared with
Document.render_to_bitmap(). Anti-aliasing differs a little between the two rasterizers, hence the tolerances.
"""

import io

import pytest

import novasvg

np = pytest.importorskip("numpy")
pdfium = pytest.importorskip("pypdfium2")
Image = pytest.importorskip("PIL.Image")

HEAD = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100" viewBox="0 0 100 100">'
GRADIENT = ('<linearGradient id="g"><stop offset="0" stop-color="#fff"/>'
            '<stop offset="1" stop-color="#000"/></linearGradient>')
TWO_CIRCLES = '<clipPath id="c"><circle cx="35" cy="50" r="25"/><circle cx="65" cy="50" r="25"/></clipPath>'


def _both(svg):
    doc = novasvg.Document.load_from_data(svg)
    pdf = doc.render_to_pdf()
    page = pdfium.PdfDocument(pdf)[0].render(scale=96 / 72).to_pil().convert("RGB")
    raster = Image.open(io.BytesIO(doc.render_to_bitmap(100, 100, 0xFFFFFFFF).to_png())).convert("RGB")
    assert page.size == raster.size
    return pdf, np.asarray(page).astype(float), np.asarray(raster).astype(float)


def _check(svg, *, mean=3.0, edge_fraction=0.01):
    pdf, page, raster = _both(svg)
    diff = np.abs(page - raster)
    assert diff.mean() < mean
    assert (diff.max(axis=2) > 64).mean() < edge_fraction
    return pdf


class TestLooksLikeTheRaster:
    def test_luminance_mask_with_a_gradient(self):
        pdf = _check(HEAD + f'<defs>{GRADIENT}<mask id="m"><rect width="100" height="100" fill="url(#g)"/></mask></defs>'
                     '<rect width="100" height="100" fill="#06c" mask="url(#m)"/></svg>')
        assert b"/S /Luminosity" in pdf and b"/Subtype /Image" not in pdf

    def test_alpha_mask(self):
        pdf = _check(HEAD + '<defs><mask id="m" style="mask-type:alpha"><circle cx="50" cy="50" r="30" fill="#000" '
                     'fill-opacity=".5"/></mask></defs><rect width="100" height="100" fill="#c30" mask="url(#m)"/></svg>')
        assert b"/S /Alpha" in pdf and b"/Subtype /Image" not in pdf

    def test_clip_path_of_two_shapes_is_their_union(self):
        pdf = _check(HEAD + f'<defs>{TWO_CIRCLES}</defs><rect width="100" height="100" fill="#093" clip-path="url(#c)"/></svg>')
        assert b"/S /Alpha" in pdf and b"/Subtype /Image" not in pdf

    def test_clip_mask_over_a_gradient(self):
        _check(HEAD + f'<defs>{GRADIENT}{TWO_CIRCLES}</defs>'
               '<rect width="100" height="100" fill="url(#g)" clip-path="url(#c)"/></svg>')

    def test_mask_region_in_user_space(self):
        _check(HEAD + '<defs><mask id="m" x="20" y="20" width="60" height="60" maskUnits="userSpaceOnUse">'
               '<rect width="100" height="100" fill="#fff"/></mask></defs>'
               '<rect width="100" height="100" fill="#06c" mask="url(#m)"/></svg>', mean=1.0)

    def test_mask_content_in_object_bounding_box_units(self):
        _check(HEAD + '<defs><mask id="m" maskContentUnits="objectBoundingBox"><circle cx=".5" cy=".5" r=".4" fill="#fff"/>'
               '</mask></defs><rect x="10" y="10" width="80" height="80" fill="#06c" mask="url(#m)"/></svg>')

    def test_mask_with_group_opacity(self):
        _check(HEAD + '<defs><mask id="m"><circle cx="50" cy="50" r="35" fill="#fff"/></mask></defs>'
               '<g opacity=".5" mask="url(#m)"><rect width="100" height="100" fill="#06c"/>'
               '<circle cx="50" cy="50" r="20" fill="#c30"/></g></svg>')

    def test_what_a_soft_mask_cannot_say_still_looks_right(self):
        # a mask with a mask of its own: the bitmap fallback
        pdf = _check(HEAD + '<defs><mask id="inner"><circle cx="50" cy="50" r="30" fill="#fff"/></mask>'
                     '<mask id="outer" mask="url(#inner)"><rect width="100" height="100" fill="#fff"/></mask></defs>'
                     '<rect width="100" height="100" fill="#06c" mask="url(#outer)"/></svg>')
        assert b"/Subtype /Image" in pdf


class TestGradientsInAGroup:
    """A gradient inside a transparency group used to be shrunk to three quarters of its box (the page's px -> pt scale
    was applied to a pattern that already sat in the group's pixel space)."""

    def test_linear_gradient_in_an_opacity_group(self):
        _check(HEAD + f'<defs>{GRADIENT}</defs><g opacity=".999"><rect width="100" height="100" fill="url(#g)"/></g></svg>', mean=2.5)

    def test_radial_gradient_in_a_blend_group(self):
        _check(HEAD + '<defs><radialGradient id="r"><stop offset="0" stop-color="#fc0"/><stop offset="1" stop-color="#06c"/>'
               '</radialGradient></defs><rect width="100" height="100" fill="#eee"/>'
               '<g style="mix-blend-mode:multiply"><circle cx="50" cy="50" r="45" fill="url(#r)"/></g></svg>', mean=2.5)

    def test_the_far_end_of_the_gradient_is_reached(self):
        _, page, _ = _both(HEAD + f'<defs>{GRADIENT}</defs><g opacity=".999"><rect width="100" height="100" fill="url(#g)"/></g></svg>')
        assert page[50, 80, 0] > 10   # it was already black at x = 75
        assert page[50, 98, 0] < 12


class TestKnownDifference:
    @pytest.mark.parametrize("fill", ["#0f0", "#f00", "#00f"])
    def test_a_saturated_colour_in_a_luminance_mask(self, fill):
        # A PDF luminosity mask weighs R, G, B as 0.30 / 0.59 / 0.11, an SVG luminance mask as 0.2125 / 0.7154 / 0.0721.
        # Grey, black and white (and so every gradient between them) agree; a saturated colour differs by up to ~13 %.
        _, page, raster = _both(HEAD + f'<defs><mask id="m"><rect width="100" height="100" fill="{fill}"/></mask></defs>'
                                '<rect width="100" height="100" fill="#000" mask="url(#m)"/></svg>')
        assert np.abs(page - raster).max() <= 40
