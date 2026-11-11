#include "doctest.h"

#include <filesystem>
#include <set>
#include <string>

#include "vg/render.h"
#include "vg/font_face.h"

namespace {
std::string fonts_dir()
{
    return (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "fonts").string();
}
} // namespace

TEST_CASE("Font face cache: directory scan finds the family and picks the face by style")
{
    using namespace nova::vg;
    font_face_cache_t* cache = font_face_cache_create();

    // data/fonts holds the four DejaVu Sans faces.
    CHECK(font_face_cache_load_dir(cache, fonts_dir().c_str()) == 4);
    CHECK(font_face_cache_load_dir(cache, "/nonexistent/fonts/dir") == 0);

    font_face_data* regular = font_face_cache_get(cache, "DejaVu Sans", false, false);
    font_face_data* bold = font_face_cache_get(cache, "DejaVu Sans", true, false);
    font_face_data* italic = font_face_cache_get(cache, "DejaVu Sans", false, true);
    font_face_data* bold_italic = font_face_cache_get(cache, "DejaVu Sans", true, true);
    REQUIRE(regular);
    REQUIRE(bold);
    REQUIRE(italic);
    REQUIRE(bold_italic);
    CHECK(std::set<font_face_data*>{regular, bold, italic, bold_italic}.size() == 4);

    // A face is loaded once and then served from the cache.
    CHECK(font_face_cache_get(cache, "DejaVu Sans", true, false) == bold);

    CHECK(font_face_cache_get(cache, "No Such Family", false, false) == nullptr);

    font_face_cache_reset(cache);
    CHECK(font_face_cache_get(cache, "DejaVu Sans", false, false) == nullptr);
    font_face_cache_destroy(cache);
}

TEST_CASE("Font face cache: faces added by hand, reference counting, the newest face wins a tie")
{
    using namespace nova::vg;
    font_face_cache_t* cache = font_face_cache_create();
    CHECK(font_face_cache_reference_count(cache) == 1);
    font_face_cache_reference(cache);
    CHECK(font_face_cache_reference_count(cache) == 2);

    const std::string regular_file = fonts_dir() + "/DejaVuSans.ttf";
    const std::string bold_file = fonts_dir() + "/DejaVuSans-Bold.ttf";
    CHECK_FALSE(font_face_cache_add_file(cache, "Mine", false, false, "/nonexistent/none.ttf", 0));
    REQUIRE(font_face_cache_add_file(cache, "Mine", false, false, regular_file.c_str(), 0));
    font_face_data* first = font_face_cache_get(cache, "Mine", false, false);
    REQUIRE(first);

    REQUIRE(font_face_cache_add_file(cache, "Mine", true, false, bold_file.c_str(), 0));
    font_face_data* second = font_face_cache_get(cache, "Mine", true, false);
    REQUIRE(second);
    CHECK(second != first);
    CHECK(font_face_cache_get(cache, "Mine", false, false) == first);

    // Italic requested, neither face is italic: the regular face matches the bold flag, the bold one matches nothing.
    CHECK(font_face_cache_get(cache, "Mine", false, true) == first);

    // Two faces that score the same: the one added last wins.
    REQUIRE(font_face_cache_add_file(cache, "Tie", false, false, regular_file.c_str(), 0));
    font_face_data* tie_first = font_face_cache_get(cache, "Tie", false, false);
    REQUIRE(font_face_cache_add_file(cache, "Tie", false, false, regular_file.c_str(), 0));
    font_face_data* tie_second = font_face_cache_get(cache, "Tie", false, false);
    REQUIRE(tie_first);
    REQUIRE(tie_second);
    CHECK(tie_second != tie_first);

    font_face_cache_destroy(cache);
    CHECK(font_face_cache_reference_count(cache) == 1);
    font_face_cache_destroy(cache);
}

TEST_CASE("Font face cache: a face is known by its family and its legacy family name, loaded once")
{
    using namespace nova::vg;
    // tests/data/libstb-info.ttc (made by libstb's make_info_test_fonts.py): four faces. Face 2 is family "Info Sans",
    // italic, weight 300, and its name id 1 (the legacy family) is "Info Sans Light".
    const std::string file = (std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "libstb-info.ttc").string();
    font_face_cache_t* cache = font_face_cache_create();
    CHECK(font_face_cache_load_file(cache, file.c_str()) == 4);
    CHECK(font_face_cache_load_file(cache, "/nonexistent/none.ttf") == 0);
    CHECK(font_face_cache_load_file(cache, file.c_str()) == 4); // the same file again: the newer entries win ties

    font_face_data* regular = font_face_cache_get(cache, "Info Sans", false, false);
    font_face_data* italic = font_face_cache_get(cache, "Info Sans", false, true);
    REQUIRE(regular);
    REQUIRE(italic);
    CHECK(regular != italic);

    // font-family="Info Sans Light" names the same face as family "Info Sans" with the italic style: one face, loaded once
    CHECK(font_face_cache_get(cache, "Info Sans Light", false, true) == italic);
    CHECK(font_face_cache_get(cache, "Info Sans Light", false, false) == italic);
    font_face_cache_destroy(cache);
}

TEST_CASE("Font face cache: of one family the face nearest the normal width and the requested weight is chosen")
{
    using namespace nova::vg;
    const std::filesystem::path dir = "/usr/share/fonts/truetype/dejavu";
    if(!std::filesystem::exists(dir / "DejaVuSansCondensed.ttf") || !std::filesystem::exists(dir / "DejaVuSans-ExtraLight.ttf"))
        return; // the system fonts of this machine are not the ones this test is about

    // DejaVu Sans, ExtraLight (weight 200) and Condensed (width 4) all say family "DejaVu Sans" (name id 16). The
    // regular face is added first and the others after it, so a tie would go to a newer one: the choice is by style.
    font_face_cache_t* cache = font_face_cache_create();
    CHECK(font_face_cache_load_file(cache, (dir / "DejaVuSans.ttf").string().c_str()) == 1);
    font_face_data* regular = font_face_cache_get(cache, "DejaVu Sans", false, false);
    CHECK(font_face_cache_load_file(cache, (dir / "DejaVuSans-ExtraLight.ttf").string().c_str()) == 1);
    CHECK(font_face_cache_load_file(cache, (dir / "DejaVuSansCondensed.ttf").string().c_str()) == 1);
    CHECK(font_face_cache_load_file(cache, (dir / "DejaVuSansCondensed-Bold.ttf").string().c_str()) == 1);
    CHECK(font_face_cache_load_file(cache, (dir / "DejaVuSans-Bold.ttf").string().c_str()) == 1);
    font_face_data* bold = font_face_cache_get(cache, "DejaVu Sans", true, false);
    REQUIRE(regular);
    REQUIRE(bold);
    CHECK(regular != bold);
    CHECK(font_face_cache_get(cache, "DejaVu Sans", false, false) == regular); // not the Light, not the Condensed
    CHECK(font_face_cache_get(cache, "DejaVu Sans", true, false) == bold);      // not the Condensed Bold

    // the legacy names reach their own faces
    font_face_data* light = font_face_cache_get(cache, "DejaVu Sans Light", false, false);
    font_face_data* condensed = font_face_cache_get(cache, "DejaVu Sans Condensed", false, false);
    REQUIRE(light);
    REQUIRE(condensed);
    CHECK(light != regular);
    CHECK(condensed != regular);
    CHECK(light != condensed);
    font_face_cache_destroy(cache);
}
