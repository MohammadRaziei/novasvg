// Definitions of Box, Transform and the Rect/Box conversions declared in include/nova/vg/geometry.h.
#include "nova/vg/geometry.h"

#include "canvas.h"
#include "internal.h"
#include "utils.h"

#include <cmath>

namespace nova::vg {

box::box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

box::box(const rect& rect_)
    : x(rect_.x), y(rect_.y), w(rect_.w), h(rect_.h)
{
}

box& box::transform(const stb::matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

box box::transformed(const stb::matrix& matrix) const
{
    return vg::transform(matrix).map_rect(*this);
}

rect::rect(const box& box_)
    : x(box_.x), y(box_.y), w(box_.w), h(box_.h)
{
}

transform::transform(const stb::matrix& matrix)
    : transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

} // namespace nova::vg
