#pragma once

#include "private.h"
#include "utils.h"

namespace novasvg {
namespace render {

int render_version(void);

const char* render_version_string(void);

#define NOVASVG_DEFAULT_STROKE_STYLE (stroke_style_t{1.f, NOVASVG_LINE_CAP_BUTT, NOVASVG_LINE_JOIN_MITER, 10.f})

state_t* state_create(void);

void state_reset(state_t* state);

void state_copy(state_t* state, const state_t* source);

void state_destroy(state_t* state);

canvas_t* canvas_create(surface_t* surface);

canvas_t* canvas_reference(canvas_t* canvas);

void canvas_destroy(canvas_t* canvas);

int canvas_get_reference_count(const canvas_t* canvas);

surface_t* canvas_get_surface(const canvas_t* canvas);

void canvas_save(canvas_t* canvas);

void canvas_restore(canvas_t* canvas);

void canvas_set_rgb(canvas_t* canvas, float r, float g, float b);

void canvas_set_rgba(canvas_t* canvas, float r, float g, float b, float a);

void canvas_set_color(canvas_t* canvas, const color_t* color);

void canvas_set_linear_gradient(canvas_t* canvas, float x1, float y1, float x2, float y2, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

void canvas_set_radial_gradient(canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

void canvas_set_texture(canvas_t* canvas, surface_t* surface, texture_type_t type, float opacity, const matrix_t* matrix);

void canvas_set_paint(canvas_t* canvas, paint_t* paint);

paint_t* canvas_get_paint(const canvas_t* canvas, color_t* color);

void canvas_set_font_face_cache(canvas_t* canvas, font_face_cache_t* cache);

font_face_cache_t* canvas_get_font_face_cache(const canvas_t* canvas);

void canvas_add_font_face(canvas_t* canvas, const char* family, bool bold, bool italic, font_face_t* face);

bool canvas_add_font_file(canvas_t* canvas, const char* family, bool bold, bool italic, const char* filename, int ttcindex);

bool canvas_select_font_face(canvas_t* canvas, const char* family, bool bold, bool italic);

void canvas_set_font(canvas_t* canvas, font_face_t* face, float size);

void canvas_set_font_face(canvas_t* canvas, font_face_t* face);

font_face_t* canvas_get_font_face(const canvas_t* canvas);

void canvas_set_font_size(canvas_t* canvas, float size);

float canvas_get_font_size(const canvas_t* canvas);

void canvas_set_fill_rule(canvas_t* canvas, fill_rule_t winding);

fill_rule_t canvas_get_fill_rule(const canvas_t* canvas);

void canvas_set_operator(canvas_t* canvas, operator_t op);

operator_t canvas_get_operator(const canvas_t* canvas);

void canvas_set_opacity(canvas_t* canvas, float opacity);

float canvas_get_opacity(const canvas_t* canvas);

void canvas_set_line_width(canvas_t* canvas, float line_width);

float canvas_get_line_width(const canvas_t* canvas);

void canvas_set_line_cap(canvas_t* canvas, line_cap_t line_cap);

line_cap_t canvas_get_line_cap(const canvas_t* canvas);

void canvas_set_line_join(canvas_t* canvas, line_join_t line_join);

line_join_t canvas_get_line_join(const canvas_t* canvas);

void canvas_set_miter_limit(canvas_t* canvas, float miter_limit);

float canvas_get_miter_limit(const canvas_t* canvas);

void canvas_set_dash(canvas_t* canvas, float offset, const float* dashes, int ndashes);

void canvas_set_dash_offset(canvas_t* canvas, float offset);

float canvas_get_dash_offset(const canvas_t* canvas);

void canvas_set_dash_array(canvas_t* canvas, const float* dashes, int ndashes);

int canvas_get_dash_array(const canvas_t* canvas, const float** dashes);

void canvas_translate(canvas_t* canvas, float tx, float ty);

void canvas_scale(canvas_t* canvas, float sx, float sy);

void canvas_shear(canvas_t* canvas, float shx, float shy);

void canvas_rotate(canvas_t* canvas, float angle);

void canvas_transform(canvas_t* canvas, const matrix_t* matrix);

void canvas_reset_matrix(canvas_t* canvas);

void canvas_set_matrix(canvas_t* canvas, const matrix_t* matrix);

void canvas_get_matrix(const canvas_t* canvas, matrix_t* matrix);

void canvas_map(const canvas_t* canvas, float x, float y, float* xx, float* yy);

void canvas_map_point(const canvas_t* canvas, const point_t* src, point_t* dst);

void canvas_map_rect(const canvas_t* canvas, const rect_t* src, rect_t* dst);

void canvas_move_to(canvas_t* canvas, float x, float y);

void canvas_line_to(canvas_t* canvas, float x, float y);

void canvas_quad_to(canvas_t* canvas, float x1, float y1, float x2, float y2);

void canvas_cubic_to(canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3);

void canvas_arc_to(canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

void canvas_rect(canvas_t* canvas, float x, float y, float w, float h);

void canvas_round_rect(canvas_t* canvas, float x, float y, float w, float h, float rx, float ry);

void canvas_ellipse(canvas_t* canvas, float cx, float cy, float rx, float ry);

void canvas_circle(canvas_t* canvas, float cx, float cy, float r);

void canvas_arc(canvas_t* canvas, float cx, float cy, float r, float a0, float a1, bool ccw);

void canvas_add_path(canvas_t* canvas, const path_t* path);

void canvas_new_path(canvas_t* canvas);

void canvas_close_path(canvas_t* canvas);

void canvas_get_current_point(const canvas_t* canvas, float* x, float* y);

path_t* canvas_get_path(const canvas_t* canvas);

bool canvas_fill_contains(canvas_t* canvas, float x, float y);

bool canvas_stroke_contains(canvas_t* canvas, float x, float y);

bool canvas_clip_contains(canvas_t* canvas, float x, float y);

void canvas_fill_extents(canvas_t *canvas, rect_t* extents);

void canvas_stroke_extents(canvas_t *canvas, rect_t* extents);

void canvas_clip_extents(canvas_t* canvas, rect_t* extents);

void canvas_fill(canvas_t* canvas);

void canvas_stroke(canvas_t* canvas);

void canvas_clip(canvas_t* canvas);

void canvas_paint(canvas_t* canvas);

void canvas_fill_preserve(canvas_t* canvas);

void canvas_stroke_preserve(canvas_t* canvas);

void canvas_clip_preserve(canvas_t* canvas);

void canvas_fill_rect(canvas_t* canvas, float x, float y, float w, float h);

void canvas_fill_path(canvas_t* canvas, const path_t* path);

void canvas_stroke_rect(canvas_t* canvas, float x, float y, float w, float h);

void canvas_stroke_path(canvas_t* canvas, const path_t* path);

void canvas_clip_rect(canvas_t* canvas, float x, float y, float w, float h);

void canvas_clip_path(canvas_t* canvas, const path_t* path);

float canvas_add_glyph(canvas_t* canvas, codepoint_t codepoint, float x, float y);

float canvas_add_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

float canvas_fill_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

float canvas_stroke_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

float canvas_clip_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

void canvas_font_metrics(const canvas_t* canvas, float* ascent, float* descent, float* line_gap, rect_t* extents);

void canvas_glyph_metrics(canvas_t* canvas, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents);

float canvas_text_extents(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, rect_t* extents);


} // namespace render
} // namespace novasvg