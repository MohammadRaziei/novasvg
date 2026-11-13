#pragma once

#ifndef NOVA_UTILS_H
#define NOVA_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <algorithm>

#include "nova/config.h"

namespace nova {
namespace vg {

// min/max/clamp were plain macros here (novasvg_min/max/clamp) -- pure
// numeric helpers with an exact STL equivalent, so they're gone in favor
// of std::min/std::max/std::clamp at every call site instead of being
// reimplemented, and so is the realloc-based dynamic array (std::vector,
// stb::span_set). What's left below (alpha/red/green/blue channel
// extraction, the refcounting helpers) has no STL equivalent doing the
// same job, so those stay as genuinely-own, C-style, prefixed macros.

#define novasvg_alpha(c) (((c) >> 24) & 0xff)
#define novasvg_red(c) (((c) >> 16) & 0xff)
#define novasvg_green(c) (((c) >> 8) & 0xff)
#define novasvg_blue(c) (((c) >> 0) & 0xff)

} // namespace vg
} // namespace nova

#endif // NOVA_UTILS_H
