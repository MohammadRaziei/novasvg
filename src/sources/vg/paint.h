#pragma once

#include "internal.h"
#include "utils.h"

#include <ctype.h>

namespace nova {
namespace vg {

static inline float hsl_component(float h, float s, float l, float n)
{
    const float k = fmodf(n + h / 30.f, 12.f);
    const float a = s * STB_MIN(l, 1.f - l);
    return l - a * STB_MAX(-1.f, STB_MIN(1.f, STB_MIN(k - 3.f, 9.f - k)));
}

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

static void* paint_create(paint_type_t type_, size_t size_)
{
    paint_t* paint_ = static_cast<paint_t*>(malloc(size_));
    novasvg_init_reference(paint_);
    paint_->type_ = type_;
    return paint_;
}

static gradient_paint_t* gradient_create(gradient_type_t type_, spread_method spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    gradient_paint_t* gradient = static_cast<gradient_paint_t*>(paint_create(
        NOVA_PAINT_TYPE_GRADIENT,
        sizeof(gradient_paint_t) + nstops * sizeof(gradient_stop_t)
    ));
    gradient->type_ = type_;
    gradient->spread = spread;
    gradient->matrix = matrix ? *matrix : NOVA_IDENTITY_MATRIX;
    gradient->stops = (gradient_stop_t*)(gradient + 1);
    gradient->nstops = nstops;

    float prev_offset = 0.f;
    for(int i = 0; i < nstops; ++i) {
        const gradient_stop_t* stop = stops + i;
        gradient->stops[i].offset = STB_MAX(prev_offset, std::clamp(stop->offset, 0.f, 1.f));
        gradient->stops[i].color_.r = std::clamp(stop->color_.r, 0.f, 1.f);
        gradient->stops[i].color_.g = std::clamp(stop->color_.g, 0.f, 1.f);
        gradient->stops[i].color_.b = std::clamp(stop->color_.b, 0.f, 1.f);
        gradient->stops[i].color_.a = std::clamp(stop->color_.a, 0.f, 1.f);
        prev_offset = gradient->stops[i].offset;
    }

    return gradient;
}

} // namespace vg
} // namespace nova
