#include "doctest.h"

#include <filesystem>
#include <string>

#include <nova/svg/svg.h>

#include <nova/vg/font.h>

// nova::svg::measure_text(): the public way to size a text without building a document. It goes through the same
// font lookup and shaping as <text>, so it must agree with the box of a real <text> element.

namespace {
std::string dejavu_path()
{
    return (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts" / "DejaVuSans.ttf").string();
}

void load_dejavu()
{
    static const int faces = nova::vg::add_font_file(dejavu_path());
    REQUIRE(faces > 0);
}

double element_width(const std::string& content, float size)
{
    const std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"100\">"
                            "<text id=\"t\" x=\"10\" y=\"60\" font-family=\"DejaVu Sans\" font-size=\"" + std::to_string(size) + "\">" + content +
                            "</text></svg>";
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    auto text = dom->element_by_id("t");
    REQUIRE_FALSE(text.is_null());
    return text.local_bounding_box().w;
}
} // namespace

TEST_CASE("measure_text agrees with the box of a <text> element")
{
    load_dejavu();
    for(const char* content : {"AV", "Hello, world", "fi ffl"}) {
        const auto m = nova::svg::measure_text(content, "DejaVu Sans", 40.f);
        CHECK(m.width == doctest::Approx(element_width(content, 40.f)).epsilon(0.001));
    }
}

TEST_CASE("measure_text: vertical metrics are positive and scale with the size")
{
    load_dejavu();
    const auto small = nova::svg::measure_text("Hg", "DejaVu Sans", 20.f);
    const auto big = nova::svg::measure_text("Hg", "DejaVu Sans", 40.f);
    CHECK(small.ascent > 0.f);
    CHECK(small.descent > 0.f);
    CHECK(big.ascent == doctest::Approx(2.f * small.ascent).epsilon(0.01));
    CHECK(big.descent == doctest::Approx(2.f * small.descent).epsilon(0.01));
    CHECK(big.width == doctest::Approx(2.f * small.width).epsilon(0.01));
}

TEST_CASE("measure_text: empty text, family lists, bold, invalid UTF-8")
{
    load_dejavu();
    CHECK(nova::svg::measure_text("", "DejaVu Sans", 20.f).width == 0.f);
    CHECK(nova::svg::measure_text("", "DejaVu Sans", 20.f).ascent > 0.f);

    const auto plain = nova::svg::measure_text("Hello", "DejaVu Sans", 20.f);
    const auto stack = nova::svg::measure_text("Hello", "No Such Font, DejaVu Sans", 20.f);
    CHECK(stack.width == doctest::Approx(plain.width));

    // Invalid UTF-8 is replaced, not rejected; the call must not fail.
    CHECK(nova::svg::measure_text("a\xff" "b", "DejaVu Sans", 20.f).width > 0.f);
}

TEST_CASE("measure_text: numeric weight, the bool forms and clamping")
{
    load_dejavu();
    const auto normal = nova::svg::measure_text("Hello", "DejaVu Sans", 20.f);
    const auto bold = nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, true);

    // 400 is normal and 700 is bold: the bool forms are these two.
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, 400).width == doctest::Approx(normal.width));
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, 700).width == doctest::Approx(bold.width));
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, 700, true).width ==
          doctest::Approx(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, true, true).width));

    // Out of range is clamped, not rejected.
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, 0).width > 0.f);
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f, 5000).width > 0.f);
}

TEST_CASE("add_font_file: registers a font under its own family name, refuses what is not a font")
{
    CHECK(nova::vg::add_font_file(dejavu_path()) > 0);
    CHECK(nova::vg::add_font_file((std::filesystem::path(dejavu_path()).parent_path() / "no-such-file.ttf").string()) == 0);
    // A readable file that is not a font.
    CHECK(nova::vg::add_font_file((std::filesystem::path(__FILE__).parent_path() / "CMakeLists.txt").string()) == 0);
    CHECK(nova::svg::measure_text("Hello", "DejaVu Sans", 20.f).width > 0.f);
}
