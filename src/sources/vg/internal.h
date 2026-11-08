#pragma once

#include <stb/outline.hpp>

#ifndef NOVA_PRIVATE_H
#define NOVA_PRIVATE_H

#include "render.h"

namespace nova {
namespace vg {

#if defined(_WIN32)

} // namespace vg
} // namespace nova
#include <windows.h>
namespace nova {
namespace vg {

typedef LONG ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count = 1)
#define novasvg_increment_reference(ob) (void)(ob && interlocked_increment(&(ob)->ref_count))
#define novasvg_destroy_reference(ob) (ob && interlocked_decrement(&(ob)->ref_count) == 0)
#define novasvg_get_reference_count(ob) ((ob) ? interlocked_compare_exchange((LONG*)&(ob)->ref_count, 0, 0) : 0)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)

} // namespace vg
} // namespace nova
#include <stdatomic.h>
namespace nova {
namespace vg {

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

typedef enum {
    NOVA_PAINT_TYPE_COLOR,
    NOVA_PAINT_TYPE_GRADIENT,
    NOVA_PAINT_TYPE_TEXTURE
} paint_type_t;

struct paint {
    ref_count_t ref_count;
    paint_type_t type_;
};

typedef struct {
    paint_t base;
    color_t color_;
} solid_paint_t;

typedef enum {
    NOVA_GRADIENT_TYPE_LINEAR,
    NOVA_GRADIENT_TYPE_RADIAL
} gradient_type_t;

typedef struct {
    paint_t base;
    gradient_type_t type_;
    spread_method spread;
    stb::matrix matrix;
    gradient_stop_t* stops;
    int nstops;
    float values[6];
} gradient_paint_t;

typedef struct {
    paint_t base;
    texture_type type_;
    float opacity;
    stb::matrix matrix;
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
        int size_;
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
        int size_;
        int capacity;
    } array;
} stroke_dash_t;

typedef struct {
    float width;
    line_cap cap;
    line_join join;
    float miter_limit;
} stroke_style_t;

typedef struct {
    stroke_style_t style;
    stroke_dash_t dash;
} stroke_data_t;

typedef struct state {
    paint_t* paint_;
    font_face_data* font_face_;
    color_t color_;
    stb::matrix matrix;
    stroke_data_t stroke;
    span_buffer_t clip_spans;
    fill_rule winding;
    blend_mode op;
    float font_size;
    float opacity;
    bool clipping;
    struct state* next;
} state_t;

struct canvas_context {
    ref_count_t ref_count;
    surface_t* surface;
    stb::outline* path_;
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

void arc_to_radians(stb::outline& path_, float rx, float ry, float angle, bool large_arc, bool sweep, float x, float y);
void add_rect_path(stb::outline& path_, float x, float y, float w, float h);
void add_round_rect_path(stb::outline& path_, float x, float y, float w, float h, float rx, float ry);
void add_ellipse_path(stb::outline& path_, float cx, float cy, float rx, float ry);
void rasterize(span_buffer_t* span_buffer, const stb::outline& path_, const stb::matrix* matrix, const rect_t* clip_rect, const stroke_data_t* stroke_data_, fill_rule winding);
void blend(canvas_t* canvas, const span_buffer_t* span_buffer);
void memfill32(unsigned int* dest, int length_, unsigned int value);

} // namespace vg
} // namespace nova

#endif // NOVA_PRIVATE_H
