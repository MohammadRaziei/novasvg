"""PdfText: text as real text in an embedded subset of the font (ADR-0019).

Read back by programs that know nothing of novasvg: pdfium extracts and draws the text, pypdf finds the embedded font
and fontTools reads its program, so a subset that renumbered a glyph wrongly, or a /CIDToGIDMap that is off by one,
shows as a different outline here.
"""

import io
import pathlib

import pytest

import novasvg
import novasvg.fonts as fonts
from novasvg import PdfText

pdfium = pytest.importorskip("pypdfium2")
pypdf = pytest.importorskip("pypdf")
ttlib = pytest.importorskip("fontTools.ttLib")
from fontTools.pens.recordingPen import DecomposingRecordingPen  # noqa: E402

ROOT = pathlib.Path(__file__).parent.parent.parent
DEJAVU = ROOT / "data" / "fonts" / "DejaVuSans.ttf"


@pytest.fixture(scope="module", autouse=True)
def _fonts(tmp_path_factory):
    fonts.add_font_face_from_file("EmbedFull", False, False, str(DEJAVU))
    # the same font with a license that forbids embedding it (OS/2 fsType: restricted license)
    restricted = ttlib.TTFont(str(DEJAVU))
    restricted["OS/2"].fsType = 0x0002
    path = tmp_path_factory.mktemp("fonts") / "restricted.ttf"
    restricted.save(str(path))
    fonts.add_font_face_from_file("EmbedRestricted", False, False, str(path))


def _svg(body, w=360, h=120):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}">{body}</svg>'


def _pdf(body, text):
    return novasvg.Document.load_from_data(_svg(body)).render_to_pdf(text=text)


def _text(body, text):
    return pdfium.PdfDocument(_pdf(body, text))[0].get_textpage().get_text_range()


def _embedded(pdf):
    """[(font dict, descendant CIDFont, descriptor)] of the fonts of page 1 that carry a font program."""
    page = pypdf.PdfReader(io.BytesIO(pdf)).pages[0]
    found = []
    for ref in page["/Resources"].get("/Font", {}).values():
        font = ref.get_object()
        cid = font["/DescendantFonts"][0].get_object()
        descriptor = cid["/FontDescriptor"].get_object()
        if "/FontFile2" in descriptor:
            found.append((font, cid, descriptor))
    return found


LATIN = '<text x="8" y="40" font-size="24" font-family="EmbedFull">Hello, world! \u00c1\u00e9\u00ef fi \u2713</text>'
PERSIAN = '<text x="8" y="90" font-size="24" font-family="EmbedFull">\u0633\u0644\u0627\u0645 \u062f\u0646\u06cc\u0627\u060c Hello \u06f1\u06f2\u06f3</text>'


class TestExtraction:
    """What pdfium reads from embed is what it reads from outlines, which is the text that was drawn."""

    @pytest.mark.parametrize("body", [
        LATIN,
        '<text x="8" y="40" font-size="24" font-family="EmbedFull">a\U00010300b</text>',
        '<text x="8" y="30" font-size="16" font-family="EmbedFull">First line</text><text x="8" y="70" font-size="16" font-family="EmbedFull">Second line</text>',
        '<text x="8" y="40" font-size="16" font-family="EmbedFull">a <tspan fill="red">b</tspan> c</text>',
        '<text transform="rotate(-30 60 90)" x="60" y="90" font-size="18" font-family="EmbedFull">Rotated</text>',
        '<g opacity=".5"><text x="8" y="40" font-size="20" font-family="EmbedFull">In a group</text></g>',
        '<defs><linearGradient id="g"><stop offset="0" stop-color="red"/><stop offset="1" stop-color="blue"/></linearGradient></defs>'
        '<text x="8" y="60" font-size="40" font-family="EmbedFull" fill="url(#g)">Gradient</text>',
    ])
    def test_same_text_as_outlines(self, body):
        assert _text(body, PdfText.EMBED) == _text(body, PdfText.OUTLINES)

    def test_right_to_left_text_has_the_same_characters_and_whole_words(self):
        # What a viewer does with right-to-left text differs (pdfium puts the runs in visual order whatever the file
        # says), so the order is not compared: the characters are, and in embed the words come back whole where the
        # invisible layer of the outlines gives them letter by letter reversed.
        embedded = _text(PERSIAN, PdfText.EMBED)
        assert sorted(embedded.replace(" ", "")) == sorted(_text(PERSIAN, PdfText.OUTLINES).replace(" ", ""))
        assert "\u0633\u0644\u0627\u0645" in embedded and "\u062f\u0646\u06cc\u0627" in embedded and "Hello" in embedded

    def test_latin_text_comes_back_as_written(self):
        assert _text(LATIN, PdfText.EMBED) == "Hello, world! \u00c1\u00e9\u00ef fi \u2713"

    def test_a_text_that_is_filled_and_stroked_is_read_once(self):
        body = '<text x="8" y="60" font-size="40" font-family="EmbedFull" fill="#390" stroke="#000" stroke-width="1">Both</text>'
        assert _text(body, PdfText.EMBED).split() == ["Both"]

    def test_the_text_of_a_mask_is_not_the_pages(self):
        body = ('<defs><mask id="m"><text x="8" y="60" font-size="40" font-family="EmbedFull" fill="#fff">MASK</text></mask></defs>'
                '<rect width="360" height="120" fill="#06c" mask="url(#m)"/>')
        assert _text(body, PdfText.EMBED).strip() == ""


class TestEmbeddedFont:
    def test_the_font_is_a_small_valid_subset_with_the_glyphs_that_were_used(self):
        pdf = _pdf(LATIN + PERSIAN, PdfText.EMBED)
        ((font, cid, descriptor),) = _embedded(pdf)
        assert font["/Subtype"] == "/Type0" and font["/Encoding"] == "/Identity-H"
        assert cid["/Subtype"] == "/CIDFontType2"
        name = str(descriptor["/FontName"])
        assert name[7] == "+" and name[1:7].isalpha() and name.endswith("DejaVuSans")

        program = ttlib.TTFont(io.BytesIO(descriptor["/FontFile2"].get_object().get_data()))
        original = ttlib.TTFont(str(DEJAVU))
        assert 0 < program["maxp"].numGlyphs < 120 < original["maxp"].numGlyphs
        assert len(pdf) < 60000

        # CID n is glyph n of the original font; /CIDToGIDMap says which glyph of the subset that is
        to_gid = cid["/CIDToGIDMap"].get_object().get_data()
        widths = cid["/W"]
        used = [int(widths[i]) for i in range(0, len(widths), 2)]
        assert len(used) > 20
        old_order, new_order = original.getGlyphOrder(), program.getGlyphOrder()
        old_set, new_set = original.getGlyphSet(), program.getGlyphSet()
        for g in used:
            new = (to_gid[2 * g] << 8) | to_gid[2 * g + 1]
            assert 0 < new < program["maxp"].numGlyphs, f"glyph {g}"
            a = DecomposingRecordingPen(old_set)
            b = DecomposingRecordingPen(new_set)
            old_set[old_order[g]].draw(a)
            new_set[new_order[new]].draw(b)
            assert a.value == b.value, f"glyph {g}"

    def test_one_font_is_embedded_once(self):
        assert len(_embedded(_pdf(LATIN + PERSIAN, PdfText.EMBED))) == 1

    def test_outlines_embed_nothing_and_embed_does_not_draw_differently(self):
        assert _embedded(_pdf(LATIN, PdfText.OUTLINES)) == []
        outlines = pdfium.PdfDocument(_pdf(LATIN + PERSIAN, PdfText.OUTLINES))[0].render(scale=2).to_numpy()
        embedded = pdfium.PdfDocument(_pdf(LATIN + PERSIAN, PdfText.EMBED))[0].render(scale=2).to_numpy()
        a = outlines[..., :3].mean(axis=2) < 128
        b = embedded[..., :3].mean(axis=2) < 128
        assert a.sum() > 1000
        assert (a ^ b).sum() / a.sum() < 0.25  # edges only: the viewer hints a font's glyphs, not a path's


class TestLicense:
    BODY = '<text x="8" y="40" font-size="24" font-family="EmbedRestricted">Hello</text>'

    def test_embed_keeps_a_font_that_forbids_it_as_outlines(self):
        pdf = _pdf(self.BODY, PdfText.EMBED)
        assert _embedded(pdf) == []
        assert _text(self.BODY, PdfText.EMBED) == "Hello"  # the invisible layer is still there

    def test_embed_ignore_license_embeds_it(self):
        assert len(_embedded(_pdf(self.BODY, PdfText.EMBED_IGNORE_LICENSE))) == 1
        assert _text(self.BODY, PdfText.EMBED_IGNORE_LICENSE) == "Hello"


def test_save_pdf_takes_the_same_option(tmp_path):
    out = tmp_path / "a.pdf"
    assert novasvg.Document.load_from_data(_svg(LATIN)).save_pdf(str(out), text=PdfText.EMBED)
    assert len(_embedded(out.read_bytes())) == 1
