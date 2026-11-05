// The embedded fallback font (a zlib+base64 payload in default_font.h), declared in src/gfx/font.h.
#include "gfx/font.h"

#include <stb/zlib.hpp>

#include "novasvg/gfx/render.h"  // base64_decode

#ifndef NOVA_DISABLE_EMBEDDED_FONT
#include "gfx/default_font.h"
#endif

#include <cstdlib>
#include <string>
#include <vector>

namespace nova::vg {

namespace {
// fontFaceFromBase64() for a payload that was zlib-compressed before it was base64-encoded. The inflater is
// libstb's (stb::zlib_decompress), which reports bad input by exception: that becomes an empty FontFace.
FontFace fontFaceFromZlibBase64(std::string_view payload)
{
    size_t compressedLength = 0;
    auto* compressed = base64_decode(payload.data(), int(payload.size()), &compressedLength);
    if(compressed == nullptr)
        return FontFace();
    std::vector<uint8_t> inflated;
    try {
        inflated = stb::zlib_decompress(compressed, compressedLength);
    } catch(const std::exception&) {
        inflated.clear();
    }
    free(compressed);
    if(inflated.empty())
        return FontFace();
    auto* owned = new std::vector<uint8_t>(std::move(inflated));
    return FontFace(owned->data(), owned->size(), [](void* closure) { delete static_cast<std::vector<uint8_t>*>(closure); }, owned);
}

}  // namespace

FontFace embeddedFallbackFont(bool bold, bool italic)
{
#ifdef NOVA_DISABLE_EMBEDDED_FONT
    (void)bold;
    (void)italic;
    return FontFace();
#else
    // Each face is built on first use only: a machine with fonts never pays for the decode, and a
    // program that never asks for bold never decodes the bold face.
    auto build = [](const char* const* chunks) {
        std::string payload;
        for(auto chunk = chunks; *chunk; ++chunk)
            payload += *chunk;
        return fontFaceFromZlibBase64(payload);
    };
    if(bold && italic) {
        static const FontFace face = build(detail::kEmbeddedFontBoldItalicZlibBase64);
        return face;
    }
    if(bold) {
        static const FontFace face = build(detail::kEmbeddedFontBoldZlibBase64);
        return face;
    }
    if(italic) {
        static const FontFace face = build(detail::kEmbeddedFontItalicZlibBase64);
        return face;
    }
    static const FontFace face = build(detail::kEmbeddedFontZlibBase64);
    return face;
#endif
}

}  // namespace nova::vg
