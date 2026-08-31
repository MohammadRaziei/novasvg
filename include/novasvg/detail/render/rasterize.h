#pragma once

#include "private.h"
#include "utils.h"

#include "vendor/ft_raster.h"
#include "vendor/ft_stroker.h"

#include <limits.h>

namespace novasvg {
namespace render {

NOVASVG_INLINE void span_buffer_init(span_buffer_t* span_buffer)
{
    novasvg_array_init(span_buffer->spans);
    span_buffer_reset(span_buffer);
}

NOVASVG_INLINE void span_buffer_init_rect(span_buffer_t* span_buffer, int x, int y, int width, int height)
{
    novasvg_array_clear(span_buffer->spans);
    novasvg_array_ensure(span_buffer->spans, height);
    span_t* spans = span_buffer->spans.data;
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
    span_buffer->spans.size = height;
}

NOVASVG_INLINE void span_buffer_reset(span_buffer_t* span_buffer)
{
    novasvg_array_clear(span_buffer->spans);
    span_buffer->x = 0;
    span_buffer->y = 0;
    span_buffer->w = -1;
    span_buffer->h = -1;
}

NOVASVG_INLINE void span_buffer_destroy(span_buffer_t* span_buffer)
{
    novasvg_array_destroy(span_buffer->spans);
}

NOVASVG_INLINE void span_buffer_copy(span_buffer_t* span_buffer, const span_buffer_t* source)
{
    novasvg_array_clear(span_buffer->spans);
    novasvg_array_append(span_buffer->spans, source->spans);
    span_buffer->x = source->x;
    span_buffer->y = source->y;
    span_buffer->w = source->w;
    span_buffer->h = source->h;
}

NOVASVG_INLINE bool span_buffer_contains(const span_buffer_t* span_buffer, float x, float y)
{
    const int ix = (int)floorf(x);
    const int iy = (int)floorf(y);

    for(int i = 0; i < span_buffer->spans.size; i++) {
        span_t* span = &span_buffer->spans.data[i];
        if(span->y != iy)
            continue;
        if(ix >= span->x && ix < (span->x + span->len)) {
            return true;
        }
    }

    return false;
}

NOVASVG_INLINE void span_buffer_update_extents(span_buffer_t* span_buffer)
{
    if(span_buffer->w != -1 && span_buffer->h != -1)
        return;
    if(span_buffer->spans.size == 0) {
        span_buffer->x = 0;
        span_buffer->y = 0;
        span_buffer->w = 0;
        span_buffer->h = 0;
        return;
    }

    span_t* spans = span_buffer->spans.data;
    int x1 = INT_MAX;
    int y1 = spans[0].y;
    int x2 = 0;
    int y2 = spans[span_buffer->spans.size - 1].y;
    for(int i = 0; i < span_buffer->spans.size; i++) {
        if(spans[i].x < x1) x1 = spans[i].x;
        if(spans[i].x + spans[i].len > x2) x2 = spans[i].x + spans[i].len;
    }

    span_buffer->x = x1;
    span_buffer->y = y1;
    span_buffer->w = x2 - x1;
    span_buffer->h = y2 - y1 + 1;
}

NOVASVG_INLINE void span_buffer_extents(span_buffer_t* span_buffer, rect_t* extents)
{
    span_buffer_update_extents(span_buffer);
    extents->x = span_buffer->x;
    extents->y = span_buffer->y;
    extents->w = span_buffer->w;
    extents->h = span_buffer->h;
}

NOVASVG_INLINE void span_buffer_intersect(span_buffer_t* span_buffer, const span_buffer_t* a, const span_buffer_t* b)
{
    span_buffer_reset(span_buffer);
    novasvg_array_ensure(span_buffer->spans, std::max(a->spans.size, b->spans.size));

    span_t* a_spans = a->spans.data;
    span_t* a_end = a_spans + a->spans.size;

    span_t* b_spans = b->spans.data;
    span_t* b_end = b_spans + b->spans.size;
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

        int x = std::max(ax1, bx1);
        int len = std::min(ax2, bx2) - x;
        if(len) {
            novasvg_array_ensure(span_buffer->spans, 1);
            span_t* span = span_buffer->spans.data + span_buffer->spans.size;
            span->x = x;
            span->len = len;
            span->y = a_spans->y;
            span->coverage = (a_spans->coverage * b_spans->coverage) / 255;
            span_buffer->spans.size += 1;
        }

        if(ax2 < bx2) {
            ++a_spans;
        } else {
            ++b_spans;
        }
    }
}

#define ALIGN_SIZE(size) (((size) + 7ul) & ~7ul)
NOVASVG_INLINE PVG_FT_Outline* ft_outline_create(int points, int contours)
{
    size_t points_size = ALIGN_SIZE((points + contours) * sizeof(PVG_FT_Vector));
    size_t tags_size = ALIGN_SIZE((points + contours) * sizeof(char));
    size_t contours_size = ALIGN_SIZE(contours * sizeof(int));
    size_t contours_flag_size = ALIGN_SIZE(contours * sizeof(char));
    PVG_FT_Outline* outline = static_cast<PVG_FT_Outline*>(malloc(points_size + tags_size + contours_size + contours_flag_size + sizeof(PVG_FT_Outline)));

    PVG_FT_Byte* outline_data = (PVG_FT_Byte*)(outline + 1);
    outline->points = (PVG_FT_Vector*)(outline_data);
    outline->tags = (char*)(outline_data + points_size);
    outline->contours = (int*)(outline_data + points_size + tags_size);
    outline->contours_flag = (char*)(outline_data + points_size + tags_size + contours_size);
    outline->n_points = 0;
    outline->n_contours = 0;
    outline->flags = 0x0;
    return outline;
}

NOVASVG_INLINE void ft_outline_destroy(PVG_FT_Outline* outline)
{
    free(outline);
}

#define FT_COORD(x) (PVG_FT_Pos)(roundf(x * 64))
NOVASVG_INLINE void ft_outline_move_to(PVG_FT_Outline* ft, float x, float y)
{
    ft->points[ft->n_points].x = FT_COORD(x);
    ft->points[ft->n_points].y = FT_COORD(y);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    if(ft->n_points) {
        ft->contours[ft->n_contours] = ft->n_points - 1;
        ft->n_contours++;
    }

    ft->contours_flag[ft->n_contours] = 1;
    ft->n_points++;
}

NOVASVG_INLINE void ft_outline_line_to(PVG_FT_Outline* ft, float x, float y)
{
    ft->points[ft->n_points].x = FT_COORD(x);
    ft->points[ft->n_points].y = FT_COORD(y);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

NOVASVG_INLINE void ft_outline_cubic_to(PVG_FT_Outline* ft, float x1, float y1, float x2, float y2, float x3, float y3)
{
    ft->points[ft->n_points].x = FT_COORD(x1);
    ft->points[ft->n_points].y = FT_COORD(y1);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_CUBIC;
    ft->n_points++;

    ft->points[ft->n_points].x = FT_COORD(x2);
    ft->points[ft->n_points].y = FT_COORD(y2);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_CUBIC;
    ft->n_points++;

    ft->points[ft->n_points].x = FT_COORD(x3);
    ft->points[ft->n_points].y = FT_COORD(y3);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

NOVASVG_INLINE void ft_outline_close(PVG_FT_Outline* ft)
{
    ft->contours_flag[ft->n_contours] = 0;
    int index = ft->n_contours ? ft->contours[ft->n_contours - 1] + 1 : 0;
    if(index == ft->n_points)
        return;
    ft->points[ft->n_points].x = ft->points[index].x;
    ft->points[ft->n_points].y = ft->points[index].y;
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

NOVASVG_INLINE void ft_outline_end(PVG_FT_Outline* ft)
{
    if(ft->n_points) {
        ft->contours[ft->n_contours] = ft->n_points - 1;
        ft->n_contours++;
    }
}

NOVASVG_INLINE PVG_FT_Outline* ft_outline_convert_stroke(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data);

NOVASVG_INLINE PVG_FT_Outline* ft_outline_convert(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data)
{
    if(stroke_data) {
        return ft_outline_convert_stroke(path, matrix, stroke_data);
    }

    path_iterator_t it;
    path_iterator_init(&it, path);

    point_t points[3];
    PVG_FT_Outline* outline = ft_outline_create(path->num_points, path->num_contours);
    while(path_iterator_has_next(&it)) {
        switch(path_iterator_next(&it, points)) {
        case NOVASVG_PATH_COMMAND_MOVE_TO:
            matrix_map_points(matrix, points, points, 1);
            ft_outline_move_to(outline, points[0].x, points[0].y);
            break;
        case NOVASVG_PATH_COMMAND_LINE_TO:
            matrix_map_points(matrix, points, points, 1);
            ft_outline_line_to(outline, points[0].x, points[0].y);
            break;
        case NOVASVG_PATH_COMMAND_CUBIC_TO:
            matrix_map_points(matrix, points, points, 3);
            ft_outline_cubic_to(outline, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        case NOVASVG_PATH_COMMAND_CLOSE:
            ft_outline_close(outline);
            break;
        }
    }

    ft_outline_end(outline);
    return outline;
}

NOVASVG_INLINE PVG_FT_Outline* ft_outline_convert_dash(const path_t* path, const matrix_t* matrix, const stroke_dash_t* stroke_dash)
{
    if(stroke_dash->array.size == 0)
        return ft_outline_convert(path, matrix, NULL);
    path_t* dashed = path_clone_dashed(path, stroke_dash->offset, stroke_dash->array.data, stroke_dash->array.size);
    PVG_FT_Outline* outline = ft_outline_convert(dashed, matrix, NULL);
    path_destroy(dashed);
    return outline;
}

NOVASVG_INLINE PVG_FT_Outline* ft_outline_convert_stroke(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data)
{
    double scale_x = sqrt(matrix->a * matrix->a + matrix->b * matrix->b);
    double scale_y = sqrt(matrix->c * matrix->c + matrix->d * matrix->d);

    double scale = hypot(scale_x, scale_y) / NOVASVG_SQRT2;
    double width = stroke_data->style.width * scale;

    PVG_FT_Fixed ftWidth = (PVG_FT_Fixed)(width * 0.5 * (1 << 6));
    PVG_FT_Fixed ftMiterLimit = (PVG_FT_Fixed)(stroke_data->style.miter_limit * (1 << 16));

    PVG_FT_Stroker_LineCap ftCap;
    switch(stroke_data->style.cap) {
    case NOVASVG_LINE_CAP_SQUARE:
        ftCap = PVG_FT_STROKER_LINECAP_SQUARE;
        break;
    case NOVASVG_LINE_CAP_ROUND:
        ftCap = PVG_FT_STROKER_LINECAP_ROUND;
        break;
    default:
        ftCap = PVG_FT_STROKER_LINECAP_BUTT;
        break;
    }

    PVG_FT_Stroker_LineJoin ftJoin;
    switch(stroke_data->style.join) {
    case NOVASVG_LINE_JOIN_BEVEL:
        ftJoin = PVG_FT_STROKER_LINEJOIN_BEVEL;
        break;
    case NOVASVG_LINE_JOIN_ROUND:
        ftJoin = PVG_FT_STROKER_LINEJOIN_ROUND;
        break;
    default:
        ftJoin = PVG_FT_STROKER_LINEJOIN_MITER_FIXED;
        break;
    }

    PVG_FT_Stroker stroker;
    PVG_FT_Stroker_New(&stroker);
    PVG_FT_Stroker_Set(stroker, ftWidth, ftCap, ftJoin, ftMiterLimit);

    PVG_FT_Outline* outline = ft_outline_convert_dash(path, matrix, &stroke_data->dash);
    PVG_FT_Stroker_ParseOutline(stroker, outline);

    PVG_FT_UInt points;
    PVG_FT_UInt contours;
    PVG_FT_Stroker_GetCounts(stroker, &points, &contours);

    PVG_FT_Outline* stroke_outline = ft_outline_create(points, contours);
    PVG_FT_Stroker_Export(stroker, stroke_outline);

    PVG_FT_Stroker_Done(stroker);
    ft_outline_destroy(outline);
    return stroke_outline;
}

NOVASVG_INLINE void spans_generation_callback(int count, const PVG_FT_Span* spans, void* user)
{
    span_buffer_t* span_buffer = (span_buffer_t*)(user);
    novasvg_array_append_data(span_buffer->spans, spans, count);
}

NOVASVG_INLINE void rasterize(span_buffer_t* span_buffer, const path_t* path, const matrix_t* matrix, const rect_t* clip_rect, const stroke_data_t* stroke_data, fill_rule_t winding)
{
    PVG_FT_Outline* outline = ft_outline_convert(path, matrix, stroke_data);
    if(stroke_data) {
        outline->flags = PVG_FT_OUTLINE_NONE;
    } else {
        switch(winding) {
        case NOVASVG_FILL_RULE_EVEN_ODD:
            outline->flags = PVG_FT_OUTLINE_EVEN_ODD_FILL;
            break;
        default:
            outline->flags = PVG_FT_OUTLINE_NONE;
            break;
        }
    }

    PVG_FT_Raster_Params params;
    params.flags = PVG_FT_RASTER_FLAG_DIRECT | PVG_FT_RASTER_FLAG_AA;
    params.gray_spans = spans_generation_callback;
    params.user = span_buffer;
    params.source = outline;
    if(clip_rect) {
        params.flags |= PVG_FT_RASTER_FLAG_CLIP;
        params.clip_box.xMin = (PVG_FT_Pos)clip_rect->x;
        params.clip_box.yMin = (PVG_FT_Pos)clip_rect->y;
        params.clip_box.xMax = (PVG_FT_Pos)(clip_rect->x + clip_rect->w);
        params.clip_box.yMax = (PVG_FT_Pos)(clip_rect->y + clip_rect->h);
    }

    span_buffer_reset(span_buffer);
    PVG_FT_Raster_Render(&params);
    ft_outline_destroy(outline);
}


} // namespace render
} // namespace novasvg