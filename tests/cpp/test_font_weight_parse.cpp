#include "doctest.h"

#include "svg/svg_impl.h"

// font-weight: a number from 1 to 1000, or normal, bold, bolder, lighter (the last two relative to the parent's weight).

using nova::svg::parse_font_weight;

TEST_CASE("font-weight: keywords and numbers")
{
    CHECK(parse_font_weight("normal", 400) == 400);
    CHECK(parse_font_weight("bold", 400) == 700);
    CHECK(parse_font_weight(" 300 ", 400) == 300);
    CHECK(parse_font_weight("650", 400) == 650);
    CHECK(parse_font_weight("1", 400) == 1);
    CHECK(parse_font_weight("1000", 400) == 1000);
    CHECK(parse_font_weight("350.4", 400) == 350);
}

TEST_CASE("font-weight: bolder and lighter follow the parent's weight")
{
    CHECK(parse_font_weight("bolder", 300) == 400);
    CHECK(parse_font_weight("bolder", 400) == 700);
    CHECK(parse_font_weight("bolder", 700) == 900);
    CHECK(parse_font_weight("lighter", 300) == 100);
    CHECK(parse_font_weight("lighter", 700) == 400);
    CHECK(parse_font_weight("lighter", 900) == 700);
}

TEST_CASE("font-weight: what is not a weight is regular")
{
    for(const char* bad : {"", "heavy", "0", "1001", "-100", "700px", "bold bold"})
        CHECK(parse_font_weight(bad, 700) == 400);
}
