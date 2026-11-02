#pragma once

#include "private.h"
#include "utils.h"

#include "vendor/ft_raster.h"
#include "vendor/ft_stroker.h"

#include <limits.h>

namespace novasvg {
namespace render {

void span_buffer_init(span_buffer_t* span_buffer);

void span_buffer_init_rect(span_buffer_t* span_buffer, int x, int y, int width, int height);

void span_buffer_reset(span_buffer_t* span_buffer);

void span_buffer_destroy(span_buffer_t* span_buffer);

void span_buffer_copy(span_buffer_t* span_buffer, const span_buffer_t* source);

bool span_buffer_contains(const span_buffer_t* span_buffer, float x, float y);

void span_buffer_update_extents(span_buffer_t* span_buffer);

void span_buffer_extents(span_buffer_t* span_buffer, rect_t* extents);

void span_buffer_intersect(span_buffer_t* span_buffer, const span_buffer_t* a, const span_buffer_t* b);

#define ALIGN_SIZE(size) (((size) + 7ul) & ~7ul)
PVG_FT_Outline* ft_outline_create(int points, int contours);

void ft_outline_destroy(PVG_FT_Outline* outline);

// roundf() without the libm call: nearest integer, halves away from zero -- exactly roundf's result. This runs
// twice for every point of every path. `v - float(t)` is exact (t is v's integer part), so the comparison with
// one half is exact too; anything too big for an int, or not a number, goes to roundf itself.
PVG_FT_Pos ft_coord_round(float v);

#define FT_COORD(x) ft_coord_round((x) * 64)
void ft_outline_move_to(PVG_FT_Outline* ft, float x, float y);

void ft_outline_line_to(PVG_FT_Outline* ft, float x, float y);

void ft_outline_cubic_to(PVG_FT_Outline* ft, float x1, float y1, float x2, float y2, float x3, float y3);

void ft_outline_close(PVG_FT_Outline* ft);

void ft_outline_end(PVG_FT_Outline* ft);

PVG_FT_Outline* ft_outline_convert_stroke(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data);

NOVASVG_INLINE PVG_FT_Outline* ft_outline_convert(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data);

PVG_FT_Outline* ft_outline_convert_dash(const path_t* path, const matrix_t* matrix, const stroke_dash_t* stroke_dash);

PVG_FT_Outline* ft_outline_convert_stroke(const path_t* path, const matrix_t* matrix, const stroke_data_t* stroke_data);

void spans_generation_callback(int count, const PVG_FT_Span* spans, void* user);

void rasterize(span_buffer_t* span_buffer, const path_t* path, const matrix_t* matrix, const rect_t* clip_rect, const stroke_data_t* stroke_data, fill_rule_t winding);


} // namespace render
} // namespace novasvg