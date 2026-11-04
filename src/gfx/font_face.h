#pragma once

// Private to src/gfx: the font-face functions that are not part of the public C API in
// include/novasvg/gfx/render.h. Implemented in font_face.cpp on top of stb::font (libstb).

#include "novasvg/gfx/render.h"

#include <stdint.h>

namespace novasvg {
namespace gfx {

// Design units per em (the font's own grid), e.g. 1000 or 2048.
float font_face_get_units_per_em(const font_face_t* face);

// Calls `callback` for every Unicode code point the face has a glyph for, in ascending order.
void font_face_enumerate_codepoints(const font_face_t* face, void (*callback)(uint32_t codepoint, void* closure), void* closure);

// Kerning between two consecutive code points, in pixels at `size` (kern table or GPOS).
float font_face_get_kern_advance(font_face_t* face, float size, codepoint_t ch1, codepoint_t ch2);

// `codepoint` was just read from `it`. If it starts an f-ligature the font applies (ffi, ffl, ff,
// fi, fl) the rest of the sequence is consumed from `it` and the ligature's code point returned.
codepoint_t font_face_apply_ligature(font_face_t* face, text_iterator_t* it, codepoint_t codepoint);

} // namespace gfx
} // namespace novasvg
