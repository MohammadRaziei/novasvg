#pragma once

#ifndef NOVASVG_PRIVATE_H
#define NOVASVG_PRIVATE_H

#include "render.h"

#if defined(_WIN32)

#include <windows.h>

typedef LONG novasvg_ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count = 1)
#define novasvg_increment_reference(ob) (void)(ob && InterlockedIncrement(&(ob)->ref_count))
#define novasvg_destroy_reference(ob) (ob && InterlockedDecrement(&(ob)->ref_count) == 0)
#define novasvg_get_reference_count(ob) ((ob) ? InterlockedCompareExchange((LONG*)&(ob)->ref_count, 0, 0) : 0)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)

#include <stdatomic.h>

typedef atomic_int novasvg_ref_count_t;

#define novasvg_init_reference(ob) atomic_init(&(ob)->ref_count, 1)
#define novasvg_increment_reference(ob) (void)(ob && atomic_fetch_add(&(ob)->ref_count, 1))
#define novasvg_destroy_reference(ob) (ob && atomic_fetch_sub(&(ob)->ref_count, 1) == 1)
#define novasvg_get_reference_count(ob) ((ob) ? atomic_load(&(ob)->ref_count) : 0)

#else

typedef int novasvg_ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count = 1)
#define novasvg_increment_reference(ob) (void)(ob && ++(ob)->ref_count)
#define novasvg_destroy_reference(ob) (ob && --(ob)->ref_count == 0)
#define novasvg_get_reference_count(ob) ((ob) ? (ob)->ref_count : 0)

#endif

struct novasvg_surface {
    novasvg_ref_count_t ref_count;
    float width;
    float height;
    int stride;
    unsigned char* data;
};

struct novasvg_path {
    novasvg_ref_count_t ref_count;
    int num_points;
    int num_contours;
    int num_curves;
    novasvg_point_t start_point;
    struct {
        novasvg_path_element_t* data;
        int size;
        int capacity;
    } elements;
};

typedef enum {
    NOVASVG_PAINT_TYPE_COLOR,
    NOVASVG_PAINT_TYPE_GRADIENT,
    NOVASVG_PAINT_TYPE_TEXTURE
} novasvg_paint_type_t;

struct novasvg_paint {
    novasvg_ref_count_t ref_count;
    novasvg_paint_type_t type;
};

typedef struct {
    novasvg_paint_t base;
    novasvg_color_t color;
} novasvg_solid_paint_t;

typedef enum {
    NOVASVG_GRADIENT_TYPE_LINEAR,
    NOVASVG_GRADIENT_TYPE_RADIAL
} novasvg_gradient_type_t;

typedef struct {
    novasvg_paint_t base;
    novasvg_gradient_type_t type;
    novasvg_spread_method_t spread;
    novasvg_matrix_t matrix;
    novasvg_gradient_stop_t* stops;
    int nstops;
    float values[6];
} novasvg_gradient_paint_t;

typedef struct {
    novasvg_paint_t base;
    novasvg_texture_type_t type;
    float opacity;
    novasvg_matrix_t matrix;
    novasvg_surface_t* surface;
} novasvg_texture_paint_t;

typedef struct {
    int x;
    int len;
    int y;
    unsigned char coverage;
} novasvg_span_t;

typedef struct {
    struct {
        novasvg_span_t* data;
        int size;
        int capacity;
    } spans;

    int x;
    int y;
    int w;
    int h;
} novasvg_span_buffer_t;

typedef struct {
    float offset;
    struct {
        float* data;
        int size;
        int capacity;
    } array;
} novasvg_stroke_dash_t;

typedef struct {
    float width;
    novasvg_line_cap_t cap;
    novasvg_line_join_t join;
    float miter_limit;
} novasvg_stroke_style_t;

typedef struct {
    novasvg_stroke_style_t style;
    novasvg_stroke_dash_t dash;
} novasvg_stroke_data_t;

typedef struct novasvg_state {
    novasvg_paint_t* paint;
    novasvg_font_face_t* font_face;
    novasvg_color_t color;
    novasvg_matrix_t matrix;
    novasvg_stroke_data_t stroke;
    novasvg_span_buffer_t clip_spans;
    novasvg_fill_rule_t winding;
    novasvg_operator_t op;
    float font_size;
    float opacity;
    bool clipping;
    struct novasvg_state* next;
} novasvg_state_t;

struct novasvg_canvas {
    novasvg_ref_count_t ref_count;
    novasvg_surface_t* surface;
    novasvg_path_t* path;
    novasvg_state_t* state;
    novasvg_state_t* freed_state;
    novasvg_font_face_cache_t* face_cache;
    novasvg_rect_t clip_rect;
    novasvg_span_buffer_t clip_spans;
    novasvg_span_buffer_t fill_spans;
};

void novasvg_span_buffer_init(novasvg_span_buffer_t* span_buffer);
void novasvg_span_buffer_init_rect(novasvg_span_buffer_t* span_buffer, int x, int y, int width, int height);
void novasvg_span_buffer_reset(novasvg_span_buffer_t* span_buffer);
void novasvg_span_buffer_destroy(novasvg_span_buffer_t* span_buffer);
void novasvg_span_buffer_copy(novasvg_span_buffer_t* span_buffer, const novasvg_span_buffer_t* source);
bool novasvg_span_buffer_contains(const novasvg_span_buffer_t* span_buffer, float x, float y);
void novasvg_span_buffer_extents(novasvg_span_buffer_t* span_buffer, novasvg_rect_t* extents);
void novasvg_span_buffer_intersect(novasvg_span_buffer_t* span_buffer, const novasvg_span_buffer_t* a, const novasvg_span_buffer_t* b);

void novasvg_rasterize(novasvg_span_buffer_t* span_buffer, const novasvg_path_t* path, const novasvg_matrix_t* matrix, const novasvg_rect_t* clip_rect, const novasvg_stroke_data_t* stroke_data, novasvg_fill_rule_t winding);
void novasvg_blend(novasvg_canvas_t* canvas, const novasvg_span_buffer_t* span_buffer);
void novasvg_memfill32(unsigned int* dest, int length, unsigned int value);

#endif // NOVASVG_PRIVATE_H
