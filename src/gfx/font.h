#pragma once

// Font: TrueType font loading, face caching, and text measurement.
// Self-contained -- doesn't need Bitmap, raster_canvas, or the svg layer -- so it
// can be used entirely on its own (e.g. to measure how wide some text
// would come out in a given font) without pulling in the rest of novasvg.

#include "novasvg/gfx/parserutils.h" // stripLeadingAndTrailingSpaces() -- generic string utils only, no svg-tree dependency, consistent with this header's own self-contained goal
#include "novasvg/gfx/render.h"  // path_move_to/line_to/cubic_to, used to extract glyph outlines

#include <string>
#include <string_view>
#include <cctype>
#include <cstddef>
#include <utility>
#include <vector>

namespace nova::vg {

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

    bool is_null() const { return m_face == nullptr; }
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
    // font_face_text_extents() in src/gfx/font_face.cpp), exposed per-pair
    // so a caller that (unlike Font::measureText()) can't call back into
    // novasvg per string -- e.g. mermaidx's V8 engine, which sums a
    // shipped per-codepoint advance table in JS instead -- can still fold
    // kerning into that sum for whichever pairs it queries up front.
    // Deliberately NOT a bulk enumeration like codepoints(): kerning may live
    // in either a legacy "kern" table or an OpenType GPOS pair-adjustment
    // lookup, which libstb (stb::font::glyph_kerning) resolves per pair.
    float kernAdvanceUnits(char32_t first, char32_t second) const;

    // The advance width used for any codepoint outside the font's cmap
    // (glyph id 0, the ".notdef" glyph) -- what advanceWidthUnits() itself
    // already falls back to for such a codepoint (the glyph index is 0
    // when the cmap has nothing for it), exposed under its own name
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

// The last-resort fonts: Latin subsets of DejaVu Sans (regular, bold, oblique and bold oblique)
// compiled into the library (see default_font.h); the face matching `bold` / `italic` is returned.
// A null face when built with NOVA_DISABLE_EMBEDDED_FONT.
// Defined in svgparser.hpp, next to the base64 decoding it shares with @font-face.
FontFace embeddedFallbackFont(bool bold = false, bool italic = false);

class FontFaceCache {
public:
    bool addFontFace(const std::string& family, bool bold, bool italic, const FontFace& face);

    // Local-only lookup: just this cache's literal-name scan (system
    // fonts by their own internal name, plus anything explicitly
    // registered via addFontFace() -- including an @font-face-embedded
    // font under the exact family name its CSS declared). No generic
    // "sans-serif"-style fallback table and no OS substitution -- used
    // by svg_layout_state::font()'s per-name stack loop so an embedded
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
    // face when the whole stack is unresolved. A generic keyword nothing on
    // the machine satisfies resolves to the embedded fallback font.
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

    bool is_null() const { return m_size <= 0.f || m_face.is_null(); }

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

} // namespace nova::vg

namespace nova::vg {






















FontFaceCache* fontFaceCache();




bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename);

bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, destroy_func_t destroy_func, void* closure);

} // namespace nova::vg
