#ifndef NOVA_CANVAS_H
#define NOVA_CANVAS_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <utility>
#include <memory>
#include <vector>
#include <array>
#include <string>

#include <stb/outline.hpp>  // stb::matrix

// raster_canvas: novasvg's 2D drawing surface (paths, fills, strokes, gradients,
// text, blending), plus the Path/Rect/Transform/Point primitives that go
// with it. Self-contained -- pulls in exactly the vg:: engine pieces
// it needs plus Font (for fill_text/stroke_text) -- so it's usable without
// the svg/parsing layer, e.g. to draw directly onto a pixmap.
#include "color.h"
#include "font.h"
#include "pixmap.h"
#include "render.h"

namespace nova::vg {

enum class LineCap : uint8_t {
    Butt = NOVA_LINE_CAP_BUTT,
    Round = NOVA_LINE_CAP_ROUND,
    Square = NOVA_LINE_CAP_SQUARE
};

enum class LineJoin : uint8_t {
    Miter = NOVA_LINE_JOIN_MITER,
    Round = NOVA_LINE_JOIN_ROUND,
    Bevel = NOVA_LINE_JOIN_BEVEL
};

enum class FillRule : uint8_t {
    NonZero = NOVA_FILL_RULE_NON_ZERO,
    EvenOdd = NOVA_FILL_RULE_EVEN_ODD
};

enum class SpreadMethod : uint8_t {
    pad = NOVA_SPREAD_METHOD_PAD,
    reflect = NOVA_SPREAD_METHOD_REFLECT,
    repeat = NOVA_SPREAD_METHOD_REPEAT
};

// `stb::color` now lives in color.h (a single merged class covering both
// this engine's needs and the richer string-parsing/blending API) --
// no separate definition here anymore.

class Point {
public:
    constexpr Point() = default;
    constexpr Point(const point_t& point) : Point(point.x, point.y) {}
    constexpr Point(float x, float y) : x(x), y(y) {}

    constexpr void move(float dx, float dy) { x += dx; y += dy; }
    constexpr void move(float d) { move(d, d); }
    constexpr void move(const Point& p) { move(p.x, p.y); }

    constexpr void scale(float sx, float sy) { x *= sx; y *= sy; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr float dot(const Point& p) const { return x * p.x + y * p.y; }

public:
    float x{0};
    float y{0};
};

constexpr Point operator+(const Point& a, const Point& b)
{
    return Point(a.x + b.x, a.y + b.y);
}

constexpr Point operator-(const Point& a, const Point& b)
{
    return Point(a.x - b.x, a.y - b.y);
}

constexpr Point operator-(const Point& a)
{
    return Point(-a.x, -a.y);
}

constexpr Point& operator+=(Point& a, const Point& b)
{
    a.move(b);
    return a;
}

constexpr Point& operator-=(Point& a, const Point& b)
{
    a.move(-b);
    return a;
}

constexpr float operator*(const Point& a, const Point& b)
{
    return a.dot(b);
}

class Size {
public:
    constexpr Size() = default;
    constexpr Size(float w, float h) : w(w), h(h) {}

    constexpr void expand(float dw, float dh) { w += dw; h += dh; }
    constexpr void expand(float d) { expand(d, d); }
    constexpr void expand(const Size& s) { expand(s.w, s.h); }

    constexpr void scale(float sw, float sh) { w *= sw; h *= sh; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr bool empty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool is_zero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool is_valid() const { return w >= 0.f && h >= 0.f; }

public:
    float w{0};
    float h{0};
};

constexpr Size operator+(const Size& a, const Size& b)
{
    return Size(a.w + b.w, a.h + b.h);
}

constexpr Size operator-(const Size& a, const Size& b)
{
    return Size(a.w - b.w, a.h - b.h);
}

constexpr Size operator-(const Size& a)
{
    return Size(-a.w, -a.h);
}

constexpr Size& operator+=(Size& a, const Size& b)
{
    a.expand(b);
    return a;
}

constexpr Size& operator-=(Size& a, const Size& b)
{
    a.expand(-b);
    return a;
}

class Box;

class Rect {
public:
    constexpr Rect() = default;
    constexpr explicit Rect(const Size& size) : Rect(size.w, size.h) {}
    constexpr Rect(float width, float height) : Rect(0, 0, width, height) {}
    constexpr Rect(const Point& origin, const Size& size) : Rect(origin.x, origin.y, size.w, size.h) {}
    constexpr Rect(const rect_t& rect) : Rect(rect.x, rect.y, rect.w, rect.h) {}
    constexpr Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    Rect(const Box& box);

    constexpr void move(float dx, float dy) { x += dx; y += dy; }
    constexpr void move(float d) { move(d, d); }
    constexpr void move(const Point& p) { move(p.x, p.y); }

    constexpr void scale(float sx, float sy) { x *= sx; y *= sy; w *= sx; h *= sy; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr void inflate(float dx, float dy) { x -= dx; y -= dy; w += dx * 2.f; h += dy * 2.f; }
    constexpr void inflate(float d) { inflate(d, d); }

    constexpr bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    constexpr bool contains(const Point& p) const { return contains(p.x, p.y); }

    constexpr Rect intersected(const Rect& rect) const;
    constexpr Rect united(const Rect& rect) const;

    constexpr Rect& intersect(const Rect& o);
    constexpr Rect& unite(const Rect& o);

    constexpr Point origin() const { return Point(x, y); }
    constexpr Size size() const { return Size(w, h); }

    constexpr float right() const { return x + w; }
    constexpr float bottom() const { return y + h; }

    constexpr bool empty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool is_zero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool is_valid() const { return w >= 0.f && h >= 0.f; }

    static const Rect Empty;
    static const Rect invalid;
    static const Rect infinite;

public:
    float x{0};
    float y{0};
    float w{0};
    float h{0};
};

constexpr Rect Rect::intersected(const Rect& rect) const
{
    if(!rect.is_valid())
        return *this;
    if(!is_valid())
        return rect;
    auto l = STB_MAX(x, rect.x);
    auto t = STB_MAX(y, rect.y);
    auto r = STB_MIN(x + w, rect.x + rect.w);
    auto b = STB_MIN(y + h, rect.y + rect.h);
    if(l >= r || t >= b)
        return Rect::Empty;
    return Rect(l, t, r - l, b - t);
}

constexpr Rect Rect::united(const Rect& rect) const
{
    if(!rect.is_valid())
        return *this;
    if(!is_valid())
        return rect;
    auto l = STB_MIN(x, rect.x);
    auto t = STB_MIN(y, rect.y);
    auto r = STB_MAX(x + w, rect.x + rect.w);
    auto b = STB_MAX(y + h, rect.y + rect.h);
    return Rect(l, t, r - l, b - t);
}

constexpr Rect& Rect::intersect(const Rect& o)
{
    *this = intersected(o);
    return *this;
}

constexpr Rect& Rect::unite(const Rect& o)
{
    *this = united(o);
    return *this;
}


class Transform {
public:
    Transform();
    Transform(const stb::matrix& matrix);
    Transform(float a, float b, float c, float d, float e, float f);
    Transform(const matrix_t& matrix) : m_matrix(matrix) {}

    Transform operator*(const Transform& transform) const;
    Transform& operator*=(const Transform& transform);

    Transform& multiply(const Transform& transform);
    Transform& translate(float tx, float ty);
    Transform& scale(float sx, float sy);
    Transform& rotate(float angle, float cx = 0.f, float cy = 0.f);
    Transform& shear(float shx, float shy);

    Transform& post_multiply(const Transform& transform);
    Transform& post_translate(float tx, float ty);
    Transform& post_scale(float sx, float sy);
    Transform& post_rotate(float angle, float cx = 0.f, float cy = 0.f);
    Transform& post_shear(float shx, float shy);

    Transform inverse() const;
    Transform& invert();

    void reset();

    Point map_point(float x, float y) const;
    Point map_point(const Point& point) const;
    Rect map_rect(const Rect& rect) const;

    float x_scale() const;
    float y_scale() const;

    const matrix_t& matrix() const { return m_matrix; }
    matrix_t& matrix() { return m_matrix; }

    bool parse(const char* data, size_t length);

    static Transform translated(float tx, float ty);
    static Transform scaled(float sx, float sy);
    static Transform rotated(float angle, float cx, float cy);
    static Transform sheared(float shx, float shy);

    static const Transform identity;

private:
    matrix_t m_matrix;
};

// Conversions between the engine's Transform / the C API's matrix_t and the public stb::matrix.
inline stb::matrix to_matrix(const matrix_t& m) { return stb::matrix{m.a, m.b, m.c, m.d, m.e, m.f}; }
inline stb::matrix to_matrix(const Transform& t) { return to_matrix(t.matrix()); }

// A path is an stb::outline. The engine needs a few things on top of it (src/sources/vg/outline_util.cpp):
//   arc_to      the SVG elliptical arc, as cubics, from the outline's current point
//   parse_path  SVG path data ("M 0 0 L ..."), replaces the content; partial data stays on error, as in SVG
//   bounding_rect  the box of all points of the outline (what the svg layer calls the fill box)
//   to_path_t   the C API's path_t (the rasterizer's input); the caller owns the result
void arc_to(stb::outline& path, float rx, float ry, float x_axis_rotation_deg, bool large_arc, bool sweep, float x, float y);
bool parse_path(stb::outline& path, const char* data, size_t length);
Rect bounding_rect(const stb::outline& path);
path_t* to_path_t(const stb::outline& path);
stb::outline from_path_t(const path_t* path);

enum class segment_kind { move_to, line_to, cubic_to, close };

// Walks an outline as move / line / cubic / close. A quadratic curve is raised to a cubic; `close` carries the
// start point of its contour in points[0] (the C API's path_t did the same).
class outline_walker {
public:
    explicit outline_walker(const stb::outline& path);
    segment_kind current_segment(std::array<Point, 3>& points) const;
    bool done() const { return m_index >= m_segments.size(); }
    void next() { ++m_index; }
private:
    struct segment {
        segment_kind command;
        Point points[3];
    };
    std::vector<segment> m_segments;
    size_t m_index = 0;
};
} // namespace nova::vg

namespace nova::vg {

enum class texture_type {
    Plain = NOVA_TEXTURE_TYPE_PLAIN,
    Tiled = NOVA_TEXTURE_TYPE_TILED
};

enum class BlendMode {
    Src = NOVA_OPERATOR_SRC,
    src_over = NOVA_OPERATOR_SRC_OVER,
    dst_in = NOVA_OPERATOR_DST_IN,
    dst_out = NOVA_OPERATOR_DST_OUT,
    Multiply = NOVA_OPERATOR_MULTIPLY,
    screen = NOVA_OPERATOR_SCREEN,
    darken = NOVA_OPERATOR_DARKEN,
    lighten = NOVA_OPERATOR_LIGHTEN,
    difference = NOVA_OPERATOR_DIFFERENCE,
    exclusion = NOVA_OPERATOR_EXCLUSION
};

using DashArray = std::vector<float>;

class StrokeData {
public:
    explicit StrokeData(float lineWidth = 1.f) : m_line_width(lineWidth) {}

    void set_line_width(float lineWidth) { m_line_width = lineWidth; }
    float lineWidth() const { return m_line_width; }

    void set_miter_limit(float miterLimit) { m_miter_limit = miterLimit; }
    float miterLimit() const { return m_miter_limit; }

    void set_dash_offset(float dashOffset) { m_dash_offset = dashOffset; }
    float dashOffset() const { return m_dash_offset; }

    void set_dash_array(DashArray dashArray) { m_dash_array = std::move(dashArray); }
    const DashArray& dashArray() const { return m_dash_array; }

    void set_line_cap(LineCap lineCap) { m_line_cap = lineCap; }
    LineCap lineCap() const { return m_line_cap; }

    void set_line_join(LineJoin lineJoin) { m_line_join = lineJoin; }
    LineJoin lineJoin() const { return m_line_join; }

private:
    float m_line_width;
    float m_miter_limit{4.f};
    float m_dash_offset{0.f};
    LineCap m_line_cap{LineCap::Butt};
    LineJoin m_line_join{LineJoin::Miter};
    DashArray m_dash_array;
};

using GradientStop = gradient_stop_t;
using GradientStops = std::vector<GradientStop>;

class pixmap;

/// Backend-independent vector drawing target.
///
/// Holds exactly the operations every backend (raster today, PDF later) can express: a paint source,
/// fill/stroke of paths and text, clipping, images, and a save/restore stack. Pixel-level operations
/// (textures, layer blending, masks, filters) are NOT part of it yet: they live in `raster_canvas`
/// and are the remaining work before an SVG document can be rendered to a non-raster backend.
class canvas {
public:
    virtual ~canvas() = default;

    virtual void set_color(const stb::color& color) = 0;
    virtual void set_color(float r, float g, float b, float a) = 0;
    virtual void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform) = 0;
    virtual void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform) = 0;

    virtual void fill_path(const stb::outline& path, FillRule fill_rule, const Transform& transform) = 0;
    virtual void stroke_path(const stb::outline& path, const StrokeData& stroke_data, const Transform& transform) = 0;

    virtual void fill_text(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform) = 0;
    virtual void stroke_text(const std::u32string_view& text, float stroke_width, const Font& font, const Point& origin, const Transform& transform) = 0;

    virtual void clip_path(const stb::outline& path, FillRule clip_rule, const Transform& transform) = 0;
    virtual void clip_rect(const Rect& rect, FillRule clip_rule, const Transform& transform) = 0;

    virtual void draw_image(const pixmap& image, const Rect& dst_rect, const Rect& src_rect, const Transform& transform) = 0;

    virtual void save() = 0;
    virtual void restore() = 0;
};

class raster_canvas final : public canvas {
public:
    static std::shared_ptr<raster_canvas> create(const pixmap& bitmap);
    static std::shared_ptr<raster_canvas> create(float x, float y, float width, float height);
    static std::shared_ptr<raster_canvas> create(const Rect& extents);

    void set_color(const stb::color& color) override;
    void set_color(float r, float g, float b, float a) override;
    void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;
    void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;
    void set_texture(const raster_canvas& source, texture_type type, float opacity, const Transform& transform);

    void fill_path(const stb::outline& path, FillRule fillRule, const Transform& transform) override;
    void stroke_path(const stb::outline& path, const StrokeData& strokeData, const Transform& transform) override;

    void fill_text(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform) override;
    void stroke_text(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform) override;

    void clip_path(const stb::outline& path, FillRule clipRule, const Transform& transform) override;
    void clip_rect(const Rect& rect, FillRule clipRule, const Transform& transform) override;

    void draw_image(const pixmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform) override;
    void blend_canvas(const raster_canvas& canvas, BlendMode blendMode, float opacity);

    void save() override;
    void restore() override;

    void convert_to_luminance_mask();
    // standard "fast almost-gaussian" trick browsers/thorvg also use)
    // rather than a true gaussian convolution kernel -- visually
    // indistinguishable for the stdDeviation ranges svg content actually
    // uses, and O(w*h) instead of O(w*h*kernel). Upgrade path: replace the
    // naive per-pixel box blur loop with a moving-sum/summed-area version
    // if profiling ever shows this mattering for very large canvases.
    void box_blur(float std_deviation_x, float std_deviation_y);

    // Composites `this` (already blurred, tinted to floodColor/floodOpacity
    // by the caller) as a drop shadow: offsets by (dx, dy) and draws
    // `original` on top of it, replacing this canvas's contents in place.
    void composite_drop_shadow_under(const raster_canvas& original, float dx, float dy);

    // Recolors every pixel to floodColor while preserving its alpha
    // (scaled by floodOpacity), for feDropShadow/feFlood.
    void tint_to_flood_color(const stb::color& floodColor, float floodOpacity);

    // Fills every pixel fully opaque white; paired with tint_to_flood_color()
    // this gives feFlood's "solid flood-color rectangle" result.
    void fill_opaque_white();

    // Shifts this canvas's pixels by (dx, dy) in place; area exposed at
    // the edges becomes transparent.
    void shift(float dx, float dy);

    // Porter-Duff compositing operators needed by feComposite/feMerge.
    // (arithmetic is intentionally not offered -- see svg_fe_composite_element)
    enum class porter_duff { Over, In, Out, atop, Xor };
    void composite_with(const raster_canvas& src, porter_duff mode);
    void composite_over(const raster_canvas& src) ;

    // feComposite operator="arithmetic": result = k1*i1*i2 + k2*i1 + k3*i2 + k4
    // per channel, in normalized [0,1] premultiplied space, clamped back to
    // [0,1]. `src` is i1, `this` (already holding in2's pixels) is i2 --
    // the result replaces `this` in place.
    void composite_arithmetic(const raster_canvas& src, float k1, float k2, float k3, float k4);

    // Raw memcpy of `other`'s pixel buffer into this one -- both must be
    // the same width/height/stride (caller's responsibility).
    void copy_pixels_from(const raster_canvas& other);

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const;
    int height() const;

    Rect extents() const { return Rect(m_x, m_y, width(), height()); }

    surface_t* surface() const { return m_surface; }
    canvas_t* handle() const { return m_canvas; }

    ~raster_canvas();

private:
    raster_canvas(const pixmap& bitmap);
    raster_canvas(int x, int y, int width, int height);
    surface_t* m_surface;
    canvas_t* m_canvas;
    matrix_t m_translation;
    const int m_x;
    const int m_y;
};

#include <cfloat>
#include <cmath>

// ---------------------------------------------------------------
// Implementation (formerly graphics.hpp, merged in -- everything
// here is NOVA_INLINE, so there is no more reason to keep
// declarations and definitions in separate files).
// ---------------------------------------------------------------

// stb::color::black()/White/Transparent are defined once, in color.h.

NOVA_INLINE const Rect Rect::Empty(0, 0, 0, 0);
NOVA_INLINE const Rect Rect::invalid(0, 0, -1, -1);
NOVA_INLINE const Rect Rect::infinite(-FLT_MAX / 2.f, -FLT_MAX / 2.f, FLT_MAX, FLT_MAX);

// Rect::Rect(const Box&) is defined in novasvg.h, right after Box itself --
// Box belongs to the svg/document layer, not here, so this file (which
// stays usable without that layer) only declares the conversion, it
// doesn't define it.

NOVA_INLINE const Transform Transform::identity(1, 0, 0, 1, 0, 0);



// Transform::Transform(const stb::matrix&) is defined in geometry.cpp, so
// this file (which stays usable without that layer) only declares the
// conversion, it doesn't define it.










































































// ponytail: single-axis box blur pass, edge-extended, operating in place
// on premultiplied ARGB. Blurring premultiplied channels directly (rather
// than un-premultiplying first) is correct and is what real
// implementations do too.
// One box-blur pass over an ARGB32 surface: every pixel becomes the average of the (2*radius+1) pixels along its
// row (horizontal) or column, with the edge pixel standing in for anything past the edge.
//
// Runs a sliding window -- add the pixel entering the window, drop the one leaving -- so the cost per pixel does
// not depend on the radius. The sums are exact integers, and each is converted back with the same
// `uint32_t(float(sum) * (1 / (2*radius+1)))` as a window summed from scratch would be, so the output is
// identical to the straightforward version.
static void box_blur_pass(unsigned char* data, int width, int height, int stride, int radius, bool horizontal)
{
    if(radius <= 0 || width <= 0 || height <= 0)
        return;
    auto* scratch = static_cast<unsigned char*>(malloc(size_t(stride) * size_t(height)));
    if(!scratch)
        return;
    memcpy(scratch, data, size_t(stride) * size_t(height));

    const float iarr = 1.f / float(radius * 2 + 1);
    auto pack = [iarr](int32_t a, int32_t r, int32_t g, int32_t b) {
        return (uint32_t(float(a) * iarr) << 24) | (uint32_t(float(r) * iarr) << 16) | (uint32_t(float(g) * iarr) << 8)
            | uint32_t(float(b) * iarr);
    };

    if(horizontal) {
        for(int y = 0; y < height; ++y) {
            const auto* in = reinterpret_cast<const uint32_t*>(scratch + size_t(y) * size_t(stride));
            auto* out = reinterpret_cast<uint32_t*>(data + size_t(y) * size_t(stride));
            auto at = [in, width](int x) { return in[x < 0 ? 0 : (x >= width ? width - 1 : x)]; };

            int32_t a = 0, r = 0, g = 0, b = 0;
            for(int k = -radius; k <= radius; ++k) {
                const uint32_t pixel = at(k);
                a += (pixel >> 24) & 0xFF;
                r += (pixel >> 16) & 0xFF;
                g += (pixel >> 8) & 0xFF;
                b += pixel & 0xFF;
            }

            for(int x = 0; x < width; ++x) {
                out[x] = pack(a, r, g, b);
                const uint32_t leaving = at(x - radius);
                const uint32_t entering = at(x + radius + 1);
                a += int32_t((entering >> 24) & 0xFF) - int32_t((leaving >> 24) & 0xFF);
                r += int32_t((entering >> 16) & 0xFF) - int32_t((leaving >> 16) & 0xFF);
                g += int32_t((entering >> 8) & 0xFF) - int32_t((leaving >> 8) & 0xFF);
                b += int32_t(entering & 0xFF) - int32_t(leaving & 0xFF);
            }
        }
    } else {
        // Column sums for the whole row at once, so memory is read row by row rather than down a column.
        std::vector<int32_t> sums(size_t(width) * 4, 0);
        auto row = [&](int y) {
            y = y < 0 ? 0 : (y >= height ? height - 1 : y);
            return reinterpret_cast<const uint32_t*>(scratch + size_t(y) * size_t(stride));
        };

        for(int k = -radius; k <= radius; ++k) {
            const uint32_t* p = row(k);
            for(int x = 0; x < width; ++x) {
                const uint32_t pixel = p[x];
                sums[size_t(x) * 4 + 0] += (pixel >> 24) & 0xFF;
                sums[size_t(x) * 4 + 1] += (pixel >> 16) & 0xFF;
                sums[size_t(x) * 4 + 2] += (pixel >> 8) & 0xFF;
                sums[size_t(x) * 4 + 3] += pixel & 0xFF;
            }
        }

        for(int y = 0; y < height; ++y) {
            auto* out = reinterpret_cast<uint32_t*>(data + size_t(y) * size_t(stride));
            const uint32_t* leaving = row(y - radius);
            const uint32_t* entering = row(y + radius + 1);
            for(int x = 0; x < width; ++x) {
                int32_t* sum = &sums[size_t(x) * 4];
                out[x] = pack(sum[0], sum[1], sum[2], sum[3]);
                const uint32_t lp = leaving[x], ep = entering[x];
                sum[0] += int32_t((ep >> 24) & 0xFF) - int32_t((lp >> 24) & 0xFF);
                sum[1] += int32_t((ep >> 16) & 0xFF) - int32_t((lp >> 16) & 0xFF);
                sum[2] += int32_t((ep >> 8) & 0xFF) - int32_t((lp >> 8) & 0xFF);
                sum[3] += int32_t(ep & 0xFF) - int32_t(lp & 0xFF);
            }
        }
    }

    free(scratch);
}

// Converts a true gaussian stdDeviation to an equivalent box-blur radius
// (the well-known 3-box approximation formula), then runs 3 passes.
//
// The svg spec's formula gives `d`, the box *width* each pass should use
// (https://www.w3.org/TR/svg11/filters.html#feGaussianBlurElement) --
// boxBlurPass below takes a *radius* (box width = 2*radius+1), so d needs
// halving before rounding. Using d itself as the radius (i.e. a box width
// of 2d+1) doubles the effective blur: for stdDeviation=4 that's an
// effective sigma of ~8.5 instead of 4, visibly wider/softer than every
// other engine's render of the same filter.
static int gaussian_radius_for_sigma(float sigma)
{
    if(sigma <= 0.f)
        return 0;
    auto d = sigma * 3.f * std::sqrt(2.f * 3.14159265f) / 4.f;
    return STB_MAX(1, int(d / 2.f + 0.5f));
}













} // namespace nova::vg

#endif // NOVA_CANVAS_H
