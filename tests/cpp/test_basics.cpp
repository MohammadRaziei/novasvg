#include "doctest.h"

#include <filesystem>
#include <string>

#include <novasvg/novasvg.h>

namespace {
std::filesystem::path project_root()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path data_path(const std::string& filename)
{
    return project_root() / "data" / filename;
}
} // namespace

TEST_CASE("Version reporting") {
    CHECK(novasvg::version() == NOVASVG_VERSION);
    CHECK(novasvg::version_string() == NOVASVG_VERSION_STRING);
}

TEST_CASE("Load SVG data from file and render") {
    auto svg_file = data_path("rect.svg");
    auto dom = novasvg::document::load_file(svg_file.string());
    REQUIRE(dom != nullptr);
    CHECK(dom->width() == doctest::Approx(64.0f));
    CHECK(dom->height() == doctest::Approx(64.0f));

    stb::image image = dom->render_to_image(64, 64);
    CHECK_FALSE(image.empty());
    CHECK(image.width() == 64);
    CHECK(image.height() == 64);
    CHECK(image.format() == stb::pixel_format::rgba);
}

TEST_CASE("Missing SVG file returns null document") {
    auto dom = novasvg::document::load_file((project_root() / "data" / "missing.svg").string());
    CHECK(dom == nullptr);
}
