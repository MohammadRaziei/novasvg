// The text parsers of the drawing layer: SVG transform lists and SVG path data, on top of parserutils.h.

#include "doctest.h"

#include "vg/canvas.h"
#include "vg/parserutils.h"
#include "vg/render.h"

#include <string_view>

using namespace nova::vg;

TEST_CASE("parse_number leaves the input and the number alone when it fails")
{
    std::string_view input = ".)";
    float number = 7.f;
    CHECK_FALSE(parse_number(input, number));
    CHECK(input == ".)");
    CHECK(number == 7.f);

    input = "-12.5e1px";
    CHECK(parse_number(input, number));
    CHECK(number == doctest::Approx(-125.f));
    CHECK(input == "px");

    input = "1ex";   // ex and em are units, not an exponent
    CHECK(parse_number(input, number));
    CHECK(number == 1.f);
    CHECK(input == "ex");
}

TEST_CASE("transform lists: units, separators and what is invalid")
{
    stb::matrix m;
    CHECK(parse_matrix(&m, "translate(10,20) scale(2)", -1));
    CHECK(m.a == doctest::Approx(2.f));    // translate(10,20) scale(2): the scale is applied to the content first,
    CHECK(m.d == doctest::Approx(2.f));    // so the translation stays what it says
    CHECK(m.e == doctest::Approx(10.f));
    CHECK(m.f == doctest::Approx(20.f));

    CHECK(parse_matrix(&m, " rotate(0.5turn)\n,skewX(10deg) ", -1));  // a comma between two transforms is allowed
    CHECK(parse_matrix(&m, "rotate(1rad)", -1));
    CHECK(m.a == doctest::Approx(0.5403f).epsilon(0.001));
    CHECK(parse_matrix(&m, "translate(5px 6pt)", -1));  // other units are dropped, the number used as it is

    CHECK_FALSE(parse_matrix(&m, "translate(3em)", -1));  // a font relative length cannot be resolved
    CHECK_FALSE(parse_matrix(&m, "translate(3ex)", -1));
    CHECK_FALSE(parse_matrix(&m, "translate(1,)", -1));   // a comma before the closing parenthesis
    CHECK_FALSE(parse_matrix(&m, "scale(1) ,", -1));      // a trailing comma
    CHECK_FALSE(parse_matrix(&m, "scale(1", -1));
    CHECK_FALSE(parse_matrix(&m, "scale 1)", -1));
    CHECK_FALSE(parse_matrix(&m, "skew(1)", -1));
    CHECK_FALSE(parse_matrix(&m, "matrix(1 2 3 4 5)", -1));  // matrix takes exactly six
}

TEST_CASE("path data: relative commands, flags without separators and partial data on error")
{
    stb::outline path;
    CHECK(parse_path(path, "M10 10 l 5 0 h-5 v-5z", 21));
    CHECK(path.size() == 5);

    CHECK(parse_path(path, "M0,0a5 5 0 1110 0", 17));  // the two flags are one character each
    CHECK(path.size() > 1);

    CHECK_FALSE(parse_path(path, "L 1 1", 5));       // data must start with a move
    CHECK_FALSE(parse_path(path, "M 1 1 L 2", 9));   // a missing coordinate
    CHECK(path.size() >= 1);                         // what was read before the error stays, as in SVG
    CHECK_FALSE(parse_path(path, "M 1 1 z z", 9));
    CHECK_FALSE(parse_path(path, "M 1 1 Y 2", 9));
}
