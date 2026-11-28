#include "doctest.h"

#include <string>

#include <nova/svg/svg.h>

// The invisible text layer of the PDF output: the page's text as characters (render mode 3, a font that has no glyphs and
// a /ToUnicode). The content stream is compressed, so what can be seen here is the structure; tests/python/
// test_pdf_text_layer.py reads the text back with pdfium.
using nova::svg::document;

namespace {

std::string pdf_of(const std::string& body)
{
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg' width='200' height='80'>" + body + "</svg>");
    REQUIRE(doc != nullptr);
    const auto bytes = doc->render_to_pdf(document::pdf_mode::vector, 192.f);
    REQUIRE(bytes.size() > 8);
    return std::string(bytes.begin(), bytes.end());
}

bool has(const std::string& pdf, const char* what) { return pdf.find(what) != std::string::npos; }

}

TEST_CASE("pdf text layer: text brings a glyph-less Type 0 font with a ToUnicode map")
{
    const auto pdf = pdf_of("<text x='8' y='40' font-size='20'>Hello</text>");
    CHECK(has(pdf, "/Font << /FTxt "));
    CHECK(has(pdf, "/Subtype /Type0"));
    CHECK(has(pdf, "/Encoding /Identity-H"));
    CHECK(has(pdf, "/Subtype /CIDFontType2"));
    CHECK(has(pdf, "/ToUnicode "));
    CHECK(!has(pdf, "/FontFile"));  // no font program is embedded: the visible glyphs are outlines
}

TEST_CASE("pdf text layer: no text, no font")
{
    const auto pdf = pdf_of("<rect width='200' height='80' fill='#06c'/>");
    CHECK(!has(pdf, "/FTxt"));
    CHECK(!has(pdf, "/Type0"));
}

TEST_CASE("pdf text layer: the text of a mask is not the page's text")
{
    const auto pdf = pdf_of("<defs><mask id='m'><text x='8' y='60' font-size='40' fill='#fff'>MASK</text></mask></defs>"
                            "<rect width='200' height='80' fill='#06c' mask='url(#m)'/>");
    CHECK(has(pdf, "/S /Luminosity"));
    CHECK(!has(pdf, "/FTxt"));
}

TEST_CASE("pdf text layer: hidden text is not in it")
{
    const auto pdf = pdf_of("<text x='8' y='30' font-size='20' visibility='hidden'>a</text>"
                            "<text x='8' y='60' font-size='20' display='none'>b</text>");
    CHECK(!has(pdf, "/FTxt"));
}
