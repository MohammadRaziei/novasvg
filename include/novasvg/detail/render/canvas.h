#pragma once

#include "private.h"
#include "utils.h"

NOVASVG_INLINE int novasvg_render_version(void)
{
    return NOVASVG_RENDER_VERSION;
}

NOVASVG_INLINE const char* novasvg_render_version_string(void)
{
    return NOVASVG_RENDER_VERSION_STRING;
}

#define NOVASVG_DEFAULT_STROKE_STYLE ((novasvg_stroke_style_t){1.f, NOVASVG_LINE_CAP_BUTT, NOVASVG_LINE_JOIN_MITER, 10.f})

static novasvg_state_t* novasvg_state_create(void)
{
    novasvg_state_t* state = static_cast<novasvg_state_t*>(malloc(sizeof(novasvg_state_t)));
    state->paint = NULL;
    state->font_face = NULL;
    state->color = NOVASVG_BLACK_COLOR;
    state->matrix = NOVASVG_IDENTITY_MATRIX;
    state->stroke.style = NOVASVG_DEFAULT_STROKE_STYLE;
    state->stroke.dash.offset = 0.f;
    novasvg_array_init(state->stroke.dash.array);
    novasvg_span_buffer_init(&state->clip_spans);
    state->winding = NOVASVG_FILL_RULE_NON_ZERO;
    state->op = NOVASVG_OPERATOR_SRC_OVER;
    state->font_size = 12.f;
    state->opacity = 1.f;
    state->clipping = false;
    state->next = NULL;
    return state;
}

static void novasvg_state_reset(novasvg_state_t* state)
{
    novasvg_paint_destroy(state->paint);
    novasvg_font_face_destroy(state->font_face);
    state->paint = NULL;
    state->font_face = NULL;
    state->color = NOVASVG_BLACK_COLOR;
    state->matrix = NOVASVG_IDENTITY_MATRIX;
    state->stroke.style = NOVASVG_DEFAULT_STROKE_STYLE;
    state->stroke.dash.offset = 0.f;
    novasvg_array_clear(state->stroke.dash.array);
    novasvg_span_buffer_reset(&state->clip_spans);
    state->winding = NOVASVG_FILL_RULE_NON_ZERO;
    state->op = NOVASVG_OPERATOR_SRC_OVER;
    state->font_size = 12.f;
    state->opacity = 1.f;
    state->clipping = false;
}

static void novasvg_state_copy(novasvg_state_t* state, const novasvg_state_t* source)
{
    state->paint = novasvg_paint_reference(source->paint);
    state->font_face = novasvg_font_face_reference(source->font_face);
    state->color = source->color;
    state->matrix = source->matrix;
    state->stroke.style = source->stroke.style;
    state->stroke.dash.offset = source->stroke.dash.offset;
    novasvg_array_clear(state->stroke.dash.array);
    novasvg_array_append(state->stroke.dash.array, source->stroke.dash.array);
    novasvg_span_buffer_copy(&state->clip_spans, &source->clip_spans);
    state->winding = source->winding;
    state->op = source->op;
    state->font_size = source->font_size;
    state->opacity = source->opacity;
    state->clipping = source->clipping;
}

static void novasvg_state_destroy(novasvg_state_t* state)
{
    novasvg_paint_destroy(state->paint);
    novasvg_font_face_destroy(state->font_face);
    novasvg_array_destroy(state->stroke.dash.array);
    novasvg_span_buffer_destroy(&state->clip_spans);
    free(state);
}

NOVASVG_INLINE novasvg_canvas_t* novasvg_canvas_create(novasvg_surface_t* surface)
{
    novasvg_canvas_t* canvas = static_cast<novasvg_canvas_t*>(malloc(sizeof(novasvg_canvas_t)));
    novasvg_init_reference(canvas);
    canvas->surface = novasvg_surface_reference(surface);
    canvas->path = novasvg_path_create();
    canvas->state = novasvg_state_create();
    canvas->freed_state = NULL;
    canvas->face_cache = NULL;
    canvas->clip_rect = NOVASVG_MAKE_RECT(0, 0, surface->width, surface->height);
    novasvg_span_buffer_init(&canvas->clip_spans);
    novasvg_span_buffer_init(&canvas->fill_spans);
    return canvas;
}

NOVASVG_INLINE novasvg_canvas_t* novasvg_canvas_reference(novasvg_canvas_t* canvas)
{
    novasvg_increment_reference(canvas);
    return canvas;
}

NOVASVG_INLINE void novasvg_canvas_destroy(novasvg_canvas_t* canvas)
{
    if(novasvg_destroy_reference(canvas)) {
        while(canvas->state) {
            novasvg_state_t* state = canvas->state;
            canvas->state = state->next;
            novasvg_state_destroy(state);
        }

        while(canvas->freed_state) {
            novasvg_state_t* state = canvas->freed_state;
            canvas->freed_state = state->next;
            novasvg_state_destroy(state);
        }

        novasvg_font_face_cache_destroy(canvas->face_cache);
        novasvg_span_buffer_destroy(&canvas->fill_spans);
        novasvg_span_buffer_destroy(&canvas->clip_spans);
        novasvg_surface_destroy(canvas->surface);
        novasvg_path_destroy(canvas->path);
        free(canvas);
    }
}

NOVASVG_INLINE int novasvg_canvas_get_reference_count(const novasvg_canvas_t* canvas)
{
    return novasvg_get_reference_count(canvas);
}

NOVASVG_INLINE novasvg_surface_t* novasvg_canvas_get_surface(const novasvg_canvas_t* canvas)
{
    return canvas->surface;
}

NOVASVG_INLINE void novasvg_canvas_save(novasvg_canvas_t* canvas)
{
    novasvg_state_t* new_state = canvas->freed_state;
    if(new_state == NULL)
        new_state = novasvg_state_create();
    else
        canvas->freed_state = new_state->next;
    novasvg_state_copy(new_state, canvas->state);
    new_state->next = canvas->state;
    canvas->state = new_state;
}

NOVASVG_INLINE void novasvg_canvas_restore(novasvg_canvas_t* canvas)
{
    if(canvas->state->next == NULL)
        return;
    novasvg_state_t* old_state = canvas->state;
    canvas->state = old_state->next;
    novasvg_state_reset(old_state);
    old_state->next = canvas->freed_state;
    canvas->freed_state = old_state;
}

NOVASVG_INLINE void novasvg_canvas_set_rgb(novasvg_canvas_t* canvas, float r, float g, float b)
{
    novasvg_canvas_set_rgba(canvas, r, g, b, 1.f);
}

NOVASVG_INLINE void novasvg_canvas_set_rgba(novasvg_canvas_t* canvas, float r, float g, float b, float a)
{
    novasvg_color_init_rgba(&canvas->state->color, r, g, b, a);
    novasvg_canvas_set_paint(canvas, NULL);
}

NOVASVG_INLINE void novasvg_canvas_set_color(novasvg_canvas_t* canvas, const novasvg_color_t* color)
{
    novasvg_canvas_set_rgba(canvas, color->r, color->g, color->b, color->a);
}

NOVASVG_INLINE void novasvg_canvas_set_linear_gradient(novasvg_canvas_t* canvas, float x1, float y1, float x2, float y2, novasvg_spread_method_t spread, const novasvg_gradient_stop_t* stops, int nstops, const novasvg_matrix_t* matrix)
{
    novasvg_paint_t* paint = novasvg_paint_create_linear_gradient(x1, y1, x2, y2, spread, stops, nstops, matrix);
    novasvg_canvas_set_paint(canvas, paint);
    novasvg_paint_destroy(paint);
}

NOVASVG_INLINE void novasvg_canvas_set_radial_gradient(novasvg_canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr, novasvg_spread_method_t spread, const novasvg_gradient_stop_t* stops, int nstops, const novasvg_matrix_t* matrix)
{
    novasvg_paint_t* paint = novasvg_paint_create_radial_gradient(cx, cy, cr, fx, fy, fr, spread, stops, nstops, matrix);
    novasvg_canvas_set_paint(canvas, paint);
    novasvg_paint_destroy(paint);
}

NOVASVG_INLINE void novasvg_canvas_set_texture(novasvg_canvas_t* canvas, novasvg_surface_t* surface, novasvg_texture_type_t type, float opacity, const novasvg_matrix_t* matrix)
{
    novasvg_paint_t* paint = novasvg_paint_create_texture(surface, type, opacity, matrix);
    novasvg_canvas_set_paint(canvas, paint);
    novasvg_paint_destroy(paint);
}

NOVASVG_INLINE void novasvg_canvas_set_paint(novasvg_canvas_t* canvas, novasvg_paint_t* paint)
{
    paint = novasvg_paint_reference(paint);
    novasvg_paint_destroy(canvas->state->paint);
    canvas->state->paint = paint;
}

NOVASVG_INLINE novasvg_paint_t* novasvg_canvas_get_paint(const novasvg_canvas_t* canvas, novasvg_color_t* color)
{
    if(color)
        *color = canvas->state->color;
    return canvas->state->paint;
}

NOVASVG_INLINE void novasvg_canvas_set_font_face_cache(novasvg_canvas_t* canvas, novasvg_font_face_cache_t* cache)
{
    cache = novasvg_font_face_cache_reference(cache);
    novasvg_font_face_cache_destroy(canvas->face_cache);
    canvas->face_cache = cache;
}

NOVASVG_INLINE novasvg_font_face_cache_t* novasvg_canvas_get_font_face_cache(const novasvg_canvas_t* canvas)
{
    return canvas->face_cache;
}

NOVASVG_INLINE void novasvg_canvas_add_font_face(novasvg_canvas_t* canvas, const char* family, bool bold, bool italic, novasvg_font_face_t* face)
{
    if(canvas->face_cache == NULL)
        canvas->face_cache = novasvg_font_face_cache_create();
    novasvg_font_face_cache_add(canvas->face_cache, family, bold, italic, face);
}

NOVASVG_INLINE bool novasvg_canvas_add_font_file(novasvg_canvas_t* canvas, const char* family, bool bold, bool italic, const char* filename, int ttcindex)
{
    if(canvas->face_cache == NULL)
        canvas->face_cache = novasvg_font_face_cache_create();
    return novasvg_font_face_cache_add_file(canvas->face_cache, family, bold, italic, filename, ttcindex);
}

NOVASVG_INLINE bool novasvg_canvas_select_font_face(novasvg_canvas_t* canvas, const char* family, bool bold, bool italic)
{
    if(canvas->face_cache == NULL)
        return false;
    novasvg_font_face_t* face = novasvg_font_face_cache_get(canvas->face_cache, family, bold, italic);
    if(face == NULL)
        return false;
    novasvg_canvas_set_font_face(canvas, face);
    return true;
}

NOVASVG_INLINE void novasvg_canvas_set_font(novasvg_canvas_t* canvas, novasvg_font_face_t* face, float size)
{
    novasvg_canvas_set_font_face(canvas, face);
    novasvg_canvas_set_font_size(canvas, size);
}

NOVASVG_INLINE void novasvg_canvas_set_font_face(novasvg_canvas_t* canvas, novasvg_font_face_t* face)
{
    face = novasvg_font_face_reference(face);
    novasvg_font_face_destroy(canvas->state->font_face);
    canvas->state->font_face = face;
}

NOVASVG_INLINE novasvg_font_face_t* novasvg_canvas_get_font_face(const novasvg_canvas_t* canvas)
{
    return canvas->state->font_face;
}

NOVASVG_INLINE void novasvg_canvas_set_font_size(novasvg_canvas_t* canvas, float size)
{
    canvas->state->font_size = size;
}

NOVASVG_INLINE float novasvg_canvas_get_font_size(const novasvg_canvas_t* canvas)
{
    return canvas->state->font_size;
}

NOVASVG_INLINE void novasvg_canvas_set_fill_rule(novasvg_canvas_t* canvas, novasvg_fill_rule_t winding)
{
    canvas->state->winding = winding;
}

NOVASVG_INLINE novasvg_fill_rule_t novasvg_canvas_get_fill_rule(const novasvg_canvas_t* canvas)
{
    return canvas->state->winding;
}

NOVASVG_INLINE void novasvg_canvas_set_operator(novasvg_canvas_t* canvas, novasvg_operator_t op)
{
    canvas->state->op = op;
}

NOVASVG_INLINE novasvg_operator_t novasvg_canvas_get_operator(const novasvg_canvas_t* canvas)
{
    return canvas->state->op;
}

NOVASVG_INLINE void novasvg_canvas_set_opacity(novasvg_canvas_t* canvas, float opacity)
{
    canvas->state->opacity = novasvg_clamp(opacity, 0.f, 1.f);
}

NOVASVG_INLINE float novasvg_canvas_get_opacity(const novasvg_canvas_t* canvas)
{
    return canvas->state->opacity;
}

NOVASVG_INLINE void novasvg_canvas_set_line_width(novasvg_canvas_t* canvas, float line_width)
{
    canvas->state->stroke.style.width = line_width;
}

NOVASVG_INLINE float novasvg_canvas_get_line_width(const novasvg_canvas_t* canvas)
{
    return canvas->state->stroke.style.width;
}

NOVASVG_INLINE void novasvg_canvas_set_line_cap(novasvg_canvas_t* canvas, novasvg_line_cap_t line_cap)
{
    canvas->state->stroke.style.cap = line_cap;
}

NOVASVG_INLINE novasvg_line_cap_t novasvg_canvas_get_line_cap(const novasvg_canvas_t* canvas)
{
    return canvas->state->stroke.style.cap;
}

NOVASVG_INLINE void novasvg_canvas_set_line_join(novasvg_canvas_t* canvas, novasvg_line_join_t line_join)
{
    canvas->state->stroke.style.join = line_join;
}

NOVASVG_INLINE novasvg_line_join_t novasvg_canvas_get_line_join(const novasvg_canvas_t* canvas)
{
    return canvas->state->stroke.style.join;
}

NOVASVG_INLINE void novasvg_canvas_set_miter_limit(novasvg_canvas_t* canvas, float miter_limit)
{
    canvas->state->stroke.style.miter_limit = miter_limit;
}

NOVASVG_INLINE float novasvg_canvas_get_miter_limit(const novasvg_canvas_t* canvas)
{
    return canvas->state->stroke.style.miter_limit;
}

NOVASVG_INLINE void novasvg_canvas_set_dash(novasvg_canvas_t* canvas, float offset, const float* dashes, int ndashes)
{
    novasvg_canvas_set_dash_offset(canvas, offset);
    novasvg_canvas_set_dash_array(canvas, dashes, ndashes);
}

NOVASVG_INLINE void novasvg_canvas_set_dash_offset(novasvg_canvas_t* canvas, float offset)
{
    canvas->state->stroke.dash.offset = offset;
}

NOVASVG_INLINE float novasvg_canvas_get_dash_offset(const novasvg_canvas_t* canvas)
{
    return canvas->state->stroke.dash.offset;
}

NOVASVG_INLINE void novasvg_canvas_set_dash_array(novasvg_canvas_t* canvas, const float* dashes, int ndashes)
{
    novasvg_array_clear(canvas->state->stroke.dash.array);
    novasvg_array_append_data(canvas->state->stroke.dash.array, dashes, ndashes);
}

NOVASVG_INLINE int novasvg_canvas_get_dash_array(const novasvg_canvas_t* canvas, const float** dashes)
{
    if(dashes)
        *dashes = canvas->state->stroke.dash.array.data;
    return canvas->state->stroke.dash.array.size;
}

NOVASVG_INLINE void novasvg_canvas_translate(novasvg_canvas_t* canvas, float tx, float ty)
{
    novasvg_matrix_translate(&canvas->state->matrix, tx, ty);
}

NOVASVG_INLINE void novasvg_canvas_scale(novasvg_canvas_t* canvas, float sx, float sy)
{
    novasvg_matrix_scale(&canvas->state->matrix, sx, sy);
}

NOVASVG_INLINE void novasvg_canvas_shear(novasvg_canvas_t* canvas, float shx, float shy)
{
    novasvg_matrix_shear(&canvas->state->matrix, shx, shy);
}

NOVASVG_INLINE void novasvg_canvas_rotate(novasvg_canvas_t* canvas, float angle)
{
    novasvg_matrix_rotate(&canvas->state->matrix, angle);
}

NOVASVG_INLINE void novasvg_canvas_transform(novasvg_canvas_t* canvas, const novasvg_matrix_t* matrix)
{
    novasvg_matrix_multiply(&canvas->state->matrix, matrix, &canvas->state->matrix);
}

NOVASVG_INLINE void novasvg_canvas_reset_matrix(novasvg_canvas_t* canvas)
{
    novasvg_matrix_init_identity(&canvas->state->matrix);
}

NOVASVG_INLINE void novasvg_canvas_set_matrix(novasvg_canvas_t* canvas, const novasvg_matrix_t* matrix)
{
    canvas->state->matrix = matrix ? *matrix : NOVASVG_IDENTITY_MATRIX;
}

NOVASVG_INLINE void novasvg_canvas_get_matrix(const novasvg_canvas_t* canvas, novasvg_matrix_t* matrix)
{
    *matrix = canvas->state->matrix;
}

NOVASVG_INLINE void novasvg_canvas_map(const novasvg_canvas_t* canvas, float x, float y, float* xx, float* yy)
{
    novasvg_matrix_map(&canvas->state->matrix, x, y, xx, yy);
}

NOVASVG_INLINE void novasvg_canvas_map_point(const novasvg_canvas_t* canvas, const novasvg_point_t* src, novasvg_point_t* dst)
{
    novasvg_matrix_map_point(&canvas->state->matrix, src, dst);
}

NOVASVG_INLINE void novasvg_canvas_map_rect(const novasvg_canvas_t* canvas, const novasvg_rect_t* src, novasvg_rect_t* dst)
{
    novasvg_matrix_map_rect(&canvas->state->matrix, src, dst);
}

NOVASVG_INLINE void novasvg_canvas_move_to(novasvg_canvas_t* canvas, float x, float y)
{
    novasvg_path_move_to(canvas->path, x, y);
}

NOVASVG_INLINE void novasvg_canvas_line_to(novasvg_canvas_t* canvas, float x, float y)
{
    novasvg_path_line_to(canvas->path, x, y);
}

NOVASVG_INLINE void novasvg_canvas_quad_to(novasvg_canvas_t* canvas, float x1, float y1, float x2, float y2)
{
    novasvg_path_quad_to(canvas->path, x1, y1, x2, y2);
}

NOVASVG_INLINE void novasvg_canvas_cubic_to(novasvg_canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3)
{
    novasvg_path_cubic_to(canvas->path, x1, y1, x2, y2, x3, y3);
}

NOVASVG_INLINE void novasvg_canvas_arc_to(novasvg_canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y)
{
    novasvg_path_arc_to(canvas->path, rx, ry, angle, large_arc_flag, sweep_flag, x, y);
}

NOVASVG_INLINE void novasvg_canvas_rect(novasvg_canvas_t* canvas, float x, float y, float w, float h)
{
    novasvg_path_add_rect(canvas->path, x, y, w, h);
}

NOVASVG_INLINE void novasvg_canvas_round_rect(novasvg_canvas_t* canvas, float x, float y, float w, float h, float rx, float ry)
{
    novasvg_path_add_round_rect(canvas->path, x, y, w, h, rx, ry);
}

NOVASVG_INLINE void novasvg_canvas_ellipse(novasvg_canvas_t* canvas, float cx, float cy, float rx, float ry)
{
    novasvg_path_add_ellipse(canvas->path, cx, cy, rx, ry);
}

NOVASVG_INLINE void novasvg_canvas_circle(novasvg_canvas_t* canvas, float cx, float cy, float r)
{
    novasvg_path_add_circle(canvas->path, cx, cy, r);
}

NOVASVG_INLINE void novasvg_canvas_arc(novasvg_canvas_t* canvas, float cx, float cy, float r, float a0, float a1, bool ccw)
{
    novasvg_path_add_arc(canvas->path, cx, cy, r, a0, a1, ccw);
}

NOVASVG_INLINE void novasvg_canvas_add_path(novasvg_canvas_t* canvas, const novasvg_path_t* path)
{
    novasvg_path_add_path(canvas->path, path, NULL);
}

NOVASVG_INLINE void novasvg_canvas_new_path(novasvg_canvas_t* canvas)
{
    novasvg_path_reset(canvas->path);
}

NOVASVG_INLINE void novasvg_canvas_close_path(novasvg_canvas_t* canvas)
{
    novasvg_path_close(canvas->path);
}

NOVASVG_INLINE void novasvg_canvas_get_current_point(const novasvg_canvas_t* canvas, float* x, float* y)
{
    novasvg_path_get_current_point(canvas->path, x, y);
}

NOVASVG_INLINE novasvg_path_t* novasvg_canvas_get_path(const novasvg_canvas_t* canvas)
{
    return canvas->path;
}

NOVASVG_INLINE bool novasvg_canvas_fill_contains(novasvg_canvas_t* canvas, float x, float y)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    return novasvg_span_buffer_contains(&canvas->fill_spans, x, y);
}

NOVASVG_INLINE bool novasvg_canvas_stroke_contains(novasvg_canvas_t* canvas, float x, float y)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    return novasvg_span_buffer_contains(&canvas->fill_spans, x, y);
}

NOVASVG_INLINE bool novasvg_canvas_clip_contains(novasvg_canvas_t* canvas, float x, float y)
{
    if(canvas->state->clipping) {
        return novasvg_span_buffer_contains(&canvas->state->clip_spans, x, y);
    }

    float l = canvas->clip_rect.x;
    float t = canvas->clip_rect.y;
    float r = canvas->clip_rect.x + canvas->clip_rect.w;
    float b = canvas->clip_rect.y + canvas->clip_rect.h;

    return x >= l && x <= r && y >= t && y <= b;
}

NOVASVG_INLINE void novasvg_canvas_fill_extents(novasvg_canvas_t *canvas, novasvg_rect_t* extents)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, NULL, NULL, canvas->state->winding);
    novasvg_span_buffer_extents(&canvas->fill_spans, extents);
}

NOVASVG_INLINE void novasvg_canvas_stroke_extents(novasvg_canvas_t *canvas, novasvg_rect_t* extents)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, NULL, &canvas->state->stroke, NOVASVG_FILL_RULE_NON_ZERO);
    novasvg_span_buffer_extents(&canvas->fill_spans, extents);
}

NOVASVG_INLINE void novasvg_canvas_clip_extents(novasvg_canvas_t* canvas, novasvg_rect_t* extents)
{
    if(canvas->state->clipping) {
        novasvg_span_buffer_extents(&canvas->state->clip_spans, extents);
    } else {
        extents->x = canvas->clip_rect.x;
        extents->y = canvas->clip_rect.y;
        extents->w = canvas->clip_rect.w;
        extents->h = canvas->clip_rect.h;
    }
}

NOVASVG_INLINE void novasvg_canvas_fill(novasvg_canvas_t* canvas)
{
    novasvg_canvas_fill_preserve(canvas);
    novasvg_canvas_new_path(canvas);
}

NOVASVG_INLINE void novasvg_canvas_stroke(novasvg_canvas_t* canvas)
{
    novasvg_canvas_stroke_preserve(canvas);
    novasvg_canvas_new_path(canvas);
}

NOVASVG_INLINE void novasvg_canvas_clip(novasvg_canvas_t* canvas)
{
    novasvg_canvas_clip_preserve(canvas);
    novasvg_canvas_new_path(canvas);
}

NOVASVG_INLINE void novasvg_canvas_paint(novasvg_canvas_t* canvas)
{
    if(canvas->state->clipping) {
        novasvg_blend(canvas, &canvas->state->clip_spans);
    } else {
        novasvg_span_buffer_init_rect(&canvas->clip_spans, 0, 0, canvas->surface->width, canvas->surface->height);
        novasvg_blend(canvas, &canvas->clip_spans);
    }
}

NOVASVG_INLINE void novasvg_canvas_fill_preserve(novasvg_canvas_t* canvas)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
    if(canvas->state->clipping) {
        novasvg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        novasvg_blend(canvas, &canvas->clip_spans);
    } else {
        novasvg_blend(canvas, &canvas->fill_spans);
    }
}

NOVASVG_INLINE void novasvg_canvas_stroke_preserve(novasvg_canvas_t* canvas)
{
    novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, &canvas->clip_rect, &canvas->state->stroke, NOVASVG_FILL_RULE_NON_ZERO);
    if(canvas->state->clipping) {
        novasvg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        novasvg_blend(canvas, &canvas->clip_spans);
    } else {
        novasvg_blend(canvas, &canvas->fill_spans);
    }
}

NOVASVG_INLINE void novasvg_canvas_clip_preserve(novasvg_canvas_t* canvas)
{
    if(canvas->state->clipping) {
        novasvg_rasterize(&canvas->fill_spans, canvas->path, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
        novasvg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &canvas->state->clip_spans);
        novasvg_span_buffer_copy(&canvas->state->clip_spans, &canvas->clip_spans);
    } else {
        novasvg_rasterize(&canvas->state->clip_spans, canvas->path, &canvas->state->matrix, &canvas->clip_rect, NULL, canvas->state->winding);
        canvas->state->clipping = true;
    }
}

NOVASVG_INLINE void novasvg_canvas_fill_rect(novasvg_canvas_t* canvas, float x, float y, float w, float h)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_rect(canvas, x, y, w, h);
    novasvg_canvas_fill(canvas);
}

NOVASVG_INLINE void novasvg_canvas_fill_path(novasvg_canvas_t* canvas, const novasvg_path_t* path)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_add_path(canvas, path);
    novasvg_canvas_fill(canvas);
}

NOVASVG_INLINE void novasvg_canvas_stroke_rect(novasvg_canvas_t* canvas, float x, float y, float w, float h)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_rect(canvas, x, y, w, h);
    novasvg_canvas_stroke(canvas);
}

NOVASVG_INLINE void novasvg_canvas_stroke_path(novasvg_canvas_t* canvas, const novasvg_path_t* path)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_add_path(canvas, path);
    novasvg_canvas_stroke(canvas);
}

NOVASVG_INLINE void novasvg_canvas_clip_rect(novasvg_canvas_t* canvas, float x, float y, float w, float h)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_rect(canvas, x, y, w, h);
    novasvg_canvas_clip(canvas);
}

NOVASVG_INLINE void novasvg_canvas_clip_path(novasvg_canvas_t* canvas, const novasvg_path_t* path)
{
    novasvg_canvas_new_path(canvas);
    novasvg_canvas_add_path(canvas, path);
    novasvg_canvas_clip(canvas);
}

NOVASVG_INLINE float novasvg_canvas_add_glyph(novasvg_canvas_t* canvas, novasvg_codepoint_t codepoint, float x, float y)
{
    novasvg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f)
        return novasvg_font_face_get_glyph_path(state->font_face, state->font_size, x, y, codepoint, canvas->path);
    return 0.f;
}

NOVASVG_INLINE float novasvg_canvas_add_text(novasvg_canvas_t* canvas, const void* text, int length, novasvg_text_encoding_t encoding, float x, float y)
{
    novasvg_state_t* state = canvas->state;
    if(state->font_face == NULL || state->font_size <= 0.f)
        return 0.f;
    novasvg_text_iterator_t it;
    novasvg_text_iterator_init(&it, text, length, encoding);
    float advance_width = 0.f;
    while(novasvg_text_iterator_has_next(&it)) {
        novasvg_codepoint_t codepoint = novasvg_text_iterator_next(&it);
        advance_width += novasvg_font_face_get_glyph_path(state->font_face, state->font_size, x + advance_width, y, codepoint, canvas->path);
    }

    return advance_width;
}

NOVASVG_INLINE float novasvg_canvas_fill_text(novasvg_canvas_t* canvas, const void* text, int length, novasvg_text_encoding_t encoding, float x, float y)
{
    novasvg_canvas_new_path(canvas);
    float advance_width = novasvg_canvas_add_text(canvas, text, length, encoding, x, y);
    novasvg_canvas_fill(canvas);
    return advance_width;
}

NOVASVG_INLINE float novasvg_canvas_stroke_text(novasvg_canvas_t* canvas, const void* text, int length, novasvg_text_encoding_t encoding, float x, float y)
{
    novasvg_canvas_new_path(canvas);
    float advance_width = novasvg_canvas_add_text(canvas, text, length, encoding, x, y);
    novasvg_canvas_stroke(canvas);
    return advance_width;
}

NOVASVG_INLINE float novasvg_canvas_clip_text(novasvg_canvas_t* canvas, const void* text, int length, novasvg_text_encoding_t encoding, float x, float y)
{
    novasvg_canvas_new_path(canvas);
    float advance_width = novasvg_canvas_add_text(canvas, text, length, encoding, x, y);
    novasvg_canvas_clip(canvas);
    return advance_width;
}

NOVASVG_INLINE void novasvg_canvas_font_metrics(const novasvg_canvas_t* canvas, float* ascent, float* descent, float* line_gap, novasvg_rect_t* extents)
{
    novasvg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        novasvg_font_face_get_metrics(state->font_face, state->font_size, ascent, descent, line_gap, extents);
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

NOVASVG_INLINE void novasvg_canvas_glyph_metrics(novasvg_canvas_t* canvas, novasvg_codepoint_t codepoint, float* advance_width, float* left_side_bearing, novasvg_rect_t* extents)
{
    novasvg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        novasvg_font_face_get_glyph_metrics(state->font_face, state->font_size, codepoint, advance_width, left_side_bearing, extents);
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

NOVASVG_INLINE float novasvg_canvas_text_extents(novasvg_canvas_t* canvas, const void* text, int length, novasvg_text_encoding_t encoding, novasvg_rect_t* extents)
{
    novasvg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        return novasvg_font_face_text_extents(state->font_face, state->font_size, text, length, encoding, extents);
    }

    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }

    return 0.f;
}
