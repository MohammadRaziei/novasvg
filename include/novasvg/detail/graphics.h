#ifndef NOVASVG_GRAPHICS_H
#define NOVASVG_GRAPHICS_H

#include <cstdint>
#include <algorithm>
#include <utility>
#include <memory>
#include <vector>
#include <array>
#include <string>

#include "novacolor.h"

namespace novasvg {
using namespace render; // render/ layer (novasvg::render), the former plutovg

enum class LineCap : uint8_t {
    Butt = NOVASVG_LINE_CAP_BUTT,
    Round = NOVASVG_LINE_CAP_ROUND,
    Square = NOVASVG_LINE_CAP_SQUARE
};

enum class LineJoin : uint8_t {
    Miter = NOVASVG_LINE_JOIN_MITER,
    Round = NOVASVG_LINE_JOIN_ROUND,
    Bevel = NOVASVG_LINE_JOIN_BEVEL
};

enum class FillRule : uint8_t {
    NonZero = NOVASVG_FILL_RULE_NON_ZERO,
    EvenOdd = NOVASVG_FILL_RULE_EVEN_ODD
};

enum class SpreadMethod : uint8_t {
    Pad = NOVASVG_SPREAD_METHOD_PAD,
    Reflect = NOVASVG_SPREAD_METHOD_REFLECT,
    Repeat = NOVASVG_SPREAD_METHOD_REPEAT
};

// `Color` now lives in novacolor.h (a single merged class covering both
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

    constexpr bool isEmpty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool isZero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool isValid() const { return w >= 0.f && h >= 0.f; }

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

    constexpr bool isEmpty() const { return w <= 0.f || h <= 0.f; }
    constexpr bool isZero() const { return w <= 0.f && h <= 0.f; }
    constexpr bool isValid() const { return w >= 0.f && h >= 0.f; }

    static const Rect Empty;
    static const Rect Invalid;
    static const Rect Infinite;

public:
    float x{0};
    float y{0};
    float w{0};
    float h{0};
};

constexpr Rect Rect::intersected(const Rect& rect) const
{
    if(!rect.isValid())
        return *this;
    if(!isValid())
        return rect;
    auto l = std::max(x, rect.x);
    auto t = std::max(y, rect.y);
    auto r = std::min(x + w, rect.x + rect.w);
    auto b = std::min(y + h, rect.y + rect.h);
    if(l >= r || t >= b)
        return Rect::Empty;
    return Rect(l, t, r - l, b - t);
}

constexpr Rect Rect::united(const Rect& rect) const
{
    if(!rect.isValid())
        return *this;
    if(!isValid())
        return rect;
    auto l = std::min(x, rect.x);
    auto t = std::min(y, rect.y);
    auto r = std::max(x + w, rect.x + rect.w);
    auto b = std::max(y + h, rect.y + rect.h);
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

class Matrix;

class Transform {
public:
    Transform();
    Transform(const Matrix& matrix);
    Transform(float a, float b, float c, float d, float e, float f);
    Transform(const matrix_t& matrix) : m_matrix(matrix) {}

    Transform operator*(const Transform& transform) const;
    Transform& operator*=(const Transform& transform);

    Transform& multiply(const Transform& transform);
    Transform& translate(float tx, float ty);
    Transform& scale(float sx, float sy);
    Transform& rotate(float angle, float cx = 0.f, float cy = 0.f);
    Transform& shear(float shx, float shy);

    Transform& postMultiply(const Transform& transform);
    Transform& postTranslate(float tx, float ty);
    Transform& postScale(float sx, float sy);
    Transform& postRotate(float angle, float cx = 0.f, float cy = 0.f);
    Transform& postShear(float shx, float shy);

    Transform inverse() const;
    Transform& invert();

    void reset();

    Point mapPoint(float x, float y) const;
    Point mapPoint(const Point& point) const;
    Rect mapRect(const Rect& rect) const;

    float xScale() const;
    float yScale() const;

    const matrix_t& matrix() const { return m_matrix; }
    matrix_t& matrix() { return m_matrix; }

    bool parse(const char* data, size_t length);

    static Transform translated(float tx, float ty);
    static Transform scaled(float sx, float sy);
    static Transform rotated(float angle, float cx, float cy);
    static Transform sheared(float shx, float shy);

    static const Transform Identity;

private:
    matrix_t m_matrix;
};

enum class PathCommand {
    MoveTo = NOVASVG_PATH_COMMAND_MOVE_TO,
    LineTo = NOVASVG_PATH_COMMAND_LINE_TO,
    CubicTo = NOVASVG_PATH_COMMAND_CUBIC_TO,
    Close = NOVASVG_PATH_COMMAND_CLOSE
};

class Path {
public:
    Path() = default;
    Path(const Path& path);
    Path(Path&& path);
    ~Path();

    Path& operator=(const Path& path);
    Path& operator=(Path&& path);

    void swap(Path& path);

    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadTo(float x1, float y1, float x2, float y2);
    void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
    void arcTo(float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, float x, float y);
    void close();

    void addEllipse(float cx, float cy, float rx, float ry);
    void addRoundRect(float x, float y, float w, float h, float rx, float ry);
    void addRect(float x, float y, float w, float h);

    void addEllipse(const Point& center, const Size& radii);
    void addRoundRect(const Rect& rect, const Size& radii);
    void addRect(const Rect& rect);

    void reset();

    Rect boundingRect() const;
    bool isEmpty() const;
    bool isUnique() const;
    bool isNull() const { return m_data == nullptr; }
    path_t* data() const { return m_data; }

    bool parse(const char* data, size_t length);

private:
    path_t* release();
    path_t* ensure();
    path_t* m_data = nullptr;
};

inline void Path::swap(Path& path)
{
    std::swap(m_data, path.m_data);
}

inline path_t* Path::release()
{
    return std::exchange(m_data, nullptr);
}

class PathIterator {
public:
    PathIterator(const Path& path);

    PathCommand currentSegment(std::array<Point, 3>& points) const;
    bool isDone() const { return m_index >= m_size; }
    void next();

private:
    const path_element_t* m_elements;
    const int m_size;
    int m_index;
};

class FontFace {
public:
    FontFace() = default;
    explicit FontFace(font_face_t* face);
    FontFace(const void* data, size_t length, destroy_func_t destroy_func, void* closure);
    FontFace(const char* filename);
    FontFace(const FontFace& face);
    FontFace(FontFace&& face);
    ~FontFace();

    FontFace& operator=(const FontFace& face);
    FontFace& operator=(FontFace&& face);

    void swap(FontFace& face);

    bool isNull() const { return m_face == nullptr; }
    font_face_t* get() const { return m_face; }

private:
    font_face_t* release();
    font_face_t* m_face = nullptr;
};

class FontFaceCache {
public:
    bool addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face);
    FontFace getFontFace(const std::string& family, bool bold, bool italic) const;

private:
    FontFaceCache();
    font_face_cache_t* m_cache;
    friend FontFaceCache* fontFaceCache();
};

FontFaceCache* fontFaceCache();

class Font {
public:
    Font() = default;
    Font(const FontFace& face, float size);

    float ascent() const { return m_ascent; }
    float descent() const { return m_descent; }
    float height() const { return m_ascent - m_descent; }
    float lineGap() const { return m_lineGap; }
    float xHeight() const;

    float measureText(const std::u32string_view& text) const;

    const FontFace& face() const { return m_face; }
    float size() const { return m_size; }

    bool isNull() const { return m_size <= 0.f || m_face.isNull(); }

private:
    FontFace m_face;
    float m_size = 0.f;
    float m_ascent = 0.f;
    float m_descent = 0.f;
    float m_lineGap = 0.f;
};

enum class TextureType {
    Plain = NOVASVG_TEXTURE_TYPE_PLAIN,
    Tiled = NOVASVG_TEXTURE_TYPE_TILED
};

enum class BlendMode {
    Src = NOVASVG_OPERATOR_SRC,
    Src_Over = NOVASVG_OPERATOR_SRC_OVER,
    Dst_In = NOVASVG_OPERATOR_DST_IN,
    Dst_Out = NOVASVG_OPERATOR_DST_OUT
};

using DashArray = std::vector<float>;

class StrokeData {
public:
    explicit StrokeData(float lineWidth = 1.f) : m_lineWidth(lineWidth) {}

    void setLineWidth(float lineWidth) { m_lineWidth = lineWidth; }
    float lineWidth() const { return m_lineWidth; }

    void setMiterLimit(float miterLimit) { m_miterLimit = miterLimit; }
    float miterLimit() const { return m_miterLimit; }

    void setDashOffset(float dashOffset) { m_dashOffset = dashOffset; }
    float dashOffset() const { return m_dashOffset; }

    void setDashArray(DashArray dashArray) { m_dashArray = std::move(dashArray); }
    const DashArray& dashArray() const { return m_dashArray; }

    void setLineCap(LineCap lineCap) { m_lineCap = lineCap; }
    LineCap lineCap() const { return m_lineCap; }

    void setLineJoin(LineJoin lineJoin) { m_lineJoin = lineJoin; }
    LineJoin lineJoin() const { return m_lineJoin; }

private:
    float m_lineWidth;
    float m_miterLimit{4.f};
    float m_dashOffset{0.f};
    LineCap m_lineCap{LineCap::Butt};
    LineJoin m_lineJoin{LineJoin::Miter};
    DashArray m_dashArray;
};

using GradientStop = gradient_stop_t;
using GradientStops = std::vector<GradientStop>;

class Bitmap;

class Canvas {
public:
    static std::shared_ptr<Canvas> create(const Bitmap& bitmap);
    static std::shared_ptr<Canvas> create(float x, float y, float width, float height);
    static std::shared_ptr<Canvas> create(const Rect& extents);

    void setColor(const Color& color);
    void setColor(float r, float g, float b, float a);
    void setLinearGradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform);
    void setRadialGradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform);
    void setTexture(const Canvas& source, TextureType type, float opacity, const Transform& transform);

    void fillPath(const Path& path, FillRule fillRule, const Transform& transform);
    void strokePath(const Path& path, const StrokeData& strokeData, const Transform& transform);

    void fillText(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform);
    void strokeText(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform);

    void clipPath(const Path& path, FillRule clipRule, const Transform& transform);
    void clipRect(const Rect& rect, FillRule clipRule, const Transform& transform);

    void drawImage(const Bitmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform);
    void blendCanvas(const Canvas& canvas, BlendMode blendMode, float opacity);

    void save();
    void restore();

    void convertToLuminanceMask();

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const;
    int height() const;

    Rect extents() const { return Rect(m_x, m_y, width(), height()); }

    surface_t* surface() const { return m_surface; }
    canvas_t* canvas() const { return m_canvas; }

    ~Canvas();

private:
    Canvas(const Bitmap& bitmap);
    Canvas(int x, int y, int width, int height);
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
// here is NOVASVG_INLINE, so there is no more reason to keep
// declarations and definitions in separate files).
// ---------------------------------------------------------------
using namespace render; // render/ layer (novasvg::render), the former plutovg

// Color::Black/White/Transparent are defined once, in novacolor.h.

NOVASVG_INLINE const Rect Rect::Empty(0, 0, 0, 0);
NOVASVG_INLINE const Rect Rect::Invalid(0, 0, -1, -1);
NOVASVG_INLINE const Rect Rect::Infinite(-FLT_MAX / 2.f, -FLT_MAX / 2.f, FLT_MAX, FLT_MAX);

NOVASVG_INLINE Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

NOVASVG_INLINE const Transform Transform::Identity(1, 0, 0, 1, 0, 0);

NOVASVG_INLINE Transform::Transform()
{
    matrix_init_identity(&m_matrix);
}

NOVASVG_INLINE Transform::Transform(float a, float b, float c, float d, float e, float f)
{
    matrix_init(&m_matrix, a, b, c, d, e, f);
}

NOVASVG_INLINE Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

NOVASVG_INLINE Transform Transform::operator*(const Transform& transform) const
{
    matrix_t result;
    matrix_multiply(&result, &transform.m_matrix, &m_matrix);
    return result;
}

NOVASVG_INLINE Transform& Transform::operator*=(const Transform& transform)
{
    return (*this = *this * transform);
}

NOVASVG_INLINE Transform& Transform::multiply(const Transform& transform)
{
    return (*this *= transform);
}

NOVASVG_INLINE Transform& Transform::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

NOVASVG_INLINE Transform& Transform::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

NOVASVG_INLINE Transform& Transform::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

NOVASVG_INLINE Transform& Transform::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

NOVASVG_INLINE Transform& Transform::postMultiply(const Transform& transform)
{
    return (*this = transform * *this);
}

NOVASVG_INLINE Transform& Transform::postTranslate(float tx, float ty)
{
    return postMultiply(translated(tx, ty));
}

NOVASVG_INLINE Transform& Transform::postScale(float sx, float sy)
{
    return postMultiply(scaled(sx, sy));
}

NOVASVG_INLINE Transform& Transform::postRotate(float angle, float cx, float cy)
{
    return postMultiply(rotated(angle, cx, cy));
}

NOVASVG_INLINE Transform& Transform::postShear(float shx, float shy)
{
    return postMultiply(sheared(shx, shy));
}

NOVASVG_INLINE Transform Transform::inverse() const
{
    matrix_t inverse;
    matrix_invert(&m_matrix, &inverse);
    return inverse;
}

NOVASVG_INLINE Transform& Transform::invert()
{
    matrix_invert(&m_matrix, &m_matrix);
    return *this;
}

NOVASVG_INLINE void Transform::reset()
{
    matrix_init_identity(&m_matrix);
}

NOVASVG_INLINE Point Transform::mapPoint(float x, float y) const
{
    matrix_map(&m_matrix, x, y, &x, &y);
    return Point(x, y);
}

NOVASVG_INLINE Point Transform::mapPoint(const Point& point) const
{
    return mapPoint(point.x, point.y);
}

NOVASVG_INLINE Rect Transform::mapRect(const Rect& rect) const
{
    if(!rect.isValid()) {
        return Rect::Invalid;
    }

    rect_t result = {rect.x, rect.y, rect.w, rect.h};
    matrix_map_rect(&m_matrix, &result, &result);
    return result;
}

NOVASVG_INLINE float Transform::xScale() const
{
    return std::sqrt(m_matrix.a * m_matrix.a + m_matrix.b * m_matrix.b);
}

NOVASVG_INLINE float Transform::yScale() const
{
    return std::sqrt(m_matrix.c * m_matrix.c + m_matrix.d * m_matrix.d);
}

NOVASVG_INLINE bool Transform::parse(const char* data, size_t length)
{
    return matrix_parse(&m_matrix, data, length);
}

NOVASVG_INLINE Transform Transform::rotated(float angle, float cx, float cy)
{
    matrix_t matrix;
    if(cx == 0.f && cy == 0.f) {
        matrix_init_rotate(&matrix, NOVASVG_DEG2RAD(angle));
    } else {
        matrix_init_translate(&matrix, cx, cy);
        matrix_rotate(&matrix, NOVASVG_DEG2RAD(angle));
        matrix_translate(&matrix, -cx, -cy);
    }

    return matrix;
}

NOVASVG_INLINE Transform Transform::scaled(float sx, float sy)
{
    matrix_t matrix;
    matrix_init_scale(&matrix, sx, sy);
    return matrix;
}

NOVASVG_INLINE Transform Transform::sheared(float shx, float shy)
{
    matrix_t matrix;
    matrix_init_shear(&matrix, NOVASVG_DEG2RAD(shx), NOVASVG_DEG2RAD(shy));
    return matrix;
}

NOVASVG_INLINE Transform Transform::translated(float tx, float ty)
{
    matrix_t matrix;
    matrix_init_translate(&matrix, tx, ty);
    return matrix;
}

NOVASVG_INLINE Path::Path(const Path& path)
    : m_data(path_reference(path.data()))
{
}

NOVASVG_INLINE Path::Path(Path&& path)
    : m_data(path.release())
{
}

NOVASVG_INLINE Path::~Path()
{
    path_destroy(m_data);
}

NOVASVG_INLINE Path& Path::operator=(const Path& path)
{
    Path(path).swap(*this);
    return *this;
}

NOVASVG_INLINE Path& Path::operator=(Path&& path)
{
    Path(std::move(path)).swap(*this);
    return *this;
}

NOVASVG_INLINE void Path::moveTo(float x, float y)
{
    path_move_to(ensure(), x, y);
}

NOVASVG_INLINE void Path::lineTo(float x, float y)
{
    path_line_to(ensure(), x, y);
}

NOVASVG_INLINE void Path::quadTo(float x1, float y1, float x2, float y2)
{
    path_quad_to(ensure(), x1, y1, x2, y2);
}

NOVASVG_INLINE void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
    path_cubic_to(ensure(), x1, y1, x2, y2, x3, y3);
}

NOVASVG_INLINE void Path::arcTo(float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, float x, float y)
{
    path_arc_to(ensure(), rx, ry, NOVASVG_DEG2RAD(xAxisRotation), largeArcFlag, sweepFlag, x, y);
}

NOVASVG_INLINE void Path::close()
{
    path_close(ensure());
}

NOVASVG_INLINE void Path::addEllipse(float cx, float cy, float rx, float ry)
{
    path_add_ellipse(ensure(), cx, cy, rx, ry);
}

NOVASVG_INLINE void Path::addRoundRect(float x, float y, float w, float h, float rx, float ry)
{
    path_add_round_rect(ensure(), x, y, w, h, rx, ry);
}

NOVASVG_INLINE void Path::addRect(float x, float y, float w, float h)
{
    path_add_rect(ensure(), x, y, w, h);
}

NOVASVG_INLINE void Path::addEllipse(const Point& center, const Size& radii)
{
    addEllipse(center.x, center.y, radii.w, radii.h);
}

NOVASVG_INLINE void Path::addRoundRect(const Rect& rect, const Size& radii)
{
    addRoundRect(rect.x, rect.y, rect.w, rect.h, radii.w, radii.h);
}

NOVASVG_INLINE void Path::addRect(const Rect& rect)
{
    addRect(rect.x, rect.y, rect.w, rect.h);
}

NOVASVG_INLINE void Path::reset()
{
    if(m_data == nullptr)
        return;
    if(isUnique()) {
        path_reset(m_data);
    } else {
        path_destroy(m_data);
        m_data = nullptr;
    }
}

NOVASVG_INLINE Rect Path::boundingRect() const
{
    if(m_data == nullptr)
        return Rect::Empty;
    rect_t extents;
    path_extents(m_data, &extents, false);
    return extents;
}

NOVASVG_INLINE bool Path::isEmpty() const
{
    if(m_data)
        return path_get_elements(m_data, nullptr) == 0;
    return true;
}

NOVASVG_INLINE bool Path::isUnique() const
{
    return path_get_reference_count(m_data) == 1;
}

NOVASVG_INLINE bool Path::parse(const char* data, size_t length)
{
    path_reset(ensure());
    return path_parse(m_data, data, length);
}

NOVASVG_INLINE path_t* Path::ensure()
{
    if(isNull()) {
        m_data = path_create();
    } else if(!isUnique()) {
        path_destroy(m_data);
        m_data = path_clone(m_data);
    }

    return m_data;
}

NOVASVG_INLINE PathIterator::PathIterator(const Path& path)
    : m_size(path_get_elements(path.data(), &m_elements))
    , m_index(0)
{
}

NOVASVG_INLINE PathCommand PathIterator::currentSegment(std::array<Point, 3>& points) const
{
    auto command = m_elements[m_index].header.command;
    switch(command) {
    case NOVASVG_PATH_COMMAND_MOVE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case NOVASVG_PATH_COMMAND_LINE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case NOVASVG_PATH_COMMAND_CUBIC_TO:
        points[0] = m_elements[m_index + 1].point;
        points[1] = m_elements[m_index + 2].point;
        points[2] = m_elements[m_index + 3].point;
        break;
    case NOVASVG_PATH_COMMAND_CLOSE:
        points[0] = m_elements[m_index + 1].point;
        break;
    }

    return PathCommand(command);
}

NOVASVG_INLINE void PathIterator::next()
{
    m_index += m_elements[m_index].header.length;
}

NOVASVG_INLINE FontFace::FontFace(font_face_t* face)
    : m_face(font_face_reference(face))
{
}

NOVASVG_INLINE FontFace::FontFace(const void* data, size_t length, destroy_func_t destroy_func, void* closure)
    : m_face(font_face_load_from_data(data, length, 0, destroy_func, closure))
{
}

NOVASVG_INLINE FontFace::FontFace(const char* filename)
    : m_face(font_face_load_from_file(filename, 0))
{
}

NOVASVG_INLINE FontFace::FontFace(const FontFace& face)
    : m_face(font_face_reference(face.get()))
{
}

NOVASVG_INLINE FontFace::FontFace(FontFace&& face)
    : m_face(face.release())
{
}

NOVASVG_INLINE FontFace::~FontFace()
{
    font_face_destroy(m_face);
}

NOVASVG_INLINE FontFace& FontFace::operator=(const FontFace& face)
{
    FontFace(face).swap(*this);
    return *this;
}

NOVASVG_INLINE FontFace& FontFace::operator=(FontFace&& face)
{
    FontFace(std::move(face)).swap(*this);
    return *this;
}

NOVASVG_INLINE void FontFace::swap(FontFace& face)
{
    std::swap(m_face, face.m_face);
}

NOVASVG_INLINE font_face_t* FontFace::release()
{
    return std::exchange(m_face, nullptr);
}

NOVASVG_INLINE bool FontFaceCache::addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face)
{
    if(!face.isNull())
        font_face_cache_add(m_cache, family.data(), bold, italic, face.get());
    return !face.isNull();
}

NOVASVG_INLINE FontFace FontFaceCache::getFontFace(const std::string& family, bool bold, bool italic) const
{
    if(auto face = font_face_cache_get(m_cache, family.data(), bold, italic)) {
        return FontFace(face);
    }

    static const struct {
        const char* generic;
        const char* fallback;
    } generic_fallbacks[] = {
#if defined(__linux__)
        {"sans-serif", "DejaVu Sans"},
        {"serif", "DejaVu Serif"},
        {"monospace", "DejaVu Sans Mono"},
#else
        {"sans-serif", "Arial"},
        {"serif", "Times New Roman"},
        {"monospace", "Courier New"},
#endif
        {"cursive", "Comic Sans MS"},
        {"fantasy", "Impact"}
    };

    for(auto value : generic_fallbacks) {
        if(value.generic == family || family.empty()) {
            return FontFace(font_face_cache_get(m_cache, value.fallback, bold, italic));
        }
    }

    return FontFace();
}

NOVASVG_INLINE FontFaceCache::FontFaceCache()
    : m_cache(font_face_cache_create())
{
#ifndef NOVASVG_DISABLE_LOAD_SYSTEM_FONTS
    font_face_cache_load_sys(m_cache);
#endif
}

NOVASVG_INLINE FontFaceCache* fontFaceCache()
{
    static FontFaceCache cache;
    return &cache;
}

NOVASVG_INLINE Font::Font(const FontFace& face, float size)
    : m_face(face), m_size(size)
{
    if(m_size > 0.f && !m_face.isNull()) {
        font_face_get_metrics(m_face.get(), m_size, &m_ascent, &m_descent, &m_lineGap, nullptr);
    }
}

NOVASVG_INLINE float Font::xHeight() const
{
    rect_t extents = {0};
    if(m_size > 0.f && !m_face.isNull())
        font_face_get_glyph_metrics(m_face.get(), m_size, 'x', nullptr, nullptr, &extents);
    return extents.h;
}

NOVASVG_INLINE float Font::measureText(const std::u32string_view& text) const
{
    if(m_size > 0.f && !m_face.isNull())
        return font_face_text_extents(m_face.get(), m_size, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, nullptr);
    return 0;
}

NOVASVG_INLINE std::shared_ptr<Canvas> Canvas::create(const Bitmap& bitmap)
{
    return std::shared_ptr<Canvas>(new Canvas(bitmap));
}

NOVASVG_INLINE std::shared_ptr<Canvas> Canvas::create(float x, float y, float width, float height)
{
    constexpr int kMaxSize = 1 << 15;
    if(width <= 0 || height <= 0 || width >= kMaxSize || height >= kMaxSize)
        return std::shared_ptr<Canvas>(new Canvas(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::shared_ptr<Canvas>(new Canvas(l, t, r - l, b - t));
}

NOVASVG_INLINE std::shared_ptr<Canvas> Canvas::create(const Rect& extents)
{
    return create(extents.x, extents.y, extents.w, extents.h);
}

NOVASVG_INLINE void Canvas::setColor(const Color& color)
{
    setColor(color.getRF(), color.getGF(), color.getBF(), color.getAF());
}

NOVASVG_INLINE void Canvas::setColor(float r, float g, float b, float a)
{
    canvas_set_rgba(m_canvas, r, g, b, a);
}

NOVASVG_INLINE void Canvas::setLinearGradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    canvas_set_linear_gradient(m_canvas, x1, y1, x2, y2, static_cast<spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

NOVASVG_INLINE void Canvas::setRadialGradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    canvas_set_radial_gradient(m_canvas, cx, cy, r, fx, fy, 0.f, static_cast<spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

NOVASVG_INLINE void Canvas::setTexture(const Canvas& source, TextureType type, float opacity, const Transform& transform)
{
    canvas_set_texture(m_canvas, source.surface(), static_cast<texture_type_t>(type), opacity, &transform.matrix());
}

NOVASVG_INLINE void Canvas::fillPath(const Path& path, FillRule fillRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(fillRule));
    canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    canvas_fill_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::strokePath(const Path& path, const StrokeData& strokeData, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_line_width(m_canvas, strokeData.lineWidth());
    canvas_set_miter_limit(m_canvas, strokeData.miterLimit());
    canvas_set_line_cap(m_canvas, static_cast<line_cap_t>(strokeData.lineCap()));
    canvas_set_line_join(m_canvas, static_cast<line_join_t>(strokeData.lineJoin()));
    canvas_set_dash_offset(m_canvas, strokeData.dashOffset());
    canvas_set_dash_array(m_canvas, strokeData.dashArray().data(), strokeData.dashArray().size());
    canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    canvas_stroke_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::fillText(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, NOVASVG_FILL_RULE_NON_ZERO);
    canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    canvas_set_font(m_canvas, font.face().get(), font.size());
    canvas_fill_text(m_canvas, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

NOVASVG_INLINE void Canvas::strokeText(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_line_width(m_canvas, strokeWidth);
    canvas_set_miter_limit(m_canvas, 4.f);
    canvas_set_line_cap(m_canvas, NOVASVG_LINE_CAP_BUTT);
    canvas_set_line_join(m_canvas, NOVASVG_LINE_JOIN_MITER);
    canvas_set_dash_offset(m_canvas, 0.f);
    canvas_set_dash_array(m_canvas, nullptr, 0);
    canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    canvas_set_font(m_canvas, font.face().get(), font.size());
    canvas_stroke_text(m_canvas, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

NOVASVG_INLINE void Canvas::clipPath(const Path& path, FillRule clipRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(clipRule));
    canvas_clip_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::clipRect(const Rect& rect, FillRule clipRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(clipRule));
    canvas_clip_rect(m_canvas, rect.x, rect.y, rect.w, rect.h);
}

NOVASVG_INLINE void Canvas::drawImage(const Bitmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform)
{
    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;
    matrix_t matrix = { xScale, 0, 0, yScale, -srcRect.x * xScale, -srcRect.y * yScale };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_translate(m_canvas, dstRect.x, dstRect.y);
    canvas_set_fill_rule(m_canvas, NOVASVG_FILL_RULE_NON_ZERO);
    canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    canvas_set_texture(m_canvas, image.surface(), NOVASVG_TEXTURE_TYPE_PLAIN, 1.f, &matrix);
    canvas_fill_rect(m_canvas, 0, 0, dstRect.w, dstRect.h);
}

NOVASVG_INLINE void Canvas::blendCanvas(const Canvas& canvas, BlendMode blendMode, float opacity)
{
    matrix_t matrix = { 1, 0, 0, 1, static_cast<float>(canvas.x()), static_cast<float>(canvas.y()) };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_set_operator(m_canvas, static_cast<operator_t>(blendMode));
    canvas_set_texture(m_canvas, canvas.surface(), NOVASVG_TEXTURE_TYPE_PLAIN, opacity, &matrix);
    canvas_paint(m_canvas);
}

NOVASVG_INLINE void Canvas::save()
{
    canvas_save(m_canvas);
}

NOVASVG_INLINE void Canvas::restore()
{
    canvas_restore(m_canvas);
}

NOVASVG_INLINE int Canvas::width() const
{
    return surface_get_width(m_surface);
}

NOVASVG_INLINE int Canvas::height() const
{
    return surface_get_height(m_surface);
}

NOVASVG_INLINE void Canvas::convertToLuminanceMask()
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);
    for(int y = 0; y < height; y++) {
        auto pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; x++) {
            auto pixel = pixels[x];
            auto a = (pixel >> 24) & 0xFF;
            auto r = (pixel >> 16) & 0xFF;
            auto g = (pixel >> 8) & 0xFF;
            auto b = (pixel >> 0) & 0xFF;
            if(a) {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            auto l = (r * 0.2125 + g * 0.7154 + b * 0.0721);
            pixels[x] = static_cast<uint32_t>(l * (a / 255.0)) << 24;
        }
    }
}

NOVASVG_INLINE Canvas::~Canvas()
{
    canvas_destroy(m_canvas);
    surface_destroy(m_surface);
}

NOVASVG_INLINE Canvas::Canvas(const Bitmap& bitmap)
    : m_surface(surface_reference(bitmap.surface()))
    , m_canvas(canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, 0, 0})
    , m_x(0), m_y(0)
{
}

NOVASVG_INLINE Canvas::Canvas(int x, int y, int width, int height)
    : m_surface(surface_create(width, height))
    , m_canvas(canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, -static_cast<float>(x), -static_cast<float>(y)})
    , m_x(x), m_y(y)
{
}

} // namespace novasvg

#endif // NOVASVG_GRAPHICS_H
