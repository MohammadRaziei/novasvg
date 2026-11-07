// The embedded fallback font (a zlib+base64 payload in default_font.h), declared in src/sources/vg/font.h.
#include "font.h"

#include <stb/zlib.hpp>

#include "render.h"  // base64_decode

#ifndef NOVA_DISABLE_EMBEDDED_FONT
#include "default_font.h"
#endif

#include <cstdlib>
#include <string>
#include <vector>

namespace nova::vg {

namespace {
// fontFaceFromBase64() for a payload that was zlib-compressed before it was base64-encoded. The inflater is
// libstb's (stb::zlib_decompress), which reports bad input by exception: that becomes an empty FontFace.
font_face font_face_from_zlib_base64(std::string_view payload)
{
    size_t compressed_length = 0;
    auto* compressed = base64_decode(payload.data(), int(payload.size()), &compressed_length);
    if(compressed == nullptr)
        return font_face();
    std::vector<uint8_t> inflated;
    try {
        inflated = stb::zlib_decompress(compressed, compressed_length);
    } catch(const std::exception&) {
        inflated.clear();
    }
    free(compressed);
    if(inflated.empty())
        return font_face();
    auto* owned = new std::vector<uint8_t>(std::move(inflated));
    return font_face(owned->data(), owned->size(), [](void* closure) { delete static_cast<std::vector<uint8_t>*>(closure); }, owned);
}

}  // namespace

font_face embedded_fallback_font(bool bold, bool italic)
{
#ifdef NOVA_DISABLE_EMBEDDED_FONT
    (void)bold;
    (void)italic;
    return font_face();
#else
    // Each face is built on first use only: a machine with fonts never pays for the decode, and a
    // program that never asks for bold never decodes the bold face.
    auto build = [](const char* const* chunks) {
        std::string payload;
        for(auto chunk = chunks; *chunk; ++chunk)
            payload += *chunk;
        return font_face_from_zlib_base64(payload);
    };
    if(bold && italic) {
        static const font_face face = build(detail::k_embedded_font_bold_italic_zlib_base64);
        return face;
    }
    if(bold) {
        static const font_face face = build(detail::k_embedded_font_bold_zlib_base64);
        return face;
    }
    if(italic) {
        static const font_face face = build(detail::k_embedded_font_italic_zlib_base64);
        return face;
    }
    static const font_face face = build(detail::k_embedded_font_zlib_base64);
    return face;
#endif
}

}  // namespace nova::vg
