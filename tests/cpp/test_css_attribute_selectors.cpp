// CSS attribute selectors on attributes novasvg has no property for (`data-*`, `aria-*`, ...), 0.11.5.
#include "doctest.h"
#include <nova/svg/svg.h>

#include <string>

namespace {

// Red (the rule matched) or white (it did not) at the centre of a 40x40 render.
bool matched(const std::string& selector, const std::string& group_open, bool on_rect = false)
{
    std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\"><style>" + selector + "{fill:#f00}</style>";
    if(on_rect)
        svg += "<rect data-k=\"1\" width=\"40\" height=\"40\" fill=\"#fff\"/></svg>";
    else
        svg += group_open + "<rect width=\"40\" height=\"40\" fill=\"#fff\"/></g></svg>";
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    const auto image = dom->render_to_image();
    return image(20, 20, 0) == 255 && image(20, 20, 1) == 0 && image(20, 20, 2) == 0;
}

} // namespace

TEST_CASE("attribute selectors match attributes novasvg has no property for")
{
    CHECK(matched(".a rect", "<g class=\"a\">"));
    CHECK(matched("[data-k] rect", "<g data-k=\"1\">"));
    CHECK(matched("[data-k] rect", "<g data-k=\"\">"));   // an empty attribute is still there
    CHECK(matched("[data-k=\"1\"] rect", "<g data-k=\"1\">"));
    CHECK(matched(".a[data-k=\"1\"] rect", "<g class=\"a\" data-k=\"1\">"));
    CHECK(matched("[data-k=\"1\"][data-j=\"2\"] rect", "<g data-k=\"1\" data-j=\"2\">"));
    CHECK(matched("[data-k^=\"ab\"] rect", "<g data-k=\"abc\">"));
    CHECK(matched("[data-k$=\"bc\"] rect", "<g data-k=\"abc\">"));
    CHECK(matched("[data-k*=\"b\"] rect", "<g data-k=\"abc\">"));
    CHECK(matched("[data-k~=\"b\"] rect", "<g data-k=\"a b c\">"));
    CHECK(matched("[data-k|=\"a\"] rect", "<g data-k=\"a-b\">"));
    CHECK(matched("rect[data-k=\"1\"]", "", true));
    CHECK(matched("#s [data-look=\"neo\"][data-color-id=\"color-0\"].node rect",
                  "<g class=\"node\" data-look=\"neo\" data-color-id=\"color-0\">") == false); // no id="s" on the svg here
}

TEST_CASE("attribute selectors do not match when the attribute or value is missing")
{
    CHECK_FALSE(matched("[data-k] rect", "<g>"));
    CHECK_FALSE(matched("[data-k=\"2\"] rect", "<g data-k=\"1\">"));
    CHECK_FALSE(matched("[data-k=\"1\"][data-j=\"2\"] rect", "<g data-k=\"1\">"));
    CHECK_FALSE(matched("[data-k^=\"b\"] rect", "<g data-k=\"abc\">"));
}

TEST_CASE("custom attributes are copied by <use>")
{
    const std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"40\" height=\"40\">"
        "<style>[data-k=\"1\"] rect{fill:#f00}</style>"
        "<defs><g id=\"t\" data-k=\"1\"><rect width=\"40\" height=\"40\" fill=\"#fff\"/></g></defs>"
        "<use xlink:href=\"#t\"/></svg>";
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    const auto image = dom->render_to_image();
    CHECK(image(20, 20, 0) == 255);
    CHECK(image(20, 20, 1) == 0);
}
