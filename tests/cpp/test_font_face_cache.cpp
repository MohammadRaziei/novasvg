#include "doctest.h"

#include <filesystem>
#include <set>
#include <string>

#include "vg/render.h"
#include "vg/font.h"

namespace {
std::string fonts_dir()
{
    return (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts").string();
}
} // namespace

TEST_CASE("Font face cache: directory scan finds the family and picks the face by style")
{
    using namespace nova::vg;
    font_face_cache cache;

    // data/fonts holds the four DejaVu Sans faces.
    CHECK(cache.load_dir(fonts_dir().c_str()) == 4);
    CHECK(cache.load_dir("/nonexistent/fonts/dir") == 0);

    const stb::font* regular = cache.get_font_face_local("DejaVu Sans", false, false).get();
    const stb::font* bold = cache.get_font_face_local("DejaVu Sans", true, false).get();
    const stb::font* italic = cache.get_font_face_local("DejaVu Sans", false, true).get();
    const stb::font* bold_italic = cache.get_font_face_local("DejaVu Sans", true, true).get();
    REQUIRE(regular);
    REQUIRE(bold);
    REQUIRE(italic);
    REQUIRE(bold_italic);
    CHECK(std::set<const stb::font*>{regular, bold, italic, bold_italic}.size() == 4);

    // A face is loaded once and then served from the cache.
    CHECK(cache.get_font_face_local("DejaVu Sans", true, false).get() == bold);

    CHECK(cache.get_font_face_local("No Such Family", false, false).get() == nullptr);

    cache.reset();
    CHECK(cache.get_font_face_local("DejaVu Sans", false, false).get() == nullptr);
}

TEST_CASE("Font face cache: faces added by hand, the newest face wins a tie")
{
    using namespace nova::vg;
    font_face_cache cache;

    const std::string regular_file = fonts_dir() + "/DejaVuSans.ttf";
    const std::string bold_file = fonts_dir() + "/DejaVuSans-Bold.ttf";
    CHECK_FALSE(cache.add_file("Mine", false, false, "/nonexistent/none.ttf", 0));
    REQUIRE(cache.add_file("Mine", false, false, regular_file.c_str(), 0));
    const stb::font* first = cache.get_font_face_local("Mine", false, false).get();
    REQUIRE(first);

    REQUIRE(cache.add_file("Mine", true, false, bold_file.c_str(), 0));
    const stb::font* second = cache.get_font_face_local("Mine", true, false).get();
    REQUIRE(second);
    CHECK(second != first);
    CHECK(cache.get_font_face_local("Mine", false, false).get() == first);

    // Italic requested, neither face is italic: the regular face matches the bold flag, the bold one matches nothing.
    CHECK(cache.get_font_face_local("Mine", false, true).get() == first);

    // Two faces that score the same: the one added last wins.
    REQUIRE(cache.add_file("Tie", false, false, regular_file.c_str(), 0));
    const stb::font* tie_first = cache.get_font_face_local("Tie", false, false).get();
    REQUIRE(cache.add_file("Tie", false, false, regular_file.c_str(), 0));
    const stb::font* tie_second = cache.get_font_face_local("Tie", false, false).get();
    REQUIRE(tie_first);
    REQUIRE(tie_second);
    CHECK(tie_second != tie_first);
}

TEST_CASE("Font face cache: a face is known by its family and its legacy family name, loaded once")
{
    using namespace nova::vg;
    // tests/data/libstb-info.ttc (made by libstb's make_info_test_fonts.py): four faces. Face 2 is family "Info Sans",
    // italic, weight 300, and its name id 1 (the legacy family) is "Info Sans Light".
    const std::string file = (std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "libstb-info.ttc").string();
    font_face_cache cache;
    CHECK(cache.load_file(file.c_str()) == 4);
    CHECK(cache.load_file("/nonexistent/none.ttf") == 0);
    CHECK(cache.load_file(file.c_str()) == 4); // the same file again: the newer entries win ties

    const stb::font* regular = cache.get_font_face_local("Info Sans", false, false).get();
    const stb::font* italic = cache.get_font_face_local("Info Sans", false, true).get();
    REQUIRE(regular);
    REQUIRE(italic);
    CHECK(regular != italic);

    // font-family="Info Sans Light" names the same face as family "Info Sans" with the italic style: one face, loaded once
    CHECK(cache.get_font_face_local("Info Sans Light", false, true).get() == italic);
    CHECK(cache.get_font_face_local("Info Sans Light", false, false).get() == italic);
}

TEST_CASE("Font face cache: of one family the face nearest the normal width and the requested weight is chosen")
{
    using namespace nova::vg;
    const std::filesystem::path dir = "/usr/share/fonts/truetype/dejavu";
    if(!std::filesystem::exists(dir / "DejaVuSansCondensed.ttf") || !std::filesystem::exists(dir / "DejaVuSans-ExtraLight.ttf"))
        return; // the system fonts of this machine are not the ones this test is about

    // DejaVu Sans, ExtraLight (weight 200) and Condensed (width 4) all say family "DejaVu Sans" (name id 16). The
    // regular face is added first and the others after it, so a tie would go to a newer one: the choice is by style.
    font_face_cache cache;
    CHECK(cache.load_file((dir / "DejaVuSans.ttf").string().c_str()) == 1);
    const stb::font* regular = cache.get_font_face_local("DejaVu Sans", false, false).get();
    CHECK(cache.load_file((dir / "DejaVuSans-ExtraLight.ttf").string().c_str()) == 1);
    CHECK(cache.load_file((dir / "DejaVuSansCondensed.ttf").string().c_str()) == 1);
    CHECK(cache.load_file((dir / "DejaVuSansCondensed-Bold.ttf").string().c_str()) == 1);
    CHECK(cache.load_file((dir / "DejaVuSans-Bold.ttf").string().c_str()) == 1);
    const stb::font* bold = cache.get_font_face_local("DejaVu Sans", true, false).get();
    REQUIRE(regular);
    REQUIRE(bold);
    CHECK(regular != bold);
    CHECK(cache.get_font_face_local("DejaVu Sans", false, false).get() == regular); // not the Light, not the Condensed
    CHECK(cache.get_font_face_local("DejaVu Sans", true, false).get() == bold);      // not the Condensed Bold

    // the legacy names reach their own faces
    const stb::font* light = cache.get_font_face_local("DejaVu Sans Light", false, false).get();
    const stb::font* condensed = cache.get_font_face_local("DejaVu Sans Condensed", false, false).get();
    REQUIRE(light);
    REQUIRE(condensed);
    CHECK(light != regular);
    CHECK(condensed != regular);
    CHECK(light != condensed);
}
