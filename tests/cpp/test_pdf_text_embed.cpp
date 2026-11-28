#include "doctest.h"

#include <vg/font.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <nova/svg/svg.h>

// pdf_text: text as real text in an embedded subset of the font (ADR-0019). The content stream is compressed, so what
// can be seen here is the structure of the file and which fonts were embedded; tests/python/test_pdf_text_embed.py reads
// the text back with pdfium and the embedded font with fontTools.
using nova::svg::document;

namespace {

namespace fs = std::filesystem;

fs::path here() { return fs::path(__FILE__).parent_path(); }

std::vector<unsigned char> read_file(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// DejaVu Sans with OS/2 fsType set to `fs_type`, written to a temporary file and registered as `family`.
void add_with_fs_type(const char* family, unsigned fs_type)
{
    std::vector<unsigned char> bytes = read_file(here().parent_path().parent_path() / "data" / "fonts" / "DejaVuSans.ttf");
    REQUIRE(bytes.size() > 12);
    const size_t tables = (size_t(bytes[4]) << 8) | bytes[5];
    size_t os2 = 0;
    for(size_t i = 0; i < tables; ++i) {
        const size_t record = 12 + 16 * i;
        if(std::string(bytes.begin() + long(record), bytes.begin() + long(record) + 4) == "OS/2")
            os2 = (size_t(bytes[record + 8]) << 24) | (size_t(bytes[record + 9]) << 16) | (size_t(bytes[record + 10]) << 8) | bytes[record + 11];
    }
    REQUIRE(os2 > 0);
    bytes[os2 + 8] = static_cast<unsigned char>(fs_type >> 8);
    bytes[os2 + 9] = static_cast<unsigned char>(fs_type);
    const fs::path file = fs::temp_directory_path() / (std::string("novasvg-embed-") + family + ".ttf");
    std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(nova::vg::add_font_face_from_file(family, false, false, file.string().c_str()));
}

// The families the tests draw with.
void register_fonts()
{
    static const bool done = [] {
        const fs::path data = here().parent_path() / "data";
        REQUIRE(nova::vg::add_font_face_from_file("EmbedFull", false, false, (here().parent_path().parent_path() / "data" / "fonts" / "DejaVuSans.ttf").string().c_str()));
        add_with_fs_type("EmbedRestricted", 0x0002);
        add_with_fs_type("EmbedPreview", 0x0004);
        add_with_fs_type("EmbedNoSubset", 0x0108);
        add_with_fs_type("EmbedBitmap", 0x0200);
        REQUIRE(nova::vg::add_font_face_from_file("EmbedCff", false, false, (data / "libstb-cff-test.otf").string().c_str()));
        REQUIRE(nova::vg::add_font_face_from_file("EmbedVariable", false, false, (data / "libstb-variable-test.ttf").string().c_str()));
        return true;
    }();
    (void)done;
}

std::string pdf_of(const std::string& body, document::pdf_text text)
{
    register_fonts();
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'>" + body + "</svg>");
    REQUIRE(doc != nullptr);
    const auto bytes = doc->render_to_pdf(document::pdf_mode::vector, 192.f, text);
    REQUIRE(bytes.size() > 8);
    return std::string(bytes.begin(), bytes.end());
}

std::string text_in(const char* family, const char* extra = "")
{
    return std::string("<text x='8' y='40' font-size='24' font-family='") + family + "' " + extra + ">Hello</text>";
}

bool has(const std::string& pdf, const char* what) { return pdf.find(what) != std::string::npos; }

}

TEST_CASE("pdf text: outlines is the default and embeds nothing")
{
    register_fonts();
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'>" + text_in("EmbedFull") + "</svg>");
    REQUIRE(doc != nullptr);
    const auto bytes = doc->render_to_pdf();
    const std::string pdf(bytes.begin(), bytes.end());
    CHECK(has(pdf, "/FTxt "));
    CHECK(!has(pdf, "/FontFile2"));
    CHECK(pdf == pdf_of(text_in("EmbedFull"), document::pdf_text::outlines));
}

TEST_CASE("pdf text: embed writes a subset of the font and no invisible layer")
{
    const auto pdf = pdf_of(text_in("EmbedFull"), document::pdf_text::embed);
    CHECK(has(pdf, "/FontFile2 "));
    CHECK(has(pdf, "/Type /FontDescriptor"));
    CHECK(has(pdf, "/Subtype /CIDFontType2"));
    CHECK(has(pdf, "/Encoding /Identity-H"));
    CHECK(has(pdf, "/CIDToGIDMap "));
    CHECK(!has(pdf, "/CIDToGIDMap /Identity"));
    CHECK(has(pdf, "/W ["));
    CHECK(has(pdf, "/ToUnicode "));
    CHECK(has(pdf, "/Font << /FE0 "));
    CHECK(!has(pdf, "/FTxt"));           // the real text is the text layer
    CHECK(has(pdf, "+DejaVuSans"));      // the subset tag and the font's own name
    CHECK(pdf.size() < 60000);           // a few glyphs, not the 750 KB font
}

TEST_CASE("pdf text: the same drawing gives the same bytes")
{
    CHECK(pdf_of(text_in("EmbedFull"), document::pdf_text::embed) == pdf_of(text_in("EmbedFull"), document::pdf_text::embed));
}

TEST_CASE("pdf text: a license that allows embedding is embedded")
{
    CHECK(has(pdf_of(text_in("EmbedPreview"), document::pdf_text::embed), "/FontFile2 "));  // preview and print
}

TEST_CASE("pdf text: embed leaves a font whose license forbids it as outlines")
{
    for(const char* family : {"EmbedRestricted", "EmbedNoSubset", "EmbedBitmap"}) {
        CAPTURE(family);
        const auto pdf = pdf_of(text_in(family), document::pdf_text::embed);
        CHECK(!has(pdf, "/FontFile2"));
        CHECK(has(pdf, "/FTxt "));  // outlines with the invisible text layer, as in the default mode
    }
}

TEST_CASE("pdf text: embed_ignore_license embeds what the license forbids")
{
    for(const char* family : {"EmbedRestricted", "EmbedNoSubset", "EmbedBitmap"}) {
        CAPTURE(family);
        const auto pdf = pdf_of(text_in(family), document::pdf_text::embed_ignore_license);
        CHECK(has(pdf, "/FontFile2 "));
        CHECK(!has(pdf, "/FTxt"));
    }
}

TEST_CASE("pdf text: a font that cannot be subset stays outlines in every mode")
{
    for(const auto mode : {document::pdf_text::embed, document::pdf_text::embed_ignore_license}) {
        const auto cff = pdf_of(text_in("EmbedCff"), mode);
        CHECK(!has(cff, "/FontFile"));
        CHECK(has(cff, "/FTxt "));
    }
}

TEST_CASE("pdf text: a variable font at its default axes embeds, at another weight it stays outlines")
{
    CHECK(has(pdf_of(text_in("EmbedVariable"), document::pdf_text::embed), "/FontFile2 "));
    const auto bold = pdf_of(text_in("EmbedVariable", "font-weight='700'"), document::pdf_text::embed);
    CHECK(!has(bold, "/FontFile2"));
}

TEST_CASE("pdf text: one font embedded once, however much text uses it")
{
    const auto pdf = pdf_of(text_in("EmbedFull") + "<text x='8' y='70' font-size='24' font-family='EmbedFull'>World</text>", document::pdf_text::embed);
    size_t count = 0;
    for(size_t at = pdf.find("/FontFile2 "); at != std::string::npos; at = pdf.find("/FontFile2 ", at + 1))
        ++count;
    CHECK(count == 1);
}

TEST_CASE("pdf text: no text, no font; the mask's text is not the page's")
{
    CHECK(!has(pdf_of("<rect width='300' height='80' fill='#06c'/>", document::pdf_text::embed), "/FontFile2"));
}
