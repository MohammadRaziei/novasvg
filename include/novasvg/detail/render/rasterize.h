#pragma once

#include "private.h"
#include "utils.h"

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

void rasterize(span_buffer_t* span_buffer, const path_t* path, const matrix_t* matrix, const rect_t* clip_rect, const stroke_data_t* stroke_data, fill_rule_t winding);


} // namespace render
} // namespace novasvg