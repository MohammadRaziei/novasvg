#include "doctest.h"
#include <vg/pdf_canvas.h>
#include <vg/font.h>

#include <cstdlib>
#include <string>

// pdf_canvas: the vector backend of the abstract canvas. These check the document's structure; the file the first
// test writes (when NOVA_PDF_OUT is set) can be opened in any PDF viewer.
using namespace nova::vg;

namespace {
std::string text_of(const std::vector<uint8_t>& bytes) { return std::string(bytes.begin(), bytes.end()); }
}

TEST_CASE("pdf_canvas: a page with shapes, a gradient, a clip and text")
{
    auto pdf = pdf_canvas::create(200, 120);

    pdf->set_color(0.9f, 0.2f, 0.1f, 1.f);
    stb::outline rect_;
    rect_.add_rect(10, 10, 80, 50);
    pdf->fill_path(rect_, fill_rule::non_zero, transform());

    stroke_data stroke(4.f);
    stroke.set_line_join(line_join::round);
    stroke.set_dash_array({6.f, 3.f});
    pdf->set_color(0.f, 0.f, 0.6f, 0.5f);
    stb::outline circle;
    circle.add_ellipse(140, 40, 40, 30);
    pdf->stroke_path(circle, stroke, transform());

    gradient_stops stops = {{0.f, stb::color(255, 255, 0)}, {0.5f, stb::color(0, 179, 51)}, {1.f, stb::color(0, 51, 230)}};
    pdf->save();
    pdf->clip_rect(rect(10, 70, 180, 40), fill_rule::non_zero, transform());
    pdf->set_linear_gradient(10, 0, 190, 0, spread_method_value::pad, stops, transform());
    stb::outline wide;
    wide.add_rect(0, 60, 200, 60);
    pdf->fill_path(wide, fill_rule::non_zero, transform());
    pdf->restore();

    pdf->set_color(0.f, 0.f, 0.f, 1.f);
    pdf->fill_text(U"Hello, PDF ffi", sized_font(embedded_fallback_font(), 16.f), point(14, 100), transform());

    auto bytes = pdf->data();
    std::string s = text_of(bytes);
    CHECK(s.rfind("%PDF-1.5", 0) == 0);
    CHECK(s.find("/MediaBox [0 0 150 90]") != std::string::npos);
    CHECK(s.find("/ShadingType 2") != std::string::npos);
    CHECK(s.find("/FunctionType 3") != std::string::npos);
    CHECK(s.find("startxref") != std::string::npos);
    CHECK(s.substr(s.size() - 6) == "%%EOF\n");

    if(const char* out = std::getenv("NOVA_PDF_OUT"))
        CHECK(pdf->write_to_file(out));
}

TEST_CASE("pdf_canvas: save and restore stay balanced")
{
    auto pdf = pdf_canvas::create(10, 10);
    pdf->save();
    pdf->save();
    pdf->restore();
    // one save() left open: data() closes it, the canvas keeps working
    auto a = pdf->data();
    pdf->restore();
    auto b = pdf->data();
    CHECK(!a.empty());
    CHECK(!b.empty());
    pdf->restore(); // extra restore is ignored
}

// ---- gradients: spreadMethod, one alpha for all stops, and the /Info entry ----

namespace {
std::string gradient_page(spread_method_value spread, const gradient_stops& stops, bool radial = false)
{
    auto pdf = pdf_canvas::create(100, 40);
    if(radial)
        pdf->set_radial_gradient(50, 20, 10, 50, 20, spread, stops, transform());
    else
        pdf->set_linear_gradient(40, 0, 50, 0, spread, stops, transform());
    stb::outline page;
    page.add_rect(0, 0, 100, 40);
    pdf->fill_path(page, fill_rule::non_zero, transform());
    return text_of(pdf->data());
}

size_t count(const std::string& text, const std::string& what)
{
    size_t n = 0;
    for(size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + 1))
        ++n;
    return n;
}

const gradient_stops red_to_blue = {{0.f, stb::color(255, 0, 0)}, {1.f, stb::color(0, 0, 255)}};
}  // namespace

TEST_CASE("pdf_canvas gradient: pad is one function, repeat and reflect are many periods")
{
    const std::string pad = gradient_page(spread_method_value::pad, red_to_blue);
    CHECK(count(pad, "/FunctionType 2") == 1);
    CHECK(pad.find("/FunctionType 3") == std::string::npos);

    // a 10 unit period over a 100 unit page: ten or so periods, every one a function of its own
    const std::string repeat = gradient_page(spread_method_value::repeat, red_to_blue);
    CHECK(count(repeat, "/FunctionType 2") >= 10);
    CHECK(repeat.find("/FunctionType 3") != std::string::npos);
    CHECK(repeat.find("/C0 [0 0 1] /C1 [1 0 0]") == std::string::npos);  // always red to blue

    const std::string reflect = gradient_page(spread_method_value::reflect, red_to_blue);
    CHECK(count(reflect, "/FunctionType 2") >= 10);
    CHECK(reflect.find("/C0 [1 0 0] /C1 [0 0 1]") != std::string::npos);
    CHECK(reflect.find("/C0 [0 0 1] /C1 [1 0 0]") != std::string::npos);  // every second period runs backwards
}

TEST_CASE("pdf_canvas gradient: a radial gradient repeats outwards")
{
    const std::string repeat = gradient_page(spread_method_value::repeat, red_to_blue, true);
    CHECK(repeat.find("/ShadingType 3") != std::string::npos);
    CHECK(count(repeat, "/FunctionType 2") >= 5);  // radius 10 on a page 100 wide: the circles reach about 55 units
}

TEST_CASE("pdf_canvas gradient: one alpha shared by all stops is a constant alpha, a fade is painted opaque")
{
    const gradient_stops half = {{0.f, stb::color(255, 0, 0, 51)}, {1.f, stb::color(0, 0, 255, 51)}};  // 51 / 255 = 0.2
    CHECK(gradient_page(spread_method_value::pad, half).find("/ca 0.2") != std::string::npos);

    const std::string opaque = gradient_page(spread_method_value::pad, red_to_blue);
    CHECK(opaque.find("/ca 0.2") == std::string::npos);

    const gradient_stops fade = {{0.f, stb::color(255, 0, 0, 255)}, {1.f, stb::color(255, 0, 0, 0)}};
    const std::string faded = gradient_page(spread_method_value::pad, fade);
    CHECK(faded.find("/ca 1") != std::string::npos);
}

TEST_CASE("pdf_canvas: the /Info entry")
{
    auto bare = pdf_canvas::create(10, 10);
    CHECK(text_of(bare->data()).find("/Info") == std::string::npos);  // nothing set, nothing written

    auto pdf = pdf_canvas::create(10, 10);
    pdf->set_info({"A (small) title", "novasvg test"});
    std::string s = text_of(pdf->data());
    CHECK(s.find("/Title (A \\(small\\) title)") != std::string::npos);  // parentheses escaped
    CHECK(s.find("/Producer (novasvg test)") != std::string::npos);
    CHECK(s.find("/Info ") != std::string::npos);
    CHECK(s.find("/Date") == std::string::npos);  // no date: the same drawing always gives the same bytes

    pdf->set_info({"\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85", ""});  // "salam" in Persian
    s = text_of(pdf->data());
    CHECK(s.find("/Title <FEFF0633064406270645>") != std::string::npos);
    CHECK(s.find("/Producer") == std::string::npos);
}
