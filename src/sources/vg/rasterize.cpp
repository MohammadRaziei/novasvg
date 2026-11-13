#include "rasterize.h"

// Anti-aliased fill and stroke come from libstb (stb/raster.hpp): a fixed-point port of the same FreeType
// algorithms (through plutovg) that used to be vendored here in src/sources/vg/vendor.
#include <stb/raster.hpp>

#include <algorithm>

namespace nova {
namespace vg {

void rasterize(stb::span_set* span_buffer, const stb::outline& path_, const stb::matrix* matrix, const rect_t* clip_rect, const stb::stroke_style* stroke_, fill_rule winding)
{
    span_buffer->clear();

    const stb::matrix& m = *matrix;

    stb::clip_rect clip;
    if(clip_rect) {
        clip.x = (int)clip_rect->x;
        clip.y = (int)clip_rect->y;
        clip.w = (int)(clip_rect->x + clip_rect->w) - clip.x;
        clip.h = (int)(clip_rect->y + clip_rect->h) - clip.y;
    } else {
        // No clip: libstb limits coordinates to +-1,000,000 anyway.
        clip.x = -1000000;
        clip.y = -1000000;
        clip.w = 2000000;
        clip.h = 2000000;
    }

    const stb::outline& outline = path_;
    if(stroke_) {
        stb::rasterize_stroke(outline, *stroke_, m, clip, stb::span_sink(*span_buffer));
    } else {
        stb::rasterize_fill(outline, winding, m, clip, stb::span_sink(*span_buffer));
    }
}

} // namespace vg
} // namespace nova
