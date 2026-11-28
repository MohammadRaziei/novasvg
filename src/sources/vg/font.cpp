// Definitions declared in src/sources/vg/font.h: font faces on top of stb::font (libstb), and a face at a size.
//
// libstb reads outlines, metrics, kerning (kern/GPOS) and GSUB/GPOS shaping, and tells what a font file is called; this
// file only applies a pixel size and hands the glyphs to the renderer.
#include "../utils/text.h"
#include "font.h"
#include <stb/logging.hpp>
#include <stb/util.hpp>

#include <algorithm>
#include <cmath>
#include <exception>

namespace nova::vg {

namespace {

stb::logging::logger& font_log() { static auto& l = stb::logging::get_logger("novasvg.font"); return l; }

// stb::font::glyph_index throws above U+10FFFF; such a code point is just "not in the font".
std::uint32_t glyph_of(const stb::font& font, char32_t codepoint)
{
    return codepoint > 0x10FFFFu ? 0u : font.glyph_index(codepoint);
}

}  // namespace

font_face font_face::from_memory(std::vector<std::uint8_t> bytes, int ttcindex)
{
    if(bytes.empty())
        return font_face();
    const size_t length_ = bytes.size();
    try {
        return font_face(stb::font::from_memory(std::move(bytes), ttcindex));
    } catch(const std::exception& e) {
        font_log().warning("cannot load a font from memory (%zu bytes, index %d): %s", length_, ttcindex, e.what());
    }
    return font_face();
}

font_face font_face::from_file(const char* filename, int ttcindex)
{
    try {
        return font_face(stb::font::open(filename, ttcindex));
    } catch(const std::exception& e) {
        font_log().warning("cannot load the font file '%s' (index %d): %s", filename, ttcindex, e.what());
    }
    return font_face();
}

font_face::font_face(const void* data, size_t length_)
    : font_face(from_memory(std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + length_)))
{
}

float font_face::units_per_em() const
{
    return is_null() ? 0.f : static_cast<float>(m_font->design_metrics().units_per_em);
}

float font_face::ascent_units() const
{
    return is_null() ? 0.f : static_cast<float>(m_font->design_metrics().ascent);
}

float font_face::descent_units() const
{
    return is_null() ? 0.f : static_cast<float>(m_font->design_metrics().descent);
}

float font_face::advance_width_units(char32_t codepoint) const
{
    return is_null() ? 0.f : m_font->glyph_info(glyph_of(*m_font, codepoint)).advance;
}

float font_face::kern_advance_units(char32_t first, char32_t second) const
{
    // The kern table or GPOS pair adjustment, per pair, as libstb resolves it.
    return is_null() ? 0.f : m_font->glyph_kerning(glyph_of(*m_font, first), glyph_of(*m_font, second));
}

std::vector<char32_t> font_face::codepoints() const
{
    return is_null() ? std::vector<char32_t>() : m_font->codepoints();
}

font_face font_face_cache::get_font_face(const std::string& family, int weight, bool italic) const
{
    if(auto face = get_font_face_local(family, weight, italic); !face.is_null()) {
        return face;
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
            if(auto face = get_font_face_local(value.fallback, weight, italic); !face.is_null())
                return face;
            // Not even the platform's stand-in is installed: use the embedded font
            // so text is still drawn (and drawn the same way) on a machine without fonts.
            return embedded_fallback_font(weight >= 600, italic, family == "monospace" ? fallback_family::mono : family == "serif" ? fallback_family::serif : fallback_family::sans);
        }
    }

    return font_face();
}

font_face font_face_cache::get_font_face_for_family_stack(const std::string& family_stack, int weight, bool italic) const
{
    // Metric-compatible stand-ins only: a substitute must lay text out the
    // same way as the font it replaces, so a name with no such stand-in
    // (verdana, trebuchet ms, ...) is skipped and the stack moves on. Each
    // key is tried under its own spelling first, so a differently-cased
    // request (`helvetica`, `ARIAL`) still finds an installed original.
    static const char* const sans[] = {"Arial", "Liberation Sans", "Arimo", "Nimbus Sans", "Nimbus Sans L", "FreeSans", nullptr};
    static const char* const serif[] = {"Times New Roman", "Liberation Serif", "Tinos", "Nimbus Roman", "Nimbus Roman No9 L", "FreeSerif", nullptr};
    static const char* const mono[] = {"Courier New", "Liberation Mono", "Cousine", "Nimbus Mono PS", "Nimbus Mono", "FreeMono", nullptr};
    static const char* const calibri[] = {"Carlito", nullptr};
    static const char* const cambria[] = {"Caladea", nullptr};
    static const char* const georgia[] = {"Gelasio", nullptr};
    static const struct {
        const char* family; // real spelling; matched case-insensitively
        const char* const* substitutes;
    } aliases[] = {
        {"Arial", sans}, {"Helvetica", sans}, {"Helvetica Neue", sans},
        {"Times New Roman", serif}, {"Times", serif},
        {"Courier New", mono}, {"Courier", mono},
        {"Calibri", calibri}, {"Cambria", cambria}, {"Georgia", georgia},
    };

    std::string_view input(family_stack);
    while(!input.empty()) {
        auto family = input.substr(0, input.find(','));
        input.remove_prefix(family.length());
        if(!input.empty() && input.front() == ',')
            input.remove_prefix(1);
        strip_leading_and_trailing_spaces(family);
        if(!family.empty() && (family.front() == '\'' || family.front() == '"')) {
            auto quote = family.front();
            family.remove_prefix(1);
            if(!family.empty() && family.back() == quote)
                family.remove_suffix(1);
            strip_leading_and_trailing_spaces(family);
        }
        if(family.empty())
            continue;

        std::string name(family);
        if(auto face = get_font_face_local(name, weight, italic); !face.is_null())
            return face;

        std::string lower(name);
        for(auto& ch : lower)
            ch = utils::to_lower(ch);

        for(const auto& alias : aliases) {
            std::string key(alias.family);
            for(auto& ch : key)
                ch = utils::to_lower(ch);
            if(lower != key)
                continue;
            if(auto face = get_font_face_local(alias.family, weight, italic); !face.is_null())
                return face;
            for(auto substitute = alias.substitutes; *substitute; ++substitute) {
                if(auto face = get_font_face_local(*substitute, weight, italic); !face.is_null())
                    return face;
            }
        }

        // Generic keywords ("sans-serif", ...). Anything else is not a
        // generic name, so this stays empty for it and the walk goes on.
        if(auto face = get_font_face(lower, weight, italic); !face.is_null())
            return face;
    }

    return font_face();
}

font_face_cache* global_font_face_cache()
{
    static font_face_cache cache;
#ifndef NOVA_DISABLE_LOAD_SYSTEM_FONTS
    static const int loaded = cache.load_system();  // once, before the first lookup
    (void)loaded;
#endif
    return &cache;
}

sized_font::sized_font(const font_face& face, float size_)
    : m_face(face), m_size(size_)
{
    if(m_size > 0.f && !m_face.is_null()) {
        const stb::font_design_metrics design = m_face.get()->design_metrics();
        m_scale = m_size / static_cast<float>(design.units_per_em);
        m_ascent = design.ascent * m_scale;
        m_descent = design.descent * m_scale;
        m_line_gap = design.line_gap * m_scale;
    }
}

float sized_font::x_height() const
{
    if(is_null())
        return 0.f;
    const stb::glyph_metrics info = m_face.get()->glyph_info(glyph_of(*m_face.get(), U'x'));
    return (info.y_min - info.y_max) * -m_scale;
}

float sized_font::measure_text(const std::u32string_view& text, const shaping_context& context) const
{
    return shape(text, nullptr, context);
}

float sized_font::shape(const std::u32string_view& text, std::vector<shaped_glyph_t>* glyphs, const shaping_context& context) const
{
    if(glyphs)
        glyphs->clear();
    if(is_null())
        return 0.f;
    std::vector<stb::shaped_glyph> shaped;
    try {
        if(!context.run.empty() && text.data() >= context.run.data() && text.data() + text.size() <= context.run.data() + context.run.size()) {
            // A part of a longer run: libstb shapes [begin, end) with the context of the whole run, and its byte offsets
            // are those of the run's UTF-8.
            const size_t begin = size_t(text.data() - context.run.data());
            const std::string utf8 = stb::utf8_encode(context.run);
            const size_t begin_byte = stb::utf8_encode(context.run.substr(0, begin)).size();
            const size_t end_byte = stb::utf8_encode(context.run.substr(0, begin + text.size())).size();
            shaped = stb::shaping(*m_face.get(), utf8).range(begin_byte, end_byte).ligatures(context.ligatures).run();
        } else {
            shaped = m_face.get()->shape(stb::utf8_encode(text));
        }
    } catch(...) {
        return 0.f;
    }
    float pen = 0.f;
    for(const stb::shaped_glyph& g : shaped) {
        if(glyphs)
            glyphs->push_back({g.glyph, pen + g.x_offset * m_scale, -g.y_offset * m_scale});
        pen += g.x_advance * m_scale;
    }
    return pen;
}

void sized_font::traverse_glyph_path(std::uint32_t glyph, float x, float y, path_traverse_func_t traverse_func, void* closure) const
{
    if(is_null())
        return;
    stb::matrix matrix = stb::matrix::translation(x, y);
    matrix.scale(m_scale, -m_scale);

    stb::point points[3];
    stb::point current_point = {0, 0};
    // ponytail: libstb parses the glyph on every call (the cache this file used to keep is gone); a scratch vector saves
    // the allocation. Upgrade: an outline cache inside stb::font, shared by every caller.
    thread_local std::vector<stb::outline_command> commands;
    m_face.get()->outline(glyph, commands);
    for(const stb::outline_command& cmd : commands) {
        switch(cmd.op) {
        case outline_op::move_to:
            points[0] = cmd.p[0];
            current_point = points[0];
            matrix.map(points, points, 1);
            traverse_func(closure, outline_op::move_to, points, 1);
            break;
        case outline_op::line_to:
            points[0] = cmd.p[0];
            current_point = points[0];
            matrix.map(points, points, 1);
            traverse_func(closure, outline_op::line_to, points, 1);
            break;
        case outline_op::quad_to: // TrueType: raised to a cubic, as the renderer only knows cubics
            points[0].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * cmd.p[1].x;
            points[1].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * cmd.p[1].y;
            points[2] = cmd.p[1];
            current_point = points[2];
            matrix.map(points, points, 3);
            traverse_func(closure, outline_op::cubic_to, points, 3);
            break;
        case outline_op::cubic_to: // CFF
            points[0] = cmd.p[0];
            points[1] = cmd.p[1];
            points[2] = cmd.p[2];
            current_point = points[2];
            matrix.map(points, points, 3);
            traverse_func(closure, outline_op::cubic_to, points, 3);
            break;
        case outline_op::close:
            // Contours were never closed explicitly here (stb_truetype gave none): the fill closes
            // them anyway, and this keeps stroked text identical.
            break;
        }
    }
}

void sized_font::add_glyph_path(std::uint32_t glyph, float x, float y, outline& path_) const
{
    traverse_glyph_path(glyph, x, y, [](void* closure, outline_op command, const stb::point* points, int) {
        outline* path_ = static_cast<outline*>(closure);
        switch(command) {
        case outline_op::move_to:
            path_->move_to(points[0].x, points[0].y);
            break;
        case outline_op::line_to:
            path_->line_to(points[0].x, points[0].y);
            break;
        case outline_op::cubic_to:
            path_->cubic_to(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        default:  // traverse_glyph_path gives only these three
            break;
        }
    }, &path_);
}

std::vector<sized_font::glyph_box> sized_font::glyph_boxes(const std::u32string_view& text) const
{
    std::vector<glyph_box> boxes;
    if(is_null())
        return boxes;

    std::vector<shaped_glyph_t> glyphs;
    shape(text, &glyphs);
    for(const shaped_glyph_t& g : glyphs) {
        const stb::glyph_metrics info = m_face.get()->glyph_info(g.glyph);
        const float x = info.x_min * m_scale, y = info.y_max * -m_scale;
        const float w = (info.x_max - info.x_min) * m_scale, h = (info.y_min - info.y_max) * -m_scale;
        if(w > 0.f || h > 0.f)
            boxes.push_back({g.x, x, x + w, y + g.y, y + h + g.y});
    }
    return boxes;
}

bool add_font_face_from_file(const char* family, bool bold, bool italic, const char* filename)
{
    return global_font_face_cache()->add_font_face(family, bold, italic, font_face(filename));
}

bool add_font_face_from_data(const char* family, bool bold, bool italic, const void* data, size_t length_)
{
    return global_font_face_cache()->add_font_face(family, bold, italic, font_face(data, length_));
}

} // namespace nova::vg
