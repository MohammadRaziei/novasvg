#pragma once

#include "internal.h"
#include "utils.h"

#include <ctype.h>

namespace nova {
namespace vg {

static void* paint_create(paint_type_t type_, size_t size_)
{
    paint_t* paint_ = static_cast<paint_t*>(malloc(size_));
    novasvg_init_reference(paint_);
    paint_->type_ = type_;
    return paint_;
}

static gradient_paint_t* gradient_create(gradient_type_t type_, spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix)
{
    gradient_paint_t* gradient = static_cast<gradient_paint_t*>(paint_create(
        NOVA_PAINT_TYPE_GRADIENT,
        sizeof(gradient_paint_t) + nstops * sizeof(gradient_stop_t)
    ));
    gradient->type_ = type_;
    gradient->spread = spread;
    gradient->matrix = matrix ? *matrix : stb::matrix{};
    gradient->stops = (gradient_stop_t*)(gradient + 1);
    gradient->nstops = nstops;

    float prev_offset = 0.f;
    for(int i = 0; i < nstops; ++i) {
        const gradient_stop_t* stop = stops + i;
        gradient->stops[i].offset = STB_MAX(prev_offset, std::clamp(stop->offset, 0.f, 1.f));
        gradient->stops[i].color_ = stop->color_;
        prev_offset = gradient->stops[i].offset;
    }

    return gradient;
}

} // namespace vg
} // namespace nova
