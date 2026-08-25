#pragma once

#ifndef NOVASVG_PRIVATE_H
#define NOVASVG_PRIVATE_H

#include "render.h"

namespace novasvg {
namespace render {

#if defined(_WIN32)

} // namespace render
} // namespace novasvg
#include <windows.h>
namespace novasvg {
namespace render {

typedef LONG ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count = 1)
#define novasvg_increment_reference(ob) (void)(ob && InterlockedIncrement(&(ob)->ref_count))
#define novasvg_destroy_reference(ob) (ob && InterlockedDecrement(&(ob)->ref_count) == 0)
#define novasvg_get_reference_count(ob) ((ob) ? InterlockedCompareExchange((LONG*)&(ob)->ref_count, 0, 0) : 0)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)

} // namespace render
} // namespace novasvg
#include <stdatomic.h>
namespace novasvg {
namespace render {

typedef atomic_int ref_count_t;

#define novasvg_init_reference(ob) atomic_init(&(ob)->ref_count, 1)
#define novasvg_increment_reference(ob) (void)(ob && atomic_fetch_add(&(ob)->ref_count, 1))
#define novasvg_destroy_reference(ob) (ob && atomic_fetch_sub(&(ob)->ref_count, 1) == 1)
#define novasvg_get_reference_count(ob) ((ob) ? atomic_load(&(ob)->ref_count) : 0)

#else

typedef int ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count = 1)
#define novasvg_increment_reference(ob) (void)(ob && ++(ob)->ref_count)
#define novasvg_destroy_reference(ob) (ob && --(ob)->ref_count == 0)
#define novasvg_get_reference_count(ob) ((ob) ? (ob)->ref_count : 0)

#endif

struct surface {
    ref_count_t ref_count;
    float width;
    float height;
    int stride;
    unsigned char* data;
};

struct path {
    ref_count_t ref_count;
    int num_points;
    int num_contours;
    int num_curves;
    point_t start_point;
    struct {
        path_element_t* data;
        int size;
        int capacity;
    } elements;
};

typedef enum {
    NOVASVG_PAINT_TYPE_COLOR,
    NOVASVG_PAINT_TYPE_GRADIENT,
    NOVASVG_PAINT_TYPE_TEXTURE
} paint_type_t;

struct paint {
    ref_count_t ref_count;
    paint_type_t type;
};

typedef struct {
    paint_t base;
    color_t color;
} solid_paint_t;

typedef enum {
    NOVASVG_GRADIENT_TYPE_LINEAR,
    NOVASVG_GRADIENT_TYPE_RADIAL
} gradient_type_t;

typedef struct {
    paint_t base;
    gradient_type_t type;
    spread_method_t spread;
    matrix_t matrix;
    gradient_stop_t* stops;
    int nstops;
    float values[6];
} gradient_paint_t;

typedef struct {
    paint_t base;
    texture_type_t type;
    float opacity;
    matrix_t matrix;
    surface_t* surface;
} texture_paint_t;

typedef struct {
    int x;
    int len;
    int y;
    unsigned char coverage;
} span_t;

typedef struct {
    struct {
        span_t* data;
        int size;
        int capacity;
    } spans;

    int x;
    int y;
    int w;
    int h;
} span_buffer_t;

typedef struct {
    float offset;
    struct {
        float* data;
        int size;
        int capacity;
    } array;
} stroke_dash_t;

typedef struct {
    float width;
    line_cap_t cap;
    line_join_t join;
    float miter_limit;
} stroke_style_t;

typedef struct {
    stroke_style_t style;
    stroke_dash_t dash;
} stroke_data_t;

typedef struct state {
    paint_t* paint;
    font_face_t* font_face;
    color_t color;
    matrix_t matrix;
    stroke_data_t stroke;
    span_buffer_t clip_spans;
    fill_rule_t winding;
    operator_t op;
    float font_size;
    float opacity;
    bool clipping;
    struct state* next;
} state_t;

struct canvas {
    ref_count_t ref_count;
    surface_t* surface;
    path_t* path;
    state_t* state;
    state_t* freed_state;
    font_face_cache_t* face_cache;
    rect_t clip_rect;
    span_buffer_t clip_spans;
    span_buffer_t fill_spans;
};

void span_buffer_init(span_buffer_t* span_buffer);
void span_buffer_init_rect(span_buffer_t* span_buffer, int x, int y, int width, int height);
void span_buffer_reset(span_buffer_t* span_buffer);
void span_buffer_destroy(span_buffer_t* span_buffer);
void span_buffer_copy(span_buffer_t* span_buffer, const span_buffer_t* source);
bool span_buffer_contains(const span_buffer_t* span_buffer, float x, float y);
void span_buffer_extents(span_buffer_t* span_buffer, rect_t* extents);
void span_buffer_intersect(span_buffer_t* span_buffer, const span_buffer_t* a, const span_buffer_t* b);

void rasterize(span_buffer_t* span_buffer, const path_t* path, const matrix_t* matrix, const rect_t* clip_rect, const stroke_data_t* stroke_data, fill_rule_t winding);
void blend(canvas_t* canvas, const span_buffer_t* span_buffer);
void memfill32(unsigned int* dest, int length, unsigned int value);

} // namespace render
} // namespace novasvg

#endif // NOVASVG_PRIVATE_H
