#include "state.h"
#include "font_face.h"

namespace nova {
namespace vg {

static state_t* state_create(void);
static void state_reset(state_t* state);
static void state_copy(state_t* state, const state_t* source);
static void state_destroy(state_t* state);

int render_version(void)
{
    return NOVA_RENDER_VERSION;
}

const char* render_version_string(void)
{
    return NOVA_RENDER_VERSION_STRING;
}

static state_t* state_create(void)
{
    state_t* state = static_cast<state_t*>(malloc(sizeof(state_t)));
    state->paint_ = NULL;
    state->font_face_ = NULL;
    state->color_ = NOVA_BLACK_COLOR;
    state->matrix = NOVA_IDENTITY_MATRIX;
    state->stroke.style = NOVA_DEFAULT_STROKE_STYLE;
    state->stroke.dash.offset = 0.f;
    novasvg_array_init(state->stroke.dash.array);
    span_buffer_init(&state->clip_spans);
    state->winding = NOVA_FILL_RULE_NON_ZERO;
    state->op = NOVA_OPERATOR_SRC_OVER;
    state->font_size = 12.f;
    state->opacity = 1.f;
    state->clipping = false;
    state->next = NULL;
    return state;
}

static void state_reset(state_t* state)
{
    paint_destroy(state->paint_);
    font_face_destroy(state->font_face_);
    state->paint_ = NULL;
    state->font_face_ = NULL;
    state->color_ = NOVA_BLACK_COLOR;
    state->matrix = NOVA_IDENTITY_MATRIX;
    state->stroke.style = NOVA_DEFAULT_STROKE_STYLE;
    state->stroke.dash.offset = 0.f;
    novasvg_array_clear(state->stroke.dash.array);
    span_buffer_reset(&state->clip_spans);
    state->winding = NOVA_FILL_RULE_NON_ZERO;
    state->op = NOVA_OPERATOR_SRC_OVER;
    state->font_size = 12.f;
    state->opacity = 1.f;
    state->clipping = false;
}

static void state_copy(state_t* state, const state_t* source)
{
    state->paint_ = paint_reference(source->paint_);
    state->font_face_ = font_face_reference(source->font_face_);
    state->color_ = source->color_;
    state->matrix = source->matrix;
    state->stroke.style = source->stroke.style;
    state->stroke.dash.offset = source->stroke.dash.offset;
    novasvg_array_clear(state->stroke.dash.array);
    novasvg_array_append(state->stroke.dash.array, source->stroke.dash.array);
    span_buffer_copy(&state->clip_spans, &source->clip_spans);
    state->winding = source->winding;
    state->op = source->op;
    state->font_size = source->font_size;
    state->opacity = source->opacity;
    state->clipping = source->clipping;
}

static void state_destroy(state_t* state)
{
    paint_destroy(state->paint_);
    font_face_destroy(state->font_face_);
    novasvg_array_destroy(state->stroke.dash.array);
    span_buffer_destroy(&state->clip_spans);
    free(state);
}

canvas_t* canvas_create(surface_t* surface)
{
    canvas_t* canvas = static_cast<canvas_t*>(malloc(sizeof(canvas_t)));
    novasvg_init_reference(canvas);
    canvas->surface = surface_reference(surface);
    canvas->path_ = new stb::outline;
    canvas->state = state_create();
    canvas->freed_state = NULL;
    canvas->face_cache = NULL;
    canvas->clip_rect = NOVA_MAKE_RECT(0, 0, surface->width, surface->height);
    span_buffer_init(&canvas->clip_spans);
    span_buffer_init(&canvas->fill_spans);
    return canvas;
}

canvas_t* canvas_reference(canvas_t* canvas)
{
    novasvg_increment_reference(canvas);
    return canvas;
}

void canvas_destroy(canvas_t* canvas)
{
    if(novasvg_destroy_reference(canvas)) {
        while(canvas->state) {
            state_t* state = canvas->state;
            canvas->state = state->next;
            state_destroy(state);
        }

        while(canvas->freed_state) {
            state_t* state = canvas->freed_state;
            canvas->freed_state = state->next;
            state_destroy(state);
        }

        font_face_cache_destroy(canvas->face_cache);
        span_buffer_destroy(&canvas->fill_spans);
        span_buffer_destroy(&canvas->clip_spans);
        surface_destroy(canvas->surface);
        delete canvas->path_;
        free(canvas);
    }
}

int canvas_get_reference_count(const canvas_t* canvas)
{
    return novasvg_get_reference_count(canvas);
}

surface_t* canvas_get_surface(const canvas_t* canvas)
{
    return canvas->surface;
}

void canvas_save(canvas_t* canvas)
{
    state_t* new_state = canvas->freed_state;
    if(new_state == NULL)
        new_state = state_create();
    else
        canvas->freed_state = new_state->next;
    state_copy(new_state, canvas->state);
    new_state->next = canvas->state;
    canvas->state = new_state;
}

void canvas_restore(canvas_t* canvas)
{
    if(canvas->state->next == NULL)
        return;
    state_t* old_state = canvas->state;
    canvas->state = old_state->next;
    state_reset(old_state);
    old_state->next = canvas->freed_state;
    canvas->freed_state = old_state;
}

void canvas_set_rgb(canvas_t* canvas, float r, float g, float b)
{
    canvas_set_rgba(canvas, r, g, b, 1.f);
}

void canvas_set_rgba(canvas_t* canvas, float r, float g, float b, float a)
{
    color_init_rgba(&canvas->state->color_, r, g, b, a);
    canvas_set_paint(canvas, NULL);
}

void canvas_set_color(canvas_t* canvas, const color_t* color_)
{
    canvas_set_rgba(canvas, color_->r, color_->g, color_->b, color_->a);
}

void canvas_set_linear_gradient(canvas_t* canvas, float x1, float y1, float x2, float y2, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    paint_t* paint_ = paint_create_linear_gradient(x1, y1, x2, y2, spread, stops, nstops, matrix);
    canvas_set_paint(canvas, paint_);
    paint_destroy(paint_);
}

void canvas_set_radial_gradient(canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    paint_t* paint_ = paint_create_radial_gradient(cx, cy, cr, fx, fy, fr, spread, stops, nstops, matrix);
    canvas_set_paint(canvas, paint_);
    paint_destroy(paint_);
}

void canvas_set_texture(canvas_t* canvas, surface_t* surface, texture_type_t type_, float opacity, const matrix_t* matrix)
{
    paint_t* paint_ = paint_create_texture(surface, type_, opacity, matrix);
    canvas_set_paint(canvas, paint_);
    paint_destroy(paint_);
}

void canvas_set_paint(canvas_t* canvas, paint_t* paint_)
{
    paint_ = paint_reference(paint_);
    paint_destroy(canvas->state->paint_);
    canvas->state->paint_ = paint_;
}

paint_t* canvas_get_paint(const canvas_t* canvas, color_t* color_)
{
    if(color_)
        *color_ = canvas->state->color_;
    return canvas->state->paint_;
}

void canvas_set_font_face_cache(canvas_t* canvas, font_face_cache_t* cache)
{
    cache = font_face_cache_reference(cache);
    font_face_cache_destroy(canvas->face_cache);
    canvas->face_cache = cache;
}

font_face_cache_t* canvas_get_font_face_cache(const canvas_t* canvas)
{
    return canvas->face_cache;
}

void canvas_add_font_face(canvas_t* canvas, const char* family, bool bold, bool italic, font_face_t* face)
{
    if(canvas->face_cache == NULL)
        canvas->face_cache = font_face_cache_create();
    font_face_cache_add(canvas->face_cache, family, bold, italic, face);
}

bool canvas_add_font_file(canvas_t* canvas, const char* family, bool bold, bool italic, const char* filename, int ttcindex)
{
    if(canvas->face_cache == NULL)
        canvas->face_cache = font_face_cache_create();
    return font_face_cache_add_file(canvas->face_cache, family, bold, italic, filename, ttcindex);
}

bool canvas_select_font_face(canvas_t* canvas, const char* family, bool bold, bool italic)
{
    if(canvas->face_cache == NULL)
        return false;
    font_face_t* face = font_face_cache_get(canvas->face_cache, family, bold, italic);
    if(face == NULL)
        return false;
    canvas_set_font_face(canvas, face);
    return true;
}

void canvas_set_font(canvas_t* canvas, font_face_t* face, float size_)
{
    canvas_set_font_face(canvas, face);
    canvas_set_font_size(canvas, size_);
}

void canvas_set_font_face(canvas_t* canvas, font_face_t* face)
{
    face = font_face_reference(face);
    font_face_destroy(canvas->state->font_face_);
    canvas->state->font_face_ = face;
}

font_face_t* canvas_get_font_face(const canvas_t* canvas)
{
    return canvas->state->font_face_;
}

void canvas_set_font_size(canvas_t* canvas, float size_)
{
    canvas->state->font_size = size_;
}

float canvas_get_font_size(const canvas_t* canvas)
{
    return canvas->state->font_size;
}

void canvas_set_fill_rule(canvas_t* canvas, fill_rule_t winding)
{
    canvas->state->winding = winding;
}

fill_rule_t canvas_get_fill_rule(const canvas_t* canvas)
{
    return canvas->state->winding;
}

void canvas_set_operator(canvas_t* canvas, operator_t op)
{
    canvas->state->op = op;
}

operator_t canvas_get_operator(const canvas_t* canvas)
{
    return canvas->state->op;
}

void canvas_set_opacity(canvas_t* canvas, float opacity)
{
    canvas->state->opacity = std::clamp(opacity, 0.f, 1.f);
}

float canvas_get_opacity(const canvas_t* canvas)
{
    return canvas->state->opacity;
}

void canvas_set_line_width(canvas_t* canvas, float line_width)
{
    canvas->state->stroke.style.width = line_width;
}

float canvas_get_line_width(const canvas_t* canvas)
{
    return canvas->state->stroke.style.width;
}

void canvas_set_line_cap(canvas_t* canvas, line_cap_t line_cap_)
{
    canvas->state->stroke.style.cap = line_cap_;
}

line_cap_t canvas_get_line_cap(const canvas_t* canvas)
{
    return canvas->state->stroke.style.cap;
}

void canvas_set_line_join(canvas_t* canvas, line_join_t line_join_)
{
    canvas->state->stroke.style.join = line_join_;
}

line_join_t canvas_get_line_join(const canvas_t* canvas)
{
    return canvas->state->stroke.style.join;
}

void canvas_set_miter_limit(canvas_t* canvas, float miter_limit)
{
    canvas->state->stroke.style.miter_limit = miter_limit;
}

float canvas_get_miter_limit(const canvas_t* canvas)
{
    return canvas->state->stroke.style.miter_limit;
}

void canvas_set_dash(canvas_t* canvas, float offset, const float* dashes, int ndashes)
{
    canvas_set_dash_offset(canvas, offset);
    canvas_set_dash_array(canvas, dashes, ndashes);
}

void canvas_set_dash_offset(canvas_t* canvas, float offset)
{
    canvas->state->stroke.dash.offset = offset;
}

float canvas_get_dash_offset(const canvas_t* canvas)
{
    return canvas->state->stroke.dash.offset;
}

void canvas_set_dash_array(canvas_t* canvas, const float* dashes, int ndashes)
{
    novasvg_array_clear(canvas->state->stroke.dash.array);
    novasvg_array_append_data(canvas->state->stroke.dash.array, dashes, ndashes);
}

int canvas_get_dash_array(const canvas_t* canvas, const float** dashes)
{
    if(dashes)
        *dashes = canvas->state->stroke.dash.array.data;
    return canvas->state->stroke.dash.array.size_;
}

void canvas_translate(canvas_t* canvas, float tx, float ty)
{
    matrix_translate(&canvas->state->matrix, tx, ty);
}

void canvas_scale(canvas_t* canvas, float sx, float sy)
{
    matrix_scale(&canvas->state->matrix, sx, sy);
}

void canvas_shear(canvas_t* canvas, float shx, float shy)
{
    matrix_shear(&canvas->state->matrix, shx, shy);
}

void canvas_rotate(canvas_t* canvas, float angle)
{
    matrix_rotate(&canvas->state->matrix, angle);
}

void canvas_transform(canvas_t* canvas, const matrix_t* matrix)
{
    matrix_multiply(&canvas->state->matrix, matrix, &canvas->state->matrix);
}

void canvas_reset_matrix(canvas_t* canvas)
{
    matrix_init_identity(&canvas->state->matrix);
}

void canvas_set_matrix(canvas_t* canvas, const matrix_t* matrix)
{
    canvas->state->matrix = matrix ? *matrix : NOVA_IDENTITY_MATRIX;
}

void canvas_get_matrix(const canvas_t* canvas, matrix_t* matrix)
{
    *matrix = canvas->state->matrix;
}

void canvas_map(const canvas_t* canvas, float x, float y, float* xx, float* yy)
{
    matrix_map(&canvas->state->matrix, x, y, xx, yy);
}

void canvas_map_point(const canvas_t* canvas, const point_t* src, point_t* dst)
{
    matrix_map_point(&canvas->state->matrix, src, dst);
}

void canvas_map_rect(const canvas_t* canvas, const rect_t* src, rect_t* dst)
{
    matrix_map_rect(&canvas->state->matrix, src, dst);
}

void canvas_move_to(canvas_t* canvas, float x, float y)
{
    canvas->path_->move_to(x, y);
}

void canvas_line_to(canvas_t* canvas, float x, float y)
{
    canvas->path_->line_to(x, y);
}

void canvas_quad_to(canvas_t* canvas, float x1, float y1, float x2, float y2)
{
    canvas->path_->quad_to(x1, y1, x2, y2);
}

void canvas_cubic_to(canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3)
{
    canvas->path_->cubic_to(x1, y1, x2, y2, x3, y3);
}

void canvas_arc_to(canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y)
{
    arc_to_radians(*canvas->path_, rx, ry, angle, large_arc_flag, sweep_flag, x, y);
}

void canvas_rect(canvas_t* canvas, float x, float y, float w, float h)
{
    add_rect_path(*canvas->path_, x, y, w, h);
}

void canvas_round_rect(canvas_t* canvas, float x, float y, float w, float h, float rx, float ry)
{
    add_round_rect_path(*canvas->path_, x, y, w, h, rx, ry);
}

void canvas_ellipse(canvas_t* canvas, float cx, float cy, float rx, float ry)
{
    add_ellipse_path(*canvas->path_, cx, cy, rx, ry);
}

void canvas_circle(canvas_t* canvas, float cx, float cy, float r)
{
    add_ellipse_path(*canvas->path_, cx, cy, r, r);
}

void canvas_add_path(canvas_t* canvas, const stb::outline& path_)
{
    canvas->path_->add_outline(path_);
}

void canvas_new_path(canvas_t* canvas)
{
    canvas->path_->clear();
}

void canvas_close_path(canvas_t* canvas)
{
    canvas->path_->close();
}

void canvas_get_current_point(const canvas_t* canvas, float* x, float* y)
{
    { const stb::point p = canvas->path_->current_point().value_or(stb::point{0.f, 0.f}); if(x) *x = p.x; if(y) *y = p.y; }
}

bool canvas_fill_contains(canvas_t* canvas, float x, float y)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    return span_buffer_contains(&canvas->fill_spans, x, y);
}

bool canvas_stroke_contains(canvas_t* canvas, float x, float y)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    return span_buffer_contains(&canvas->fill_spans, x, y);
}

bool canvas_clip_contains(canvas_t* canvas, float x, float y)
{
    if(canvas->state->clipping) {
        return span_buffer_contains(&canvas->state->clip_spans, x, y);
    }

    float l = canvas->clip_rect.x;
    float t = canvas->clip_rect.y;
    float r = canvas->clip_rect.x + canvas->clip_rect.w;
    float b = canvas->clip_rect.y + canvas->clip_rect.h;

    return x >= l && x <= r && y >= t && y <= b;
}

void canvas_fill_extents(canvas_t *canvas, rect_t* extents)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    span_buffer_extents(&canvas->fill_spans, extents);
}

void canvas_stroke_extents(canvas_t *canvas, rect_t* extents)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, NULL, &canvas->state->stroke, NOVA_FILL_RULE_NON_ZERO);
    span_buffer_extents(&canvas->fill_spans, extents);
}

void canvas_clip_extents(canvas_t* canvas, rect_t* extents)
{
    if(canvas->state->clipping) {
        span_buffer_extents(&canvas->state->clip_spans, extents);
    } else {
        extents->x = canvas->clip_rect.x;
        extents->y = canvas->clip_rect.y;
        extents->w = canvas->clip_rect.w;
        extents->h = canvas->clip_rect.h;
    }
}

void canvas_fill(canvas_t* canvas)
{
    canvas_fill_preserve(canvas);
    canvas_new_path(canvas);
}

void canvas_stroke(canvas_t* canvas)
{
    canvas_stroke_preserve(canvas);
    canvas_new_path(canvas);
}

void canvas_clip(canvas_t* canvas)
{
    canvas_clip_preserve(canvas);
    canvas_new_path(canvas);
}

void canvas_paint(canvas_t* canvas)
{
    if(canvas->state->clipping) {
        blend(canvas, &canvas->state->clip_spans);
    } else {
        span_buffer_init_rect(&canvas->clip_spans, 0, 0, canvas->surface->width, canvas->surface->height);
        blend(canvas, &canvas->clip_spans);
    }
}

void canvas_fill_preserve(canvas_t* canvas)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
    if(canvas->state->clipping) {
        span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        blend(canvas, &canvas->clip_spans);
    } else {
        blend(canvas, &canvas->fill_spans);
    }
}

void canvas_stroke_preserve(canvas_t* canvas)
{
    rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, &canvas->clip_rect, &canvas->state->stroke, NOVA_FILL_RULE_NON_ZERO);
    if(canvas->state->clipping) {
        span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        blend(canvas, &canvas->clip_spans);
    } else {
        blend(canvas, &canvas->fill_spans);
    }
}

void canvas_clip_preserve(canvas_t* canvas)
{
    if(canvas->state->clipping) {
        rasterize(&canvas->fill_spans, *canvas->path_, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
        span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        span_buffer_copy(&canvas->state->clip_spans, &canvas->clip_spans);
    } else {
        rasterize(&canvas->state->clip_spans, *canvas->path_, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
        canvas->state->clipping = true;
    }
}

void canvas_fill_rect(canvas_t* canvas, float x, float y, float w, float h)
{
    canvas_new_path(canvas);
    canvas_rect(canvas, x, y, w, h);
    canvas_fill(canvas);
}

void canvas_fill_path(canvas_t* canvas, const stb::outline& path_)
{
    canvas_new_path(canvas);
    canvas_add_path(canvas, path_);
    canvas_fill(canvas);
}

void canvas_stroke_rect(canvas_t* canvas, float x, float y, float w, float h)
{
    canvas_new_path(canvas);
    canvas_rect(canvas, x, y, w, h);
    canvas_stroke(canvas);
}

void canvas_stroke_path(canvas_t* canvas, const stb::outline& path_)
{
    canvas_new_path(canvas);
    canvas_add_path(canvas, path_);
    canvas_stroke(canvas);
}

void canvas_clip_rect(canvas_t* canvas, float x, float y, float w, float h)
{
    canvas_new_path(canvas);
    canvas_rect(canvas, x, y, w, h);
    canvas_clip(canvas);
}

void canvas_clip_path(canvas_t* canvas, const stb::outline& path_)
{
    canvas_new_path(canvas);
    canvas_add_path(canvas, path_);
    canvas_clip(canvas);
}

float canvas_add_glyph(canvas_t* canvas, codepoint_t codepoint, float x, float y)
{
    state_t* state = canvas->state;
    if(state->font_face_ && state->font_size > 0.f)
        return font_face_get_glyph_path(state->font_face_, state->font_size, x, y, codepoint, *canvas->path_);
    return 0.f;
}

float canvas_add_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y)
{
    state_t* state = canvas->state;
    if(state->font_face_ == NULL || state->font_size <= 0.f)
        return 0.f;
    std::vector<shaped_glyph_t> glyphs;
    const float advance_width = font_face_shape(state->font_face_, state->font_size, text, length_, encoding, &glyphs);
    for(const shaped_glyph_t& g : glyphs)
        font_face_get_glyph_index_path(state->font_face_, state->font_size, x + g.x, y + g.y, g.glyph, *canvas->path_);
    return advance_width;
}

float canvas_fill_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y)
{
    canvas_new_path(canvas);
    float advance_width = canvas_add_text(canvas, text, length_, encoding, x, y);
    canvas_fill(canvas);
    return advance_width;
}

float canvas_stroke_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y)
{
    canvas_new_path(canvas);
    float advance_width = canvas_add_text(canvas, text, length_, encoding, x, y);
    canvas_stroke(canvas);
    return advance_width;
}

float canvas_clip_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y)
{
    canvas_new_path(canvas);
    float advance_width = canvas_add_text(canvas, text, length_, encoding, x, y);
    canvas_clip(canvas);
    return advance_width;
}

void canvas_font_metrics(const canvas_t* canvas, float* ascent, float* descent, float* line_gap, rect_t* extents)
{
    state_t* state = canvas->state;
    if(state->font_face_ && state->font_size > 0.f) {
        font_face_get_metrics(state->font_face_, state->font_size, ascent, descent, line_gap, extents);
        return;
    }

    if(ascent) *ascent = 0.f;
    if(descent) *descent = 0.f;
    if(line_gap) *line_gap = 0.f;
    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }
}

void canvas_glyph_metrics(canvas_t* canvas, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents)
{
    state_t* state = canvas->state;
    if(state->font_face_ && state->font_size > 0.f) {
        font_face_get_glyph_metrics(state->font_face_, state->font_size, codepoint, advance_width, left_side_bearing, extents);
        return;
    }

    if(advance_width) *advance_width = 0.f;
    if(left_side_bearing) *left_side_bearing = 0.f;
    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }
}

float canvas_text_extents(canvas_t* canvas, const void* text, int length_, text_encoding encoding, rect_t* extents)
{
    state_t* state = canvas->state;
    if(state->font_face_ && state->font_size > 0.f) {
        return font_face_text_extents(state->font_face_, state->font_size, text, length_, encoding, extents);
    }

    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }

    return 0.f;
}

} // namespace vg
} // namespace nova
