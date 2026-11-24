#include "doctest.h"

#include <filesystem>
#include <string>

#include <nova/svg/svg.h>
#include <nova/vg/font.h>

#include "svg/svgelement.h"   // private header: measure_foreign_object() and make_font()
#include "vg/font.h"

// Soft wrapping inside <foreignObject>: a break after a hyphen, and the monospace tags. The same cases as
// tests/python/test_foreign_object.py, which go through the Python binding.
using namespace nova::svg;
using namespace nova::vg;

namespace {

const std::string family = "DejaVu Sans";

// DejaVu Sans from data/fonts, so the widths do not depend on the fonts a machine has installed.
const auto& base_font()
{
    static const auto f = [] {
        const auto path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts" / "DejaVuSans.ttf";
        REQUIRE(add_font_file(path.string().c_str()) > 0);
        return make_font(family, 16.f, 400, false);
    }();
    return f;
}

std::optional<foreign_object_size> measure(const std::string& html, float max_width)
{
    const auto renderer = default_foreign_object_renderer();
    REQUIRE(renderer != nullptr);
    const std::string markup = "<div xmlns=\"http://www.w3.org/1999/xhtml\"><span><p>" + html + "</p></span></div>";
    return measure_foreign_object(*renderer, markup, base_font(), family, 400, false, max_width);
}

int lines(const std::string& html, float max_width)
{
    const auto size = measure(html, max_width);
    REQUIRE(size.has_value());
    return size->line_count;
}

}

TEST_CASE("foreignObject wrap: a line breaks after a hyphen inside a word, like a browser")
{
    // Mermaid's `DELIVERY-ADDRESS` ER entity: no space in it, but a browser puts it on two lines.
    const auto size = measure("DELIVERY-ADDRESS", 120.f);
    REQUIRE(size.has_value());
    CHECK(size->line_count == 2);
    CHECK(size->width <= 120.f + 1.f / 64.f);

    CHECK(lines("DELIVERY-ADDRESS", 300.f) == 1);   // only when the line is full
    CHECK(lines("one-two three-four", 60.f) >= 3);  // hyphen and space breaks mix
    CHECK(lines("a-b-c-d-e-f-g-h-i-j-k-l-m-n", 40.f) >= 3);  // more than one hyphen in a word
}

TEST_CASE("foreignObject wrap: no break after a hyphen that opens the word, or before a digit or another hyphen")
{
    for(const char* text : {"WIDEWORD-12345678", "WIDEWORD--WIDEWORD", "-WIDEWORD", "WIDEWORD-"})
        CHECK_MESSAGE(lines(text, 60.f) == 1, text);
}

TEST_CASE("foreignObject wrap: a hyphen break works across styled runs too")
{
    CHECK(lines("<b>DELIVERY</b>-ADDRESS", 120.f) == 2);
    CHECK(lines("<b>DELIVERY-</b>ADDRESS", 120.f) == 2);
}

TEST_CASE("foreignObject: <code>, <tt>, <kbd>, <samp> and font-family: monospace use the monospace face")
{
    const auto no_wrap = [](const std::string& html) {
        const auto size = measure("<span style=\"white-space:nowrap\">" + html + "</span>", -1.f);
        REQUIRE(size.has_value());
        return size->width;
    };
    // Any monospace face is wider than DejaVu Sans for a string of i's.
    const float plain = no_wrap("iiiiiiiiii");
    const float mono = make_font("monospace", 16.f, 400, false).measure_text(U"iiiiiiiiii");
    REQUIRE(mono > plain + 1.f);

    for(const char* tag : {"code", "tt", "kbd", "samp"})
        CHECK_MESSAGE(no_wrap(std::string("<") + tag + ">iiiiiiiiii</" + tag + ">") == doctest::Approx(mono).epsilon(0.001), tag);
    CHECK(no_wrap("<span style=\"font-family: monospace\">iiiiiiiiii</span>") == doctest::Approx(mono).epsilon(0.001));
    CHECK(no_wrap("<span style=\"font-family: 'Courier New', monospace\">iiiiiiiiii</span>") == doctest::Approx(mono).epsilon(0.001));
    CHECK(no_wrap("a <code>b</code> c") > 0.f);
    CHECK(lines("a <code>b</code> c", 300.f) == 1);   // <code> stays in the run it is in
}
