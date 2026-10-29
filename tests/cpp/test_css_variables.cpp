#include "doctest.h"

#include <array>
#include <string>

#include <novasvg/novasvg.h>

// CSS custom properties (`--x: value`) and `var(--x, fallback)`.
//
// Every case paints one rect over a 40x40 canvas and reads the centre pixel, so what is checked is the colour
// the renderer ends up with -- not how the attribute was stored.
namespace {
using Pixel = std::array<int, 4>;

Pixel centre(const std::string& svg)
{
    auto document = novasvg::Document::loadFromData(svg);
    REQUIRE(document != nullptr);
    auto bitmap = document->renderToBitmap(40, 40);
    bitmap.convertToRGBA();
    const uint8_t* p = bitmap.data() + 20 * bitmap.stride() + 20 * 4;
    return {p[0], p[1], p[2], p[3]};
}

std::string svg(const std::string& rootStyle, const std::string& body, const std::string& sheet = "")
{
    return "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
           "width=\"40\" height=\"40\" style=\"" + rootStyle + "\">" +
           (sheet.empty() ? "" : "<style>" + sheet + "</style>") + body + "</svg>";
}

const std::string rect = "<rect width=\"40\" height=\"40\" ";
const Pixel red{255, 0, 0, 255};
const Pixel green{0, 128, 0, 255};
const Pixel blue{0, 0, 255, 255};
const Pixel black{0, 0, 0, 255};
} // namespace

TEST_CASE("var(): a custom property of the root, in a presentation attribute")
{
    CHECK(centre(svg("--c:red", rect + "fill=\"var(--c)\"/>")) == red);
}

TEST_CASE("var(): in the style attribute")
{
    CHECK(centre(svg("--c:red", rect + "style=\"fill:var(--c)\"/>")) == red);
}

TEST_CASE("var(): the fallback is used when the property does not exist")
{
    CHECK(centre(svg("", rect + "fill=\"var(--nope, blue)\"/>")) == blue);
    CHECK(centre(svg("--c:red", rect + "fill=\"var(--nope,var(--c))\"/>")) == red);
    // The property wins over the fallback when it exists.
    CHECK(centre(svg("--c:red", rect + "fill=\"var(--c, blue)\"/>")) == red);
}

TEST_CASE("var(): no property and no fallback leaves the value unparsed (the default fill)")
{
    CHECK(centre(svg("", rect + "fill=\"var(--nope)\"/>")) == black);
}

TEST_CASE("var(): a custom property can use var() itself (what pele's output does)")
{
    // --_fg: var(--pele-fg, #00f) on the root, fill="var(--_fg)" below it.
    const std::string body = rect + "fill=\"var(--_fg)\"/>";
    CHECK(centre(svg("--_fg:var(--pele-fg,#00f)", body)) == blue);
    CHECK(centre(svg("--_fg:var(--pele-fg,#00f);--pele-fg:red", body)) == red);
}

TEST_CASE("var(): custom properties inherit, and a descendant can override them")
{
    CHECK(centre(svg("--c:red", "<g>" + rect + "fill=\"var(--c)\"/></g>")) == red);
    CHECK(centre(svg("--c:red", "<g style=\"--c:green\">" + rect + "fill=\"var(--c)\"/></g>")) == green);
    // An override is seen by the descendants of the element that has it only.
    CHECK(centre(svg("--c:red", "<g style=\"--c:green\"/>" + rect + "fill=\"var(--c)\"/>")) == red);
}

TEST_CASE("var(): custom properties from a stylesheet, which can come after the element")
{
    CHECK(centre(svg("", rect + "fill=\"var(--c)\"/>", ":root{--c:blue}")) == blue);
    CHECK(centre(svg("", rect + "class=\"a\" fill=\"var(--c)\"/>", ".a{--c:green}")) == green);
    CHECK(centre(svg("", rect + "fill=\"var(--c)\"/>", "svg{--c:red}")) == red);
}

TEST_CASE("var(): custom property names are case-sensitive")
{
    CHECK(centre(svg("--Color:red", rect + "fill=\"var(--color, blue)\"/>")) == blue);
    CHECK(centre(svg("--Color:red", rect + "fill=\"var(--Color, blue)\"/>")) == red);
}

TEST_CASE("var(): the cascade between an inline style and a stylesheet")
{
    CHECK(centre(svg("--c:red", rect + "fill=\"var(--c)\"/>", ":root{--c:blue}")) == red);
    CHECK(centre(svg("--c:red", rect + "fill=\"var(--c)\"/>", ":root{--c:blue !important}")) == blue);
}

TEST_CASE("var(): a cycle is an unresolvable property, so the fallback is used")
{
    CHECK(centre(svg("--a:var(--b);--b:var(--a)", rect + "fill=\"var(--a, blue)\"/>")) == blue);
    CHECK(centre(svg("--a:var(--a)", rect + "fill=\"var(--a)\"/>")) == black);
}

TEST_CASE("var(): in other properties than a colour")
{
    const std::string body = "<rect width=\"var(--w)\" height=\"40\" fill=\"red\"/>";
    CHECK(centre(svg("--w:40", body)) == red);
    // Too narrow to reach the centre.
    CHECK(centre(svg("--w:10", body)) == Pixel{0, 0, 0, 0});
}

TEST_CASE("var(): a <use> copy gets the value its original computed")
{
    const std::string body = "<defs><rect id=\"r\" width=\"40\" height=\"40\" fill=\"var(--c)\"/></defs>"
                             "<use xlink:href=\"#r\"/>";
    CHECK(centre(svg("--c:green", body)) == green);
}

TEST_CASE("var(): a stylesheet applied after loading changes what var() gives")
{
    auto document = novasvg::Document::loadFromData(svg("--c:red", rect + "fill=\"var(--c)\"/>"));
    REQUIRE(document != nullptr);
    document->applyStyleSheet(":root{--c:blue !important}");
    auto bitmap = document->renderToBitmap(40, 40);
    bitmap.convertToRGBA();
    const uint8_t* p = bitmap.data() + 20 * bitmap.stride() + 20 * 4;
    CHECK(Pixel{p[0], p[1], p[2], p[3]} == blue);
}
