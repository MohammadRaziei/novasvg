#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <filesystem>
#include <string>

#include "novasvg.h"

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
    CHECK(novasvg_version() == NOVASVG_VERSION);
    CHECK(std::string(novasvg_version_string()) == NOVASVG_VERSION_STRING);
}

TEST_CASE("Load SVG data from file and render") {
    auto svg_file = data_path("rect.svg");
    auto document = novasvg::Document::loadFromFile(svg_file.string());
    REQUIRE(document != nullptr);
    CHECK(document->width() == doctest::Approx(64.0f));
    CHECK(document->height() == doctest::Approx(64.0f));

    novasvg::Bitmap bitmap = document->renderToBitmap(64, 64);
    CHECK_FALSE(bitmap.isNull());
    CHECK(bitmap.width() == 64);
    CHECK(bitmap.height() == 64);
}

TEST_CASE("Missing SVG file returns null document") {
    auto document = novasvg::Document::loadFromFile((project_root() / "data" / "missing.svg").string());
    CHECK(document == nullptr);
}
