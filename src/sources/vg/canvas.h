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

enum class line_cap : uint8_t {
    butt = NOVA_LINE_CAP_BUTT,
    round = NOVA_LINE_CAP_ROUND,
    square = NOVA_LINE_CAP_SQUARE
};

enum class line_join : uint8_t {
    miter = NOVA_LINE_JOIN_MITER,
    round = NOVA_LINE_JOIN_ROUND,
    bevel = NOVA_LINE_JOIN_BEVEL
};

enum class fill_rule : uint8_t {
    non_zero = NOVA_FILL_RULE_NON_ZERO,
    even_odd = NOVA_FILL_RULE_EVEN_ODD
};

enum class SpreadMethod : uint8_t {
    pad = NOVA_SPREAD_METHOD_PAD,
    reflect = NOVA_SPREAD_METHOD_REFLECT,
    repeat = NOVA_SPREAD_METHOD_REPEAT
};

// `stb::color` now lives in color.h (a single merged class covering both
// this engine's needs and the richer string-parsing/blending API) --
// no separate definition here anymore.

class point {
public:
    constexpr point() = default;
    constexpr point(const point_t& point_) : point(point_.x, point_.y) {}
    constexpr point(float x, float y) : x(x), y(y) {}

    constexpr void move(float dx, float dy) { x += dx; y += dy; }
    constexpr void move(float d) { move(d, d); }
    constexpr void move(const point& p) { move(p.x, p.y); }

    constexpr void scale(float sx, float sy) { x *= sx; y *= sy; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr float dot(const point& p) const { return x * p.x + y * p.y; }

public:
    float x{0};
    float y{0};
};

constexpr point operator+(const point& a, const point& b)
{
    return point(a.x + b.x, a.y + b.y);
}

constexpr point operator-(const point& a, const point& b)
{
    return point(a.x - b.x, a.y - b.y);
}

constexpr point operator-(const point& a)
{
    return point(-a.x, -a.y);
}

constexpr point& operator+=(point& a, const point& b)
{
    a.move(b);
    return a;
}

constexpr point& operator-=(point& a, const point& b)
{
    a.move(-b);
    return a;
}

constexpr float operator*(const point& a, const point& b)
{
    return a.dot(b);
}

class size {
public:
    constexpr size() = default;
    constexpr size(float w, float h) : w(w), h(h) {}

    constexpr void expand(float dw, float dh) { w += dw; h += dh; }
    constexpr void expand(float d) { expand(d, d); }
    constexpr void expand(const size& s) { expand(s.w, s.h); }

    constexpr void scale(float sw, float sh) { w *= sw; h *= sh; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr bool empty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool is_zero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool is_valid() const { return w >= 0.f && h >= 0.f; }

public:
    float w{0};
    float h{0};
};

constexpr size operator+(const size& a, const size& b)
{
    return size(a.w + b.w, a.h + b.h);
}

constexpr size operator-(const size& a, const size& b)
{
    return size(a.w - b.w, a.h - b.h);
}

constexpr size operator-(const size& a)
{
    return size(-a.w, -a.h);
}

constexpr size& operator+=(size& a, const size& b)
{
    a.expand(b);
    return a;
}

constexpr size& operator-=(size& a, const size& b)
{
    a.expand(-b);
    return a;
}

class box;

class rect {
public:
    constexpr rect() = default;
    constexpr explicit rect(const size& size_) : rect(size_.w, size_.h) {}
    constexpr rect(float width, float height) : rect(0, 0, width, height) {}
    constexpr rect(const point& origin, const size& size_) : rect(origin.x, origin.y, size_.w, size_.h) {}
    constexpr rect(const rect_t& rect_) : rect(rect_.x, rect_.y, rect_.w, rect_.h) {}
    constexpr rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    rect(const box& box_);
    operator box() const;

    constexpr void move(float dx, float dy) { x += dx; y += dy; }
    constexpr void move(float d) { move(d, d); }
    constexpr void move(const point& p) { move(p.x, p.y); }

    constexpr void scale(float sx, float sy) { x *= sx; y *= sy; w *= sx; h *= sy; }
    constexpr void scale(float s) { scale(s, s); }

    constexpr void inflate(float dx, float dy) { x -= dx; y -= dy; w += dx * 2.f; h += dy * 2.f; }
    constexpr void inflate(float d) { inflate(d, d); }

    constexpr bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    constexpr bool contains(const point& p) const { return contains(p.x, p.y); }

    constexpr rect intersected(const rect& rect_) const;
    constexpr rect united(const rect& rect_) const;

    constexpr rect& intersect(const rect& o);
    constexpr rect& unite(const rect& o);

    constexpr point origin() const { return point(x, y); }
    constexpr size size_() const { return size(w, h); }

    constexpr float right() const { return x + w; }
    constexpr float bottom() const { return y + h; }

    constexpr bool empty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool is_zero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool is_valid() const { return w >= 0.f && h >= 0.f; }

    static const rect empty_rect;
    static const rect invalid;
    static const rect infinite;

public:
    float x{0};
    float y{0};
    float w{0};
    float h{0};
};

constexpr rect rect::intersected(const rect& rect_) const
{
    if(!rect_.is_valid())
        return *this;
    if(!is_valid())
        return rect_;
    auto l = STB_MAX(x, rect_.x);
    auto t = STB_MAX(y, rect_.y);
    auto r = STB_MIN(x + w, rect_.x + rect_.w);
    auto b = STB_MIN(y + h, rect_.y + rect_.h);
    if(l >= r || t >= b)
        return rect::empty_rect;
    return rect(l, t, r - l, b - t);
}

constexpr rect rect::united(const rect& rect_) const
{
    if(!rect_.is_valid())
        return *this;
    if(!is_valid())
        return rect_;
    auto l = STB_MIN(x, rect_.x);
    auto t = STB_MIN(y, rect_.y);
    auto r = STB_MAX(x + w, rect_.x + rect_.w);
    auto b = STB_MAX(y + h, rect_.y + rect_.h);
    return rect(l, t, r - l, b - t);
}

constexpr rect& rect::intersect(const rect& o)
{
    *this = intersected(o);
    return *this;
}

constexpr rect& rect::unite(const rect& o)
{
    *this = united(o);
    return *this;
}


class transform {
public:
    transform();
    transform(const stb::matrix& matrix);
    transform(float a, float b, float c, float d, float e, float f);
    transform(const matrix_t& matrix) : m_matrix(matrix) {}

    transform operator*(const transform& transform_) const;
    transform& operator*=(const transform& transform_);

    transform& multiply(const transform& transform_);
    transform& translate(float tx, float ty);
    transform& scale(float sx, float sy);
    transform& rotate(float angle, float cx = 0.f, float cy = 0.f);
    transform& shear(float shx, float shy);

    transform& post_multiply(const transform& transform_);
    transform& post_translate(float tx, float ty);
    transform& post_scale(float sx, float sy);
    transform& post_rotate(float angle, float cx = 0.f, float cy = 0.f);
    transform& post_shear(float shx, float shy);

    transform inverse() const;
    transform& invert();

    void reset();

    point map_point(float x, float y) const;
    point map_point(const point& point_) const;
    rect map_rect(const rect& rect_) const;

    float x_scale() const;
    float y_scale() const;

    const matrix_t& matrix() const { return m_matrix; }
    matrix_t& matrix() { return m_matrix; }

    bool parse(const char* data, size_t length_);

    static transform translated(float tx, float ty);
    static transform scaled(float sx, float sy);
    static transform rotated(float angle, float cx, float cy);
    static transform sheared(float shx, float shy);

    static const transform identity;

private:
    matrix_t m_matrix;
};

// Conversions between the engine's Transform / the C API's matrix_t and the public stb::matrix.
inline stb::matrix to_matrix(const matrix_t& m) { return stb::matrix{m.a, m.b, m.c, m.d, m.e, m.f}; }
inline stb::matrix to_matrix(const transform& t) { return to_matrix(t.matrix()); }

// A path is an stb::outline. The engine needs a few things on top of it (src/sources/vg/outline_util.cpp):
//   arc_to      the SVG elliptical arc, as cubics, from the outline's current point
//   parse_path  SVG path data ("M 0 0 L ..."), replaces the content; partial data stays on error, as in SVG
//   bounding_rect  the box of all points of the outline (what the svg layer calls the fill box)
void arc_to(stb::outline& path_, float rx, float ry, float x_axis_rotation_deg, bool large_arc, bool sweep, float x, float y);
bool parse_path(stb::outline& path_, const char* data, size_t length_);
void arc_to_radians(stb::outline& path_, float rx, float ry, float angle, bool large_arc, bool sweep, float x, float y);
void add_rect_path(stb::outline& path_, float x, float y, float w, float h);
void add_round_rect_path(stb::outline& path_, float x, float y, float w, float h, float rx, float ry);
void add_ellipse_path(stb::outline& path_, float cx, float cy, float rx, float ry);
rect bounding_rect(const stb::outline& path_);

enum class segment_kind { move_to, line_to, cubic_to, close };

// Walks an outline as move / line / cubic / close. A quadratic curve is raised to a cubic; `close` carries the
// start point of its contour in points[0].
class outline_walker {
public:
    explicit outline_walker(const stb::outline& path_);
    segment_kind current_segment(std::array<point, 3>& points) const;
    bool done() const { return m_index >= m_segments.size(); }
    void next() { ++m_index; }
private:
    struct segment {
        segment_kind command;
        point points[3];
    };
    std::vector<segment> m_segments;
    size_t m_index = 0;
};
} // namespace nova::vg

namespace nova::vg {

enum class texture_type {
    plain = NOVA_TEXTURE_TYPE_PLAIN,
    tiled = NOVA_TEXTURE_TYPE_TILED
};

enum class blend_mode {
    src = NOVA_OPERATOR_SRC,
    src_over = NOVA_OPERATOR_SRC_OVER,
    dst_in = NOVA_OPERATOR_DST_IN,
    dst_out = NOVA_OPERATOR_DST_OUT,
    multiply = NOVA_OPERATOR_MULTIPLY,
    screen = NOVA_OPERATOR_SCREEN,
    darken = NOVA_OPERATOR_DARKEN,
    lighten = NOVA_OPERATOR_LIGHTEN,
    difference = NOVA_OPERATOR_DIFFERENCE,
    exclusion = NOVA_OPERATOR_EXCLUSION
};

using DashArray = std::vector<float>;

class stroke_data {
public:
    explicit stroke_data(float line_width = 1.f) : m_line_width(line_width) {}

    void set_line_width(float line_width) { m_line_width = line_width; }
    float line_width() const { return m_line_width; }

    void set_miter_limit(float miter_limit) { m_miter_limit = miter_limit; }
    float miter_limit() const { return m_miter_limit; }

    void set_dash_offset(float dash_offset) { m_dash_offset = dash_offset; }
    float dash_offset() const { return m_dash_offset; }

    void set_dash_array(DashArray dashArray) { m_dash_array = std::move(dashArray); }
    const DashArray& dashArray() const { return m_dash_array; }

    void set_line_cap(line_cap line_cap_) { m_line_cap = line_cap_; }
    line_cap line_cap_() const { return m_line_cap; }

    void set_line_join(line_join line_join_) { m_line_join = line_join_; }
    line_join line_join_() const { return m_line_join; }

private:
    float m_line_width;
    float m_miter_limit{4.f};
    float m_dash_offset{0.f};
    line_cap m_line_cap{line_cap::butt};
    line_join m_line_join{line_join::miter};
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

    virtual void set_color(const stb::color& color_) = 0;
    virtual void set_color(float r, float g, float b, float a) = 0;
    virtual void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const transform& transform_) = 0;
    virtual void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const transform& transform_) = 0;

    virtual void fill_path(const stb::outline& path_, fill_rule fill_rule_, const transform& transform_) = 0;
    virtual void stroke_path(const stb::outline& path_, const stroke_data& stroke_data_, const transform& transform_) = 0;

    virtual void fill_text(const std::u32string_view& text, const font& font_, const point& origin, const transform& transform_) = 0;
    virtual void stroke_text(const std::u32string_view& text, float stroke_width, const font& font_, const point& origin, const transform& transform_) = 0;

    virtual void clip_path(const stb::outline& path_, fill_rule clip_rule, const transform& transform_) = 0;
    virtual void clip_rect(const rect& rect_, fill_rule clip_rule, const transform& transform_) = 0;

    virtual void draw_image(const pixmap& image, const rect& dst_rect, const rect& src_rect, const transform& transform_) = 0;

    virtual void save() = 0;
    virtual void restore() = 0;
};

class raster_canvas final : public canvas {
public:
    static std::shared_ptr<raster_canvas> create(const pixmap& bitmap);
    static std::shared_ptr<raster_canvas> create(float x, float y, float width, float height);
    static std::shared_ptr<raster_canvas> create(const rect& extents);

    void set_color(const stb::color& color_) override;
    void set_color(float r, float g, float b, float a) override;
    void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const transform& transform_) override;
    void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const transform& transform_) override;
    void set_texture(const raster_canvas& source, texture_type type_, float opacity, const transform& transform_);

    void fill_path(const stb::outline& path_, fill_rule fill_rule_, const transform& transform_) override;
    void stroke_path(const stb::outline& path_, const stroke_data& stroke_data_, const transform& transform_) override;

    void fill_text(const std::u32string_view& text, const font& font_, const point& origin, const transform& transform_) override;
    void stroke_text(const std::u32string_view& text, float stroke_width, const font& font_, const point& origin, const transform& transform_) override;

    void clip_path(const stb::outline& path_, fill_rule clip_rule, const transform& transform_) override;
    void clip_rect(const rect& rect_, fill_rule clip_rule, const transform& transform_) override;

    void draw_image(const pixmap& image, const rect& dst_rect, const rect& src_rect, const transform& transform_) override;
    void blend_canvas(const raster_canvas& canvas, blend_mode blend_mode_, float opacity);

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
    enum class porter_duff { over, in, out, atop, xor_ };
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

    rect extents() const { return rect(m_x, m_y, width(), height()); }

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

inline const rect rect::empty_rect(0, 0, 0, 0);
inline const rect rect::invalid(0, 0, -1, -1);
inline const rect rect::infinite(-FLT_MAX / 2.f, -FLT_MAX / 2.f, FLT_MAX, FLT_MAX);

// Rect::Rect(const Box&) is defined in novasvg.h, right after Box itself --
// Box belongs to the svg/document layer, not here, so this file (which
// stays usable without that layer) only declares the conversion, it
// doesn't define it.

inline const transform transform::identity(1, 0, 0, 1, 0, 0);



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
