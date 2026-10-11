// Built with NOVASVG_DISABLE_LOAD_SYSTEM_FONTS and no font registered: the machine has no fonts at all
// as far as novasvg can tell, which is the situation the embedded fallback font exists for.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../doctest.h"

#include <novasvg/novasvg.h>

#include <string>

#ifndef NOVASVG_DISABLE_LOAD_SYSTEM_FONTS
#error "this test must be built with NOVASVG_DISABLE_LOAD_SYSTEM_FONTS"
#endif

namespace {
size_t inked_pixels(const std::string& svg)
{
    auto document = novasvg::Document::loadFromData(svg);
    REQUIRE(document != nullptr);
    auto bitmap = document->renderToBitmap();
    size_t inked = 0;
    for(int y = 0; y < bitmap.height(); ++y) {
        const auto* row = bitmap.data() + size_t(y) * bitmap.stride();
        for(int x = 0; x < bitmap.width(); ++x)
            inked += row[x * 4 + 3] != 0;
    }
    return inked;
}

std::string text_svg(const std::string& family)
{
    return "<svg xmlns='http://www.w3.org/2000/svg' width='200' height='60'>"
           "<text x='5' y='40' font-size='32' font-family='" + family + "'>Hello</text></svg>";
}
} // namespace

TEST_CASE("text is drawn although the machine has no fonts")
{
    CHECK(inked_pixels(text_svg("sans-serif")) > 200);
    CHECK(inked_pixels(text_svg("Some Family, Another, sans-serif")) > 200);
    CHECK(inked_pixels(text_svg("serif")) > 200);
}

TEST_CASE("the embedded font lays text out exactly like the full DejaVu Sans")
{
    REQUIRE(novasvg::addFontFaceFromFile("Full Probe", false, false, NOVASVG_TEST_FONT_PATH));
    auto* cache = novasvg::fontFaceCache();
    auto full = cache->getFontFace("Full Probe", false, false);
    auto embedded = cache->getFontFace("sans-serif", false, false);
    REQUIRE(!full.isNull());
    REQUIRE(!embedded.isNull());
    CHECK(embedded.unitsPerEm() == full.unitsPerEm());

    // kerning pairs, ligatures, digits, punctuation, Latin-1
    const std::u32string samples[] = {
        U"Hello World", U"AV To LT Ty Yo WA", U"fi fl ffi ffl office", U"0123456789 +-*/=<>",
        U"The quick brown fox jumps over the lazy dog", U"Caf\u00e9 \u00d1and\u00fa \u00fcber \u00c5ngstr\u00f6m",
        U"\u201cquoted\u201d \u2013 dash \u2014 \u2026 \u20ac \u2122 \u2022",
    };
    int index = 0;
    for(const auto& text : samples) {
        ++index;
        for(float size : {10.0f, 16.0f, 31.0f}) {
            CAPTURE(index);
            CAPTURE(size);
            CHECK(novasvg::Font(embedded, size).measureText(text) == novasvg::Font(full, size).measureText(text));
        }
    }
}

namespace {
// The directory the regular test font lives in; the bold/oblique faces sit beside it.
std::string font_dir()
{
    std::string path = NOVASVG_TEST_FONT_PATH;
    return path.substr(0, path.find_last_of("/\\") + 1);
}
} // namespace

TEST_CASE("bold and italic requests get the bold / oblique embedded faces, laid out like the full fonts")
{
    struct Face { const char* file; bool bold; bool italic; };
    const Face faces[] = {
        {"DejaVuSans-Bold.ttf", true, false},
        {"DejaVuSans-Oblique.ttf", false, true},
        {"DejaVuSans-BoldOblique.ttf", true, true},
    };
    auto* cache = novasvg::fontFaceCache();
    const auto regular = cache->getFontFace("sans-serif", false, false);
    REQUIRE(!regular.isNull());
    const std::u32string samples[] = {U"Hello World", U"AV To LT Ty Yo WA", U"fi fl ffi ffl office", U"The quick brown fox jumps over the lazy dog"};

    for(const auto& spec : faces) {
        CAPTURE(spec.file);
        const std::string family = std::string("Full ") + spec.file;
        REQUIRE(novasvg::addFontFaceFromFile(family.c_str(), spec.bold, spec.italic, (font_dir() + spec.file).c_str()));
        const auto full = cache->getFontFace(family, spec.bold, spec.italic);
        const auto embedded = cache->getFontFace("sans-serif", spec.bold, spec.italic);
        REQUIRE(!full.isNull());
        REQUIRE(!embedded.isNull());
        CHECK(embedded.get() != regular.get()); // not just the regular face under another request
        for(const auto& text : samples)
            CHECK(novasvg::Font(embedded, 16.f).measureText(text) == novasvg::Font(full, 16.f).measureText(text));
        // and it is a different design from the regular face (bold is wider, oblique has other advances)
        CHECK(novasvg::Font(embedded, 16.f).measureText(samples[3]) != novasvg::Font(regular, 16.f).measureText(samples[3]));
    }
}

TEST_CASE("bold text is drawn heavier than regular text although the machine has no fonts")
{
    auto text = [](const char* attrs) {
        return std::string("<svg xmlns='http://www.w3.org/2000/svg' width='200' height='60'><text x='5' y='40' font-size='32' font-family='sans-serif' ") + attrs + ">Hello</text></svg>";
    };
    const auto regular = inked_pixels(text(""));
    CHECK(inked_pixels(text("font-weight='bold'")) > regular * 11 / 10);
    CHECK(inked_pixels(text("font-style='italic'")) != regular);
}

TEST_CASE("bold and italic runs inside a foreignObject use the embedded faces")
{
    auto label = [](const char* inner) {
        return std::string("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='60'><foreignObject x='0' y='0' width='300' height='60'>"
                           "<div xmlns='http://www.w3.org/1999/xhtml' style='font-size:24px;font-family:sans-serif;text-align:center'>") + inner + "</div></foreignObject></svg>";
    };
    const auto plain = inked_pixels(label("Hello world"));
    CHECK(inked_pixels(label("<b>Hello world</b>")) > plain * 11 / 10);
    CHECK(inked_pixels(label("<i>Hello world</i>")) != plain);
    CHECK(inked_pixels(label("Hello <b>world</b>")) > plain);
}
