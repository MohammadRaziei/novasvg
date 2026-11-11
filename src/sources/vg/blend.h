#pragma once

#include "internal.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>

namespace nova {
namespace vg {

#define COLOR_TABLE_SIZE 1024
typedef struct {
    stb::matrix matrix;
    spread_method_value spread;
    uint32_t colortable[COLOR_TABLE_SIZE];
    union {
        struct {
            float x1, y1;
            float x2, y2;
        } linear;
        struct {
            float cx, cy, cr;
            float fx, fy, fr;
        } radial;
    } values;
} gradient_data_t;

typedef struct {
    stb::matrix matrix;
    uint8_t* data;
    int width;
    int height;
    int stride;
    int const_alpha;
} texture_data_t;

typedef struct {
    float dx;
    float dy;
    float l;
    float off;
} linear_gradient_values_t;

typedef struct {
    float dx;
    float dy;
    float dr;
    float sqrfr;
    float a;
    bool extended;
} radial_gradient_values_t;

uint32_t premultiply_color_with_opacity(const stb::color* color_, float opacity);

uint32_t INTERPOLATE_PIXEL_255(uint32_t x, uint32_t a, uint32_t y, uint32_t b);

uint32_t INTERPOLATE_PIXEL_256(uint32_t x, uint32_t a, uint32_t y, uint32_t b);

uint32_t BYTE_MUL(uint32_t x, uint32_t a);



int gradient_clamp(const gradient_data_t* gradient, int ipos);

#define FIXPT_BITS 8
#define FIXPT_SIZE (1 << FIXPT_BITS)
uint32_t gradient_pixel_fixed(const gradient_data_t* gradient, int fixed_pos);

uint32_t gradient_pixel(const gradient_data_t* gradient, float pos);

void fetch_linear_gradient(uint32_t* buffer, const linear_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length_);

void fetch_radial_gradient(uint32_t* buffer, const radial_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length_);


void blend_solid(surface_t* surface, blend_mode op, uint32_t solid, const span_buffer_t* span_buffer);

#define BUFFER_SIZE 1024
void blend_linear_gradient(surface_t* surface, blend_mode op, const gradient_data_t* gradient, const span_buffer_t* span_buffer);

void blend_radial_gradient(surface_t* surface, blend_mode op, const gradient_data_t* gradient, const span_buffer_t* span_buffer);

void blend_untransformed_argb(surface_t* surface, blend_mode op, const texture_data_t* texture, const span_buffer_t* span_buffer);

#define FIXED_SCALE (1 << 16)
void blend_transformed_argb(surface_t* surface, blend_mode op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_untransformed_tiled_argb(surface_t* surface, blend_mode op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_transformed_tiled_argb(surface_t* surface, blend_mode op, const texture_data_t* texture, const span_buffer_t* span_buffer);

uint32_t interpolate_4_pixels(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br, uint32_t distx, uint32_t disty);

#define HALF_POINT (1 << 15)
void blend_transformed_bilinear_tiled_argb(surface_t* surface, blend_mode op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_color(canvas_t* canvas, const stb::color* color_, const span_buffer_t* span_buffer);

void blend_gradient(canvas_t* canvas, const gradient_paint_t* gradient, const span_buffer_t* span_buffer);

void blend_texture(canvas_t* canvas, const texture_paint_t* texture, const span_buffer_t* span_buffer);

} // namespace vg
} // namespace nova
