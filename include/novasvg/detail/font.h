#pragma once

// Font: TrueType font loading, face caching, and text measurement.
// Self-contained -- doesn't need Bitmap, Canvas, or the SVG layer -- so it
// can be used entirely on its own (e.g. to measure how wide some text
// would come out in a given font) without pulling in the rest of novasvg.

#include "svgparserutils.h" // stripLeadingAndTrailingSpaces() -- generic string utils only, no SVG-tree dependency, consistent with this header's own self-contained goal
#include "render/path.h"    // path_move_to/line_to/cubic_to, used to extract glyph outlines
#include "render/font.h"

#include <string>
#include <string_view>
#include <cctype>
#include <cstddef>
#include <utility>
#include <vector>

namespace novasvg {
using namespace render;

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

    // Raw font design units (unscaled by any particular pixel size) -- the
    // same space a font file's own tables (head.unitsPerEm, hmtx advance
    // widths) are defined in. Together these let a caller build a
    // size-independent advance table once and rescale it later (by
    // size / unitsPerEm) without coming back through novasvg per size --
    // e.g. handing the whole table to a JS engine that can't make
    // synchronous calls back into Python/C++ (see mermaidx's v8_engine,
    // which needs exactly this to replace its own hand-rolled TTF-table
    // parser). Font::measureText()/ascent()/etc. remain the right choice
    // whenever a single already-known pixel size is enough.
    float unitsPerEm() const;
    float ascentUnits() const;
    float descentUnits() const;
    float advanceWidthUnits(char32_t codepoint) const;

    // Kerning adjustment between two consecutive codepoints, in raw font
    // design units (unscaled) -- the same "kern"/GPOS lookup
    // Font::measureText() itself applies between every glyph pair (see
    // font_face_text_extents() in detail/render/font.h), exposed per-pair
    // so a caller that (unlike Font::measureText()) can't call back into
    // novasvg per string -- e.g. mermaidx's V8 engine, which sums a
    // shipped per-codepoint advance table in JS instead -- can still fold
    // kerning into that sum for whichever pairs it queries up front.
    // Deliberately NOT a bulk enumeration like codepoints(): unlike the
    // simple, single-format cmap table codepoints() walks, kerning may
    // live in either a legacy "kern" table or an OpenType GPOS Pair
    // Adjustment Positioning lookup (format 1, a per-pair list, or format
    // 2, a glyph-class matrix) -- stb_truetype's own bulk accessors
    // (stbtt_GetKerningTable()) only cover the "kern" table (see that
    // function's own comment), not GPOS, so a from-scratch GPOS enumerator
    // would be a second, substantially larger parser to maintain
    // alongside stb_truetype's -- while this per-pair query reuses
    // stb_truetype's own GPOS-aware lookup path exactly as-is.
    float kernAdvanceUnits(char32_t first, char32_t second) const;

    // The advance width used for any codepoint outside the font's cmap
    // (glyph id 0, the ".notdef" glyph) -- what advanceWidthUnits() itself
    // already falls back to for such a codepoint (stbtt_FindGlyphIndex()
    // returns glyph 0 when nothing matches), exposed under its own name
    // purely so callers building a fallback value don't have to know that.
    float notdefAdvanceWidthUnits() const { return advanceWidthUnits(0); }

    // Every Unicode codepoint this face's cmap maps to a glyph (see
    // font_face_enumerate_codepoints() for exactly which cmap formats are
    // covered). Pair with advanceWidthUnits()/unitsPerEm() to build a
    // complete, size-independent advance table -- e.g. for a caller that
    // needs to reproduce Font::measureText() without being able to call
    // back into novasvg per string (see mermaidx's v8_engine, which ships
    // such a table into a V8 isolate once at boot).
    std::vector<char32_t> codepoints() const;

private:
    font_face_t* release();
    font_face_t* m_face = nullptr;
};

class FontFaceCache {
public:
    bool addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face);

    // Local-only lookup: just this cache's literal-name scan (system
    // fonts by their own internal name, plus anything explicitly
    // registered via addFontFace() -- including an @font-face-embedded
    // font under the exact family name its CSS declared). No generic
    // "sans-serif"-style fallback table and no OS substitution -- used
    // by SVGLayoutState::font()'s per-name stack loop so an embedded
    // font is found (and wins) before falling through to guessing at a
    // substitute for names later in the stack.
    FontFace getFontFaceLocal(const std::string& family, bool bold, bool italic) const;

    // getFontFaceLocal(), then the hardcoded generic-family table
    // ("sans-serif" -> "DejaVu Sans" and similar) as a last resort.
    FontFace getFontFace(const std::string& family, bool bold, bool italic) const;

    // Resolves a CSS-style comma-separated family stack (e.g.
    // `"trebuchet ms", verdana, arial, sans-serif`) the way a browser walks
    // it: name by name, in order, and the first name that yields a face
    // wins. For each name: a font registered or installed under exactly
    // that name, then a metric-compatible substitute from novasvg's own
    // table (`arial` -> Liberation Sans / Arimo / ..., matched
    // case-insensitively), then the generic keywords ("sans-serif", ...).
    // A name nothing matches is skipped, not guessed at; returns a null
    // face when the whole stack is unresolved.
    FontFace getFontFaceForFamilyStack(const std::string& familyStack, bool bold, bool italic) const;

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

    // One entry per glyph that has an outline (spaces and other empty glyphs
    // are left out): where its pen starts and where its ink begins/ends, in
    // pixels. inkLeft/inkRight are along the baseline relative to the glyph's
    // own pen; inkTop/inkBottom are relative to the baseline with y pointing
    // DOWN (so ink above the baseline is negative). Walks the text exactly
    // like measureText() (ligatures, then kerning), so `pen` is where the
    // glyph is painted. These are the raw outline extents -- no pixel
    // rounding; that is a consumer's (browser-emulation) decision, not the
    // font engine's.
    struct GlyphBox {
        float pen;
        float inkLeft;
        float inkRight;
        float inkTop;
        float inkBottom;
    };
    std::vector<GlyphBox> glyphBoxes(const std::u32string_view& text) const;

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

/**
* @brief Add a font face from a file to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param filename The path to the font file.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename);

/**
* @brief Add a font face from memory to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param data A pointer to the memory buffer containing the font data.
* @param length The size of the memory buffer in bytes.
* @param destroy_func Callback function to free the memory buffer when it is no longer needed.
* @param closure User-defined pointer passed to the `destroy_func` callback.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, novasvg_destroy_func_t destroy_func, void* closure);

} // namespace novasvg

namespace novasvg {

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

NOVASVG_INLINE float FontFace::unitsPerEm() const
{
    if(isNull())
        return 0.f;
    return font_face_get_units_per_em(m_face);
}

NOVASVG_INLINE float FontFace::ascentUnits() const
{
    if(isNull())
        return 0.f;
    float ascent = 0.f;
    font_face_get_metrics(m_face, unitsPerEm(), &ascent, nullptr, nullptr, nullptr);
    return ascent;
}

NOVASVG_INLINE float FontFace::descentUnits() const
{
    if(isNull())
        return 0.f;
    float descent = 0.f;
    font_face_get_metrics(m_face, unitsPerEm(), nullptr, &descent, nullptr, nullptr);
    return descent;
}

NOVASVG_INLINE float FontFace::advanceWidthUnits(char32_t codepoint) const
{
    if(isNull())
        return 0.f;
    // font_face_get_glyph_metrics() always returns advance_width * scale
    // (see detail/render/font.h) -- passing unitsPerEm() as the "size"
    // makes scale == 1, i.e. the raw, size-independent font-unit value,
    // with no separate unscaled code path needed on the render side.
    float advance = 0.f;
    font_face_get_glyph_metrics(m_face, unitsPerEm(), codepoint, &advance, nullptr, nullptr);
    return advance;
}

NOVASVG_INLINE float FontFace::kernAdvanceUnits(char32_t first, char32_t second) const
{
    if(isNull())
        return 0.f;
    // Same unitsPerEm()-as-size trick as advanceWidthUnits(): makes
    // font_face_get_kern_advance()'s internal scale multiplication a
    // no-op, yielding the raw, size-independent value.
    return font_face_get_kern_advance(m_face, unitsPerEm(), first, second);
}

NOVASVG_INLINE std::vector<char32_t> FontFace::codepoints() const
{
    std::vector<char32_t> result;
    if(isNull())
        return result;
    font_face_enumerate_codepoints(m_face, [](uint32_t codepoint, void* closure) {
        static_cast<std::vector<char32_t>*>(closure)->push_back(static_cast<char32_t>(codepoint));
    }, &result);
    return result;
}

NOVASVG_INLINE bool FontFaceCache::addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face)
{
    if(!face.isNull())
        font_face_cache_add(m_cache, family.data(), bold, italic, face.get());
    return !face.isNull();
}

NOVASVG_INLINE FontFace FontFaceCache::getFontFaceLocal(const std::string& family, bool bold, bool italic) const
{
    return FontFace(font_face_cache_get(m_cache, family.data(), bold, italic));
}

NOVASVG_INLINE FontFace FontFaceCache::getFontFace(const std::string& family, bool bold, bool italic) const
{
    if(auto face = getFontFaceLocal(family, bold, italic); !face.isNull()) {
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
            return FontFace(font_face_cache_get(m_cache, value.fallback, bold, italic));
        }
    }

    return FontFace();
}

NOVASVG_INLINE FontFace FontFaceCache::getFontFaceForFamilyStack(const std::string& familyStack, bool bold, bool italic) const
{
    // Metric-compatible stand-ins only: a substitute must lay text out the
    // same way as the font it replaces, so a name with no such stand-in
    // (verdana, trebuchet ms, ...) is skipped and the stack moves on. Each
    // list starts with the family's real name, so a differently-cased
    // request still finds an installed original.
    static const char* const sans[] = {"Arial", "Liberation Sans", "Arimo", "Nimbus Sans", "Nimbus Sans L", "FreeSans", nullptr};
    static const char* const serif[] = {"Times New Roman", "Liberation Serif", "Tinos", "Nimbus Roman", "Nimbus Roman No9 L", "FreeSerif", nullptr};
    static const char* const mono[] = {"Courier New", "Liberation Mono", "Cousine", "Nimbus Mono PS", "Nimbus Mono", "FreeMono", nullptr};
    static const char* const calibri[] = {"Calibri", "Carlito", nullptr};
    static const char* const cambria[] = {"Cambria", "Caladea", nullptr};
    static const char* const georgia[] = {"Georgia", "Gelasio", nullptr};
    static const struct {
        const char* family; // lower case
        const char* const* substitutes;
    } aliases[] = {
        {"arial", sans}, {"helvetica", sans}, {"helvetica neue", sans},
        {"times new roman", serif}, {"times", serif},
        {"courier new", mono}, {"courier", mono},
        {"calibri", calibri}, {"cambria", cambria}, {"georgia", georgia},
    };

    std::string_view input(familyStack);
    while(!input.empty()) {
        auto family = input.substr(0, input.find(','));
        input.remove_prefix(family.length());
        if(!input.empty() && input.front() == ',')
            input.remove_prefix(1);
        stripLeadingAndTrailingSpaces(family);
        if(!family.empty() && (family.front() == '\'' || family.front() == '"')) {
            auto quote = family.front();
            family.remove_prefix(1);
            if(!family.empty() && family.back() == quote)
                family.remove_suffix(1);
            stripLeadingAndTrailingSpaces(family);
        }
        if(family.empty())
            continue;

        std::string name(family);
        if(auto face = getFontFaceLocal(name, bold, italic); !face.isNull())
            return face;

        std::string lower(name);
        for(auto& ch : lower)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        for(const auto& alias : aliases) {
            if(lower != alias.family)
                continue;
            for(auto substitute = alias.substitutes; *substitute; ++substitute) {
                if(auto face = getFontFaceLocal(*substitute, bold, italic); !face.isNull())
                    return face;
            }
        }

        // Generic keywords ("sans-serif", ...). Anything else is not a
        // generic name, so this stays empty for it and the walk goes on.
        if(auto face = getFontFace(lower, bold, italic); !face.isNull())
            return face;
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
NOVASVG_INLINE std::vector<Font::GlyphBox> Font::glyphBoxes(const std::u32string_view& text) const
{
    std::vector<GlyphBox> boxes;
    if(m_size <= 0.f || m_face.isNull())
        return boxes;

    font_face_t* face = m_face.get();
    text_iterator_t it;
    text_iterator_init(&it, text.data(), static_cast<int>(text.length()), NOVASVG_TEXT_ENCODING_UTF32);
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

NOVASVG_INLINE bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(filename));
}

NOVASVG_INLINE bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, destroy_func_t destroy_func, void* closure)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(data, length, destroy_func, closure));
}

} // namespace novasvg
