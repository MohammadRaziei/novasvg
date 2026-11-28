"""Document.title: the root <svg>'s first <title> child, which is also the PDF's /Title."""

import pytest

import novasvg

HEAD = '<svg xmlns="http://www.w3.org/2000/svg" width="40" height="20">'


def _doc(body):
    return novasvg.Document.load_from_data(HEAD + body + "</svg>")


def test_title_text():
    assert _doc("<title>Release plan</title><rect width='40' height='20'/>").title == "Release plan"
    assert _doc("<rect width='40' height='20'/>").title == ""


def test_entities_whitespace_first_and_nesting():
    assert _doc("<title>\n  Q&amp;A &lt;draft&gt; \n</title>").title == "Q&A <draft>"
    assert _doc("<title>first</title><title>second</title>").title == "first"
    assert _doc("<g><title>a tooltip</title></g>").title == ""


@pytest.mark.parametrize("title", ["Release plan", "Release (v2) plan", "Caf\u00e9 \u2713", "\u0646\u0645\u0648\u062f\u0627\u0631"])
def test_pdf_title_round_trip(title):
    pdfium = pytest.importorskip("pypdfium2")
    data = _doc(f"<title>{title}</title><rect width='40' height='20'/>").render_to_pdf()
    assert pdfium.PdfDocument(data).get_metadata_value("Title") == title


def test_no_title_no_pdf_title():
    pdfium = pytest.importorskip("pypdfium2")
    data = _doc("<rect width='40' height='20'/>").render_to_pdf()
    assert pdfium.PdfDocument(data).get_metadata_value("Title") == ""
