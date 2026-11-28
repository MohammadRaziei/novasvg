#pragma once

#include "internal.h"
#include "utils.h"

namespace nova {
namespace vg {

// novasvg's miter limit is 10 (libstb's default is 4).
inline stb::stroke_style default_stroke_style()
{
    stb::stroke_style style;
    style.miter_limit = 10.f;
    return style;
}

} // namespace vg
} // namespace nova
