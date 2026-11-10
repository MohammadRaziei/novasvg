#include "doctest.h"

#include <filesystem>
#include <string>
#include <vector>

#include <nova/svg/svg.h>

#include "vg/font.h"
#include "vg/font_face.h"

// ADR-0012: a text is shaped as a run; <tspan>s and per-character positions cut it into fragments afterwards, so
// kerning and ligatures that cross a fragment boundary are kept.

namespace {
double text_width(const std::string& content)
{
    static const bool font_loaded = nova::vg::add_font_face_from_file(
        "DejaVu Sans", false, false,
        (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts" / "DejaVuSans.ttf").string().c_str());
    REQUIRE(font_loaded);
    const std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"100\">"
                            "<text id=\"t\" x=\"10\" y=\"60\" font-family=\"DejaVu Sans\" font-size=\"40\">" + content + "</text></svg>";
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    auto text = dom->element_by_id("t");
    REQUIRE_FALSE(text.is_null());
    return text.local_bounding_box().w;
}
} // namespace

TEST_CASE("Text runs: kerning across a <tspan> boundary is kept")
{
    const double whole = text_width("AV");
    // The pair is kerned, otherwise this test says nothing.
    REQUIRE(whole < text_width("A") + text_width("V") - 0.5);
    CHECK(text_width("A<tspan fill=\"red\">V</tspan>") == doctest::Approx(whole).epsilon(0.001));
}

TEST_CASE("Text runs: a ligature crossing a fragment boundary stays in the fragment of its first letter")
{
    text_width("");  // loads DejaVu Sans
    auto face = nova::vg::global_font_face_cache()->get_font_face_local("DejaVu Sans", false, false);
    REQUIRE_FALSE(face.is_null());
    const std::u32string run = U"fi";
    std::vector<nova::vg::shaped_glyph_t> glyphs;

    face.get()->shape(40.f, run.data(), 2, nova::vg::text_encoding::utf32, &glyphs);
    REQUIRE(glyphs.size() == 1);  // the fi ligature of DejaVu Sans

    nova::vg::shaping_context context{run, true};
    face.get()->shape(40.f, run.data(), 1, nova::vg::text_encoding::utf32, &glyphs, context);
    CHECK(glyphs.size() == 1);
    face.get()->shape(40.f, run.data() + 1, 1, nova::vg::text_encoding::utf32, &glyphs, context);
    CHECK(glyphs.empty());

    // letter-spacing: no ligature, the two letters are separate glyphs in their own fragments.
    context.ligatures = false;
    face.get()->shape(40.f, run.data(), 1, nova::vg::text_encoding::utf32, &glyphs, context);
    CHECK(glyphs.size() == 1);
    face.get()->shape(40.f, run.data() + 1, 1, nova::vg::text_encoding::utf32, &glyphs, context);
    CHECK(glyphs.size() == 1);
}
