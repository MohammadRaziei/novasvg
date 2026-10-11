#include "doctest.h"
#include <novasvg/novasvg.h>

// Bold / italic runs inside <foreignObject>: the HTML scanner (no fonts needed).
using namespace novasvg;

namespace {
std::string describe(std::string_view html, unsigned char base = 0)
{
    std::string out;
    for(const auto& s : foreignObjectStyledText(html, base)) {
        std::string t = s.text;
        for(auto& c : t) if(c == '\n') c = '|';
        out += "[" + t + ":" + char('0' + s.style) + "]";
    }
    return out;
}
}

TEST_CASE("foreignObject style: bold and italic tags")
{
    CHECK(describe("<b>x</b>") == "[x:1]");
    CHECK(describe("<strong>x</strong>") == "[x:1]");
    CHECK(describe("<i>x</i>") == "[x:2]");
    CHECK(describe("<em>x</em>") == "[x:2]");
    CHECK(describe("<B>x</B>") == "[x:1]");
    CHECK(describe("a<b><i>x</i></b>b") == "[a:0][x:3][b:0]");
    CHECK(describe("<b>a <i>b</i> c</b>") == "[a :1][b:3][ c:1]");
}

TEST_CASE("foreignObject style: inline style attribute")
{
    CHECK(describe("<span style=\"font-weight: bold\">x</span>") == "[x:1]");
    CHECK(describe("<span style=\"font-weight:700\">x</span>") == "[x:1]");
    CHECK(describe("<span style=\"font-style:italic\">x</span>") == "[x:2]");
    CHECK(describe("<span style=\"font-style: oblique !important\">x</span>") == "[x:2]");
    CHECK(describe("<b><span style=\"font-weight:normal\">x</span>y</b>") == "[x:0][y:1]");
    CHECK(describe("<span data-style=\"font-weight:bold\">x</span>") == "[x:0]");
    CHECK(describe("<span title=\"a>b\" style=\"font-weight:bold\">x</span>") == "[x:1]");
}

TEST_CASE("foreignObject style: inherited base style")
{
    CHECK(describe("x", 1) == "[x:1]");
    CHECK(describe("<i>x</i>y", 1) == "[x:3][y:1]");
    CHECK(describe("<span style=\"font-weight:normal\">x</span>", 1) == "[x:0]");
}

TEST_CASE("foreignObject style: inline tags never split a word, other tags still do")
{
    CHECK(foreignObjectPlainText("un<b>believ</b>able") == "unbelievable");
    CHECK(foreignObjectPlainText("a<span>b</span>c") == "abc");
    CHECK(foreignObjectPlainText("Hello<br/><b>world!</b>") == "Hello\nworld!");
    CHECK(foreignObjectPlainText("a</td><td>b") == "a b");   // glued words were a real past bug
    CHECK(foreignObjectPlainText("<b>a </b> b") == "a b");   // whitespace collapses across runs
    CHECK(foreignObjectPlainText("<b/>x") == "x");           // self-closing does not open
}

TEST_CASE("foreignObject style: mismatched and unclosed tags")
{
    CHECK(describe("</b>x") == "[x:0]");
    CHECK(describe("<b>x") == "[x:1]");
    CHECK(describe("<b>x</i>y</b>z") == "[xy:1][z:0]");
}
