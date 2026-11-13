#pragma once

#include <stb/outline.hpp>

#include <atomic>

#ifndef NOVA_PRIVATE_H
#define NOVA_PRIVATE_H

#include "render.h"

namespace nova {
namespace vg {

// Reference counts are atomic: the objects are shared by reference between canvases, paints and the font cache.
typedef std::atomic<int> ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count.store(1))
#define novasvg_increment_reference(ob) (void)(ob && (ob)->ref_count.fetch_add(1))
#define novasvg_destroy_reference(ob) (ob && (ob)->ref_count.fetch_sub(1) == 1)
#define novasvg_get_reference_count(ob) ((ob) ? (ob)->ref_count.load() : 0)

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
    stb::color color_;
} solid_paint_t;

typedef enum {
    NOVA_GRADIENT_TYPE_LINEAR,
    NOVA_GRADIENT_TYPE_RADIAL
} gradient_type_t;

typedef struct {
    paint_t base;
    gradient_type_t type_;
    spread_method_value spread;
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
    std::shared_ptr<surface_t> surface;  // constructed in place: the paint is malloc'd
} texture_paint_t;

typedef struct state {
    paint_t* paint_;
    font_face_data* font_face_;
    stb::color color_;
    stb::matrix matrix;
    stb::stroke_style stroke;
    stb::span_set clip_spans;
    fill_rule winding;
    stb::composite_op op;
    stb::blend_mode mode;
    float font_size;
    float opacity;
    bool clipping;
    struct state* next;
} state_t;

struct canvas_context {
    ref_count_t ref_count;
    std::shared_ptr<surface_t> surface;  // constructed in place: the canvas is malloc'd
    stb::outline* path_;
    state_t* state;
    state_t* freed_state;
    font_face_cache_t* face_cache;
    rect_t clip_rect;
    stb::span_set clip_spans;
    stb::span_set fill_spans;
};


void rasterize(stb::span_set* span_buffer, const stb::outline& path_, const stb::matrix* matrix, const rect_t* clip_rect, const stb::stroke_style* stroke_, fill_rule winding);
void blend(canvas_t* canvas, const stb::span_set* span_buffer);

} // namespace vg
} // namespace nova

#endif // NOVA_PRIVATE_H
