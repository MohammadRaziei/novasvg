// Definitions declared in svgparser.hpp.
#include "svgelement.h"
#include "svgparser.hpp"

namespace novasvg {
using namespace gfx;

bool RuleData::match(const svg_element* el) const
{
    return matchSelector(m_selector, el);
}

FontFace embeddedFallbackFont(bool bold, bool italic)
{
#ifdef NOVASVG_DISABLE_EMBEDDED_FONT
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

} // namespace novasvg
