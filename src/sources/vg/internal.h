#pragma once

#include <stb/outline.hpp>

#include <atomic>
#include <memory>
#include <variant>
#include <vector>

#ifndef NOVA_PRIVATE_H
#define NOVA_PRIVATE_H

#include "render.h"
#include "font.h"

namespace nova {
namespace vg {

// Reference counts are atomic: the objects are shared by reference between canvases, paints and the font cache.
typedef std::atomic<int> ref_count_t;

#define novasvg_init_reference(ob) ((ob)->ref_count.store(1))
#define novasvg_increment_reference(ob) (void)(ob && (ob)->ref_count.fetch_add(1))
#define novasvg_destroy_reference(ob) (ob && (ob)->ref_count.fetch_sub(1) == 1)
#define novasvg_get_reference_count(ob) ((ob) ? (ob)->ref_count.load() : 0)

// What a canvas fills with besides its plain color: a gradient or a texture. Immutable and shared, so a saved state
// and the canvas hold the same one.
struct gradient_paint {
    enum class kind { linear, radial };
    kind type_ = kind::linear;
    spread_method_value spread{};
    stb::matrix matrix;
    std::vector<gradient_stop_t> stops;  // offsets within [0, 1], never decreasing
    float values[6] = {};                // linear: x1 y1 x2 y2; radial: cx cy cr fx fy fr
};

struct texture_paint {
    texture_type type_{};
    float opacity = 1.f;
    stb::matrix matrix;
    std::shared_ptr<surface_t> surface;
};

using paint_source = std::variant<gradient_paint, texture_paint>;

typedef struct state {
    std::shared_ptr<const paint_source> paint_;  // none: fill with color_
    font_face font_face_;
    color color_;
    stb::matrix matrix;
    stb::stroke_style stroke;
    span_set clip_spans;
    fill_rule winding;
    composite_op op;
    blend_mode mode;
    float font_size;
    float opacity;
    bool clipping;
    struct state* next;
} state_t;

struct canvas_context {
    ref_count_t ref_count;
    std::shared_ptr<surface_t> surface;  // constructed in place: the canvas is malloc'd
    outline* path_;
    state_t* state;
    state_t* freed_state;
    rect_t clip_rect;
    span_set clip_spans;
    span_set fill_spans;
};


void rasterize(span_set* span_buffer, const outline& path_, const stb::matrix* matrix, const rect_t* clip_rect, const stb::stroke_style* stroke_, fill_rule winding);
void blend(canvas_t* canvas, const span_set* span_buffer);

} // namespace vg
} // namespace nova

#endif // NOVA_PRIVATE_H
