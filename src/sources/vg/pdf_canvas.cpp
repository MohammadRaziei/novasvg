#include "pdf_canvas.h"
#include <stb/logging.hpp>
#include <stb/util.hpp>

#include <stb/zlib.hpp>

#include <atomic>
#include <algorithm>
#include <cmath>
#include <cctype>
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

// `first` is applied first, then `second` (PDF row-vector convention, same as stb::matrix).
stb::matrix then(const stb::matrix& first, const stb::matrix& second)
{
    stb::matrix r;
    r.a = first.a * second.a + first.b * second.c;
    r.b = first.a * second.b + first.b * second.d;
    r.c = first.c * second.a + first.d * second.c;
    r.d = first.c * second.b + first.d * second.d;
    r.e = first.e * second.a + first.f * second.c + second.e;
    r.f = first.e * second.b + first.f * second.d + second.f;
    return r;
}

point apply(const stb::matrix& m, float x, float y)
{
    return point(m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f);
}

std::string matrix_str(const stb::matrix& m)
{
    return num(m.a) + " " + num(m.b) + " " + num(m.c) + " " + num(m.d) + " " + num(m.e) + " " + num(m.f);
}

float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

std::string rgb_str(float r, float g, float b) { return num(clamp01(r)) + " " + num(clamp01(g)) + " " + num(clamp01(b)); }

// The UTF-16 units of a character, as four hex digits each (one unit, or two for a surrogate pair).
std::string utf16_hex(char32_t cp)
{
    char unit[16];
    if(cp >= 0x10000) {
        const char32_t v = cp - 0x10000;
        snprintf(unit, sizeof unit, "%04X%04X", unsigned(0xD800 + (v >> 10)), unsigned(0xDC00 + (v & 0x3FF)));
    } else {
        snprintf(unit, sizeof unit, "%04X", unsigned(cp));
    }
    return unit;
}

// The /ToUnicode CMap of a font: for each CID the character it stands for.
std::string to_unicode_cmap(const std::vector<std::pair<std::uint32_t, char32_t>>& by_cid)
{
    std::string cmap = "/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n"
                       "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
                       "/CMapName /Adobe-Identity-UCS def\n/CMapType 2 def\n"
                       "1 begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n";
    for(size_t from = 0; from < by_cid.size(); from += 100) {
        const size_t to = std::min(by_cid.size(), from + 100);
        cmap += std::to_string(to - from) + " beginbfchar\n";
        for(size_t i = from; i < to; ++i) {
            char line[16];
            snprintf(line, sizeof line, "<%04X> <", unsigned(by_cid[i].first));
            cmap += line + utf16_hex(by_cid[i].second) + ">\n";
        }
        cmap += "endbfchar\n";
    }
    return cmap + "endcmap\nCMapName currentdict /CMap defineresource pop\nend\nend\n";
}

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
    stb::matrix base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
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

// The PDF names of the W3C blend modes (ISO 32000-1 11.3.5). `add` has none and stays Normal.
static const char* pdf_blend_name(blend_mode mode)
{
    switch(mode) {
    case blend_mode::multiply: return "Multiply";
    case blend_mode::screen: return "Screen";
    case blend_mode::overlay: return "Overlay";
    case blend_mode::darken: return "Darken";
    case blend_mode::lighten: return "Lighten";
    case blend_mode::color_dodge: return "ColorDodge";
    case blend_mode::color_burn: return "ColorBurn";
    case blend_mode::hard_light: return "HardLight";
    case blend_mode::soft_light: return "SoftLight";
    case blend_mode::difference: return "Difference";
    case blend_mode::exclusion: return "Exclusion";
    case blend_mode::hue: return "Hue";
    case blend_mode::saturation: return "Saturation";
    case blend_mode::color: return "Color";
    case blend_mode::luminosity: return "Luminosity";
    default: return "Normal";
    }
}

std::string pdf_canvas::alpha_state(float alpha, blend_mode blend_mode_, int soft_mask, bool mask_alpha)
{
    std::string alpha_text = num(clamp01(alpha));
    std::string blend = pdf_blend_name(blend_mode_);
    std::string key = alpha_text + "|" + blend + "|" + std::to_string(soft_mask) + (mask_alpha ? "a" : "l");
    auto it = m_ext_gstates.find(key);
    if(it != m_ext_gstates.end())
        return it->second.name;
    std::string name = "GS" + std::to_string(m_ext_gstates.size());
    m_ext_gstates.emplace(key, ext_gstate{name, alpha_text, blend, soft_mask, mask_alpha});
    return name;
}

void pdf_canvas::set_color(const color& color_)
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
// The one alpha of a gradient whose stops all share it (what `fill-opacity` makes of a gradient), 0..1; -1 when the
// stops differ, a fade that this backend cannot paint.
float uniform_stop_alpha(const gradient_stops& stops)
{
    if(stops.empty())
        return 1.f;
    for(const auto& s : stops)
        if(s.color_.a() != stops.front().color_.a())
            return -1.f;
    return alpha_f(stops.front().color_);
}

// A PDF text string: plain ASCII in parentheses, anything else as UTF-16BE with a byte order mark.
std::string pdf_text_string(const std::string& utf8)
{
    bool ascii = true;
    for(const unsigned char c : utf8)
        if(c < 0x20 || c > 0x7e)
            ascii = false;
    if(ascii) {
        std::string out = "(";
        for(const char c : utf8) {
            if(c == '(' || c == ')' || c == '\\')
                out += '\\';
            out += c;
        }
        return out + ")";
    }
    std::string out = "<FEFF";
    char unit[8];
    for(const char32_t cp : stb::utf8_decode_lossy(utf8)) {
        if(cp >= 0x10000) {  // a surrogate pair
            const char32_t v = cp - 0x10000;
            snprintf(unit, sizeof unit, "%04X", unsigned(0xD800 + (v >> 10)));
            out += unit;
            snprintf(unit, sizeof unit, "%04X", unsigned(0xDC00 + (v & 0x3FF)));
        } else {
            snprintf(unit, sizeof unit, "%04X", unsigned(cp));
        }
        out += unit;
    }
    return out + ">";
}

// The inverse of `m`, false when it has none.
bool invert_matrix(const stb::matrix& m, stb::matrix& out)
{
    const float det = m.a * m.d - m.b * m.c;
    if(!(std::fabs(det) > 1e-12f))
        return false;
    out.a = m.d / det;
    out.b = -m.b / det;
    out.c = -m.c / det;
    out.d = m.a / det;
    out.e = (m.c * m.f - m.d * m.e) / det;
    out.f = (m.b * m.e - m.a * m.f) / det;
    return true;
}

// A gradient that repeats or reflects is painted as one shading over a stretch of `periods` periods, long enough to
// cover the page; past that the last colour is padded. The cap keeps the function a sane size.
constexpr int max_spread_periods = 128;

// `periods` copies of the stops side by side as one function over [0, 1]. With `reflect`, the copies whose index
// (counted from `first_period`, which may be negative) is odd run backwards.
std::string spread_function(const std::vector<gradient_stop_t>& stops, int periods, bool reflect, int first_period)
{
    std::string functions, bounds, encode;
    float previous_bound = 0.f;
    const size_t segments = stops.size() - 1;
    for(int j = 0; j < periods; ++j) {
        const bool backwards = reflect && ((first_period + j) & 1);
        for(size_t k = 0; k < segments; ++k) {
            const size_t i = backwards ? segments - 1 - k : k;
            const auto& from = backwards ? stops[i + 1] : stops[i];
            const auto& to = backwards ? stops[i] : stops[i + 1];
            functions += "<< /FunctionType 2 /Domain [0 1] /N 1 /C0 [" + rgb_str(red_f(from.color_), green_f(from.color_), blue_f(from.color_))
                + "] /C1 [" + rgb_str(red_f(to.color_), green_f(to.color_), blue_f(to.color_)) + "] >> ";
            encode += "0 1 ";
            const bool last = j + 1 == periods && k + 1 == segments;
            if(!last) {
                const float end = backwards ? 1.f - stops[i].offset : stops[i + 1].offset;
                float bound = (float(j) + end) / float(periods);
                if(bound <= previous_bound)  // a hard stop: bounds must increase
                    bound = previous_bound + 1e-6f;
                bounds += num(bound) + " ";
                previous_bound = bound;
            }
        }
    }
    return "<< /FunctionType 3 /Domain [0 1] /Functions [" + functions + "] /Bounds [" + bounds + "] /Encode [" + encode + "] >>";
}
}  // namespace

// Once per page: a drawing with many gradients would otherwise repeat the same message.
void pdf_canvas::set_linear_gradient(float x1, float y1, float x2, float y2, spread_method_value spread, const gradient_stops& stops, const transform& transform_)
{
    paint& p = m_paints.back();
    p = paint();
    p.type_ = paint::kind::linear;
    p.p[0] = x1; p.p[1] = y1; p.p[2] = x2; p.p[3] = y2;
    p.spread = spread;
    p.stops = stops;
    p.matrix = transform_.matrix();
}

void pdf_canvas::set_radial_gradient(float cx, float cy, float r, float fx, float fy, spread_method_value spread, const gradient_stops& stops, const transform& transform_)
{
    paint& p = m_paints.back();
    p = paint();
    p.type_ = paint::kind::radial;
    p.p[0] = cx; p.p[1] = cy; p.p[2] = r; p.p[3] = fx; p.p[4] = fy;
    p.spread = spread;
    p.stops = stops;
    p.matrix = transform_.matrix();
}

// A shading pattern with the geometry of the current gradient paint and the colours of `gradient` (the paint's own
// stops, or grey levels for a fade mask), drawn through `transform` (the matrix of the operation). Returns the
// pattern's object number, or 0 when there are no stops (nothing to paint).
int pdf_canvas::shading_pattern(const stb::matrix& transform_, const gradient_stops& gradient, bool in_form)
{
    const paint& p = m_paints.back();
    if(gradient.empty())
        return 0;

    // Stops, extended to cover 0..1 so pad works through /Extend alone.
    std::vector<gradient_stop_t> stops = gradient;
    if(stops.front().offset > 0.f)
        stops.insert(stops.begin(), gradient_stop_t{0.f, stops.front().color_});
    if(stops.back().offset < 1.f)
        stops.push_back(gradient_stop_t{1.f, stops.back().color_});
    for(size_t i = 1; i < stops.size(); ++i)
        if(stops[i].offset < stops[i - 1].offset)
            stops[i].offset = stops[i - 1].offset;

    // reflect and repeat: one shading over as many periods as the page needs (see spread_function)
    std::string function, coords;
    if(p.spread != spread_method_value::pad && stops.size() >= 2) {
        const bool reflect = p.spread == spread_method_value::reflect;
        stb::matrix to_gradient;
        if(invert_matrix(then(p.matrix, transform_), to_gradient)) {
            const point corners[4] = {apply(to_gradient, 0, 0), apply(to_gradient, m_width, 0), apply(to_gradient, 0, m_height),
                                      apply(to_gradient, m_width, m_height)};
            if(p.type_ == paint::kind::linear) {
                const float dx = p.p[2] - p.p[0], dy = p.p[3] - p.p[1], length_squared = dx * dx + dy * dy;
                if(length_squared > 0.f) {
                    float t_min = 1e30f, t_max = -1e30f;
                    for(const point& c : corners) {
                        const float t = ((c.x - p.p[0]) * dx + (c.y - p.p[1]) * dy) / length_squared;
                        t_min = std::min(t_min, t);
                        t_max = std::max(t_max, t);
                    }
                    const int first = int(std::floor(t_min));
                    const int last = std::min(std::max(int(std::ceil(t_max)), first + 1), first + max_spread_periods);
                    function = spread_function(stops, last - first, reflect, first);
                    coords = num(p.p[0] + float(first) * dx) + " " + num(p.p[1] + float(first) * dy) + " " + num(p.p[0] + float(last) * dx) + " "
                        + num(p.p[1] + float(last) * dy);
                }
            } else if(p.p[2] > 0.f) {
                const float reach = std::hypot(p.p[3] - p.p[0], p.p[4] - p.p[1]);
                float farthest = 0.f;
                for(const point& c : corners)
                    farthest = std::max(farthest, std::hypot(c.x - p.p[0], c.y - p.p[1]));
                const int periods = std::min(std::max(int(std::ceil((farthest + reach) / p.p[2])), 1), max_spread_periods);
                function = spread_function(stops, periods, reflect, 0);
                coords = num(p.p[3]) + " " + num(p.p[4]) + " 0 " + num(p.p[3] + float(periods) * (p.p[0] - p.p[3])) + " "
                    + num(p.p[4] + float(periods) * (p.p[1] - p.p[4])) + " " + num(float(periods) * p.p[2]);
            }
        }
    }

    if(!function.empty()) {
        // done above
    } else if(stops.size() == 2) {
        function = "<< /FunctionType 2 /Domain [0 1] /N 1 /C0 [" + rgb_str(red_f(stops[0].color_), green_f(stops[0].color_), blue_f(stops[0].color_))
            + "] /C1 [" + rgb_str(red_f(stops[1].color_), green_f(stops[1].color_), blue_f(stops[1].color_)) + "] >>";
    } else {
        std::string functions, bounds, encode;
        for(size_t i = 0; i + 1 < stops.size(); ++i) {
            functions += "<< /FunctionType 2 /Domain [0 1] /N 1 /C0 [" + rgb_str(red_f(stops[i].color_), green_f(stops[i].color_), blue_f(stops[i].color_))
                + "] /C1 [" + rgb_str(red_f(stops[i + 1].color_), green_f(stops[i + 1].color_), blue_f(stops[i + 1].color_)) + "] >> ";
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
        if(coords.empty())
            coords = num(p.p[0]) + " " + num(p.p[1]) + " " + num(p.p[2]) + " " + num(p.p[3]);
        shading = "<< /ShadingType 2 /ColorSpace /DeviceRGB /Coords [" + coords + "] /Function " + function + " /Extend [true true] >>";
    } else {
        // Focal point as a zero-radius start circle.
        if(coords.empty())
            coords = num(p.p[3]) + " " + num(p.p[4]) + " 0 " + num(p.p[0]) + " " + num(p.p[1]) + " " + num(p.p[2]);
        shading = "<< /ShadingType 3 /ColorSpace /DeviceRGB /Coords [" + coords + "] /Function " + function + " /Extend [true true] >>";
    }

    // The pattern matrix maps pattern space to the default space of the content stream that uses it, not to the
    // current CTM: for the page that is point space with y up (hence `base`), for a Form XObject it is the form's
    // own space, which is the pixel space already.
    stb::matrix base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
    stb::matrix m = in_form ? then(p.matrix, transform_) : then(then(p.matrix, transform_), base);
    int obj = add_object("<< /Type /Pattern /PatternType 2 /Shading " + shading + " /Matrix [" + matrix_str(m) + "] >>");
    std::string name = "P" + std::to_string(m_patterns.size());
    m_patterns.emplace_back(name, obj);
    return obj;
}

int pdf_canvas::fade_mask(const stb::matrix& transform_)
{
    // The stop alphas as grey: luminosity of (a, a, a) is a, so the soft mask passes exactly the stops' alpha.
    gradient_stops grey = m_paints.back().stops;
    for(auto& stop : grey)
        stop.color_ = stb::color(stop.color_.a(), stop.color_.a(), stop.color_.a(), 255);
    if(shading_pattern(transform_, grey, true) == 0)
        return 0;
    const std::string pattern = m_patterns.back().first;
    const std::string page = "0 0 " + num(m_width) + " " + num(m_height);
    // BBox and the rectangle are in pixels: the soft mask is read in the space the `gs` that names it runs in.
    return add_stream("/Type /XObject /Subtype /Form /BBox [" + page + "] /Group << /S /Transparency /CS /DeviceRGB >> /Resources %%RES%%",
                      "/Pattern cs /" + pattern + " scn\n" + page + " re f\n", true);
}

// Selects the current paint as fill or stroke color in the content stream. `transform` is the operation's matrix
// (only gradients need it). Returns false when there is nothing to paint.
void pdf_canvas::begin_paint(bool stroke, const stb::matrix& transform_)
{
    const paint& p = m_paints.back();
    if(p.type_ == paint::kind::solid) {
        m_content += "/" + alpha_state(p.a) + " gs\n";
        m_content += rgb_str(p.r, p.g, p.b) + (stroke ? " RG\n" : " rg\n");
        return;
    }

    // Inside a group (a Form XObject, drawn in pixels) a pattern matrix is read in the form's space, not the page's.
    const bool in_form = !m_groups.empty();
    if(p.type_ == paint::kind::tiled) {
        tiling_pattern(transform_, in_form);
        m_content += "/" + alpha_state(p.a) + " gs\n";
        m_content += stroke ? "/Pattern CS /" : "/Pattern cs /";
        m_content += m_patterns.back().first + (stroke ? " SCN\n" : " scn\n");
        return;
    }

    if(shading_pattern(transform_, p.stops, in_form) == 0) {
        m_content += "/" + alpha_state(0.f) + " gs\n";
        return;
    }
    const std::string color_pattern = m_patterns.back().first;
    // What `fill-opacity` makes of a gradient is one alpha shared by all its stops: an ordinary constant alpha. Stops
    // with different alpha (a fade) become a soft mask over the same gradient.
    const float gradient_alpha = uniform_stop_alpha(p.stops);
    const int mask = gradient_alpha < 0.f ? fade_mask(transform_) : 0;
    m_content += "/" + alpha_state(gradient_alpha < 0.f ? 1.f : gradient_alpha, blend_mode::normal, mask) + " gs\n";
    m_content += stroke ? "/Pattern CS /" : "/Pattern cs /";
    m_content += color_pattern + (stroke ? " SCN\n" : " scn\n");
}

void pdf_canvas::emit_path(const outline& path_, const stb::matrix* transform_)
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

void pdf_canvas::emit_rect_path(const rect& rect_, const stb::matrix* transform_)
{
    outline path_;
    path_.add_rect(rect_.x, rect_.y, rect_.w, rect_.h);
    emit_path(path_, transform_);
}

void pdf_canvas::emit_text_path(const std::u32string_view& text, const sized_font& font_, const point& origin, const stb::matrix* transform_, const shaping_context& context)
{
    if(font_.is_null())
        return;

    struct sink {
        pdf_canvas* self;
        const stb::matrix* transform_;
    } s{this, transform_};

    auto traverse = [](void* closure, outline_op command, const stb::point* points, int) {
        auto* s = static_cast<sink*>(closure);
        auto pt = [&](const stb::point& p) {
            point q = s->transform_ ? apply(*s->transform_, p.x, p.y) : point(p.x, p.y);
            return num(q.x) + " " + num(q.y);
        };
        switch(command) {
        case outline_op::move_to:
            s->self->m_content += pt(points[0]) + " m\n";
            break;
        case outline_op::line_to:
            s->self->m_content += pt(points[0]) + " l\n";
            break;
        case outline_op::cubic_to:
            s->self->m_content += pt(points[0]) + " " + pt(points[1]) + " " + pt(points[2]) + " c\n";
            break;
        case outline_op::close:
            s->self->m_content += "h\n";
            break;
        }
    };

    // Same layout as canvas_add_text(): the shaped glyphs, each with its outline.
    std::vector<shaped_glyph_t> glyphs;
    font_.shape(text, &glyphs, context);
    for(const shaped_glyph_t& g : glyphs)
        font_.traverse_glyph_path(g.glyph, origin.x + g.x, origin.y + g.y, traverse, &s);
}

void pdf_canvas::fill_path(const outline& path_, fill_rule fill_rule_, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(false, transform_.matrix());
    emit_path(path_, &transform_.matrix());
    m_content += fill_rule_ == fill_rule::even_odd ? "f*\n" : "f\n";
    m_content += "Q\n";
}

void pdf_canvas::stroke_path(const outline& path_, const stroke_data& sd, const transform& transform_)
{
    m_content += "q\n";
    begin_paint(true, transform_.matrix());
    // The path stays in its own space and goes through `cm`, so the line width scales with the matrix, anisotropically if need be.
    m_content += matrix_str(transform_.matrix()) + " cm\n";
    m_content += num(sd.line_width()) + " w\n";
    m_content += std::to_string(int(sd.line_cap())) + " J\n";
    m_content += std::to_string(int(sd.line_join())) + " j\n";
    m_content += num(sd.miter_limit() < 1.f ? 1.f : sd.miter_limit()) + " M\n";
    if(!sd.dash_array().empty()) {
        m_content += "[";
        for(float d : sd.dash_array())
            m_content += num(d) + " ";
        m_content += "] " + num(sd.dash_offset()) + " d\n";
    }
    emit_path(path_, nullptr);
    m_content += "S\nQ\n";
}

void pdf_canvas::fill_text(const std::u32string_view& text, const sized_font& font_, const point& origin, const transform& transform_, const shaping_context& context)
{
    if(emit_embedded_text(text, font_, origin, transform_, context, nullptr))
        return;
    m_content += "q\n";
    begin_paint(false, transform_.matrix());
    emit_text_path(text, font_, origin, &transform_.matrix(), context);
    m_content += "f\nQ\n";
    emit_text_layer(text, font_, origin, transform_.matrix(), context);
}

void pdf_canvas::emit_text_layer(const std::u32string_view& text, const sized_font& font_, const point& origin,
                                 const stb::matrix& transform_, const shaping_context& context)
{
    if(m_mask_depth > 0 || font_.is_null() || !(font_.size() > 0.f))
        return;
    // One two-byte code per character (the CID), every glyph of the font 0.5 em wide: the run is then stretched
    // (Tz) to the width the shaper measured, so a selection covers what the outlines cover.
    std::string codes;
    int count = 0;
    for(char32_t ch : text) {
        if(ch == U'\t' || ch == U'\n' || ch == U'\r')
            ch = U' ';
        if(ch < 0x20 || ch == 0x7f)
            continue;
        auto it = m_text_cids.find(ch);
        if(it == m_text_cids.end()) {
            if(m_text_cids.size() >= 0xFFFF)
                continue;
            it = m_text_cids.emplace(ch, int(m_text_cids.size()) + 1).first;
        }
        char hex[8];
        std::snprintf(hex, sizeof hex, "%04X", it->second);
        codes += hex;
        ++count;
    }
    if(count == 0)
        return;
    const float size = font_.size();
    if(repeats_last_text(text, origin, transform_, size))
        return;  // the other half of a text that is both filled and stroked
    const float advance = font_.measure_text(text, context);
    const float scale = advance > 0.f ? 100.f * advance / (0.5f * size * float(count)) : 100.f;
    // The matrix of the text: its x axis is the element's, its y axis points up on the page, which is -y here.
    const stb::matrix& m = transform_;
    const point at = apply(m, origin.x, origin.y);
    m_content += "q\nBT\n3 Tr\n/FTxt " + num(size) + " Tf\n" + num(scale) + " Tz\n" + num(m.a) + " " + num(m.b) + " "
        + num(-m.c) + " " + num(-m.d) + " " + num(at.x) + " " + num(at.y) + " Tm\n<" + codes + "> Tj\nET\nQ\n";
}

bool pdf_canvas::repeats_last_text(const std::u32string_view& text, const point& origin, const stb::matrix& m, float size)
{
    const std::array<float, 9> at_ = {origin.x, origin.y, m.a, m.b, m.c, m.d, m.e, m.f, size};
    if(text == m_last_text && at_ == m_last_text_at)
        return true;
    m_last_text.assign(text.data(), text.size());
    m_last_text_at = at_;
    return false;
}

pdf_canvas::embedded_font* pdf_canvas::embedded_font_for(const font_face& face)
{
    for(embedded_font& e : m_embedded)
        if(e.face.get() == face.get())
            return e.usable ? &e : nullptr;

    embedded_font e;
    e.face = face;
    const stb::font& f = *face.get();
    const std::string family = f.family_name().empty() ? std::string("a font") : "the font '" + f.family_name() + "'";
    const stb::subsetter subsetter(f);
    if(!subsetter.supported()) {
        pdf_log().warning("%s cannot be embedded (it has CFF outlines or a variation applied): its text is written as outlines", family.c_str());
    } else if(m_text_embedding == text_embedding::embed) {
        const stb::font_embedding license = f.embedding();
        if(license.permission() == stb::font_embedding::level::restricted)
            pdf_log().warning("the license of %s forbids embedding it: its text is written as outlines", family.c_str());
        else if(license.bitmap_only())
            pdf_log().warning("the license of %s allows only bitmaps to be embedded: its text is written as outlines", family.c_str());
        else if(!license.subsetting_allowed())
            pdf_log().warning("the license of %s allows it to be embedded whole only: its text is written as outlines", family.c_str());
        else
            e.usable = true;
    } else {
        e.usable = true;  // embed_ignore_license
    }
    if(e.usable)
        e.name = "FE" + std::to_string(m_embedded.size());
    m_embedded.push_back(std::move(e));
    return m_embedded.back().usable ? &m_embedded.back() : nullptr;
}

bool pdf_canvas::emit_embedded_text(const std::u32string_view& text, const sized_font& font_, const point& origin, const transform& transform_,
                                    const shaping_context& context, const float* stroke_width)
{
    if(m_text_embedding == text_embedding::outlines || font_.is_null() || !(font_.size() > 0.f))
        return false;
    embedded_font* font = embedded_font_for(font_.face());
    if(!font)
        return false;

    std::vector<shaped_glyph_t> glyphs;
    font_.shape(text, &glyphs, context);
    const stb::matrix& m = transform_.matrix();

    // The text a viewer extracts. A glyph that is the font's own glyph for the character at its place says so in
    // /ToUnicode, and a run that is all such glyphs needs nothing more. Any other run (a ligature, joined or reordered
    // letters, a character the font has no glyph for) names its whole logical text in /ActualText, which a glyph's
    // /ToUnicode entry cannot say. A text that is the other half of a fill + stroke, or part of a mask, says none
    // (an /ActualText of nothing; /Artifact would say the same, but poppler ignores it and would read the run twice).
    // ponytail: pdfium reads an /ActualText past the BMP as nothing; such a character the font has is in a simple run, so it is read.
    std::u32string chars;
    for(char32_t ch : text) {
        if(ch == U'\t' || ch == U'\n' || ch == U'\r')
            ch = U' ';
        if(ch >= 0x20 && ch != 0x7f)
            chars += ch;
    }
    const stb::font& face = *font_.face().get();
    bool simple = glyphs.size() == chars.size();
    for(size_t i = 0; i < chars.size(); ++i) {
        const std::uint32_t g = face.glyph_index(chars[i]);
        if(g != 0)
            font->unicode.emplace(g, chars[i]);
        if(simple && (g == 0 || glyphs[i].glyph != g))
            simple = false;
    }
    const bool again = m_mask_depth > 0 || repeats_last_text(text, origin, m, font_.size());
    std::string actual;
    if(again)
        actual = "<FEFF>";
    else if(!simple) {
        actual = "<FEFF";
        for(const char32_t ch : chars)
            actual += utf16_hex(ch);
        actual += ">";
    }
    if(!actual.empty())
        m_content += "/Span << /ActualText " + actual + " >> BDC\n";
    m_content += "q\n";
    begin_paint(stroke_width != nullptr, m);
    // The text keeps its own space and goes through `cm`, so the line width of a stroke scales with the matrix as the
    // outlines' does; a glyph is put at (x, y) of that space, flipped (y points down here, up in a font).
    m_content += matrix_str(m) + " cm\n";
    if(stroke_width)
        m_content += num(*stroke_width) + " w\n0 J\n0 j\n4 M\n";
    m_content += std::string("BT\n") + (stroke_width ? "1" : "0") + " Tr\n/" + font->name + " " + num(font_.size()) + " Tf\n";
    for(const shaped_glyph_t& g : glyphs) {
        font->glyphs.insert(g.glyph);
        char code[16];
        snprintf(code, sizeof code, "<%04X>", unsigned(g.glyph & 0xFFFFu));
        m_content += "1 0 0 -1 " + num(origin.x + g.x) + " " + num(origin.y + g.y) + " Tm " + code + " Tj\n";
    }
    m_content += "ET\nQ\n";
    if(!actual.empty())
        m_content += "EMC\n";
    return true;
}

void pdf_canvas::stroke_text(const std::u32string_view& text, float stroke_width, const sized_font& font_, const point& origin, const transform& transform_, const shaping_context& context)
{
    if(emit_embedded_text(text, font_, origin, transform_, context, &stroke_width))
        return;
    m_content += "q\n";
    begin_paint(true, transform_.matrix());
    m_content += matrix_str(transform_.matrix()) + " cm\n";
    m_content += num(stroke_width) + " w\n0 J\n0 j\n4 M\n";
    emit_text_path(text, font_, origin, nullptr, context);
    m_content += "S\nQ\n";
    emit_text_layer(text, font_, origin, transform_.matrix(), context);
}

void pdf_canvas::clip_path(const outline& path_, fill_rule clip_rule, const transform& transform_)
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
void pdf_canvas::place_image(int object, int w, int h, const rect& dst, const rect& src, const stb::matrix* transform_, float alpha, blend_mode blend_mode_)
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
    place_image(object, w, h, dst, src, &transform_.matrix(), 1.f, blend_mode::normal);
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

void pdf_canvas::set_pattern(const pixmap& tile, const transform& transform_, float opacity)
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
int pdf_canvas::tiling_pattern(const stb::matrix& transform_, bool in_form)
{
    const paint& p = m_paints.back();
    stb::matrix base = {k_px_to_pt, 0, 0, -k_px_to_pt, 0, m_height * k_px_to_pt};
    stb::matrix m = in_form ? then(p.matrix, transform_) : then(then(p.matrix, transform_), base);
    const std::string w = std::to_string(p.tile_w), h = std::to_string(p.tile_h);
    // Tile space is y-down like the bitmap: the image's top row sits at y = 0.
    std::string content = w + " 0 0 -" + h + " 0 " + h + " cm\n/T Do\n";
    int obj = add_stream("/Type /Pattern /PatternType 1 /PaintType 1 /TilingType 1 /BBox [0 0 " + w + " " + h + "] /XStep " + w + " /YStep " + h
        + " /Matrix [" + matrix_str(m) + "] /Resources << /XObject << /T " + std::to_string(p.tile_object) + " 0 R >> >>", content, false);
    std::string name = "P" + std::to_string(m_patterns.size());
    m_patterns.emplace_back(name, obj);
    return obj;
}

layer_scope pdf_canvas::begin_layer(const layer_params& params)
{
    layer_scope scope;
    scope.params = params;
    if(!params.needs_pixels) {
        begin_group();
        return scope;
    }
    // The group needs its pixels: a bitmap the size of its bounds on this page, drawn at raster_scale().
    auto bounds = params.bounds;
    bounds.intersect(extents());
    const float s = m_raster_scale;
    scope.target = raster_canvas::create(rect(bounds.x * s, bounds.y * s, bounds.w * s, bounds.h * s));
    scope.pre_transform = transform::scaled(s, s);
    return scope;
}

void pdf_canvas::end_layer(layer_scope&& scope, const layer_effects& effects)
{
    if(scope.in_place()) {
        // The mask is painted now, after the content, as a group of its own that the group is composited through.
        const auto& mask = scope.params.mask;
        const int soft_mask = mask && mask->draw ? mask_group(*mask) : 0;
        end_group(scope.params.blend, scope.params.opacity, soft_mask, mask && !mask->luminance);
        return;
    }
    auto& layer = static_cast<raster_canvas&>(*scope.target);
    if(effects)
        effects(layer);
    const float s = m_raster_scale;
    const rect e = layer.extents();
    draw_raster_group(pixmap(layer.surface()), rect(e.x / s, e.y / s, e.w / s, e.h / s), scope.params.blend, scope.params.opacity);
}

void pdf_canvas::begin_group()
{
    m_groups.push_back({std::move(m_content), m_open_saves});
    m_content.clear();
    m_paints.push_back(m_paints.back());
}

int pdf_canvas::finish_group()
{
    if(m_groups.empty())
        return 0;
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
    return add_stream("/Type /XObject /Subtype /Form /BBox [0 0 " + num(m_width) + " " + num(m_height)
        + "] /Group << /S /Transparency /CS /DeviceRGB >> /Resources %%RES%%", body, true);
}

void pdf_canvas::end_group(blend_mode blend_mode_, float opacity, int soft_mask, bool mask_alpha)
{
    const int form = finish_group();
    if(form == 0)
        return;
    std::string name = "Fm" + std::to_string(m_xobjects.size());
    m_xobjects.emplace_back(name, form);
    m_content += "q\n/" + alpha_state(opacity, blend_mode_, soft_mask, mask_alpha) + " gs\n/" + name + " Do\nQ\n";
}

int pdf_canvas::mask_group(const layer_mask& mask)
{
    // Painted in the page's own space: a soft mask is read in the space of the `gs` that names it, and the group it
    // masks is placed in that same space. Whatever it leaves unpainted is a mask value of 0 (black backdrop for a
    // luminosity mask, no alpha for an alpha one), so the masked group is hidden there.
    begin_group();
    ++m_mask_depth;  // what a mask is made of is not the page's text
    mask.draw();
    --m_mask_depth;
    return finish_group();
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

    // A Flate stream object: `extra` is the dictionary entries besides /Filter and /Length.
    auto add_flate = [&](const std::string& bytes, const std::string& extra = std::string()) {
        object o;
        o.has_stream = true;
        const auto z = stb::zlib_compress(bytes.data(), bytes.size());
        o.stream.assign(reinterpret_cast<const char*>(z.data()), z.size());
        o.dict = "<< /Filter /FlateDecode /Length " + std::to_string(o.stream.size()) + extra + " >>";
        objects.push_back(std::move(o));
        return int(objects.size());
    };

    std::string font_entries;

    // The font of the invisible text layer, when there is any: a Type 0 font that has no glyphs, only a /ToUnicode.
    if(!m_text_cids.empty()) {
        std::vector<std::pair<std::uint32_t, char32_t>> by_cid;
        for(const auto& c : m_text_cids)
            by_cid.emplace_back(std::uint32_t(c.second), c.first);
        std::sort(by_cid.begin(), by_cid.end());
        const int to_unicode_id = add_flate(to_unicode_cmap(by_cid));
        const int descriptor = add("<< /Type /FontDescriptor /FontName /NovaSvgText /Flags 4 /FontBBox [0 -200 1000 800] "
                                   "/ItalicAngle 0 /Ascent 800 /Descent -200 /CapHeight 700 /StemV 80 >>");
        const int cid_font = add("<< /Type /Font /Subtype /CIDFontType2 /BaseFont /NovaSvgText "
                                 "/CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> /FontDescriptor "
                                 + std::to_string(descriptor) + " 0 R /DW 500 /CIDToGIDMap /Identity >>");
        const int type0 = add("<< /Type /Font /Subtype /Type0 /BaseFont /NovaSvgText /Encoding /Identity-H /DescendantFonts ["
                              + std::to_string(cid_font) + " 0 R] /ToUnicode " + std::to_string(to_unicode_id) + " 0 R >>");
        font_entries += "/FTxt " + std::to_string(type0) + " 0 R ";
    }

    // The embedded fonts (text_embedding): each a Type 0 font over a CIDFontType2 whose program is the subset of the
    // glyphs used. A glyph's CID is its index in the original font, so the content did not have to wait for the
    // subset; /CIDToGIDMap says where each of those went in the subset (libstb numbers the glyphs again).
    for(const embedded_font& e : m_embedded) {
        if(!e.usable || e.glyphs.empty())
            continue;
        const stb::font& f = *e.face.get();
        const stb::font_design_metrics d = f.design_metrics();
        const float k = 1000.f / float(d.units_per_em > 0 ? d.units_per_em : 1000);

        std::string base = "Font";
        if(!f.family_name().empty()) {
            base.clear();
            for(const char c : f.family_name())
                if(std::isalnum(static_cast<unsigned char>(c)))
                    base += c;
            if(base.empty())
                base = "Font";
        }
        std::uint32_t hash = 2166136261u;  // FNV-1a over the glyphs: the same drawing always gives the same bytes
        for(const std::uint32_t g : e.glyphs)
            hash = (hash ^ g) * 16777619u;
        std::string tag;
        for(int i = 0; i < 6; ++i, hash /= 26)
            tag += char('A' + hash % 26);
        const std::string name = tag + "+" + base;

        std::string file_entry, gid_map_entry = " /CIDToGIDMap /Identity";
        try {
            stb::subsetter subsetter(f);
            for(const std::uint32_t g : e.glyphs)
                subsetter.glyph(g);
            const stb::font_subset subset = subsetter.run();
            const std::string program(reinterpret_cast<const char*>(subset.data().data()), subset.data().size());
            const int program_id = add_flate(program, " /Length1 " + std::to_string(program.size()));
            file_entry = " /FontFile2 " + std::to_string(program_id) + " 0 R";
            std::string map(2 * (size_t(*e.glyphs.rbegin()) + 1), '\0');
            for(const std::uint32_t g : e.glyphs) {
                const std::uint32_t to = subset.new_index(g);
                map[2 * g] = char(to >> 8);
                map[2 * g + 1] = char(to & 0xFF);
            }
            gid_map_entry = " /CIDToGIDMap " + std::to_string(add_flate(map)) + " 0 R";
        } catch(const stb::error& error) {
            // The text is already in the content, so there is nothing to fall back to: the font goes without its
            // program (a viewer substitutes one; the text still extracts) and the failure is reported.
            pdf_log().error("cannot subset the font for embedding: %s", error.what());
        }

        std::string widths;
        for(const std::uint32_t g : e.glyphs)
            widths += std::to_string(g) + " [" + std::to_string(int(std::lround(f.glyph_info(g).advance * k))) + "] ";
        std::vector<std::pair<std::uint32_t, char32_t>> by_cid(e.unicode.begin(), e.unicode.end());
        const int to_unicode_id = add_flate(to_unicode_cmap(by_cid));
        const int descriptor = add("<< /Type /FontDescriptor /FontName /" + name + " /Flags 4 /FontBBox [" + std::to_string(int(std::lround(d.x_min * k)))
                                   + " " + std::to_string(int(std::lround(d.y_min * k))) + " " + std::to_string(int(std::lround(d.x_max * k))) + " "
                                   + std::to_string(int(std::lround(d.y_max * k))) + "] /ItalicAngle 0 /Ascent " + std::to_string(int(std::lround(d.ascent * k)))
                                   + " /Descent " + std::to_string(int(std::lround(d.descent * k))) + " /CapHeight " + std::to_string(int(std::lround(d.ascent * k)))
                                   + " /StemV 80" + file_entry + " >>");
        const int cid_font = add("<< /Type /Font /Subtype /CIDFontType2 /BaseFont /" + name + " /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> "
                                 "/FontDescriptor " + std::to_string(descriptor) + " 0 R /DW 1000 /W [" + widths + "]" + gid_map_entry + " >>");
        const int type0 = add("<< /Type /Font /Subtype /Type0 /BaseFont /" + name + " /Encoding /Identity-H /DescendantFonts [" + std::to_string(cid_font)
                              + " 0 R] /ToUnicode " + std::to_string(to_unicode_id) + " 0 R >>");
        font_entries += "/" + e.name + " " + std::to_string(type0) + " 0 R ";
    }
    const std::string font_resource = font_entries.empty() ? std::string() : " /Font << " + font_entries + ">>";

    std::string gs, pat, xo;
    for(const auto& e : m_ext_gstates)
        gs += "/" + e.second.name + " << /ca " + e.second.alpha + " /CA " + e.second.alpha + " /BM /" + e.second.blend
            + (e.second.soft_mask ? std::string(" /SMask << /Type /Mask /S ") + (e.second.mask_alpha ? "/Alpha" : "/Luminosity") + " /G " + std::to_string(e.second.soft_mask) + " 0 R >> >> " : " /SMask /None >> ");
    for(const auto& p : m_patterns)
        pat += "/" + p.first + " " + std::to_string(p.second) + " 0 R ";
    for(const auto& x : m_xobjects)
        xo += "/" + x.first + " " + std::to_string(x.second) + " 0 R ";
    const std::string resources = "<< /ExtGState << " + gs + ">> /Pattern << " + pat + ">> /XObject << " + xo + ">>" + font_resource + " >>";
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
    int info_object = 0;
    if(!m_info.title.empty() || !m_info.producer.empty()) {
        std::string dict = "<<";
        if(!m_info.title.empty())
            dict += " /Title " + pdf_text_string(m_info.title);
        if(!m_info.producer.empty())
            dict += " /Producer " + pdf_text_string(m_info.producer);
        info_object = add(dict + " >>");
    }

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
    out += "trailer\n<< /Size " + std::to_string(objects.size() + 1) + " /Root " + std::to_string(catalog) + " 0 R"
        + (info_object ? " /Info " + std::to_string(info_object) + " 0 R" : std::string()) + " >>\nstartxref\n" + std::to_string(xref) + "\n%%EOF\n";
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
