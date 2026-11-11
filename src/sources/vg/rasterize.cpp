#include "rasterize.h"

// Anti-aliased fill and stroke come from libstb (stb/raster.hpp): a fixed-point port of the same FreeType
// algorithms (through plutovg) that used to be vendored here in src/sources/vg/vendor.
#include <stb/raster.hpp>

#include <algorithm>

namespace nova {
namespace vg {

static void span_buffer_update_extents(span_buffer_t* span_buffer);

void span_buffer_init(span_buffer_t* span_buffer)
{
    novasvg_array_init(span_buffer->spans);
    span_buffer_reset(span_buffer);
}

void span_buffer_init_rect(span_buffer_t* span_buffer, int x, int y, int width, int height)
{
    novasvg_array_clear(span_buffer->spans);
    novasvg_array_ensure(span_buffer->spans, height);
    stb::span* spans = span_buffer->spans.data;
    for(int i = 0; i < height; i++) {
        spans[i].x = x;
        spans[i].y = y + i;
        spans[i].len = width;
        spans[i].coverage = 255;
    }

    span_buffer->x = x;
    span_buffer->y = y;
    span_buffer->w = width;
    span_buffer->h = height;
    span_buffer->spans.size_ = height;
}

void span_buffer_reset(span_buffer_t* span_buffer)
{
    novasvg_array_clear(span_buffer->spans);
    span_buffer->x = 0;
    span_buffer->y = 0;
    span_buffer->w = -1;
    span_buffer->h = -1;
}

void span_buffer_destroy(span_buffer_t* span_buffer)
{
    novasvg_array_destroy(span_buffer->spans);
}

void span_buffer_copy(span_buffer_t* span_buffer, const span_buffer_t* source)
{
    novasvg_array_clear(span_buffer->spans);
    novasvg_array_append(span_buffer->spans, source->spans);
    span_buffer->x = source->x;
    span_buffer->y = source->y;
    span_buffer->w = source->w;
    span_buffer->h = source->h;
}

bool span_buffer_contains(const span_buffer_t* span_buffer, float x, float y)
{
    const int ix = (int)floorf(x);
    const int iy = (int)floorf(y);

    for(int i = 0; i < span_buffer->spans.size_; i++) {
        stb::span* span = &span_buffer->spans.data[i];
        if(span->y != iy)
            continue;
        if(ix >= span->x && ix < (span->x + span->len)) {
            return true;
        }
    }

    return false;
}

static void span_buffer_update_extents(span_buffer_t* span_buffer)
{
    if(span_buffer->w != -1 && span_buffer->h != -1)
        return;
    if(span_buffer->spans.size_ == 0) {
        span_buffer->x = 0;
        span_buffer->y = 0;
        span_buffer->w = 0;
        span_buffer->h = 0;
        return;
    }

    stb::span* spans = span_buffer->spans.data;
    int x1 = INT_MAX;
    int y1 = spans[0].y;
    int x2 = 0;
    int y2 = spans[span_buffer->spans.size_ - 1].y;
    for(int i = 0; i < span_buffer->spans.size_; i++) {
        if(spans[i].x < x1) x1 = spans[i].x;
        if(spans[i].x + spans[i].len > x2) x2 = spans[i].x + spans[i].len;
    }

    span_buffer->x = x1;
    span_buffer->y = y1;
    span_buffer->w = x2 - x1;
    span_buffer->h = y2 - y1 + 1;
}

void span_buffer_extents(span_buffer_t* span_buffer, rect_t* extents)
{
    span_buffer_update_extents(span_buffer);
    extents->x = span_buffer->x;
    extents->y = span_buffer->y;
    extents->w = span_buffer->w;
    extents->h = span_buffer->h;
}

void span_buffer_intersect(span_buffer_t* span_buffer, const span_buffer_t* a, const span_buffer_t* b)
{
    span_buffer_reset(span_buffer);
    novasvg_array_ensure(span_buffer->spans, STB_MAX(a->spans.size_, b->spans.size_));

    stb::span* a_spans = a->spans.data;
    stb::span* a_end = a_spans + a->spans.size_;

    stb::span* b_spans = b->spans.data;
    stb::span* b_end = b_spans + b->spans.size_;
    while(a_spans < a_end && b_spans < b_end) {
        if(b_spans->y > a_spans->y) {
            ++a_spans;
            continue;
        }

        if(a_spans->y != b_spans->y) {
            ++b_spans;
            continue;
        }

        int ax1 = a_spans->x;
        int ax2 = ax1 + a_spans->len;
        int bx1 = b_spans->x;
        int bx2 = bx1 + b_spans->len;
        if(bx1 < ax1 && bx2 < ax1) {
            ++b_spans;
            continue;
        }

        if(ax1 < bx1 && ax2 < bx1) {
            ++a_spans;
            continue;
        }

        int x = STB_MAX(ax1, bx1);
        int len = STB_MIN(ax2, bx2) - x;
        if(len) {
            novasvg_array_ensure(span_buffer->spans, 1);
            stb::span* span = span_buffer->spans.data + span_buffer->spans.size_;
            span->x = x;
            span->len = len;
            span->y = a_spans->y;
            span->coverage = (a_spans->coverage * b_spans->coverage) / 255;
            span_buffer->spans.size_ += 1;
        }

        if(ax2 < bx2) {
            ++a_spans;
        } else {
            ++b_spans;
        }
    }
}

void rasterize(span_buffer_t* span_buffer, const stb::outline& path_, const stb::matrix* matrix, const rect_t* clip_rect, const stb::stroke_style* stroke_, fill_rule winding)
{
    span_buffer_reset(span_buffer);

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

    auto sink = [span_buffer](stb::span_view view) {
        const int count = (int)view.size;
        novasvg_array_ensure(span_buffer->spans, count);
        stb::span* out = span_buffer->spans.data + span_buffer->spans.size_;
        std::copy(view.data, view.data + count, out);

        span_buffer->spans.size_ += count;
    };

    const stb::outline& outline = path_;
    if(stroke_) {
        stb::rasterize_stroke(outline, *stroke_, m, clip, sink);
    } else {
        stb::rasterize_fill(outline, winding, m, clip, sink);
    }
}

} // namespace vg
} // namespace nova
