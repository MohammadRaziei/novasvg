#include "doctest.h"
#include <gfx/pdf_canvas.h>
#include <gfx/font.h>

#include <cstdlib>
#include <string>

// pdf_canvas: the vector backend of the abstract canvas. These check the document's structure; the file the first
// test writes (when NOVASVG_PDF_OUT is set) can be opened in any PDF viewer.
using namespace novasvg;

namespace {
std::string text_of(const std::vector<uint8_t>& bytes) { return std::string(bytes.begin(), bytes.end()); }
}

TEST_CASE("pdf_canvas: a page with shapes, a gradient, a clip and text")
{
    auto pdf = pdf_canvas::create(200, 120);

    pdf->set_color(0.9f, 0.2f, 0.1f, 1.f);
    Path rect;
    rect.addRect(10, 10, 80, 50);
    pdf->fill_path(rect, FillRule::NonZero, Transform());

    StrokeData stroke(4.f);
    stroke.setLineJoin(LineJoin::Round);
    stroke.setDashArray({6.f, 3.f});
    pdf->set_color(0.f, 0.f, 0.6f, 0.5f);
    Path circle;
    circle.addEllipse(140, 40, 40, 30);
    pdf->stroke_path(circle, stroke, Transform());

    GradientStops stops = {{0.f, {1.f, 1.f, 0.f, 1.f}}, {0.5f, {0.f, 0.7f, 0.2f, 1.f}}, {1.f, {0.f, 0.2f, 0.9f, 1.f}}};
    pdf->save();
    pdf->clip_rect(Rect(10, 70, 180, 40), FillRule::NonZero, Transform());
    pdf->set_linear_gradient(10, 0, 190, 0, SpreadMethod::Pad, stops, Transform());
    Path wide;
    wide.addRect(0, 60, 200, 60);
    pdf->fill_path(wide, FillRule::NonZero, Transform());
    pdf->restore();

    pdf->set_color(0.f, 0.f, 0.f, 1.f);
    pdf->fill_text(U"Hello, PDF ffi", Font(embeddedFallbackFont(), 16.f), Point(14, 100), Transform());

    auto bytes = pdf->data();
    std::string s = text_of(bytes);
    CHECK(s.rfind("%PDF-1.5", 0) == 0);
    CHECK(s.find("/MediaBox [0 0 150 90]") != std::string::npos);
    CHECK(s.find("/ShadingType 2") != std::string::npos);
    CHECK(s.find("/FunctionType 3") != std::string::npos);
    CHECK(s.find("startxref") != std::string::npos);
    CHECK(s.substr(s.size() - 6) == "%%EOF\n");

    if(const char* out = std::getenv("NOVASVG_PDF_OUT"))
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
