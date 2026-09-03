#pragma once

#include "utils.h"

namespace novasvg {
namespace render {

NOVASVG_INLINE void matrix_init(matrix_t* matrix, float a, float b, float c, float d, float e, float f)
{
    matrix->a = a; matrix->b = b;
    matrix->c = c; matrix->d = d;
    matrix->e = e; matrix->f = f;
}

NOVASVG_INLINE void matrix_init_identity(matrix_t* matrix)
{
    matrix_init(matrix, 1, 0, 0, 1, 0, 0);
}

NOVASVG_INLINE void matrix_init_translate(matrix_t* matrix, float tx, float ty)
{
    matrix_init(matrix, 1, 0, 0, 1, tx, ty);
}

NOVASVG_INLINE void matrix_init_scale(matrix_t* matrix, float sx, float sy)
{
    matrix_init(matrix, sx, 0, 0, sy, 0, 0);
}

NOVASVG_INLINE void matrix_init_rotate(matrix_t* matrix, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    matrix_init(matrix, c, s, -s, c, 0, 0);
}

NOVASVG_INLINE void matrix_init_shear(matrix_t* matrix, float shx, float shy)
{
    matrix_init(matrix, 1, tanf(shy), tanf(shx), 1, 0, 0);
}

NOVASVG_INLINE void matrix_translate(matrix_t* matrix, float tx, float ty)
{
    matrix_t m;
    matrix_init_translate(&m, tx, ty);
    matrix_multiply(matrix, &m, matrix);
}

NOVASVG_INLINE void matrix_scale(matrix_t* matrix, float sx, float sy)
{
    matrix_t m;
    matrix_init_scale(&m, sx, sy);
    matrix_multiply(matrix, &m, matrix);
}

NOVASVG_INLINE void matrix_rotate(matrix_t* matrix, float angle)
{
    matrix_t m;
    matrix_init_rotate(&m, angle);
    matrix_multiply(matrix, &m, matrix);
}

NOVASVG_INLINE void matrix_shear(matrix_t* matrix, float shx, float shy)
{
    matrix_t m;
    matrix_init_shear(&m, shx, shy);
    matrix_multiply(matrix, &m, matrix);
}

NOVASVG_INLINE void matrix_multiply(matrix_t* matrix, const matrix_t* left, const matrix_t* right)
{
    float a = left->a * right->a + left->b * right->c;
    float b = left->a * right->b + left->b * right->d;
    float c = left->c * right->a + left->d * right->c;
    float d = left->c * right->b + left->d * right->d;
    float e = left->e * right->a + left->f * right->c + right->e;
    float f = left->e * right->b + left->f * right->d + right->f;
    matrix_init(matrix, a, b, c, d, e, f);
}

NOVASVG_INLINE bool matrix_invert(const matrix_t* matrix, matrix_t* inverse)
{
    float det = (matrix->a * matrix->d - matrix->b * matrix->c);
    if(det == 0.f)
        return false;
    if(inverse) {
        float inv_det = 1.f / det;
        float a = matrix->a * inv_det;
        float b = matrix->b * inv_det;
        float c = matrix->c * inv_det;
        float d = matrix->d * inv_det;
        float e = (matrix->c * matrix->f - matrix->d * matrix->e) * inv_det;
        float f = (matrix->b * matrix->e - matrix->a * matrix->f) * inv_det;
        matrix_init(inverse, d, -b, -c, a, e, f);
    }

    return true;
}

NOVASVG_INLINE void matrix_map(const matrix_t* matrix, float x, float y, float* xx, float* yy)
{
    *xx = x * matrix->a + y * matrix->c + matrix->e;
    *yy = x * matrix->b + y * matrix->d + matrix->f;
}

NOVASVG_INLINE void matrix_map_point(const matrix_t* matrix, const point_t* src, point_t* dst)
{
    matrix_map(matrix, src->x, src->y, &dst->x, &dst->y);
}

NOVASVG_INLINE void matrix_map_points(const matrix_t* matrix, const point_t* src, point_t* dst, int count)
{
    for(int i = 0; i < count; ++i) {
        matrix_map_point(matrix, &src[i], &dst[i]);
    }
}

NOVASVG_INLINE void matrix_map_rect(const matrix_t* matrix, const rect_t* src, rect_t* dst)
{
    point_t p[4];
    p[0].x = src->x;
    p[0].y = src->y;
    p[1].x = src->x + src->w;
    p[1].y = src->y;
    p[2].x = src->x + src->w;
    p[2].y = src->y + src->h;
    p[3].x = src->x;
    p[3].y = src->y + src->h;
    matrix_map_points(matrix, p, p, 4);

    float l = p[0].x;
    float t = p[0].y;
    float r = p[0].x;
    float b = p[0].y;

    for(int i = 1; i < 4; i++) {
        if(p[i].x < l) l = p[i].x;
        if(p[i].x > r) r = p[i].x;
        if(p[i].y < t) t = p[i].y;
        if(p[i].y > b) b = p[i].y;
    }

    dst->x = l;
    dst->y = t;
    dst->w = r - l;
    dst->h = b - t;
}

NOVASVG_INLINE int parse_matrix_parameters(const char** begin, const char* end, float values[6], int required, int optional)
{
    if(!skip_ws_and_delim(begin, end, '('))
        return 0;
    int count = 0;
    int max_count = required + optional;
    bool has_trailing_comma = false;
    for(; count < max_count; ++count) {
        if(!parse_number(begin, end, values + count))
            break;
        skip_css_unit(begin, end, values + count);
        skip_ws_or_comma(begin, end, &has_trailing_comma);
    }

    if(!has_trailing_comma && (count == required || count == max_count)
        && skip_delim(begin, end, ')')) {
        return count;
    }

    return 0;
}

NOVASVG_INLINE bool matrix_parse(matrix_t* matrix, const char* data, int length)
{
    float values[6];
    matrix_init_identity(matrix);
    if(length == -1)
        length = strlen(data);
    const char* it = data;
    const char* end = it + length;
    bool has_trailing_comma = false;
    skip_ws(&it, end);
    while(it < end) {
        if(skip_string(&it, end, "matrix")) {
            int count = parse_matrix_parameters(&it, end, values, 6, 0);
            if(count == 0)
                return false;
            matrix_t m = { values[0], values[1], values[2], values[3], values[4], values[5] };
            matrix_multiply(matrix, &m, matrix);
        } else if(skip_string(&it, end, "translate")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix_translate(matrix, values[0], 0);
            } else {
                matrix_translate(matrix, values[0], values[1]);
            }
        } else if(skip_string(&it, end, "scale")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix_scale(matrix, values[0], values[0]);
            } else {
                matrix_scale(matrix, values[0], values[1]);
            }
        } else if(skip_string(&it, end, "rotate")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 2);
            if(count == 0)
                return false;
            if(count == 3)
                matrix_translate(matrix, values[1], values[2]);
            matrix_rotate(matrix, NOVASVG_DEG2RAD(values[0]));
            if(count == 3) {
                matrix_translate(matrix, -values[1], -values[2]);
            }
        } else if(skip_string(&it, end, "skewX")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 0);
            if(count == 0)
                return false;
            matrix_shear(matrix, NOVASVG_DEG2RAD(values[0]), 0);
        } else if(skip_string(&it, end, "skewY")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 0);
            if(count == 0)
                return false;
            matrix_shear(matrix, 0, NOVASVG_DEG2RAD(values[0]));
        } else {
            return false;
        }

        skip_ws_or_comma(&it, end, &has_trailing_comma);
    }

    return !has_trailing_comma;
}


} // namespace render
} // namespace novasvg