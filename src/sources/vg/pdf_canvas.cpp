#include "pdf_canvas.h"
#include "font_face.h"
#include <stb/logging.hpp>

#include <stb/zlib.hpp>

#include <atomic>
#include <cmath>
#include <cstdio>

namespace nova::vg {
namespace {
stb::logging::logger& pdf_log() { static auto& l = stb::logging::get_logger("novasvg.pdf"); return l; }
}  // namespace

namespace {

constexpr float k_px_to_pt = 0.75f;

std::string num(float v)
{
    if(!std::isfinite(v))
        v = 0.f;
    char buf[48];
    snprintf(buf, sizeof buf, "%.4f", v);
    std::string s = buf;
    while(!s.empty() && s.back() == '0')
        s.pop_back();
    if(!s.empty() && s.back() == '.')
        s.pop_back();
    if(s.empty() || s == "-0")
        s = "0";
    return s;
}

// `first` is applied first, then `second` (PDF row-vector convention, same as matrix_t).
matrix_t then(const matrix_t& first, const matrix_t& second)
{
    matrix_t r;
    r.a = first.a * second.a + first.b * second.c;
    r.b = first.a * second.b + first.b * second.d;
    r.c = first.c * second.a + first.d * second.c;
    r.d = first.c * second.b + first.d * second.d;
    r.e = first.e * second.a + first.f * second.c + second.e;
    r.f = first.e * second.b + first.f * second.d + second.f;
    return r;
}

point apply(const matrix_t& m, float x, float y)
{
    return point(m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f);
}

std::string matrix_str(const matrix_t& m)
{
    return num(m.a) + " " + num(m.b) + " " + num(m.c) + " " + num(m.d) + " " + num(m.e) + " " + num(m.f);
}

float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

std::string rgb_str(float r, float g, float b) { return num(clamp01(r)) + " " + num(clamp01(g)) + " " + num(clamp01(b)); }

} // namespace

std::shared_ptr<pdf_canvas> pdf_canvas::create(float width, float height)
{
    return std::shared_ptr<pdf_canvas>(new pdf_canvas(width, height));
}

pdf_canvas::pdf_canvas(float width, float height)
    : m_width(width > 0.f ? width : 1.f)
    , m_height(height > 0.f ? height : 1.f)
{
    m_paints.emplace_back();
    // y points down in novasvg; one cm at the top of the page turns PDF space into that.
    matrix_t base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
    m_content += matrix_str(base) + " cm\n";
}

int pdf_canvas::add_object(std::string dict)
{
    object o;
    o.dict = std::move(dict);
    m_objects.push_back(std::move(o));
    return int(m_objects.size());
}

int pdf_canvas::add_stream(std::string dict, const std::string& bytes, bool compress)
{
    object o;
    o.has_stream = true;
    if(compress) {
        auto z = stb::zlib_compress(bytes.data(), bytes.size());
        o.stream.assign(reinterpret_cast<const char*>(z.data()), z.size());
        dict += " /Filter /FlateDecode";
    } else {
        o.stream = bytes;
    }
    o.dict = "<< " + dict + " /Length " + std::to_string(o.stream.size()) + " >>";
    m_objects.push_back(std::move(o));
    return int(m_objects.size());
}

static const char* pdf_blend_name(blend_mode mode)
{
    switch(mode) {
    case blend_mode::multiply: return "Multiply";
    case blend_mode::screen: return "Screen";
    case blend_mode::darken: return "Darken";
    case blend_mode::lighten: return "Lighten";
    case blend_mode::difference: return "Difference";
    case blend_mode::exclusion: return "Exclusion";
    default: return "Normal";
    }
}

std::string pdf_canvas::alpha_state(float alpha, blend_mode blend_mode_)
{
    std::string alpha_text = num(clamp01(alpha));
    std::string blend = pdf_blend_name(blend_mode_);
    std::string key = alpha_text + "|" + blend;
    auto it = m_ext_gstates.find(key);
    if(it != m_ext_gstates.end())
        return it->second.name;
    std::string name = "GS" + std::to_string(m_ext_gstates.size());
    m_ext_gstates.emplace(key, ext_gstate{name, alpha_text, blend});
    return name;
}

void pdf_canvas::set_color(const stb::color& color_)
{
    set_color(red_f(color_), green_f(color_), blue_f(color_), alpha_f(color_));
}

void pdf_canvas::set_color(float r, float g, float b, float a)
{
    paint& p = m_paints.back();
    p = paint();
    p.r = r;
    p.g = g;
    p.b = b;
    p.a = a;
}

namespace {
// Once per process: a document with many gradients would otherwise repeat the same message.
void warn_gradient_limits(SpreadMethod spread, const GradientStops& stops)
{
    static std::atomic<bool> spread_said{false}, alpha_said{false};
    if(spread != SpreadMethod::pad && !spread_said.exchange(true))
        pdf_log().warning("gradient spreadMethod reflect/repeat is written as pad in PDF output");
    if(!alpha_said.load()) {
        for(const auto& s : stops) {
            if(s.color_.a < 1.f) {
                if(!alpha_said.exchange(true))
                    pdf_log().warning("per-stop alpha of a gradient is ignored in PDF output");
                break;
            }
        }
    }
}
}  // namespace

void pdf_canvas::set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const transform& transform_)
{
    paint& p = m_paints.back();
    p = paint();
    p.type_ = paint::kind::linear;
    warn_gradient_limits(spread, stops);
    p.p[0] = x1; p.p[1] = y1; p.p[2] = x2; p.p[3] = y2;
    p.spread = spread;
    p.stops = stops;
    p.matrix = transform_.matrix();
}

void pdf_canvas::set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const transform& transform_)
{
    paint& p = m_paints.back();
    p = paint();
    p.type_ = paint::kind::radial;
    warn_gradient_limits(spread, stops);
    p.p[0] = cx; p.p[1] = cy; p.p[2] = r; p.p[3] = fx; p.p[4] = fy;
    p.spread = spread;
    p.stops = stops;
    p.matrix = transform_.matrix();
}

// A shading pattern for the current gradient paint, drawn through `transform` (the matrix of the operation).
// Returns the pattern's object number, or 0 when the gradient has no stops (nothing to paint).
int pdf_canvas::shading_pattern(const matrix_t& transform_)
{
    const paint& p = m_paints.back();
    if(p.stops.empty())
        return 0;

    // Stops, extended to cover 0..1 so pad works through /Extend alone.
    std::vector<gradient_stop_t> stops = p.stops;
    if(stops.front().offset > 0.f)
        stops.insert(stops.begin(), gradient_stop_t{0.f, stops.front().color_});
    if(stops.back().offset < 1.f)
        stops.push_back(gradient_stop_t{1.f, stops.back().color_});
    for(size_t i = 1; i < stops.size(); ++i)
        if(stops[i].offset < stops[i - 1].offset)
            stops[i].offset = stops[i - 1].offset;

    std::string function;
    if(stops.size() == 2) {
        function = "<< /FunctionType 2 /Domain [0 1] /N 1 /C0 [" + rgb_str(stops[0].color_.r, stops[0].color_.g, stops[0].color_.b)
            + "] /C1 [" + rgb_str(stops[1].color_.r, stops[1].color_.g, stops[1].color_.b) + "] >>";
    } else {
        std::string functions, bounds, encode;
        for(size_t i = 0; i + 1 < stops.size(); ++i) {
            functions += "<< /FunctionType 2 /Domain [0 1] /N 1 /C0 [" + rgb_str(stops[i].color_.r, stops[i].color_.g, stops[i].color_.b)
                + "] /C1 [" + rgb_str(stops[i + 1].color_.r, stops[i + 1].color_.g, stops[i + 1].color_.b) + "] >> ";
            encode += "0 1 ";
            if(i + 2 < stops.size()) {
                float bound = stops[i + 1].offset;
                // Bounds must increase: nudge a hard stop's second half a hair.
                if(i > 0 && bound <= stops[i].offset)
                    bound = stops[i].offset + 1e-5f;
                bounds += num(bound) + " ";
            }
        }
        function = "<< /FunctionType 3 /Domain [0 1] /Functions [" + functions + "] /Bounds [" + bounds + "] /Encode [" + encode + "] >>";
    }

    std::string shading;
    if(p.type_ == paint::kind::linear) {
        shading = "<< /ShadingType 2 /ColorSpace /DeviceRGB /Coords [" + num(p.p[0]) + " " + num(p.p[1]) + " " + num(p.p[2]) + " " + num(p.p[3])
            + "] /Function " + function + " /Extend [true true] >>";
    } else {
        // Focal point as a zero-radius start circle.
        shading = "<< /ShadingType 3 /ColorSpace /DeviceRGB /Coords [" + num(p.p[3]) + " " + num(p.p[4]) + " 0 " + num(p.p[0]) + " " + num(p.p[1]) + " "
            + num(p.p[2]) + "] /Function " + function + " /Extend [true true] >>";
    }

    // The pattern matrix maps pattern space to the *default page space*, not to the current CTM.
    matrix_t base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
    matrix_t m = then(then(p.matrix, transform_), base);
    int obj = add_object("<< /Type /Pattern /PatternType 2 /Shading " + shading + " /Matrix [" + matrix_str(m) + "] >>");
    std::string name = "P" + std::to_string(m_patterns.size());
    m_patterns.emplace_back(name, obj);
    return obj;
}

// Selects the current paint as fill or stroke color in the content stream. `transform` is the operation's matrix
// (only gradients need it). Returns false when there is nothing to paint.
void pdf_canvas::begin_paint(bool stroke, const matrix_t& transform_)
{
    const paint& p = m_paints.back();
    if(p.type_ == paint::kind::solid) {
        m_content += "/" + alpha_state(p.a) + " gs\n";
        m_content += rgb_str(p.r, p.g, p.b) + (stroke ? " RG\n" : " rg\n");
        return;
    }

    if(p.type_ == paint::kind::tiled) {
        tiling_pattern(transform_);
        m_content += "/" + alpha_state(p.a) + " gs\n";
        m_content += stroke ? "/Pattern CS /" : "/Pattern cs /";
        m_content += m_patterns.back().first + (stroke ? " SCN\n" : " scn\n");
        return;
    }

    if(shading_pattern(transform_) == 0) {
        m_content += "/" + alpha_state(0.f) + " gs\n";
        return;
    }
    m_content += "/" + alpha_state(1.f) + " gs\n";
    m_content += stroke ? "/Pattern CS /" : "/Pattern cs /";
    m_content += m_patterns.back().first + (stroke ? " SCN\n" : " scn\n");
}

void pdf_canvas::emit_path(const stb::outline& path_, const matrix_t* transform_)
{
    if(path_.empty())
        return;
    auto pt = [&](const point& p) {
        point q = transform_ ? apply(*transform_, p.x, p.y) : p;
        return num(q.x) + " " + num(q.y);
    };
    std::array<point, 3> pts;
    for(outline_walker it(path_); !it.done(); it.next()) {
        switch(it.current_segment(pts)) {
        case segment_kind::move_to:
            m_content += pt(pts[0]) + " m\n";
            break;
        case segment_kind::line_to:
            m_content += pt(pts[0]) + " l\n";
            break;
        case segment_kind::cubic_to:
            m_content += pt(pts[0]) + " " + pt(pts[1]) + " " + pt(pts[2]) + " c\n";
            break;
        case segment_kind::close:
            m_content += "h\n";
            break;
        }
    }
}

void pdf_canvas::emit_rect_path(const rect& rect_, const matrix_t* transform_)
{
    stb::outline path_;
    path_.add_rect(rect_.x, rect_.y, rect_.w, rect_.h);
    emit_path(path_, transform_);
}

void pdf_canvas::emit_text_path(const std::u32string_view& text, const font& font_, const point& origin, const matrix_t* transform_)
{
    font_face_t* face = font_.face().get();
    if(face == nullptr || font_.size_() <= 0.f)
        return;

    struct sink {
        pdf_canvas* self;
        const matrix_t* transform_;
    } s{this, transform_};

    auto traverse = [](void* closure, path_command_t command, const point_t* points, int) {
        auto* s = static_cast<sink*>(closure);
        auto pt = [&](const point_t& p) {
            point q = s->transform_ ? apply(*s->transform_, p.x, p.y) : point(p.x, p.y);
            return num(q.x) + " " + num(q.y);
        };
        switch(command) {
        case NOVA_PATH_COMMAND_MOVE_TO:
            s->self->m_content += pt(points[0]) + " m\n";
            break;
        case NOVA_PATH_COMMAND_LINE_TO:
            s->self->m_content += pt(points[0]) + " l\n";
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            s->self->m_content += pt(points[0]) + " " + pt(points[1]) + " " + pt(points[2]) + " c\n";
            break;
        case NOVA_PATH_COMMAND_CLOSE:
            s->self->m_content += "h\n";
            break;
        }
    };

    // Same layout as canvas_add_text(): ligatures, then kerning, then the glyph's outline.
    text_iterator_t it;
    text_iterator_init(&it, text.data(), int(text.length()), NOVA_TEXT_ENCODING_UTF32);
    float advance = 0.f;
    codepoint_t previous = 0;
    bool has_previous = false;
    while(text_iterator_has_next(&it)) {
        codepoint_t codepoint = text_iterator_next(&it);
        codepoint = font_face_apply_ligature(face, &it, codepoint);
        if(has_previous)
            advance += font_face_get_kern_advance(face, font_.size_(), previous, codepoint);
        advance += font_face_traverse_glyph_path(face, font_.size_(), origin.x + advance, origin.y, codepoint, traverse, &s);
        previous = codepoint;
        has_previous = true;
    }
}

void pdf_canvas::fill_path(const stb::outline& path_, fill_rule fill_rule_, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(false, transform_.matrix());
    emit_path(path_, &transform_.matrix());
    m_content += fill_rule_ == fill_rule::even_odd ? "f*\n" : "f\n";
    m_content += "Q\n";
}

void pdf_canvas::stroke_path(const stb::outline& path_, const stroke_data& sd, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(true, transform_.matrix());
    // The path stays in its own space and goes through `cm`, so the line width scales with the matrix, anisotropically if need be.
    m_content += matrix_str(transform_.matrix()) + " cm\n";
    m_content += num(sd.line_width()) + " w\n";
    m_content += std::to_string(int(sd.line_cap_())) + " J\n";
    m_content += std::to_string(int(sd.line_join_())) + " j\n";
    m_content += num(sd.miter_limit() < 1.f ? 1.f : sd.miter_limit()) + " M\n";
    if(!sd.dashArray().empty()) {
        m_content += "[";
        for(float d : sd.dashArray())
            m_content += num(d) + " ";
        m_content += "] " + num(sd.dash_offset()) + " d\n";
    }
    emit_path(path_, nullptr);
    m_content += "S\nQ\n";
}

void pdf_canvas::fill_text(const std::u32string_view& text, const font& font_, const point& origin, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(false, transform_.matrix());
    emit_text_path(text, font_, origin, &transform_.matrix());
    m_content += "f\nQ\n";
}

void pdf_canvas::stroke_text(const std::u32string_view& text, float stroke_width, const font& font_, const point& origin, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(true, transform_.matrix());
    m_content += matrix_str(transform_.matrix()) + " cm\n";
    m_content += num(stroke_width) + " w\n0 J\n0 j\n4 M\n";
    emit_text_path(text, font_, origin, nullptr);
    m_content += "S\nQ\n";
}

void pdf_canvas::clip_path(const stb::outline& path_, fill_rule clip_rule, const transform& transform_)
{
    emit_path(path_, &transform_.matrix());
    m_content += clip_rule == fill_rule::even_odd ? "W* n\n" : "W n\n";
}

void pdf_canvas::clip_rect(const rect& rect_, fill_rule clip_rule, const transform& transform_)
{
    emit_rect_path(rect_, &transform_.matrix());
    m_content += clip_rule == fill_rule::even_odd ? "W* n\n" : "W n\n";
}

int pdf_canvas::add_image(const pixmap& image, int* width, int* height)
{
    stb::image rgba = bitmap_to_rgba_image(image);
    const int w = rgba.width();
    const int h = rgba.height();
    *width = w;
    *height = h;
    if(w <= 0 || h <= 0)
        return 0;

    std::string color_(size_t(w) * h * 3, '\0'), alpha(size_t(w) * h, '\0');
    bool translucent = false;
    const uint8_t* px = rgba.data();
    for(size_t i = 0, n = size_t(w) * h; i < n; ++i) {
        color_[i * 3 + 0] = char(px[i * 4 + 0]);
        color_[i * 3 + 1] = char(px[i * 4 + 1]);
        color_[i * 3 + 2] = char(px[i * 4 + 2]);
        alpha[i] = char(px[i * 4 + 3]);
        translucent |= px[i * 4 + 3] != 255;
    }

    std::string dict = "/Type /XObject /Subtype /Image /Width " + std::to_string(w) + " /Height " + std::to_string(h)
        + " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Interpolate true";
    if(translucent) {
        int mask = add_stream("/Type /XObject /Subtype /Image /Width " + std::to_string(w) + " /Height " + std::to_string(h)
            + " /ColorSpace /DeviceGray /BitsPerComponent 8 /Interpolate true", alpha, true);
        dict += " /SMask " + std::to_string(mask) + " 0 R";
    }
    return add_stream(dict, color_, true);
}

// The whole image (w x h px) scaled so that `src` (in image pixels) lands on `dst`, clipped to `dst`.
void pdf_canvas::place_image(int object, int w, int h, const rect& dst, const rect& src, const matrix_t* transform_, float alpha, blend_mode blend_mode_)
{
    std::string name = "Im" + std::to_string(m_xobjects.size());
    m_xobjects.emplace_back(name, object);

    const float xs = dst.w / src.w, ys = dst.h / src.h;
    const float ix = dst.x - src.x * xs, iy = dst.y - src.y * ys;
    const float iw = w * xs, ih = h * ys;
    m_content += "q\n";
    if(transform_)
        m_content += matrix_str(*transform_) + " cm\n";
    emit_rect_path(dst, nullptr);
    m_content += "W n\n/" + alpha_state(alpha, blend_mode_) + " gs\n";
    m_content += num(iw) + " 0 0 " + num(-ih) + " " + num(ix) + " " + num(iy + ih) + " cm\n/" + name + " Do\nQ\n";
}

void pdf_canvas::draw_image(const pixmap& image, const rect& dst, const rect& src, const transform& transform_)
{
    if(image.is_null() || src.w <= 0.f || src.h <= 0.f || dst.w <= 0.f || dst.h <= 0.f)
        return;
    int w, h;
    int object = add_image(image, &w, &h);
    if(object == 0)
        return;
    place_image(object, w, h, dst, src, &transform_.matrix(), 1.f, blend_mode::src_over);
}

void pdf_canvas::draw_raster_group(const pixmap& image, const rect& dst, blend_mode blend_mode_, float opacity)
{
    if(image.is_null() || dst.w <= 0.f || dst.h <= 0.f)
        return;
    int w, h;
    int object = add_image(image, &w, &h);
    if(object == 0)
        return;
    place_image(object, w, h, dst, rect(0, 0, float(w), float(h)), nullptr, opacity, blend_mode_);
}

void pdf_canvas::set_tiled_image(const pixmap& tile, const transform& transform_, float opacity)
{
    if(tile.is_null())
        return;
    int w, h;
    int object = add_image(tile, &w, &h);
    if(object == 0)
        return;
    paint& p = m_paints.back();
    p = paint();
    p.type_ = paint::kind::tiled;
    p.tile_object = object;
    p.tile_w = w;
    p.tile_h = h;
    p.a = opacity;
    p.matrix = transform_.matrix();
}

// A tiling pattern for the current tiled paint, drawn through `transform` (the matrix of the operation).
int pdf_canvas::tiling_pattern(const matrix_t& transform_)
{
    const paint& p = m_paints.back();
    matrix_t base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
    matrix_t m = then(then(p.matrix, transform_), base);
    const std::string w = std::to_string(p.tile_w), h = std::to_string(p.tile_h);
    // Tile space is y-down like the bitmap: the image's top row sits at y = 0.
    std::string content = w + " 0 0 -" + h + " 0 " + h + " cm\n/T Do\n";
    int obj = add_stream("/Type /Pattern /PatternType 1 /PaintType 1 /TilingType 1 /BBox [0 0 " + w + " " + h + "] /XStep " + w + " /YStep " + h
        + " /Matrix [" + matrix_str(m) + "] /Resources << /XObject << /T " + std::to_string(p.tile_object) + " 0 R >> >>", content, false);
    std::string name = "P" + std::to_string(m_patterns.size());
    m_patterns.emplace_back(name, obj);
    return obj;
}

void pdf_canvas::begin_group()
{
    m_groups.push_back({std::move(m_content), m_open_saves});
    m_content.clear();
    m_paints.push_back(m_paints.back());
}

void pdf_canvas::end_group(blend_mode blend_mode_, float opacity)
{
    if(m_groups.empty())
        return;
    group_frame frame = std::move(m_groups.back());
    m_groups.pop_back();

    // Saves opened inside the group and never restored die with the form.
    while(m_open_saves > frame.open_saves) {
        m_content += "Q\n";
        m_paints.pop_back();
        --m_open_saves;
    }
    m_paints.pop_back();

    std::string body = std::move(m_content);
    m_content = std::move(frame.outer_content);
    int form = add_stream("/Type /XObject /Subtype /Form /BBox [0 0 " + num(m_width) + " " + num(m_height)
        + "] /Group << /S /Transparency /CS /DeviceRGB >> /Resources %%RES%%", body, true);
    std::string name = "Fm" + std::to_string(m_xobjects.size());
    m_xobjects.emplace_back(name, form);
    m_content += "q\n/" + alpha_state(opacity, blend_mode_) + " gs\n/" + name + " Do\nQ\n";
}

void pdf_canvas::save()
{
    m_content += "q\n";
    m_paints.push_back(m_paints.back());
    ++m_open_saves;
}

void pdf_canvas::restore()
{
    if(m_open_saves == (m_groups.empty() ? 0 : m_groups.back().open_saves))
        return;
    m_content += "Q\n";
    m_paints.pop_back();
    --m_open_saves;
}

std::vector<uint8_t> pdf_canvas::data() const
{
    // Work on a copy so the canvas stays usable: add the page's own objects after the ones drawing created.
    std::vector<object> objects = m_objects;
    auto add = [&](std::string dict) {
        object o;
        o.dict = std::move(dict);
        objects.push_back(std::move(o));
        return int(objects.size());
    };

    std::string content = m_content;
    for(int i = 0; i < m_open_saves; ++i)
        content += "Q\n";
    auto z = stb::zlib_compress(content.data(), content.size());

    object stream;
    stream.has_stream = true;
    stream.stream.assign(reinterpret_cast<const char*>(z.data()), z.size());
    stream.dict = "<< /Filter /FlateDecode /Length " + std::to_string(stream.stream.size()) + " >>";
    objects.push_back(std::move(stream));
    const int contents = int(objects.size());

    std::string gs, pat, xo;
    for(const auto& e : m_ext_gstates)
        gs += "/" + e.second.name + " << /ca " + e.second.alpha + " /CA " + e.second.alpha + " /BM /" + e.second.blend + " >> ";
    for(const auto& p : m_patterns)
        pat += "/" + p.first + " " + std::to_string(p.second) + " 0 R ";
    for(const auto& x : m_xobjects)
        xo += "/" + x.first + " " + std::to_string(x.second) + " 0 R ";
    const std::string resources = "<< /ExtGState << " + gs + ">> /Pattern << " + pat + ">> /XObject << " + xo + ">> >>";
    for(auto& o : objects) {
        for(size_t at = o.dict.find("%%RES%%"); at != std::string::npos; at = o.dict.find("%%RES%%", at + resources.size()))
            o.dict.replace(at, 7, resources);
    }

    const int pages_id = int(objects.size()) + 2; // page = +1, pages = +2
    const int page = add("<< /Type /Page /Parent " + std::to_string(pages_id) + " 0 R /MediaBox [0 0 " + num(m_width * k_px_to_pt) + " " + num(m_height * k_px_to_pt)
        + "] /Contents " + std::to_string(contents) + " 0 R /Resources " + resources
        + " /Group << /S /Transparency /CS /DeviceRGB >> >>");
    const int pages = add("<< /Type /Pages /Kids [" + std::to_string(page) + " 0 R] /Count 1 >>");
    const int catalog = add("<< /Type /Catalog /Pages " + std::to_string(pages) + " 0 R >>");

    std::string out = "%PDF-1.5\n%\xE2\xE3\xCF\xD3\n";
    std::vector<size_t> offsets;
    for(size_t i = 0; i < objects.size(); ++i) {
        offsets.push_back(out.size());
        out += std::to_string(i + 1) + " 0 obj\n" + objects[i].dict + "\n";
        if(objects[i].has_stream)
            out += "stream\n" + objects[i].stream + "\nendstream\n";
        out += "endobj\n";
    }

    const size_t xref = out.size();
    out += "xref\n0 " + std::to_string(objects.size() + 1) + "\n0000000000 65535 f \n";
    for(size_t off : offsets) {
        char line[32];
        snprintf(line, sizeof line, "%010zu 00000 n \n", off);
        out += line;
    }
    out += "trailer\n<< /Size " + std::to_string(objects.size() + 1) + " /Root " + std::to_string(catalog) + " 0 R >>\nstartxref\n" + std::to_string(xref) + "\n%%EOF\n";
    return std::vector<uint8_t>(out.begin(), out.end());
}

bool pdf_canvas::write_to_file(const char* filename) const
{
    FILE* fp = fopen(filename, "wb");
    if(!fp)
        return false;
    auto bytes = data();
    bool ok = fwrite(bytes.data(), 1, bytes.size(), fp) == bytes.size();
    return fclose(fp) == 0 && ok;
}

} // namespace nova::vg
