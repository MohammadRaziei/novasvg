#include "paint.h"

namespace nova {
namespace vg {

paint_t* paint_create_rgb(float r, float g, float b)
{
    return paint_create_rgba(r, g, b, 1.f);
}

paint_t* paint_create_rgba(float r, float g, float b, float a)
{
    solid_paint_t* solid = static_cast<solid_paint_t*>(paint_create(
        NOVA_PAINT_TYPE_COLOR,
        sizeof(solid_paint_t)
    ));
    solid->color_ = color_from_floats(r, g, b, a);
    return &solid->base;
}

paint_t* paint_create_color(const stb::color* color_)
{
    solid_paint_t* solid = static_cast<solid_paint_t*>(paint_create(
        NOVA_PAINT_TYPE_COLOR,
        sizeof(solid_paint_t)
    ));
    solid->color_ = *color_;
    return &solid->base;
}

paint_t* paint_create_linear_gradient(float x1, float y1, float x2, float y2, spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix)
{
    gradient_paint_t* gradient = gradient_create(NOVA_GRADIENT_TYPE_LINEAR, spread, stops, nstops, matrix);
    gradient->values[0] = x1;
    gradient->values[1] = y1;
    gradient->values[2] = x2;
    gradient->values[3] = y2;
    return &gradient->base;
}

paint_t* paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr, spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix)
{
    gradient_paint_t* gradient = gradient_create(NOVA_GRADIENT_TYPE_RADIAL, spread, stops, nstops, matrix);
    gradient->values[0] = cx;
    gradient->values[1] = cy;
    gradient->values[2] = cr;
    gradient->values[3] = fx;
    gradient->values[4] = fy;
    gradient->values[5] = fr;
    return &gradient->base;
}

paint_t* paint_create_texture(surface_t* surface, texture_type type_, float opacity, const stb::matrix* matrix)
{
    texture_paint_t* texture = static_cast<texture_paint_t*>(paint_create(
        NOVA_PAINT_TYPE_TEXTURE,
        sizeof(texture_paint_t)
    ));
    texture->type_ = type_;
    texture->opacity = std::clamp(opacity, 0.f, 1.f);
    texture->matrix = matrix ? *matrix : stb::matrix{};
    texture->surface = surface_reference(surface);
    return &texture->base;
}

paint_t* paint_reference(paint_t* paint_)
{
    novasvg_increment_reference(paint_);
    return paint_;
}

void paint_destroy(paint_t* paint_)
{
    if(novasvg_destroy_reference(paint_)) {
        if(paint_->type_ == NOVA_PAINT_TYPE_TEXTURE) {
            texture_paint_t* texture = (texture_paint_t*)(paint_);
            surface_destroy(texture->surface);
        }

        free(paint_);
    }
}

int paint_get_reference_count(const paint_t* paint_)
{
    return novasvg_get_reference_count(paint_);
}

} // namespace vg
} // namespace nova
