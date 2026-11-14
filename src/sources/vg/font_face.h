#pragma once

// Private to src/sources/vg: what novasvg keeps per font on top of stb::font (libstb): the design metrics, a cache of
// glyph outlines and the shaping glue. Shared through font_face (a shared_ptr), never by hand.

#include "render.h"

#include <stb/font.hpp>

#include <stdint.h>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nova {
namespace vg {

// A text shaped by libstb (font::shape): the glyphs to draw, in visual order, with the pen already placed (kerning,
// ligatures, joined and mirrored letters, marks), in pixels at the font size, y down.
struct shaped_glyph_t {
    std::uint32_t glyph; // glyph index of the font
    float x;             // where the glyph is drawn, from the start of the text
    float y;             // offset from the baseline (mark placement)
};

// What is cached per code point: its glyph index, horizontal metrics, ink box and outline, all in
// font units. Entries are never removed while the face lives, so pointers stay valid.
struct glyph_entry {
    uint32_t index = 0;
    float advance_width = 0;
    float left_side_bearing = 0;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::vector<stb::outline_command> outline;
};

class font_face_data {
public:
    explicit font_face_data(stb::font f);
    font_face_data(const font_face_data&) = delete;
    font_face_data& operator=(const font_face_data&) = delete;

    // Design units per em (the font's own grid), e.g. 1000 or 2048.
    float units_per_em() const;

    // Every Unicode code point the face has a glyph for, in ascending order.
    std::vector<char32_t> codepoints() const;

    void get_metrics(float size_, float* ascent, float* descent, float* line_gap, rect_t* extents) const;
    void get_glyph_metrics(float size_, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents) const;
    void get_glyph_index_metrics(float size_, std::uint32_t glyph_index, float* advance_width, float* left_side_bearing, rect_t* extents) const;

    // Kerning between two consecutive code points, in pixels at `size_` (kern table or GPOS).
    float get_kern_advance(float size_, codepoint_t ch1, codepoint_t ch2) const;

    // The outline of a glyph (an index, as shape() returns) at (x, y); returns its own advance.
    float get_glyph_index_path(float size_, float x, float y, std::uint32_t glyph, outline& path_) const;
    float traverse_glyph_index_path(float size_, float x, float y, std::uint32_t glyph_index, path_traverse_func_t traverse_func, void* closure) const;

    // Shapes `text`; fills `glyphs` (when not null) and returns the advance of the whole text. With a `context` the
    // text (a view into context.run) is shaped as a part of that run, see shaping_context.
    float shape(float size_, std::u32string_view text, std::vector<shaped_glyph_t>* glyphs, const shaping_context& context = {}) const;

    // The advance of `text`, and (when not null) its bounding box.
    float text_extents(float size_, std::u32string_view text, rect_t* extents, const shaping_context& context = {}) const;

private:
    uint32_t glyph_index_of(codepoint_t codepoint) const;
    glyph_entry* glyph_by_index(uint32_t index) const;
    glyph_entry* glyph(codepoint_t codepoint) const;
    float size_scale(float size_) const;

    stb::font ft;
    stb::font_design_metrics design;
    mutable std::mutex mutex;
    mutable std::unordered_map<codepoint_t, std::unique_ptr<glyph_entry>> cache;
};

} // namespace vg
} // namespace nova
