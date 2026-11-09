// Definitions declared in src/sources/vg/canvas.h.
#include "canvas.h"

namespace nova::vg {

transform::transform()
{
    m_matrix = stb::matrix{};
}

transform::transform(float a, float b, float c, float d, float e, float f)
{
    m_matrix = stb::matrix{a, b, c, d, e, f};
}

transform transform::operator*(const transform& transform_) const
{
    stb::matrix result;
    result = transform_.m_matrix.then(m_matrix);
    return result;
}

transform& transform::operator*=(const transform& transform_)
{
    return (*this = *this * transform_);
}

transform& transform::multiply(const transform& transform_)
{
    return (*this *= transform_);
}

transform& transform::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

transform& transform::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

transform& transform::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

transform& transform::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

transform& transform::post_multiply(const transform& transform_)
{
    return (*this = transform_ * *this);
}

transform& transform::post_translate(float tx, float ty)
{
    return post_multiply(translated(tx, ty));
}

transform& transform::post_scale(float sx, float sy)
{
    return post_multiply(scaled(sx, sy));
}

transform& transform::post_rotate(float angle, float cx, float cy)
{
    return post_multiply(rotated(angle, cx, cy));
}

transform& transform::post_shear(float shx, float shy)
{
    return post_multiply(sheared(shx, shy));
}

transform transform::inverse() const
{
    // A matrix that cannot be inverted gives the identity (it used to give an uninitialised matrix).
    return m_matrix.inverse().value_or(stb::matrix{});
}

transform& transform::invert()
{
    if(const auto inverse = m_matrix.inverse())
        m_matrix = *inverse;
    return *this;
}

void transform::reset()
{
    m_matrix = stb::matrix{};
}

point transform::map_point(float x, float y) const
{
    { const stb::point p_ = m_matrix.map(x, y); x = p_.x; y = p_.y; }
    return point(x, y);
}

point transform::map_point(const point& point_) const
{
    return map_point(point_.x, point_.y);
}

rect transform::map_rect(const rect& rect_) const
{
    if(!rect_.is_valid()) {
        return rect::invalid;
    }

    const stb::outline_bounds b = m_matrix.map_rect({rect_.x, rect_.y, rect_.x + rect_.w, rect_.y + rect_.h});
    return rect(b.x0, b.y0, b.width(), b.height());
}

float transform::x_scale() const
{
    return std::sqrt(m_matrix.a * m_matrix.a + m_matrix.b * m_matrix.b);
}

float transform::y_scale() const
{
    return std::sqrt(m_matrix.c * m_matrix.c + m_matrix.d * m_matrix.d);
}

bool transform::parse(const char* data, size_t length_)
{
    return parse_matrix(&m_matrix, data, length_);
}

transform transform::rotated(float angle, float cx, float cy)
{
    stb::matrix matrix;
    if(cx == 0.f && cy == 0.f) {
        matrix = stb::matrix::rotation(NOVA_DEG2RAD(angle));
    } else {
        matrix = stb::matrix::translation(cx, cy);
        matrix.rotate(NOVA_DEG2RAD(angle));
        matrix.translate(-cx, -cy);
    }

    return matrix;
}

transform transform::scaled(float sx, float sy)
{
    stb::matrix matrix;
    matrix = stb::matrix::scaling(sx, sy);
    return matrix;
}

transform transform::sheared(float shx, float shy)
{
    stb::matrix matrix;
    matrix = stb::matrix::shearing(NOVA_DEG2RAD(shx), NOVA_DEG2RAD(shy));
    return matrix;
}

transform transform::translated(float tx, float ty)
{
    stb::matrix matrix;
    matrix = stb::matrix::translation(tx, ty);
    return matrix;
}

namespace {
} // namespace

std::shared_ptr<raster_canvas> raster_canvas::create(const pixmap& bitmap)
{
    return std::shared_ptr<raster_canvas>(new raster_canvas(bitmap));
}

std::shared_ptr<raster_canvas> raster_canvas::create(float x, float y, float width, float height)
{
    constexpr int k_max_size = 1 << 15;
    if(width <= 0 || height <= 0 || width >= k_max_size || height >= k_max_size)
        return std::shared_ptr<raster_canvas>(new raster_canvas(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::shared_ptr<raster_canvas>(new raster_canvas(l, t, r - l, b - t));
}

std::shared_ptr<raster_canvas> raster_canvas::create(const rect& extents)
{
    return create(extents.x, extents.y, extents.w, extents.h);
}

void raster_canvas::set_color(const stb::color& color_)
{
    set_color(red_f(color_), green_f(color_), blue_f(color_), alpha_f(color_));
}

void raster_canvas::set_color(float r, float g, float b, float a)
{
    canvas_set_rgba(m_canvas, r, g, b, a);
}

void raster_canvas::set_linear_gradient(float x1, float y1, float x2, float y2, spread_method_value spread, const gradient_stops& stops, const transform& transform_)
{
    canvas_set_linear_gradient(m_canvas, x1, y1, x2, y2, static_cast<spread_method_value>(spread), stops.data(), stops.size(), &transform_.matrix());
}

void raster_canvas::set_radial_gradient(float cx, float cy, float r, float fx, float fy, spread_method_value spread, const gradient_stops& stops, const transform& transform_)
{
    canvas_set_radial_gradient(m_canvas, cx, cy, r, fx, fy, 0.f, static_cast<spread_method_value>(spread), stops.data(), stops.size(), &transform_.matrix());
}

void raster_canvas::set_texture(const raster_canvas& source, texture_type type_, float opacity, const transform& transform_)
{
    canvas_set_texture(m_canvas, source.surface(), static_cast<texture_type>(type_), opacity, &transform_.matrix());
}

void raster_canvas::fill_path(const stb::outline& path_, fill_rule fill_rule_, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule>(fill_rule_));
    canvas_set_operator(m_canvas, blend_mode::src_over);
        canvas_fill_path(m_canvas, path_);
}

void raster_canvas::stroke_path(const stb::outline& path_, const stroke_data& stroke_data_, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_line_width(m_canvas, stroke_data_.line_width());
    canvas_set_miter_limit(m_canvas, stroke_data_.miter_limit());
    canvas_set_line_cap(m_canvas, static_cast<line_cap>(stroke_data_.line_cap()));
    canvas_set_line_join(m_canvas, static_cast<line_join>(stroke_data_.line_join()));
    canvas_set_dash_offset(m_canvas, stroke_data_.dash_offset());
    canvas_set_dash_array(m_canvas, stroke_data_.dash_array().data(), stroke_data_.dash_array().size());
    canvas_set_operator(m_canvas, blend_mode::src_over);
        canvas_stroke_path(m_canvas, path_);
}

void raster_canvas::fill_text(const std::u32string_view& text, const sized_font& font_, const point& origin, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_fill_rule(m_canvas, fill_rule::non_zero);
    canvas_set_operator(m_canvas, blend_mode::src_over);
    canvas_set_font(m_canvas, font_.face().get(), font_.size());
    canvas_fill_text(m_canvas, text.data(), text.length(), text_encoding::utf32, origin.x, origin.y);
}

void raster_canvas::stroke_text(const std::u32string_view& text, float stroke_width, const sized_font& font_, const point& origin, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_line_width(m_canvas, stroke_width);
    canvas_set_miter_limit(m_canvas, 4.f);
    canvas_set_line_cap(m_canvas, line_cap::butt);
    canvas_set_line_join(m_canvas, line_join::miter);
    canvas_set_dash_offset(m_canvas, 0.f);
    canvas_set_dash_array(m_canvas, nullptr, 0);
    canvas_set_operator(m_canvas, blend_mode::src_over);
    canvas_set_font(m_canvas, font_.face().get(), font_.size());
    canvas_stroke_text(m_canvas, text.data(), text.length(), text_encoding::utf32, origin.x, origin.y);
}

void raster_canvas::clip_path(const stb::outline& path_, fill_rule clip_rule, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule>(clip_rule));
        canvas_clip_path(m_canvas, path_);
}

void raster_canvas::clip_rect(const rect& rect_, fill_rule clip_rule, const transform& transform_)
{
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_set_fill_rule(m_canvas, static_cast<fill_rule>(clip_rule));
    canvas_clip_rect(m_canvas, rect_.x, rect_.y, rect_.w, rect_.h);
}

void raster_canvas::draw_image(const pixmap& image, const rect& dst_rect, const rect& src_rect, const transform& transform_)
{
    auto x_scale = dst_rect.w / src_rect.w;
    auto y_scale = dst_rect.h / src_rect.h;
    stb::matrix matrix = { x_scale, 0, 0, y_scale, -src_rect.x * x_scale, -src_rect.y * y_scale };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_transform(m_canvas, &transform_.matrix());
    canvas_translate(m_canvas, dst_rect.x, dst_rect.y);
    canvas_set_fill_rule(m_canvas, fill_rule::non_zero);
    canvas_set_operator(m_canvas, blend_mode::src_over);
    canvas_set_texture(m_canvas, image.surface(), texture_type::plain, 1.f, &matrix);
    canvas_fill_rect(m_canvas, 0, 0, dst_rect.w, dst_rect.h);
}

void raster_canvas::blend_canvas(const raster_canvas& canvas, blend_mode blend_mode_, float opacity)
{
    stb::matrix matrix = { 1, 0, 0, 1, static_cast<float>(canvas.x()), static_cast<float>(canvas.y()) };
    canvas_set_matrix(m_canvas, &m_translation);
    canvas_set_operator(m_canvas, static_cast<blend_mode>(blend_mode_));
    canvas_set_texture(m_canvas, canvas.surface(), texture_type::plain, opacity, &matrix);
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

void raster_canvas::box_blur(float std_deviation_x, float std_deviation_y)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto rx = gaussian_radius_for_sigma(std_deviation_x);
    auto ry = gaussian_radius_for_sigma(std_deviation_y);
    for(int pass = 0; pass < 3; ++pass) {
        if(rx > 0) box_blur_pass(data, width, height, stride, rx, true);
        if(ry > 0) box_blur_pass(data, width, height, stride, ry, false);
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

void raster_canvas::tint_to_flood_color(const stb::color& flood_color, float flood_opacity)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto r = uint32_t(flood_color.r());
    auto g = uint32_t(flood_color.g());
    auto b = uint32_t(flood_color.b());
    for(int y = 0; y < height; ++y) {
        auto* pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; ++x) {
            auto src_alpha = (pixels[x] >> 24) & 0xFF;
            auto alpha = uint32_t(src_alpha * flood_opacity);
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

    auto s_width = surface_get_width(src.m_surface);
    auto s_height = surface_get_height(src.m_surface);
    auto s_stride = surface_get_stride(src.m_surface);
    auto s_data = surface_get_data(src.m_surface);

    for(int y = 0; y < height && y < s_height; ++y) {
        auto* dst = reinterpret_cast<uint32_t*>(data + y * stride);
        auto* src_row = reinterpret_cast<uint32_t*>(s_data + y * s_stride);
        for(int x = 0; x < width && x < s_width; ++x) {
            auto s = src_row[x];
            auto d = dst[x];
            auto s_a = (s >> 24) & 0xFF;
            auto d_a = (d >> 24) & 0xFF;

            // Porter-Duff coefficients (premultiplied): result = s*Fs + d*Fd
            int fs, fd;
            switch(mode) {
            case porter_duff::in:   fs = d_a;       fd = 0;             break;
            case porter_duff::out:  fs = 255 - d_a; fd = 0;             break;
            case porter_duff::atop: fs = d_a;       fd = 255 - s_a;      break;
            case porter_duff::exclusive_or:  fs = 255 - d_a; fd = 255 - s_a;      break;
            case porter_duff::over: default:       fs = 255;          fd = 255 - s_a; break;
            }

            auto sr = (s >> 16) & 0xFF, sg = (s >> 8) & 0xFF, sb = s & 0xFF;
            auto dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
            auto out_a = STB_MIN(255, int(s_a * fs + d_a * fd) / 255);
            auto out_r = STB_MIN(255, int(sr * fs + dr * fd) / 255);
            auto out_g = STB_MIN(255, int(sg * fs + dg * fd) / 255);
            auto out_b = STB_MIN(255, int(sb * fs + db * fd) / 255);
            dst[x] = (uint32_t(out_a) << 24) | (uint32_t(out_r) << 16) | (uint32_t(out_g) << 8) | uint32_t(out_b);
        }
    }
}

void raster_canvas::composite_over(const raster_canvas& src)
{
    composite_with(src, porter_duff::over);
}

void raster_canvas::composite_arithmetic(const raster_canvas& src, float k1, float k2, float k3, float k4)
{
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    auto data = surface_get_data(m_surface);

    auto s_width = surface_get_width(src.m_surface);
    auto s_height = surface_get_height(src.m_surface);
    auto s_stride = surface_get_stride(src.m_surface);
    auto s_data = surface_get_data(src.m_surface);

    auto solve = [&](int i1, int i2) -> int {
        auto f1 = float(i1) / 255.f;
        auto f2 = float(i2) / 255.f;
        auto result = k1 * f1 * f2 + k2 * f1 + k3 * f2 + k4;
        return int(std::clamp(result, 0.f, 1.f) * 255.f + 0.5f);
    };

    for(int y = 0; y < height; ++y) {
        auto* dst = reinterpret_cast<uint32_t*>(data + y * stride);
        auto* src_row = y < s_height ? reinterpret_cast<uint32_t*>(s_data + y * s_stride) : nullptr;
        for(int x = 0; x < width; ++x) {
            auto s = (src_row && x < s_width) ? src_row[x] : 0u;
            auto d = dst[x];
            auto out_a = solve((s >> 24) & 0xFF, (d >> 24) & 0xFF);
            auto out_r = solve((s >> 16) & 0xFF, (d >> 16) & 0xFF);
            auto out_g = solve((s >> 8) & 0xFF, (d >> 8) & 0xFF);
            auto out_b = solve(s & 0xFF, d & 0xFF);
            dst[x] = (uint32_t(out_a) << 24) | (uint32_t(out_r) << 16) | (uint32_t(out_g) << 8) | uint32_t(out_b);
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
