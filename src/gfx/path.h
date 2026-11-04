#pragma once

#include "gfx/internal.h"
#include "gfx/utils.h"

#include <assert.h>

namespace novasvg {
namespace gfx {

typedef struct {
    float x1; float y1;
    float x2; float y2;
    float x3; float y3;
    float x4; float y4;
} bezier_t;

typedef struct {
    const float* dashes; int ndashes;
    float start_phase; float phase;
    int start_index; int index;
    bool start_toggle; bool toggle;
    point_t current_point;
    path_traverse_func_t traverse_func;
    void* closure;
} dasher_t;

typedef struct {
    point_t current_point;
    bool is_first_point;
    float length;
    float x1;
    float y1;
    float x2;
    float y2;
} extents_calculator_t;

} // namespace gfx
} // namespace novasvg
