// Definitions declared in src/sources/vg/canvas.h.
#include "canvas.h"

namespace nova::vg {

Transform::Transform()
{
    matrix_init_identity(&m_matrix);
}

Transform::Transform(float a, float b, float c, float d, float e, float f)
{
    matrix_init(&m_matrix, a, b, c, d, e, f);
}

Transform Transform::operator*(const Transform& transform) const
{
    matrix_t result;
    matrix_multiply(&result, &transform.m_matrix, &m_matrix);
    return result;
}

Transform& Transform::operator*=(const Transform& transform)
{
    return (*this = *this * transform);
}

Transform& Transform::multiply(const Transform& transform)
{
    return (*this *= transform);
}

Transform& Transform::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Transform& Transform::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Transform& Transform::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Transform& Transform::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Transform& Transform::post_multiply(const Transform& transform)
{
    return (*this = transform * *this);
}

Transform& Transform::postTranslate(float tx, float ty)
{
    return post_multiply(translated(tx, ty));
}

Transform& Transform::postScale(float sx, float sy)
{
    return post_multiply(scaled(sx, sy));
}

Transform& Transform::postRotate(float angle, float cx, float cy)
{
    return post_multiply(rotated(angle, cx, cy));
}

Transform& Transform::postShear(float shx, float shy)
{
    return post_multiply(sheared(shx, shy));
}

Transform Transform::inverse() const
{
    matrix_t inverse;
    matrix_invert(&m_matrix, &inverse);
    return inverse;
}

Transform& Transform::invert()
{
    matrix_invert(&m_matrix, &m_matrix);
    return *this;
}

void Transform::reset()
{
    matrix_init_identity(&m_matrix);
}

Point Transform::mapPoint(float x, float y) const
{
    matrix_map(&m_matrix, x, y, &x, &y);
    return Point(x, y);
}

Point Transform::mapPoint(const Point& point) const
{
    return mapPoint(point.x, point.y);
}

Rect Transform::map_rect(const Rect& rect) const
{
    if(!rect.isValid()) {
        return Rect::Invalid;
    }

    rect_t result = {rect.x, rect.y, rect.w, rect.h};
    matrix_map_rect(&m_matrix, &result, &result);
    return result;
}

float Transform::xScale() const
{
    return std::sqrt(m_matrix.a * m_matrix.a + m_matrix.b * m_matrix.b);
}

float Transform::yScale() const
{
    return std::sqrt(m_matrix.c * m_matrix.c + m_matrix.d * m_matrix.d);
}

bool Transform::parse(const char* data, size_t length)
{
    return matrix_parse(&m_matrix, data, length);
}

Transform Transform::rotated(float angle, float cx, float cy)
{
    matrix_t matrix;
    if(cx == 0.f && cy == 0.f) {
        matrix_init_rotate(&matrix, NOVA_DEG2RAD(angle));
    } else {
        matrix_init_translate(&matrix, cx, cy);
        matrix_rotate(&matrix, NOVA_DEG2RAD(angle));
        matrix_translate(&matrix, -cx, -cy);
    }

    return matrix;
}

Transform Transform::scaled(float sx, float sy)
{
    matrix_t matrix;
    matrix_init_scale(&matrix, sx, sy);
    return matrix;
}

Transform Transform::sheared(float shx, float shy)
{
    matrix_t matrix;
    matrix_init_shear(&matrix, NOVA_DEG2RAD(shx), NOVA_DEG2RAD(shy));
    return matrix;
}

Transform Transform::translated(float tx, float ty)
{
    matrix_t matrix;
    matrix_init_translate(&matrix, tx, ty);
    return matrix;
}

namespace {
struct path_guard {
    explicit path_guard(path_t* p) : m_path(p) {}
    ~path_guard() { path_destroy(m_path); }
    path_guard(const path_guard&) = delete;
    path_guard& operator=(const path_guard&) = delete;
    path_t* get() const { return m_path; }
    path_t* m_path;
};
} // namespace

std::shared_ptr<raster_canvas> raster_canvas::create(const pixmap& bitmap)
{
    return std::shared_ptr<raster_canvas>(new raster_canvas(bitmap));
}

std::shared_ptr<raster_canvas> raster_canvas::create(float x, float y, float width, float height)
{
    constexpr int kMaxSize = 1 << 15;
    if(width <= 0 || height <= 0 || width >= kMaxSize || height >= kMaxSize)
        return std::shared_ptr<raster_canvas>(new raster_canvas(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::shared_ptr<raster_canvas>(new raster_canvas(l, t, r - l, b - t));
}

std::shared_ptr<raster_canvas> raster_canvas::create(const Rect& extents)
{
    return create(extents.x, extents.y, extents.w, extents.h);
}

void raster_canvas::set_color(const stb::color& color)
{
    set_color(red_f(color), green_f(color), blue_f(color), alpha_f(color));
}

void raster_canvas::set_color(float r, float g, float b, float a)
{
    canvas_set_rgba(m_canvas, r, g, b, a);
}

void raster_canvas::set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    canvas_set_linear_gradient(m_canvas, x1, y1, x2, y2, static_cast<spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

void raster_canvas::set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    canvas_set_radial_gradient(m_canvas, cx, cy, r, fx, fy, 0.f, static_cast<spread_method_t>(spread), stops.data(), stops.size(), &transform.matrix());
}

void raster_canvas::set_texture(const raster_canvas& source, TextureType type, float opacity, const Transform& transform)
{
    canvas_set_texture(m_canvas, source.surface(), static_cast<texture_type_t>(type), opacity, &transform.matrix());
}

void raster_canvas::fill_path(const stb::outline& path, FillRule fillRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(fillRule));
    canvas_set_operator(m_canvas, NOVA_OPERATOR_SRC_OVER);
    path_guard guard(to_path_t(path));
    canvas_fill_path(m_canvas, guard.get());
}

void raster_canvas::stroke_path(const stb::outline& path, const StrokeData& strokeData, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_line_width(m_canvas, strokeData.lineWidth());
    canvas_set_miter_limit(m_canvas, strokeData.miterLimit());
    canvas_set_line_cap(m_canvas, static_cast<line_cap_t>(strokeData.lineCap()));
    canvas_set_line_join(m_canvas, static_cast<line_join_t>(strokeData.lineJoin()));
    canvas_set_dash_offset(m_canvas, strokeData.dashOffset());
    canvas_set_dash_array(m_canvas, strokeData.dashArray().data(), strokeData.dashArray().size());
    canvas_set_operator(m_canvas, NOVA_OPERATOR_SRC_OVER);
    path_guard guard(to_path_t(path));
    canvas_stroke_path(m_canvas, guard.get());
}

void raster_canvas::fill_text(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, NOVA_FILL_RULE_NON_ZERO);
    canvas_set_operator(m_canvas, NOVA_OPERATOR_SRC_OVER);
    canvas_set_font(m_canvas, font.face().get(), font.size());
    canvas_fill_text(m_canvas, text.data(), text.length(), NOVA_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

void raster_canvas::stroke_text(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_line_width(m_canvas, strokeWidth);
    canvas_set_miter_limit(m_canvas, 4.f);
    canvas_set_line_cap(m_canvas, NOVA_LINE_CAP_BUTT);
    canvas_set_line_join(m_canvas, NOVA_LINE_JOIN_MITER);
    canvas_set_dash_offset(m_canvas, 0.f);
    canvas_set_dash_array(m_canvas, nullptr, 0);
    canvas_set_operator(m_canvas, NOVA_OPERATOR_SRC_OVER);
    canvas_set_font(m_canvas, font.face().get(), font.size());
    canvas_stroke_text(m_canvas, text.data(), text.length(), NOVA_TEXT_ENCODING_UTF32, origin.x, origin.y);
}

void raster_canvas::clip_path(const stb::outline& path, FillRule clipRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(clipRule));
    path_guard guard(to_path_t(path));
    canvas_clip_path(m_canvas, guard.get());
}

void raster_canvas::clip_rect(const Rect& rect, FillRule clipRule, const Transform& transform)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule_t>(clipRule));
    canvas_clip_rect(m_canvas, rect.x, rect.y, rect.w, rect.h);
}

void raster_canvas::draw_image(const pixmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform)
{
    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;
    matrix_t matrix = { xScale, 0, 0, yScale, -srcRect.x * xScale, -srcRect.y * yScale };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform.matrix());
    canvas_translate(m_canvas, dstRect.x, dstRect.y);
    canvas_set_fill_rule(m_canvas, NOVA_FILL_RULE_NON_ZERO);
    canvas_set_operator(m_canvas, NOVA_OPERATOR_SRC_OVER);
    canvas_set_texture(m_canvas, image.surface(), NOVA_TEXTURE_TYPE_PLAIN, 1.f, &matrix);
    canvas_fill_rect(m_canvas, 0, 0, dstRect.w, dstRect.h);
}

void raster_canvas::blend_canvas(const raster_canvas& canvas, BlendMode blendMode, float opacity)
{
    matrix_t matrix = { 1, 0, 0, 1, static_cast<float>(canvas.x()), static_cast<float>(canvas.y()) };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_set_operator(m_canvas, static_cast<operator_t>(blendMode));
    canvas_set_texture(m_canvas, canvas.surface(), NOVA_TEXTURE_TYPE_PLAIN, opacity, &matrix);
    canvas_paint(m_canvas);
}

void raster_canvas::save()
{
    canvas_save(m_canvas);
}

void raster_canvas::restore()
{
    canvas_restore(m_canvas);
}

int raster_canvas::width() const
{
    return surface_get_width(m_surface);
}

int raster_canvas::height() const
{
    return surface_get_height(m_surface);
}

void raster_canvas::convert_to_luminance_mask()
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

void raster_canvas::box_blur(float stdDeviationX, float stdDeviationY)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto rx = gaussianRadiusForSigma(stdDeviationX);
    auto ry = gaussianRadiusForSigma(stdDeviationY);
    for(int pass = 0; pass < 3; ++pass) {
        if(rx > 0) boxBlurPass(data, width, height, stride, rx, true);
        if(ry > 0) boxBlurPass(data, width, height, stride, ry, false);
    }
}

void raster_canvas::fill_opaque_white()
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);
    for(int y = 0; y < height; ++y) {
        auto* pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; ++x)
            pixels[x] = 0xFFFFFFFFu;
    }
}

void raster_canvas::tint_to_flood_color(const stb::color& floodColor, float floodOpacity)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto r = uint32_t(floodColor.r());
    auto g = uint32_t(floodColor.g());
    auto b = uint32_t(floodColor.b());
    for(int y = 0; y < height; ++y) {
        auto* pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; ++x) {
            auto srcAlpha = (pixels[x] >> 24) & 0xFF;
            auto alpha = uint32_t(srcAlpha * floodOpacity);
            // premultiplied: scale color channels by (alpha / 255)
            pixels[x] = (alpha << 24) | (((r * alpha) / 255) << 16) | (((g * alpha) / 255) << 8) | ((b * alpha) / 255);
        }
    }
}

void raster_canvas::shift(float dx, float dy)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);
    auto idx = int(dx);
    auto idy = int(dy);
    if(idx == 0 && idy == 0)
        return;

    auto* scratch = static_cast<unsigned char*>(malloc(size_t(stride) * size_t(height)));
    if(!scratch)
        return;
    memset(scratch, 0, size_t(stride) * size_t(height));
    for(int y = 0; y < height; ++y) {
        auto sy = y - idy;
        if(sy < 0 || sy >= height) continue;
        auto* src = reinterpret_cast<uint32_t*>(data + sy * stride);
        auto* dst = reinterpret_cast<uint32_t*>(scratch + y * stride);
        for(int x = 0; x < width; ++x) {
            auto sx = x - idx;
            if(sx < 0 || sx >= width) continue;
            dst[x] = src[sx];
        }
    }
    memcpy(data, scratch, size_t(stride) * size_t(height));
    free(scratch);
}

void raster_canvas::composite_with(const raster_canvas& src, porter_duff mode)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto sWidth = surface_get_width(src.m_surface);
    auto sHeight = surface_get_height(src.m_surface);
    auto sStride = surface_get_stride(src.m_surface);
    auto sData = surface_get_data(src.m_surface);

    for(int y = 0; y < height && y < sHeight; ++y) {
        auto* dst = reinterpret_cast<uint32_t*>(data + y * stride);
        auto* srcRow = reinterpret_cast<uint32_t*>(sData + y * sStride);
        for(int x = 0; x < width && x < sWidth; ++x) {
            auto s = srcRow[x];
            auto d = dst[x];
            auto sA = (s >> 24) & 0xFF;
            auto dA = (d >> 24) & 0xFF;

            // Porter-Duff coefficients (premultiplied): result = s*Fs + d*Fd
            int Fs, Fd;
            switch(mode) {
            case porter_duff::In:   Fs = dA;       Fd = 0;             break;
            case porter_duff::Out:  Fs = 255 - dA; Fd = 0;             break;
            case porter_duff::Atop: Fs = dA;       Fd = 255 - sA;      break;
            case porter_duff::Xor:  Fs = 255 - dA; Fd = 255 - sA;      break;
            case porter_duff::Over: default:       Fs = 255;          Fd = 255 - sA; break;
            }

            auto sr = (s >> 16) & 0xFF, sg = (s >> 8) & 0xFF, sb = s & 0xFF;
            auto dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
            auto outA = STB_MIN(255, int(sA * Fs + dA * Fd) / 255);
            auto outR = STB_MIN(255, int(sr * Fs + dr * Fd) / 255);
            auto outG = STB_MIN(255, int(sg * Fs + dg * Fd) / 255);
            auto outB = STB_MIN(255, int(sb * Fs + db * Fd) / 255);
            dst[x] = (uint32_t(outA) << 24) | (uint32_t(outR) << 16) | (uint32_t(outG) << 8) | uint32_t(outB);
        }
    }
}

void raster_canvas::composite_over(const raster_canvas& src)
{
    composite_with(src, porter_duff::Over);
}

void raster_canvas::composite_arithmetic(const raster_canvas& src, float k1, float k2, float k3, float k4)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto sWidth = surface_get_width(src.m_surface);
    auto sHeight = surface_get_height(src.m_surface);
    auto sStride = surface_get_stride(src.m_surface);
    auto sData = surface_get_data(src.m_surface);

    auto solve = [&](int i1, int i2) -> int {
        auto f1 = float(i1) / 255.f;
        auto f2 = float(i2) / 255.f;
        auto result = k1 * f1 * f2 + k2 * f1 + k3 * f2 + k4;
        return int(std::clamp(result, 0.f, 1.f) * 255.f + 0.5f);
    };

    for(int y = 0; y < height; ++y) {
        auto* dst = reinterpret_cast<uint32_t*>(data + y * stride);
        auto* srcRow = y < sHeight ? reinterpret_cast<uint32_t*>(sData + y * sStride) : nullptr;
        for(int x = 0; x < width; ++x) {
            auto s = (srcRow && x < sWidth) ? srcRow[x] : 0u;
            auto d = dst[x];
            auto outA = solve((s >> 24) & 0xFF, (d >> 24) & 0xFF);
            auto outR = solve((s >> 16) & 0xFF, (d >> 16) & 0xFF);
            auto outG = solve((s >> 8) & 0xFF, (d >> 8) & 0xFF);
            auto outB = solve(s & 0xFF, d & 0xFF);
            dst[x] = (uint32_t(outA) << 24) | (uint32_t(outR) << 16) | (uint32_t(outG) << 8) | uint32_t(outB);
        }
    }
}

void raster_canvas::composite_drop_shadow_under(const raster_canvas& original, float dx, float dy)
{
    shift(dx, dy);
    composite_over(original);
}

void raster_canvas::copy_pixels_from(const raster_canvas& other)
{
    auto stride = surface_get_stride(m_surface);
    auto height = surface_get_height(m_surface);
    memcpy(surface_get_data(m_surface), surface_get_data(other.m_surface), size_t(stride) * size_t(height));
}

raster_canvas::~raster_canvas()
{
    canvas_destroy(m_canvas);
    surface_destroy(m_surface);
}

raster_canvas::raster_canvas(const pixmap& bitmap)
    : m_surface(surface_reference(bitmap.surface()))
    , m_canvas(canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, 0, 0})
    , m_x(0), m_y(0)
{
}

raster_canvas::raster_canvas(int x, int y, int width, int height)
    : m_surface(surface_create(width, height))
    , m_canvas(canvas_create(m_surface))
    , m_translation({1, 0, 0, 1, -static_cast<float>(x), -static_cast<float>(y)})
    , m_x(x), m_y(y)
{
}

} // namespace nova::vg
