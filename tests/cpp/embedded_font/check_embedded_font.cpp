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
