#include "doctest.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <nova/svg/svg.h>

#include "svg/svgelement.h"   // private header: measure_foreign_object() and make_font()
#include "vg/font.h"

// The `font-weight` descriptor of an @font-face rule: one weight, or a range (`300 800`).
// tests/data/libstb-variable-test.ttf is a subset of Open Sans with a weight axis 300..800 (SIL OFL).

namespace {

std::string base64(const std::vector<std::uint8_t>& bytes)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for(std::size_t i = 0; i < bytes.size(); i += 3) {
        const std::uint32_t n = (std::uint32_t(bytes[i]) << 16) | (i + 1 < bytes.size() ? std::uint32_t(bytes[i + 1]) << 8 : 0) |
                                (i + 2 < bytes.size() ? std::uint32_t(bytes[i + 2]) : 0);
        out += alphabet[(n >> 18) & 63];
        out += alphabet[(n >> 12) & 63];
        out += i + 1 < bytes.size() ? alphabet[(n >> 6) & 63] : '=';
        out += i + 2 < bytes.size() ? alphabet[n & 63] : '=';
    }
    return out;
}

// An SVG with one @font-face rule for `family`, the embedded variable font and `descriptors` after the src.
void load_rule(const std::string& family, const std::string& descriptors)
{
    static const std::string payload = [] {
        const auto file = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "libstb-variable-test.ttf";
        std::ifstream in(file, std::ios::binary);
        REQUIRE(in.good());
        return base64(std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
    }();
    const std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"40\"><style>@font-face { font-family: '" + family +
                            "'; src: url(data:font/ttf;base64," + payload + "); " + descriptors + " }</style>"
                            "<text x=\"5\" y=\"25\" font-family=\"" + family + "\">Hi</text></svg>";
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
}

int weight_served(const std::string& family, int asked)
{
    const auto face = nova::vg::global_font_face_cache()->get_font_face_local(family, asked, false);
    REQUIRE_FALSE(face.is_null());
    return face.get()->weight();
}

}  // namespace

TEST_CASE("@font-face font-weight: a range limits a variable font to the part of its axis in it")
{
    load_rule("Range Test", "font-weight: 400 600;");
    CHECK(weight_served("Range Test", 500) == 500);
    CHECK(weight_served("Range Test", 700) == 600);  // the axis reaches 800 but the rule says up to 600
    CHECK(weight_served("Range Test", 300) == 400);
}

TEST_CASE("@font-face font-weight: one weight, the keywords, and no descriptor at all")
{
    load_rule("Single Test", "font-weight: 700;");
    CHECK(weight_served("Single Test", 700) == 700);
    CHECK(weight_served("Single Test", 400) == 700);  // the only face of the family

    load_rule("Keyword Test", "font-weight: bold;");
    CHECK(weight_served("Keyword Test", 700) == 700);

    load_rule("Open Test", "");  // nothing declared: the whole axis
    CHECK(weight_served("Open Test", 300) == 300);
    CHECK(weight_served("Open Test", 800) == 800);
}

TEST_CASE("@font-face font-weight: two rules of one family are two weights, not one regular face")
{
    using namespace nova::vg;
    font_face_cache cache;
    // two static faces of a family, declared 300 and 700: each request gets the one that was declared for it
    const std::string dir = (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts").string();
    const font_face light = font_face::from_file((dir + "/DejaVuSans.ttf").c_str(), 0);
    const font_face heavy = font_face::from_file((dir + "/DejaVuSans-Bold.ttf").c_str(), 0);
    REQUIRE(cache.add_font_face("Pair", 300, 300, false, light));
    REQUIRE(cache.add_font_face("Pair", 700, 700, false, heavy));
    CHECK(cache.get_font_face_local("Pair", 300, false).get() == light.get());
    CHECK(cache.get_font_face_local("Pair", 700, false).get() == heavy.get());
    CHECK(cache.get_font_face_local("Pair", 200, false).get() == light.get());   // nearest
    CHECK(cache.get_font_face_local("Pair", 900, false).get() == heavy.get());
    CHECK(cache.get_font_face_local("Pair", 500, false).get() == light.get());   // equally near: up to 500 CSS tries the lighter face first
    CHECK(cache.get_font_face_local("Pair", 600, false).get() == heavy.get());   // nearer to 700
}

TEST_CASE("foreignObject text takes any weight of a variable font, not just regular and bold")
{
    using namespace nova::svg;
    load_rule("FO Weight Test", "");  // the whole axis, 300..800
    const auto renderer = default_foreign_object_renderer();
    REQUIRE(renderer != nullptr);
    const std::string family = "FO Weight Test";
    const auto base = make_font(family, 20.f, 400, false);
    REQUIRE_FALSE(base.is_null());

    const auto width = [&](const char* weight) {
        const std::string html = std::string("<span style=\"font-weight:") + weight + "\">Hamburgefonstiv</span>";
        const auto size = measure_foreign_object(*renderer, html, base, family, 400, false, -1.f);
        REQUIRE(size.has_value());
        return size->width;
    };
    const float light = width("300"), medium = width("550"), heavy = width("800");
    CHECK(light < medium);
    CHECK(medium < heavy);

    // <b> is bolder than the base weight, so a bold base makes it heavier still
    const auto bold_in_bold = measure_foreign_object(*renderer, "<b>Hamburgefonstiv</b>", base, family, 700, false, -1.f);
    const auto bold_in_regular = measure_foreign_object(*renderer, "<b>Hamburgefonstiv</b>", base, family, 400, false, -1.f);
    REQUIRE((bold_in_bold && bold_in_regular));
    CHECK(bold_in_bold->width > bold_in_regular->width);
}
