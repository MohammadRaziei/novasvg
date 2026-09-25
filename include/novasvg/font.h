#pragma once

// Font: TrueType font loading, face caching, and text measurement.
// Self-contained -- doesn't need Bitmap, Canvas, or the SVG layer -- so it
// can be used entirely on its own (e.g. to measure how wide some text
// would come out in a given font) without pulling in the rest of novasvg.

#include "detail/config.h"
#include "detail/svgparserutils.h" // stripLeadingAndTrailingSpaces() -- generic string utils only, no SVG-tree dependency, consistent with this header's own self-contained goal
#include "detail/render/path.h"    // path_move_to/line_to/cubic_to, used to extract glyph outlines
#include "detail/render/font.h"

#include <string>
#include <string_view>
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

    // Real OS font substitution for an entire CSS-style comma-separated
    // family stack (e.g. `"trebuchet ms", verdana, arial, sans-serif`),
    // used only after getFontFace() has already failed for every name in
    // the stack individually (see SVGLayoutState::font() -- that's what
    // builds `familyStack` in the first place). One fontconfig call
    // covering the whole stack, not one call per name: fontconfig's own
    // FcFontMatch always returns *some* match (its job is to never come
    // up empty), so probing it name-by-name would "succeed" on the very
    // first name with fontconfig's generic default substitution and
    // never reach a later, better-aliased name (verified: this is
    // exactly what happened when this was first tried per-name here).
    // Handing fontconfig the complete ordered stack in one pattern lets
    // its own substitution logic pick the best-aliased entry instead.
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
    std::vector<std::string> names;
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
        if(!family.empty())
            names.emplace_back(family);
    }

    if(names.empty())
        return FontFace();

    std::vector<const char*> cNames;
    cNames.reserve(names.size());
    for(auto& name : names)
        cNames.push_back(name.c_str());

    return FontFace(font_face_cache_match_fontconfig_stack(m_cache, cNames.data(), int(cNames.size()), bold, italic));
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
NOVASVG_INLINE bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(filename));
}

NOVASVG_INLINE bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, destroy_func_t destroy_func, void* closure)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(data, length, destroy_func, closure));
}

} // namespace novasvg
