#pragma once

#include "render.h"
#include "utils.h"

namespace novasvg {
namespace render {

void matrix_init(matrix_t* matrix, float a, float b, float c, float d, float e, float f);

void matrix_init_identity(matrix_t* matrix);

void matrix_init_translate(matrix_t* matrix, float tx, float ty);

void matrix_init_scale(matrix_t* matrix, float sx, float sy);

void matrix_init_rotate(matrix_t* matrix, float angle);

void matrix_init_shear(matrix_t* matrix, float shx, float shy);

void matrix_translate(matrix_t* matrix, float tx, float ty);

void matrix_scale(matrix_t* matrix, float sx, float sy);

void matrix_rotate(matrix_t* matrix, float angle);

void matrix_shear(matrix_t* matrix, float shx, float shy);

void matrix_multiply(matrix_t* matrix, const matrix_t* left, const matrix_t* right);

bool matrix_invert(const matrix_t* matrix, matrix_t* inverse);

void matrix_map(const matrix_t* matrix, float x, float y, float* xx, float* yy);

void matrix_map_point(const matrix_t* matrix, const point_t* src, point_t* dst);

void matrix_map_points(const matrix_t* matrix, const point_t* src, point_t* dst, int count);

void matrix_map_rect(const matrix_t* matrix, const rect_t* src, rect_t* dst);

int parse_matrix_parameters(const char** begin, const char* end, float values[6], int required, int optional);

bool matrix_parse(matrix_t* matrix, const char* data, int length);


} // namespace render
} // namespace novasvg