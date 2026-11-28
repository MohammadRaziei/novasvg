// The embedded fallback fonts: byte arrays that cmake/EmbedFile.cmake generates (nova_embedded_fonts.hpp), declared in
// src/sources/vg/font.h.
#include "font.h"

#include <stb/zlib.hpp>

#include "render.h"

#ifndef NOVA_DISABLE_EMBEDDED_FONT
#include "nova_embedded_fonts.hpp"
#endif

#include <cstddef>
#include <exception>

namespace nova::vg {

#ifndef NOVA_DISABLE_EMBEDDED_FONT
namespace {
// A face from an embedded array: the zlib stream of the TrueType bytes (tools/gen_default_font.py). The inflater is
// libstb's (stb::zlib_decompress), which reports bad input by exception: that becomes an empty font_face.
font_face font_face_from_embedded(const unsigned char* data, std::size_t size)
{
    try {
        return font_face(stb::zlib_decompress(data, size));
    } catch(const std::exception&) {
        return font_face();
    }
}
}  // namespace
#endif

font_face embedded_fallback_font(bool bold, bool italic, fallback_family family)
{
#ifdef NOVA_DISABLE_EMBEDDED_FONT
    (void)bold;
    (void)italic;
    (void)family;
    return font_face();
#else
    // Each face is built on first use only: a machine with fonts never pays for the decode, and a program that
    // never asks for bold never decodes the bold face.
    using namespace detail;
    // Mono and Serif have no oblique: an italic request gets the upright face.
    if(family == fallback_family::mono) {
        if(bold) {
            static const font_face face = font_face_from_embedded(embedded_font_mono_bold, embedded_font_mono_bold_size);
            return face;
        }
        static const font_face face = font_face_from_embedded(embedded_font_mono, embedded_font_mono_size);
        return face;
    }
    if(family == fallback_family::serif) {
        if(bold) {
            static const font_face face = font_face_from_embedded(embedded_font_serif_bold, embedded_font_serif_bold_size);
            return face;
        }
        static const font_face face = font_face_from_embedded(embedded_font_serif, embedded_font_serif_size);
        return face;
    }
    if(bold && italic) {
        static const font_face face = font_face_from_embedded(embedded_font_bold_italic, embedded_font_bold_italic_size);
        return face;
    }
    if(bold) {
        static const font_face face = font_face_from_embedded(embedded_font_bold, embedded_font_bold_size);
        return face;
    }
    if(italic) {
        static const font_face face = font_face_from_embedded(embedded_font_italic, embedded_font_italic_size);
        return face;
    }
    static const font_face face = font_face_from_embedded(embedded_font_regular, embedded_font_regular_size);
    return face;
#endif
}

}  // namespace nova::vg
