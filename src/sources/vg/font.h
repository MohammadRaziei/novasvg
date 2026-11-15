#pragma once

// Font: TrueType font loading, face caching, and text measurement.
// Self-contained -- doesn't need pixmap, raster_canvas, or the svg layer -- so it
// can be used entirely on its own (e.g. to measure how wide some text
// would come out in a given font) without pulling in the rest of novasvg.

#include "parserutils.h" // stripLeadingAndTrailingSpaces() -- generic string utils only, no svg-tree dependency, consistent with this header's own self-contained goal
#include "render.h"  // path_move_to/line_to/cubic_to, used to extract glyph outlines
#include <stb/font.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <cctype>
#include <cstddef>
#include <utility>
#include <vector>

namespace nova::vg {

// A text shaped by libstb (font::shape): the glyphs to draw, in visual order, with the pen already placed (kerning,
// ligatures, joined and mirrored letters, marks), in pixels at the font size, y down.
struct shaped_glyph_t {
    std::uint32_t glyph; // glyph index of the font
    float x;             // where the glyph is drawn, from the start of the text
    float y;             // offset from the baseline (mark placement)
};

// A font as libstb reads it, shared between every holder (a copy of a font_face is a pointer copy; two copies are the
// same face when get() is equal). Null when there is no font.
class font_face {
public:
    font_face() = default;
    explicit font_face(stb::font font) : m_font(std::make_shared<const stb::font>(std::move(font))) {}
    /// Copies the bytes of a font file (TrueType, OpenType or WOFF 1.0); a null face when they are not one libstb reads.
    font_face(const void* data, size_t length_);
    explicit font_face(std::vector<uint8_t>&& bytes) : font_face(from_memory(std::move(bytes))) {}
    font_face(const char* filename) : font_face(from_file(filename)) {}
    /// A null face (and a logged warning) when the bytes or the file are not a font libstb reads; `ttcindex` picks the
    /// face of a collection.
    static font_face from_memory(std::vector<uint8_t> bytes, int ttcindex = 0);
    static font_face from_file(const char* filename, int ttcindex = 0);

    bool is_null() const { return m_font == nullptr; }
    const stb::font* get() const { return m_font.get(); }

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
    float units_per_em() const;
    float ascent_units() const;
    float descent_units() const;
    float advance_width_units(char32_t codepoint) const;

    // Kerning adjustment between two consecutive codepoints, in raw font
    // design units (unscaled) -- the same "kern"/GPOS lookup
    // Font::measureText() itself applies between every glyph pair (see
    // sized_font::measure_text() in src/sources/vg/font.cpp), exposed per-pair
    // so a caller that (unlike Font::measureText()) can't call back into
    // novasvg per string -- e.g. mermaidx's V8 engine, which sums a
    // shipped per-codepoint advance table in JS instead -- can still fold
    // kerning into that sum for whichever pairs it queries up front.
    // Deliberately NOT a bulk enumeration like codepoints(): kerning may live
    // in either a legacy "kern" table or an OpenType GPOS pair-adjustment
    // lookup, which libstb (stb::font::glyph_kerning) resolves per pair.
    float kern_advance_units(char32_t first, char32_t second) const;

    // The advance width used for any codepoint outside the font's cmap
    // (glyph id 0, the ".notdef" glyph) -- what advanceWidthUnits() itself
    // already falls back to for such a codepoint (the glyph index is 0
    // when the cmap has nothing for it), exposed under its own name
    // purely so callers building a fallback value don't have to know that.
    float notdef_advance_width_units() const { return advance_width_units(0); }

    // Every Unicode codepoint this face's cmap maps to a glyph (see
    // font_face_enumerate_codepoints() for exactly which cmap formats are
    // covered). Pair with advanceWidthUnits()/unitsPerEm() to build a
    // complete, size-independent advance table -- e.g. for a caller that
    // needs to reproduce Font::measureText() without being able to call
    // back into novasvg per string (see mermaidx's v8_engine, which ships
    // such a table into a V8 isolate once at boot).
    std::vector<char32_t> codepoints() const;

private:
    std::shared_ptr<const stb::font> m_font;
};

// The last-resort fonts: Latin subsets of DejaVu Sans (regular, bold, oblique and bold oblique)
// compiled into the library (see default_font.h); the face matching `bold` / `italic` is returned.
// A null face when built with NOVA_DISABLE_EMBEDDED_FONT.
// Defined in svgparser.hpp, next to the base64 decoding it shares with @font-face.
font_face embedded_fallback_font(bool bold = false, bool italic = false);

class font_face_cache {
public:
    bool add_font_face(const std::string& family, bool bold, bool italic, const font_face& face);

    // Local-only lookup: just this cache's literal-name scan (system
    // fonts by their own internal name, plus anything explicitly
    // registered via addFontFace() -- including an @font-face-embedded
    // font under the exact family name its CSS declared). No generic
    // "sans-serif"-style fallback table and no OS substitution -- used
    // by svg_layout_state::font()'s per-name stack loop so an embedded
    // font is found (and wins) before falling through to guessing at a
    // substitute for names later in the stack.
    font_face get_font_face_local(const std::string& family, bool bold, bool italic) const;

    // getFontFaceLocal(), then the hardcoded generic-family table
    // ("sans-serif" -> "DejaVu Sans" and similar) as a last resort.
    font_face get_font_face(const std::string& family, bool bold, bool italic) const;

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
    font_face get_font_face_for_family_stack(const std::string& family_stack, bool bold, bool italic) const;

private:
    font_face_cache();
    font_face_cache_t* m_cache;
    friend font_face_cache* global_font_face_cache();
};

font_face_cache* global_font_face_cache();

class sized_font {
public:
    sized_font() = default;
    sized_font(const font_face& face, float size_);

    float ascent() const { return m_ascent; }
    float descent() const { return m_descent; }
    float height() const { return m_ascent - m_descent; }
    float line_gap() const { return m_line_gap; }
    float x_height() const;

    // `context`: shape `text` (a view into context.run) as a part of its run, see shaping_context.
    float measure_text(const std::u32string_view& text, const shaping_context& context = {}) const;

    // Shapes `text`: fills `glyphs` (when not null) and returns the advance of the whole text.
    float shape(const std::u32string_view& text, std::vector<shaped_glyph_t>* glyphs, const shaping_context& context = {}) const;

    // The outline of a glyph (an index, as shape() returns it) with its pen at (x, y): quadratics are raised to cubics,
    // contours are left to the fill to close.
    void traverse_glyph_path(std::uint32_t glyph, float x, float y, path_traverse_func_t traverse_func, void* closure) const;
    void add_glyph_path(std::uint32_t glyph, float x, float y, outline& path_) const;

    // One entry per glyph that has an outline (spaces and other empty glyphs
    // are left out): where its pen starts and where its ink begins/ends, in
    // pixels. inkLeft/inkRight are along the baseline relative to the glyph's
    // own pen; inkTop/inkBottom are relative to the baseline with y pointing
    // DOWN (so ink above the baseline is negative). Walks the text exactly
    // like measure_text() (shaped by libstb), so `pen` is where the
    // glyph is painted. These are the raw outline extents -- no pixel
    // rounding; that is a consumer's (browser-emulation) decision, not the
    // font engine's.
    struct glyph_box {
        float pen;
        float ink_left;
        float ink_right;
        float ink_top;
        float ink_bottom;
    };
    std::vector<glyph_box> glyph_boxes(const std::u32string_view& text) const;

    const font_face& face() const { return m_face; }
    float size() const { return m_size; }

    bool is_null() const { return m_size <= 0.f || m_face.is_null(); }

private:
    font_face m_face;
    float m_size = 0.f;
    float m_scale = 0.f;   // pixels per design unit
    float m_ascent = 0.f;
    float m_descent = 0.f;
    float m_line_gap = 0.f;
};

/**
* @brief Add a font face from a file to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param filename The path to the font file.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool add_font_face_from_file(const char* family, bool bold, bool italic, const char* filename);

/**
* @brief Add a font face from memory to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param data A pointer to the memory buffer containing the font data; it is copied.
* @param length The size of the memory buffer in bytes.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool add_font_face_from_data(const char* family, bool bold, bool italic, const void* data, size_t length_);

} // namespace nova::vg
