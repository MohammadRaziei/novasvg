#include "doctest.h"

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

