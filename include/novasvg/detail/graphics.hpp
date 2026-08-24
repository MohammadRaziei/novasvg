#include "graphics.h"

#include <cfloat>
#include <cmath>

namespace novasvg {

NOVASVG_INLINE const Color Color::Black(0xFF000000);
NOVASVG_INLINE const Color Color::White(0xFFFFFFFF);
NOVASVG_INLINE const Color Color::Transparent(0x00000000);

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
    novasvg_matrix_init_identity(&m_matrix);
}

NOVASVG_INLINE Transform::Transform(float a, float b, float c, float d, float e, float f)
{
    novasvg_matrix_init(&m_matrix, a, b, c, d, e, f);
}

NOVASVG_INLINE Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

NOVASVG_INLINE Transform Transform::operator*(const Transform& transform) const
{
    novasvg_matrix_t result;
    novasvg_matrix_multiply(&result, &transform.m_matrix, &m_matrix);
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
    novasvg_matrix_t inverse;
    novasvg_matrix_invert(&m_matrix, &inverse);
    return inverse;
}

NOVASVG_INLINE Transform& Transform::invert()
{
    novasvg_matrix_invert(&m_matrix, &m_matrix);
    return *this;
}

NOVASVG_INLINE void Transform::reset()
{
    novasvg_matrix_init_identity(&m_matrix);
}

NOVASVG_INLINE Point Transform::mapPoint(float x, float y) const
{
    novasvg_matrix_map(&m_matrix, x, y, &x, &y);
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

    novasvg_rect_t result = {rect.x, rect.y, rect.w, rect.h};
    novasvg_matrix_map_rect(&m_matrix, &result, &result);
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
    return novasvg_matrix_parse(&m_matrix, data, length);
}

NOVASVG_INLINE Transform Transform::rotated(float angle, float cx, float cy)
{
    novasvg_matrix_t matrix;
    if(cx == 0.f && cy == 0.f) {
        novasvg_matrix_init_rotate(&matrix, NOVASVG_DEG2RAD(angle));
    } else {
        novasvg_matrix_init_translate(&matrix, cx, cy);
        novasvg_matrix_rotate(&matrix, NOVASVG_DEG2RAD(angle));
        novasvg_matrix_translate(&matrix, -cx, -cy);
    }

    return matrix;
}

NOVASVG_INLINE Transform Transform::scaled(float sx, float sy)
{
    novasvg_matrix_t matrix;
    novasvg_matrix_init_scale(&matrix, sx, sy);
    return matrix;
}

NOVASVG_INLINE Transform Transform::sheared(float shx, float shy)
{
    novasvg_matrix_t matrix;
    novasvg_matrix_init_shear(&matrix, NOVASVG_DEG2RAD(shx), NOVASVG_DEG2RAD(shy));
    return matrix;
}

NOVASVG_INLINE Transform Transform::translated(float tx, float ty)
{
    novasvg_matrix_t matrix;
    novasvg_matrix_init_translate(&matrix, tx, ty);
    return matrix;
}

NOVASVG_INLINE Path::Path(const Path& path)
    : m_data(novasvg_path_reference(path.data()))
{
}

NOVASVG_INLINE Path::Path(Path&& path)
    : m_data(path.release())
{
}

NOVASVG_INLINE Path::~Path()
{
    novasvg_path_destroy(m_data);
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
    novasvg_path_move_to(ensure(), x, y);
}

NOVASVG_INLINE void Path::lineTo(float x, float y)
{
    novasvg_path_line_to(ensure(), x, y);
}

NOVASVG_INLINE void Path::quadTo(float x1, float y1, float x2, float y2)
{
    novasvg_path_quad_to(ensure(), x1, y1, x2, y2);
}

NOVASVG_INLINE void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
    novasvg_path_cubic_to(ensure(), x1, y1, x2, y2, x3, y3);
}

NOVASVG_INLINE void Path::arcTo(float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, float x, float y)
{
    novasvg_path_arc_to(ensure(), rx, ry, NOVASVG_DEG2RAD(xAxisRotation), largeArcFlag, sweepFlag, x, y);
}

NOVASVG_INLINE void Path::close()
{
    novasvg_path_close(ensure());
}

NOVASVG_INLINE void Path::addEllipse(float cx, float cy, float rx, float ry)
{
    novasvg_path_add_ellipse(ensure(), cx, cy, rx, ry);
}

NOVASVG_INLINE void Path::addRoundRect(float x, float y, float w, float h, float rx, float ry)
{
    novasvg_path_add_round_rect(ensure(), x, y, w, h, rx, ry);
}

NOVASVG_INLINE void Path::addRect(float x, float y, float w, float h)
{
    novasvg_path_add_rect(ensure(), x, y, w, h);
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
        novasvg_path_reset(m_data);
    } else {
        novasvg_path_destroy(m_data);
        m_data = nullptr;
    }
}

NOVASVG_INLINE Rect Path::boundingRect() const
{
    if(m_data == nullptr)
        return Rect::Empty;
    novasvg_rect_t extents;
    novasvg_path_extents(m_data, &extents, false);
    return extents;
}

NOVASVG_INLINE bool Path::isEmpty() const
{
    if(m_data)
        return novasvg_path_get_elements(m_data, nullptr) == 0;
    return true;
}

NOVASVG_INLINE bool Path::isUnique() const
{
    return novasvg_path_get_reference_count(m_data) == 1;
}

NOVASVG_INLINE bool Path::parse(const char* data, size_t length)
{
    novasvg_path_reset(ensure());
    return novasvg_path_parse(m_data, data, length);
}

NOVASVG_INLINE novasvg_path_t* Path::ensure()
{
    if(isNull()) {
        m_data = novasvg_path_create();
    } else if(!isUnique()) {
        novasvg_path_destroy(m_data);
        m_data = novasvg_path_clone(m_data);
    }

    return m_data;
}

NOVASVG_INLINE PathIterator::PathIterator(const Path& path)
    : m_size(novasvg_path_get_elements(path.data(), &m_elements))
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

NOVASVG_INLINE FontFace::FontFace(novasvg_font_face_t* face)
    : m_face(novasvg_font_face_reference(face))
{
}

NOVASVG_INLINE FontFace::FontFace(const void* data, size_t length, novasvg_destroy_func_t destroy_func, void* closure)
    : m_face(novasvg_font_face_load_from_data(data, length, 0, destroy_func, closure))
{
}

NOVASVG_INLINE FontFace::FontFace(const char* filename)
    : m_face(novasvg_font_face_load_from_file(filename, 0))
{
}

NOVASVG_INLINE FontFace::FontFace(const FontFace& face)
    : m_face(novasvg_font_face_reference(face.get()))
{
}

NOVASVG_INLINE FontFace::FontFace(FontFace&& face)
    : m_face(face.release())
{
}

NOVASVG_INLINE FontFace::~FontFace()
{
    novasvg_font_face_destroy(m_face);
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

NOVASVG_INLINE novasvg_font_face_t* FontFace::release()
{
    return std::exchange(m_face, nullptr);
}

NOVASVG_INLINE bool FontFaceCache::addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face)
{
    if(!face.isNull())
        novasvg_font_face_cache_add(m_cache, family.data(), bold, italic, face.get());
    return !face.isNull();
}

NOVASVG_INLINE FontFace FontFaceCache::getFontFace(const std::string& family, bool bold, bool italic) const
{
    if(auto face = novasvg_font_face_cache_get(m_cache, family.data(), bold, italic)) {
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
            return FontFace(novasvg_font_face_cache_get(m_cache, value.fallback, bold, italic));
        }
    }

    return FontFace();
}

NOVASVG_INLINE FontFaceCache::FontFaceCache()
    : m_cache(novasvg_font_face_cache_create())
{
#ifndef NOVASVG_DISABLE_LOAD_SYSTEM_FONTS
    novasvg_font_face_cache_load_sys(m_cache);
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
        novasvg_font_face_get_metrics(m_face.get(), m_size, &m_ascent, &m_descent, &m_lineGap, nullptr);
    }
}

NOVASVG_INLINE float Font::xHeight() const
{
    novasvg_rect_t extents = {0};
    if(m_size > 0.f && !m_face.isNull())
        novasvg_font_face_get_glyph_metrics(m_face.get(), m_size, 'x', nullptr, nullptr, &extents);
    return extents.h;
}

NOVASVG_INLINE float Font::measureText(const std::u32string_view& text) const
{
    if(m_size > 0.f && !m_face.isNull())
        return novasvg_font_face_text_extents(m_face.get(), m_size, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, nullptr);
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
    setColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

NOVASVG_INLINE void Canvas::setColor(float r, float g, float b, float a)
{
    novasvg_canvas_set_rgba(m_canvas, r, g, b, a);
}

NOVASVG_INLINE void Canvas::setLinearGradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    novasvg_canvas_set_linear_gradient(m_canvas, x1, y1, x2, y2, static_cast<novasvg_spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

NOVASVG_INLINE void Canvas::setRadialGradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    novasvg_canvas_set_radial_gradient(m_canvas, cx, cy, r, fx, fy, 0.f, static_cast<novasvg_spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

NOVASVG_INLINE void Canvas::setTexture(const Canvas& source, TextureType type, float opacity, const Transform& transform)
{
    novasvg_canvas_set_texture(m_canvas, source.surface(), static_cast<novasvg_texture_type_t>(type), opacity, &transform.matrix());
}

NOVASVG_INLINE void Canvas::fillPath(const Path& path, FillRule fillRule, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_fill_rule(m_canvas, static_cast<novasvg_fill_rule_t>(fillRule));
    novasvg_canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    novasvg_canvas_fill_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::strokePath(const Path& path, const StrokeData& strokeData, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_line_width(m_canvas, strokeData.lineWidth());
    novasvg_canvas_set_miter_limit(m_canvas, strokeData.miterLimit());
    novasvg_canvas_set_line_cap(m_canvas, static_cast<novasvg_line_cap_t>(strokeData.lineCap()));
    novasvg_canvas_set_line_join(m_canvas, static_cast<novasvg_line_join_t>(strokeData.lineJoin()));
    novasvg_canvas_set_dash_offset(m_canvas, strokeData.dashOffset());
    novasvg_canvas_set_dash_array(m_canvas, strokeData.dashArray().data(), strokeData.dashArray().size());
    novasvg_canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    novasvg_canvas_stroke_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::fillText(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_fill_rule(m_canvas, NOVASVG_FILL_RULE_NON_ZERO);
    novasvg_canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    novasvg_canvas_set_font(m_canvas, font.face().get(), font.size());
    novasvg_canvas_fill_text(m_canvas, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

NOVASVG_INLINE void Canvas::strokeText(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_line_width(m_canvas, strokeWidth);
    novasvg_canvas_set_miter_limit(m_canvas, 4.f);
    novasvg_canvas_set_line_cap(m_canvas, NOVASVG_LINE_CAP_BUTT);
    novasvg_canvas_set_line_join(m_canvas, NOVASVG_LINE_JOIN_MITER);
    novasvg_canvas_set_dash_offset(m_canvas, 0.f);
    novasvg_canvas_set_dash_array(m_canvas, nullptr, 0);
    novasvg_canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    novasvg_canvas_set_font(m_canvas, font.face().get(), font.size());
    novasvg_canvas_stroke_text(m_canvas, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

NOVASVG_INLINE void Canvas::clipPath(const Path& path, FillRule clipRule, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_fill_rule(m_canvas, static_cast<novasvg_fill_rule_t>(clipRule));
    novasvg_canvas_clip_path(m_canvas, path.data());
}

NOVASVG_INLINE void Canvas::clipRect(const Rect& rect, FillRule clipRule, const Transform& transform)
{
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_set_fill_rule(m_canvas, static_cast<novasvg_fill_rule_t>(clipRule));
    novasvg_canvas_clip_rect(m_canvas, rect.x, rect.y, rect.w, rect.h);
}

NOVASVG_INLINE void Canvas::drawImage(const Bitmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform)
{
    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;
    novasvg_matrix_t matrix = { xScale, 0, 0, yScale, -srcRect.x * xScale, -srcRect.y * yScale };
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_transform(m_canvas, &transform.matrix());
    novasvg_canvas_translate(m_canvas, dstRect.x, dstRect.y);
    novasvg_canvas_set_fill_rule(m_canvas, NOVASVG_FILL_RULE_NON_ZERO);
    novasvg_canvas_set_operator(m_canvas, NOVASVG_OPERATOR_SRC_OVER);
    novasvg_canvas_set_texture(m_canvas, image.surface(), NOVASVG_TEXTURE_TYPE_PLAIN, 1.f, &matrix);
    novasvg_canvas_fill_rect(m_canvas, 0, 0, dstRect.w, dstRect.h);
}

NOVASVG_INLINE void Canvas::blendCanvas(const Canvas& canvas, BlendMode blendMode, float opacity)
{
    novasvg_matrix_t matrix = { 1, 0, 0, 1, static_cast<float>(canvas.x()), static_cast<float>(canvas.y()) };
    novasvg_canvas_set_matrix(m_canvas, &m_translation);
    novasvg_canvas_set_operator(m_canvas, static_cast<novasvg_operator_t>(blendMode));
    novasvg_canvas_set_texture(m_canvas, canvas.surface(), NOVASVG_TEXTURE_TYPE_PLAIN, opacity, &matrix);
    novasvg_canvas_paint(m_canvas);
}

NOVASVG_INLINE void Canvas::save()
{
    novasvg_canvas_save(m_canvas);
}

NOVASVG_INLINE void Canvas::restore()
{
    novasvg_canvas_restore(m_canvas);
}

NOVASVG_INLINE int Canvas::width() const
{
    return novasvg_surface_get_width(m_surface);
}

NOVASVG_INLINE int Canvas::height() const
{
    return novasvg_surface_get_height(m_surface);
}

NOVASVG_INLINE void Canvas::convertToLuminanceMask()
{
    auto width = novasvg_surface_get_width(m_surface);
    auto height = novasvg_surface_get_height(m_surface);
    auto stride = novasvg_surface_get_stride(m_surface);
    auto data = novasvg_surface_get_data(m_surface);
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
    novasvg_canvas_destroy(m_canvas);
    novasvg_surface_destroy(m_surface);
}

NOVASVG_INLINE Canvas::Canvas(const Bitmap& bitmap)
    : m_surface(novasvg_surface_reference(bitmap.surface()))
    , m_canvas(novasvg_canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, 0, 0})
    , m_x(0), m_y(0)
{
}

NOVASVG_INLINE Canvas::Canvas(int x, int y, int width, int height)
    : m_surface(novasvg_surface_create(width, height))
    , m_canvas(novasvg_canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, -static_cast<float>(x), -static_cast<float>(y)})
    , m_x(x), m_y(y)
{
}

} // namespace novasvg
