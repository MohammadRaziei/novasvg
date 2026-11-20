#include "doctest.h"
#include <nova/svg/svg.h>
#include <vg/canvas.h>
#include <vg/font.h>
#include "svg/svgelement.h" // private header: this test exercises the HTML scanner directly
#include "svg/foreign_object/html_text.h"

// Bold / italic runs inside <foreignObject>: the HTML scanner (no fonts needed).
using namespace nova::svg;
using namespace nova::vg;

namespace {
// "[text:weight]" per run, with an "i" after the weight for italic: bold is 700, bold italic 700i.
std::string describe(std::string_view html, foreign_object_style base = {})
{
    std::string out;
    for(const auto& s : foreign_object_styled_text(html, base)) {
        std::string t = s.text;
        for(auto& c : t) if(c == '\n') c = '|';
        out += "[" + t + ":" + std::to_string(s.style.weight) + (s.style.italic ? "i" : "") + "]";
    }
    return out;
}
}

TEST_CASE("foreignObject style: bold and italic tags")
{
    CHECK(describe("<b>x</b>") == "[x:700]");
    CHECK(describe("<strong>x</strong>") == "[x:700]");
    CHECK(describe("<i>x</i>") == "[x:400i]");
    CHECK(describe("<em>x</em>") == "[x:400i]");
    CHECK(describe("<B>x</B>") == "[x:700]");
    CHECK(describe("a<b><i>x</i></b>b") == "[a:400][x:700i][b:400]");
    CHECK(describe("<b>a <i>b</i> c</b>") == "[a :700][b:700i][ c:700]");
    CHECK(describe("<b><b>x</b></b>") == "[x:900]");   // <b> is `bolder`: relative to where it sits
}

TEST_CASE("foreignObject style: inline style attribute")
{
    CHECK(describe("<span style=\"font-weight: bold\">x</span>") == "[x:700]");
    CHECK(describe("<span style=\"font-weight:700\">x</span>") == "[x:700]");
    CHECK(describe("<span style=\"font-weight:300\">x</span>") == "[x:300]");   // a weight, not just bold or not
    CHECK(describe("<span style=\"font-weight:550\">x</span>") == "[x:550]");
    CHECK(describe("<span style=\"font-weight:bolder\">x</span>") == "[x:700]");
    CHECK(describe("<b><span style=\"font-weight:lighter\">x</span></b>") == "[x:400]");
    CHECK(describe("<span style=\"font-style:italic\">x</span>") == "[x:400i]");
    CHECK(describe("<span style=\"font-style: oblique !important\">x</span>") == "[x:400i]");
    CHECK(describe("<b><span style=\"font-weight:normal\">x</span>y</b>") == "[x:400][y:700]");
    CHECK(describe("<span data-style=\"font-weight:bold\">x</span>") == "[x:400]");
    CHECK(describe("<span title=\"a>b\" style=\"font-weight:bold\">x</span>") == "[x:700]");
}

TEST_CASE("foreignObject style: inherited base style")
{
    CHECK(describe("x", {700, false}) == "[x:700]");
    CHECK(describe("x", {300, false}) == "[x:300]");
    CHECK(describe("<i>x</i>y", {700, false}) == "[x:700i][y:700]");
    CHECK(describe("<span style=\"font-weight:normal\">x</span>", {700, false}) == "[x:400]");
    CHECK(describe("<b>x</b>", {300, false}) == "[x:400]");   // bolder than 300 is 400
}

TEST_CASE("foreignObject style: inline tags never split a word, other tags still do")
{
    CHECK(foreign_object_plain_text("un<b>believ</b>able") == "unbelievable");
    CHECK(foreign_object_plain_text("a<span>b</span>c") == "abc");
    CHECK(foreign_object_plain_text("Hello<br/><b>world!</b>") == "Hello\nworld!");
    CHECK(foreign_object_plain_text("a</td><td>b") == "a b");   // glued words were a real past bug
    CHECK(foreign_object_plain_text("<b>a </b> b") == "a b");   // whitespace collapses across runs
    CHECK(foreign_object_plain_text("<b/>x") == "x");           // self-closing does not open
}

TEST_CASE("foreignObject style: mismatched and unclosed tags")
{
    CHECK(describe("</b>x") == "[x:400]");
    CHECK(describe("<b>x") == "[x:700]");
    CHECK(describe("<b>x</i>y</b>z") == "[xy:700][z:400]");
}
