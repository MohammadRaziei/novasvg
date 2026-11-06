// Definitions declared in src/sources/vg/font.h.
#include "font.h"
#include "font_face.h"

namespace nova::vg {

FontFace::FontFace(font_face_t* face)
    : m_face(font_face_reference(face))
{
}

FontFace::FontFace(const void* data, size_t length, destroy_func_t destroy_func, void* closure)
    : m_face(font_face_load_from_data(data, length, 0, destroy_func, closure))
{
}

FontFace::FontFace(const char* filename)
    : m_face(font_face_load_from_file(filename, 0))
{
}

FontFace::FontFace(const FontFace& face)
    : m_face(font_face_reference(face.get()))
{
}

FontFace::FontFace(FontFace&& face)
    : m_face(face.release())
{
}

FontFace::~FontFace()
{
    font_face_destroy(m_face);
}

FontFace& FontFace::operator=(const FontFace& face)
{
    FontFace(face).swap(*this);
    return *this;
}

FontFace& FontFace::operator=(FontFace&& face)
{
    FontFace(std::move(face)).swap(*this);
    return *this;
}

void FontFace::swap(FontFace& face)
{
    std::swap(m_face, face.m_face);
}

font_face_t* FontFace::release()
{
    return std::exchange(m_face, nullptr);
}

float FontFace::unitsPerEm() const
{
    if(is_null())
        return 0.f;
    return font_face_get_units_per_em(m_face);
}

float FontFace::ascent_units() const
{
    if(is_null())
        return 0.f;
    float ascent = 0.f;
    font_face_get_metrics(m_face, unitsPerEm(), &ascent, nullptr, nullptr, nullptr);
    return ascent;
}

float FontFace::descent_units() const
{
    if(is_null())
        return 0.f;
    float descent = 0.f;
    font_face_get_metrics(m_face, unitsPerEm(), nullptr, &descent, nullptr, nullptr);
    return descent;
}

float FontFace::advance_width_units(char32_t codepoint) const
{
    if(is_null())
        return 0.f;
    // font_face_get_glyph_metrics() always returns advance_width * scale
    // (see font_face.cpp) -- passing unitsPerEm() as the "size"
    // makes scale == 1, i.e. the raw, size-independent font-unit value,
    // with no separate unscaled code path needed on the render side.
    float advance = 0.f;
    font_face_get_glyph_metrics(m_face, unitsPerEm(), codepoint, &advance, nullptr, nullptr);
    return advance;
}

float FontFace::kern_advance_units(char32_t first, char32_t second) const
{
    if(is_null())
        return 0.f;
    // Same unitsPerEm()-as-size trick as advanceWidthUnits(): makes
    // font_face_get_kern_advance()'s internal scale multiplication a
    // no-op, yielding the raw, size-independent value.
    return font_face_get_kern_advance(m_face, unitsPerEm(), first, second);
}

std::vector<char32_t> FontFace::codepoints() const
{
    std::vector<char32_t> result;
    if(is_null())
        return result;
    font_face_enumerate_codepoints(m_face, [](uint32_t codepoint, void* closure) {
        static_cast<std::vector<char32_t>*>(closure)->push_back(static_cast<char32_t>(codepoint));
    }, &result);
    return result;
}

bool FontFaceCache::add_font_face(const std::string& family, bool bold, bool italic, const FontFace& face)
{
    if(!face.is_null())
        font_face_cache_add(m_cache, family.data(), bold, italic, face.get());
    return !face.is_null();
}

FontFace FontFaceCache::get_font_face_local(const std::string& family, bool bold, bool italic) const
{
    return FontFace(font_face_cache_get(m_cache, family.data(), bold, italic));
}

FontFace FontFaceCache::get_font_face(const std::string& family, bool bold, bool italic) const
{
    if(auto face = get_font_face_local(family, bold, italic); !face.is_null()) {
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
            if(auto face = FontFace(font_face_cache_get(m_cache, value.fallback, bold, italic)); !face.is_null())
                return face;
            // Not even the platform's stand-in is installed: use the embedded font
            // so text is still drawn (and drawn the same way) on a machine without fonts.
            return embedded_fallback_font(bold, italic);
        }
    }

    return FontFace();
}

FontFace FontFaceCache::get_font_face_for_family_stack(const std::string& familyStack, bool bold, bool italic) const
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

    std::string_view input(familyStack);
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
        if(auto face = get_font_face_local(name, bold, italic); !face.is_null())
            return face;

        std::string lower(name);
        for(auto& ch : lower)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        for(const auto& alias : aliases) {
            std::string key(alias.family);
            for(auto& ch : key)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if(lower != key)
                continue;
            if(auto face = get_font_face_local(alias.family, bold, italic); !face.is_null())
                return face;
            for(auto substitute = alias.substitutes; *substitute; ++substitute) {
                if(auto face = get_font_face_local(*substitute, bold, italic); !face.is_null())
                    return face;
            }
        }

        // Generic keywords ("sans-serif", ...). Anything else is not a
        // generic name, so this stays empty for it and the walk goes on.
        if(auto face = get_font_face(lower, bold, italic); !face.is_null())
            return face;
    }

    return FontFace();
}

FontFaceCache::FontFaceCache()
    : m_cache(font_face_cache_create())
{
#ifndef NOVA_DISABLE_LOAD_SYSTEM_FONTS
    font_face_cache_load_sys(m_cache);
#endif
}

FontFaceCache* fontFaceCache()
{
    static FontFaceCache cache;
    return &cache;
}

Font::Font(const FontFace& face, float size)
    : m_face(face), m_size(size)
{
    if(m_size > 0.f && !m_face.is_null()) {
        font_face_get_metrics(m_face.get(), m_size, &m_ascent, &m_descent, &m_line_gap, nullptr);
    }
}

float Font::x_height() const
{
    rect_t extents = {0};
    if(m_size > 0.f && !m_face.is_null())
        font_face_get_glyph_metrics(m_face.get(), m_size, 'x', nullptr, nullptr, &extents);
    return extents.h;
}

float Font::measure_text(const std::u32string_view& text) const
{
    if(m_size > 0.f && !m_face.is_null())
        return font_face_text_extents(m_face.get(), m_size, text.data(), text.length(), NOVA_TEXT_ENCODING_UTF32, nullptr);
    return 0;
}

std::vector<Font::glyph_box> Font::glyph_boxes(const std::u32string_view& text) const
{
    std::vector<glyph_box> boxes;
    if(m_size <= 0.f || m_face.is_null())
        return boxes;

    font_face_t* face = m_face.get();
    text_iterator_t it;
    text_iterator_init(&it, text.data(), static_cast<int>(text.length()), NOVA_TEXT_ENCODING_UTF32);
    float pen = 0.f;
    codepoint_t previous = 0;
    bool hasPrevious = false;
    while(text_iterator_has_next(&it)) {
        codepoint_t codepoint = text_iterator_next(&it);
        codepoint = font_face_apply_ligature(face, &it, codepoint);
        if(hasPrevious)
            pen += font_face_get_kern_advance(face, m_size, previous, codepoint);
        previous = codepoint;
        hasPrevious = true;

        float advance = 0.f;
        rect_t extents = {0};
        font_face_get_glyph_metrics(face, m_size, codepoint, &advance, nullptr, &extents);
        if(extents.w > 0.f || extents.h > 0.f)
            boxes.push_back({pen, extents.x, extents.x + extents.w, extents.y, extents.y + extents.h});
        pen += advance;
    }
    return boxes;
}

bool add_font_face_from_file(const char* family, bool bold, bool italic, const char* filename)
{
    return fontFaceCache()->add_font_face(family, bold, italic, FontFace(filename));
}

bool add_font_face_from_data(const char* family, bool bold, bool italic, const void* data, size_t length, destroy_func_t destroy_func, void* closure)
{
    return fontFaceCache()->add_font_face(family, bold, italic, FontFace(data, length, destroy_func, closure));
}

} // namespace nova::vg
