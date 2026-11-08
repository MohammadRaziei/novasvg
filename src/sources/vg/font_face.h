#pragma once

// Private to src/sources/vg: the font-face functions that are not part of the public C API in
// include/nova/vg/render.h. Implemented in font_face.cpp on top of stb::font (libstb).

#include "render.h"

#include <stdint.h>
#include <cstdint>
#include <vector>

namespace nova {
namespace vg {

// Design units per em (the font's own grid), e.g. 1000 or 2048.
float font_face_get_units_per_em(const font_face_t* face);

// Calls `callback` for every Unicode code point the face has a glyph for, in ascending order.
void font_face_enumerate_codepoints(const font_face_t* face, void (*callback)(uint32_t codepoint, void* closure), void* closure);

// Kerning between two consecutive code points, in pixels at `size` (kern table or GPOS).
float font_face_get_kern_advance(font_face_t* face, float size_, codepoint_t ch1, codepoint_t ch2);

// A text shaped by libstb (font::shape): the glyphs to draw, in visual order, with the pen already placed (kerning,
// ligatures, joined and mirrored letters, marks), in pixels at the font size, y down.
struct shaped_glyph_t {
    std::uint32_t glyph; // glyph index of the font
    float x;             // where the glyph is drawn, from the start of the text
    float y;             // offset from the baseline (mark placement)
};

// Shapes `text`; fills `glyphs` (when not null) and returns the advance of the whole text. UTF-8, UTF-16 or UTF-32.
float font_face_shape(font_face_t* face, float size_, const void* text, int length_, text_encoding_t encoding, std::vector<shaped_glyph_t>* glyphs);

void font_face_get_glyph_index_metrics(font_face_t* face, float size_, std::uint32_t glyph_index, float* advance_width, float* left_side_bearing, rect_t* extents);

// The outline of a glyph by index (what font_face_shape returns), at (x, y); returns its own advance.
float font_face_get_glyph_index_path(font_face_t* face, float size_, float x, float y, std::uint32_t glyph, stb::outline& path_);
float font_face_traverse_glyph_index_path(font_face_t* face, float size_, float x, float y, std::uint32_t glyph, path_traverse_func_t traverse_func, void* closure);

} // namespace vg
} // namespace nova
