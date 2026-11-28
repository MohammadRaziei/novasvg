#include "doctest.h"

#include <cmath>
#include <cstdint>
#include <string>

#include <nova/svg/svg.h>
#include <stb/image.hpp>

// render() hands back stb::image (straight RGBA) and can draw onto an existing one (a transparent layer that
// is composited with stb::image::blend).

namespace {

std::string svg(const std::string& body, const std::string& defs = "")
{
    return "<svg xmlns='http://www.w3.org/2000/svg' width='40' height='40'>" + defs + body + "</svg>";
}

}  // namespace

TEST_CASE("renderToImage returns straight-alpha RGBA")
{
    auto doc = nova::svg::document::load_data(
        svg("<rect x='0' y='0' width='40' height='40' fill='#0000ff' fill-opacity='0.5'/>"));
    REQUIRE(doc != nullptr);
    const stb::image image = doc->render_to_image(40, 40);
    REQUIRE(!image.empty());
    CHECK(image.format() == stb::pixel_format::rgba);
    const std::uint8_t* p = image.data() + 20 * image.stride() + 20 * 4;
    CHECK(p[0] == 0);
    CHECK(p[1] == 0);
    CHECK(p[2] == 255);  // straight: the blue is not darkened by the alpha
    CHECK((p[3] == 127 || p[3] == 128));  // 0.5 * 255 rounds either way
}

TEST_CASE("renderToImage with a background colour, and an empty image when there is nothing to draw")
{
    auto doc = nova::svg::document::load_data(svg("<rect width='10' height='10' fill='red'/>"));
    REQUIRE(doc != nullptr);
    const stb::image image = doc->render_to_image(40, 40, stb::color(255, 255, 255, 255));
    const std::uint8_t* corner = image.data() + 39 * image.stride() + 39 * 4;
    CHECK(corner[0] == 255);
    CHECK(corner[1] == 255);
    CHECK(corner[2] == 255);
    CHECK(corner[3] == 255);

    auto blank = nova::svg::document::load_data("<svg xmlns='http://www.w3.org/2000/svg'></svg>");
    REQUIRE(blank != nullptr);
    CHECK(blank->render_to_image(-1, 100).empty());  // nothing to size from, same as render_to_pixmap's null
}

TEST_CASE("render onto an existing image keeps its pixels where nothing is drawn, and its format")
{
    auto doc = nova::svg::document::load_data(svg("<rect width='10' height='10' fill='red'/>"));
    REQUIRE(doc != nullptr);

    stb::image rgba(40, 40, stb::pixel_format::rgba);
    rgba.fill(stb::color{10, 20, 30, 255});
    doc->render(rgba);
    CHECK(rgba.format() == stb::pixel_format::rgba);
    CHECK(rgba.data()[(39 * 40 + 39) * 4 + 0] == 10);  // untouched corner
    CHECK(rgba.data()[(5 * 40 + 5) * 4 + 0] == 255);   // red square

    stb::image rgb(40, 40, stb::pixel_format::rgb);
    doc->render(rgb);
    CHECK(rgb.format() == stb::pixel_format::rgb);
    CHECK(rgb.data()[(5 * 40 + 5) * 3 + 0] == 255);

    stb::image none;
    doc->render(none);  // an empty image is a no-op, not a crash
    CHECK(none.empty());
}


TEST_CASE("mix-blend-mode: overlay follows the W3C formula")
{
    // opaque backdrop (204, 102, 51) under an opaque (64, 128, 192) square: overlay = multiply where the backdrop
    // channel is dark, screen where it is light
    auto doc = nova::svg::document::load_data(
        svg("<rect width='40' height='40' fill='#cc6633'/>"
            "<rect x='10' y='10' width='20' height='20' fill='#4080c0' style='mix-blend-mode:overlay'/>"));
    REQUIRE(doc != nullptr);
    const stb::image image = doc->render_to_image(40, 40);
    REQUIRE(!image.empty());
    const std::uint8_t* p = image.data() + 20 * image.stride() + 20 * 4;
    const int backdrop[3] = {204, 102, 51};
    const int source[3] = {64, 128, 192};
    for(int i = 0; i < 3; ++i) {
        const float b = backdrop[i] / 255.f, s = source[i] / 255.f;
        const float expected = (b <= 0.5f ? 2.f * b * s : 1.f - 2.f * (1.f - b) * (1.f - s)) * 255.f;
        CHECK(std::abs(p[i] - expected) <= 1.5f);
        CHECK(p[i] != source[i]);  // it is not painted as normal any more
    }
    CHECK(p[3] == 255);
}

TEST_CASE("mix-blend-mode: every CSS keyword follows the W3C formulas")
{
    // opaque backdrop (204, 102, 51) under an opaque (64, 128, 192) square, expected colours worked out from the spec
    struct { const char* keyword; int r, g, b; } cases[] = {
        {"color-dodge", 255, 205, 206}, {"color-burn", 52, 0, 0},    {"hard-light", 102, 103, 154},
        {"soft-light", 184, 102, 83},   {"hue", 65, 142, 218},       {"saturation", 191, 106, 63},
        {"color", 75, 139, 203},        {"luminosity", 193, 91, 40}, {"HARD-LIGHT", 102, 103, 154},
    };
    for(const auto& c : cases) {
        CAPTURE(c.keyword);
        auto doc = nova::svg::document::load_data(
            svg("<rect width='40' height='40' fill='#cc6633'/>"
                "<rect x='10' y='10' width='20' height='20' fill='#4080c0' style='mix-blend-mode:" + std::string(c.keyword) + "'/>"));
        REQUIRE(doc != nullptr);
        const stb::image image = doc->render_to_image(40, 40);
        REQUIRE(!image.empty());
        const std::uint8_t* p = image.data() + 20 * image.stride() + 20 * 4;
        CHECK(std::abs(p[0] - c.r) <= 2);
        CHECK(std::abs(p[1] - c.g) <= 2);
        CHECK(std::abs(p[2] - c.b) <= 2);
        CHECK(p[3] == 255);
    }
    // plus-lighter is a composite operator in CSS, not a mix-blend-mode keyword: an unknown value paints as normal
    auto doc = nova::svg::document::load_data(
        svg("<rect width='40' height='40' fill='#cc6633'/>"
            "<rect x='10' y='10' width='20' height='20' fill='#4080c0' style='mix-blend-mode:plus-lighter'/>"));
    REQUIRE(doc != nullptr);
    const stb::image image = doc->render_to_image(40, 40);
    const std::uint8_t* p = image.data() + 20 * image.stride() + 20 * 4;
    CHECK(int(p[0]) == 0x40);
    CHECK(int(p[1]) == 0x80);
    CHECK(int(p[2]) == 0xc0);
}

TEST_CASE("a group with opacity that holds only a line running left or up is drawn")
{
    // d3's axis ticks (`<g class="tick" opacity="..."><line y2="-111"/></g>`, mermaid's gantt grid): the line's box
    // used to have a negative height, so the group's layer was empty and nothing reached the page.
    const auto covered = [](const std::string& body) {
        auto doc = nova::svg::document::load_data(svg(body));
        REQUIRE(doc != nullptr);
        const stb::image image = doc->render_to_image(40, 40);
        REQUIRE(!image.empty());
        int n = 0;
        for(int y = 0; y < image.height(); ++y)
            for(int x = 0; x < image.width(); ++x)
                n += image.data()[y * image.stride() + x * 4 + 3] > 0;
        return n;
    };
    const char* stroke = "stroke='black'";
    CHECK(covered(std::string("<g transform='translate(20.5,35)' opacity='0.8'><line y2='-30' ") + stroke + "/></g>") == 30);  // up
    CHECK(covered(std::string("<g transform='translate(35,20.5)' opacity='0.8'><line x2='-30' ") + stroke + "/></g>") == 30);  // left
    CHECK(covered(std::string("<g opacity='0.8'><line x1='20.5' x2='20.5' y1='35' y2='5' ") + stroke + "/></g>") == 30);        // y1 > y2
    CHECK(covered(std::string("<g transform='translate(20.5,5)' opacity='0.8'><line y2='30' ") + stroke + "/></g>") == 30);   // down: unchanged
}

TEST_CASE("foreignObject: a word too long for a wrapping label overflows at its natural size, a nowrap line is condensed")
{
    // CSS lets an unbreakable word overflow the box of a label that may wrap, starting at the box's left edge; only a
    // `white-space: nowrap` line wider than its box is condensed to fit.
    const auto ink_width = [](const std::string& div_style) {
        const std::string html = "<div xmlns='http://www.w3.org/1999/xhtml' style='" + div_style +
                                 "'>Supercalifragilisticexpialidocious</div>";
        auto doc = nova::svg::document::load_data(
            "<svg xmlns='http://www.w3.org/2000/svg' width='300' height='60'>"
            "<foreignObject x='5' y='5' width='60' height='40' font-size='16'>" + html + "</foreignObject></svg>");
        REQUIRE(doc != nullptr);
        const stb::image image = doc->render_to_image(300, 60);
        REQUIRE(!image.empty());
        int first = -1, last = -1;
        for(int x = 0; x < image.width(); ++x)
            for(int y = 0; y < image.height(); ++y)
                if(image.data()[y * image.stride() + x * 4 + 3] > 0) {
                    if(first < 0)
                        first = x;
                    last = x;
                    break;
                }
        return first < 0 ? 0 : last - first + 1;
    };
    CHECK(ink_width("white-space: break-spaces; line-height: 1.5") > 60);  // not squeezed into the 60 px box
    CHECK(ink_width("white-space: nowrap; line-height: 1.5") <= 62);       // still condensed (AA edge pixels: +2)
}
