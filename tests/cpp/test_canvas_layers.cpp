#include "doctest.h"

#include <cstdint>
#include <string>

#include <vg/pdf_canvas.h>

// The layer and pattern calls of the abstract canvas (ADR-0014), on both backends: a group drawn between
// begin_layer() and end_layer(), what a backend does when the group needs its pixels, and set_pattern().
using namespace nova::vg;

namespace {

// premultiplied ARGB32 at (x, y) of a raster canvas whose extents start at (0, 0)
std::uint32_t pixel(const raster_canvas& c, int x, int y)
{
    const auto& s = c.surface();
    pixmap p(s);
    return *reinterpret_cast<const std::uint32_t*>(p.data() + std::size_t(y) * std::size_t(p.stride()) + std::size_t(x) * 4);
}

int alpha(std::uint32_t argb) { return int(argb >> 24); }
int red(std::uint32_t argb) { return int((argb >> 16) & 0xff); }

void fill_rect(canvas& c, float x, float y, float w, float h, float r, float g, float b, float a)
{
    c.set_color(r, g, b, a);
    stb::outline path;
    path.add_rect(x, y, w, h);
    c.fill_path(path, fill_rule::non_zero, transform());
}

std::string text_of(const std::vector<std::uint8_t>& bytes) { return std::string(bytes.begin(), bytes.end()); }

}  // namespace

TEST_CASE("raster_canvas layer: the group is drawn on a canvas of its own and composited with its opacity")
{
    auto canvas_ = raster_canvas::create(0, 0, 8, 8);

    layer_params params;
    params.bounds = rect(2, 2, 4, 4);
    params.opacity = 0.5f;
    layer_scope scope = canvas_->begin_layer(params);
    REQUIRE(scope.target != nullptr);
    CHECK_FALSE(scope.in_place());
    CHECK(scope.target.get() != canvas_.get());
    CHECK(scope.pre_transform.x_scale() == doctest::Approx(1.f));

    // the layer covers its bounds: drawing in canvas coordinates lands inside it
    fill_rect(*scope.target, 0, 0, 8, 8, 1.f, 0.f, 0.f, 1.f);
    canvas_->end_layer(std::move(scope));

    CHECK(alpha(pixel(*canvas_, 3, 3)) == doctest::Approx(128).epsilon(0.02));  // half of an opaque red
    CHECK(red(pixel(*canvas_, 3, 3)) > 100);
    CHECK(alpha(pixel(*canvas_, 0, 0)) == 0);                                   // outside the bounds nothing is touched
    CHECK(alpha(pixel(*canvas_, 7, 7)) == 0);
}

TEST_CASE("raster_canvas layer: the bounds are clipped to the canvas")
{
    auto canvas_ = raster_canvas::create(0, 0, 8, 8);
    layer_params params;
    params.bounds = rect(-100, -100, 1000, 1000);
    layer_scope scope = canvas_->begin_layer(params);
    auto& layer = static_cast<raster_canvas&>(*scope.target);
    CHECK(layer.width() == 8);
    CHECK(layer.height() == 8);
    canvas_->end_layer(std::move(scope));
}

TEST_CASE("raster_canvas layer: effects run on the finished pixels, and only for a group that asked for them")
{
    auto canvas_ = raster_canvas::create(0, 0, 8, 8);

    int calls = 0;
    layer_effects count = [&](raster_canvas& layer) {
        ++calls;
        layer.convert_to_luminance_mask();  // any change to the pixels will do
    };

    layer_params plain;
    plain.bounds = rect(0, 0, 8, 8);
    layer_scope a = canvas_->begin_layer(plain);
    fill_rect(*a.target, 0, 0, 8, 8, 1.f, 0.f, 0.f, 1.f);
    canvas_->end_layer(std::move(a), count);
    CHECK(calls == 0);  // needs_pixels was not set: the effects are not called

    layer_params wanted = plain;
    wanted.needs_pixels = true;
    layer_scope b = canvas_->begin_layer(wanted);
    fill_rect(*b.target, 0, 0, 8, 8, 1.f, 0.f, 0.f, 1.f);
    canvas_->end_layer(std::move(b), count);
    CHECK(calls == 1);
}

TEST_CASE("pdf_canvas layer: a group without effects stays a vector transparency group")
{
    auto pdf = pdf_canvas::create(100, 60);
    layer_params params;
    params.bounds = rect(0, 0, 100, 60);
    params.opacity = 0.4f;
    layer_scope scope = pdf->begin_layer(params);
    CHECK(scope.in_place());
    CHECK(scope.target == nullptr);
    fill_rect(*pdf, 10, 10, 40, 30, 0.f, 0.f, 1.f, 1.f);  // drawn on the page itself
    pdf->end_layer(std::move(scope));

    const std::string s = text_of(pdf->data());
    CHECK(s.find("/Subtype /Form") != std::string::npos);
    CHECK(s.find("/S /Transparency") != std::string::npos);
    CHECK(s.find("/Subtype /Image") == std::string::npos);  // no bitmap was made
}

TEST_CASE("pdf_canvas layer: a group that needs pixels is a bitmap at raster_scale(), through the effects")
{
    auto pdf = pdf_canvas::create(100, 60);
    pdf->set_raster_scale(2.f);
    CHECK(pdf->raster_scale() == doctest::Approx(2.f));

    layer_params params;
    params.bounds = rect(10, 10, 40, 30);
    params.needs_pixels = true;
    layer_scope scope = pdf->begin_layer(params);
    REQUIRE(scope.target != nullptr);
    CHECK_FALSE(scope.in_place());
    CHECK(scope.pre_transform.x_scale() == doctest::Approx(2.f));  // children draw at twice the page's units

    // the bitmap is 40 x 30 user units = 80 x 60 pixels
    auto& layer = static_cast<raster_canvas&>(*scope.target);
    CHECK(layer.width() == 80);
    CHECK(layer.height() == 60);

    int calls = 0;
    pdf->end_layer(std::move(scope), [&](raster_canvas& given) {
        ++calls;
        CHECK(&given == &layer);
    });
    CHECK(calls == 1);
    CHECK(text_of(pdf->data()).find("/Subtype /Image") != std::string::npos);
}

TEST_CASE("set_pattern: a tile repeated, on both backends")
{
    // a 2 x 2 tile: left column red, right column blue
    auto tile_canvas = raster_canvas::create(0, 0, 2, 2);
    fill_rect(*tile_canvas, 0, 0, 1, 2, 1.f, 0.f, 0.f, 1.f);
    fill_rect(*tile_canvas, 1, 0, 1, 2, 0.f, 0.f, 1.f, 1.f);
    const pixmap tile(tile_canvas->surface());

    auto target = raster_canvas::create(0, 0, 8, 4);
    CHECK(target->raster_scale() == doctest::Approx(1.f));
    target->set_pattern(tile, transform(), 1.f);
    stb::outline all;
    all.add_rect(0, 0, 8, 4);
    target->fill_path(all, fill_rule::non_zero, transform());
    CHECK(red(pixel(*target, 0, 0)) == 255);  // red where the tile is red
    CHECK(red(pixel(*target, 1, 0)) == 0);    // blue next to it
    CHECK(red(pixel(*target, 4, 3)) == 255);  // and again, four pixels on

    auto pdf = pdf_canvas::create(8, 4);
    pdf->set_pattern(tile, transform(), 1.f);
    pdf->fill_path(all, fill_rule::non_zero, transform());
    const std::string s = text_of(pdf->data());
    CHECK(s.find("/PatternType 1") != std::string::npos);
    CHECK(s.find("/Subtype /Image") != std::string::npos);
}
