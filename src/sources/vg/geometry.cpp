// Definitions of Box, Transform and the Rect/Box conversions declared in include/nova/vg/geometry.h.
#include "nova/vg/geometry.h"

#include "canvas.h"
#include "internal.h"
#include "utils.h"

#include <cmath>

namespace nova::vg {

Box::Box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

Box& Box::transform(const stb::matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

Box Box::transformed(const stb::matrix& matrix) const
{
    return Transform(matrix).map_rect(*this);
}

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

Transform::Transform(const stb::matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

} // namespace nova::vg
