#pragma once

#include "private.h"
#include "utils.h"

#include <assert.h>

namespace novasvg {
namespace render {

void path_iterator_init(path_iterator_t* it, const path_t* path);

bool path_iterator_has_next(const path_iterator_t* it);

path_command_t path_iterator_next(path_iterator_t* it, point_t points[3]);

path_t* path_create(void);

path_t* path_reference(path_t* path);

void path_destroy(path_t* path);

int path_get_reference_count(const path_t* path);

int path_get_elements(const path_t* path, const path_element_t** elements);

path_element_t* path_add_command(path_t* path, path_command_t command, int npoints);

void path_move_to(path_t* path, float x, float y);

void path_line_to(path_t* path, float x, float y);

void path_quad_to(path_t* path, float x1, float y1, float x2, float y2);

void path_cubic_to(path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);

void path_arc_to(path_t* path, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

void path_close(path_t* path);

void path_get_current_point(const path_t* path, float* x, float* y);

void path_reserve(path_t* path, int count);

void path_reset(path_t* path);

void path_add_rect(path_t* path, float x, float y, float w, float h);

void path_add_round_rect(path_t* path, float x, float y, float w, float h, float rx, float ry);

void path_add_ellipse(path_t* path, float cx, float cy, float rx, float ry);

void path_add_circle(path_t* path, float cx, float cy, float r);

void path_add_arc(path_t* path, float cx, float cy, float r, float a0, float a1, bool ccw);

void path_transform(path_t* path, const matrix_t* matrix);

void path_add_path(path_t* path, const path_t* source, const matrix_t* matrix);

void path_traverse(const path_t* path, path_traverse_func_t traverse_func, void* closure);

typedef struct {
    float x1; float y1;
    float x2; float y2;
    float x3; float y3;
    float x4; float y4;
} bezier_t;

void split_bezier(const bezier_t* b, bezier_t* first, bezier_t* second);

void path_traverse_flatten(const path_t* path, path_traverse_func_t traverse_func, void* closure);

typedef struct {
    const float* dashes; int ndashes;
    float start_phase; float phase;
    int start_index; int index;
    bool start_toggle; bool toggle;
    point_t current_point;
    path_traverse_func_t traverse_func;
    void* closure;
} dasher_t;

void dash_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);

void path_traverse_dashed(const path_t* path, float offset, const float* dashes, int ndashes, path_traverse_func_t traverse_func, void* closure);

path_t* path_clone(const path_t* path);

void clone_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);

path_t* path_clone_flatten(const path_t* path);

path_t* path_clone_dashed(const path_t* path, float offset, const float* dashes, int ndashes);

typedef struct {
    point_t current_point;
    bool is_first_point;
    float length;
    float x1;
    float y1;
    float x2;
    float y2;
} extents_calculator_t;

void extents_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);

float path_extents(const path_t* path, rect_t* extents, bool tight);

float path_length(const path_t* path);

bool parse_arc_flag(const char** begin, const char* end, bool* flag);

bool parse_path_coordinates(const char** begin, const char* end, float values[6], int offset, int count);

bool path_parse(path_t* path, const char* data, int length);


} // namespace render
} // namespace novasvg