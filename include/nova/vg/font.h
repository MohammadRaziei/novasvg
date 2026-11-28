#pragma once

// Fonts: registering a font file so text can use it. Nothing here knows SVG.

#include "../config.h"

#include <string>

namespace nova::vg {

/**
 * @brief Registers every face in a font file with the font cache that text rendering and measure_text() use.
 *
 * Each face is filed under the family name, weight range and italic flag the font itself declares, so there is
 * nothing to pass but the path. A variable font is registered with the weight range of its `wght` axis.
 *
 * @param path A font file (a `.ttc` collection registers all its faces).
 * @return The number of faces registered: 0 if the file is not a font or cannot be read, and -1 in a build
 *         that cannot load font files (`NOVA_DISABLE_FONT_FACE_CACHE_LOAD`).
 */
NOVA_API int add_font_file(const std::string& path);

} // namespace nova::vg
