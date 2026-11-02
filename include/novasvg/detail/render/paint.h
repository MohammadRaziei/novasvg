#pragma once

#include "private.h"
#include "utils.h"

#include <ctype.h>

namespace novasvg {
namespace render {

void color_init_rgb(color_t* color, float r, float g, float b);

void color_init_rgba(color_t* color, float r, float g, float b, float a);

void color_init_rgb8(color_t* color, int r, int g, int b);

void color_init_rgba8(color_t* color, int r, int g, int b, int a);

void color_init_rgba32(color_t* color, unsigned int value);

void color_init_argb32(color_t* color, unsigned int value);

void color_init_hsl(color_t* color, float h, float s, float l);

static inline float hsl_component(float h, float s, float l, float n)
{
    const float k = fmodf(n + h / 30.f, 12.f);
    const float a = s * NOVASVG_MIN(l, 1.f - l);
    return l - a * NOVASVG_MAX(-1.f, NOVASVG_MIN(1.f, NOVASVG_MIN(k - 3.f, 9.f - k)));
}

void color_init_hsla(color_t* color, float h, float s, float l, float a);

unsigned int color_to_rgba32(const color_t* color);

unsigned int color_to_argb32(const color_t* color);

static inline uint8_t hex_digit(uint8_t c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return 10 + c - 'A';
}

static inline uint8_t hex_byte(uint8_t c1, uint8_t c2)
{
    uint8_t h1 = hex_digit(c1);
    uint8_t h2 = hex_digit(c2);
    return (h1 << 4) | h2;
}

#define MAX_NAME 20
typedef struct {
    const char* name;
    uint32_t value;
} color_entry_t;

static int color_entry_compare(const void* a, const void* b)
{
    const char* name = static_cast<const char*>(a);
    const color_entry_t* entry = static_cast<const color_entry_t*>(b);
    return strcmp(name, entry->name);
}

static bool parse_rgb_component(const char** begin, const char* end, float* component)
{
    float value = 0;
    if(!parse_number(begin, end, &value))
        return false;
    if(skip_delim(begin, end, '%'))
        value *= 2.55f;
    *component = std::clamp(value, 0.f, 255.f) / 255.f;
    return true;
}

static bool parse_alpha_component(const char** begin, const char* end, float* component)
{
    float value = 0;
    if(!parse_number(begin, end, &value))
        return false;
    if(skip_delim(begin, end, '%'))
        value /= 100.f;
    *component = std::clamp(value, 0.f, 1.f);
    return true;
}

int color_parse(color_t* color, const char* data, int length);

static void* paint_create(paint_type_t type, size_t size)
{
    paint_t* paint = static_cast<paint_t*>(malloc(size));
    novasvg_init_reference(paint);
    paint->type = type;
    return paint;
}

paint_t* paint_create_rgb(float r, float g, float b);

paint_t* paint_create_rgba(float r, float g, float b, float a);

paint_t* paint_create_color(const color_t* color);

static gradient_paint_t* gradient_create(gradient_type_t type, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    gradient_paint_t* gradient = static_cast<gradient_paint_t*>(paint_create(
        NOVASVG_PAINT_TYPE_GRADIENT,
        sizeof(gradient_paint_t) + nstops * sizeof(gradient_stop_t)
    ));
    gradient->type = type;
    gradient->spread = spread;
    gradient->matrix = matrix ? *matrix : NOVASVG_IDENTITY_MATRIX;
    gradient->stops = (gradient_stop_t*)(gradient + 1);
    gradient->nstops = nstops;

    float prev_offset = 0.f;
    for(int i = 0; i < nstops; ++i) {
        const gradient_stop_t* stop = stops + i;
        gradient->stops[i].offset = NOVASVG_MAX(prev_offset, std::clamp(stop->offset, 0.f, 1.f));
        gradient->stops[i].color.r = std::clamp(stop->color.r, 0.f, 1.f);
        gradient->stops[i].color.g = std::clamp(stop->color.g, 0.f, 1.f);
        gradient->stops[i].color.b = std::clamp(stop->color.b, 0.f, 1.f);
        gradient->stops[i].color.a = std::clamp(stop->color.a, 0.f, 1.f);
        prev_offset = gradient->stops[i].offset;
    }

    return gradient;
}

paint_t* paint_create_linear_gradient(float x1, float y1, float x2, float y2, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

paint_t* paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

paint_t* paint_create_texture(surface_t* surface, texture_type_t type, float opacity, const matrix_t* matrix);

paint_t* paint_reference(paint_t* paint);

void paint_destroy(paint_t* paint);

int paint_get_reference_count(const paint_t* paint);


} // namespace render
} // namespace novasvg