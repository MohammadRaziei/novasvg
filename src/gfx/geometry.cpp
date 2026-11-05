// Definitions of Box, Matrix, Transform and the Rect/Box conversions declared in include/novasvg/novasvg.h.
#include "novasvg/novasvg.h"

#include "gfx/canvas.h"
#include "gfx/internal.h"
#include "gfx/utils.h"

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

Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).map_rect(*this);
}

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

Matrix::Matrix(float a, float b, float c, float d, float e, float f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

Matrix::Matrix(const matrix_t& matrix)
    : a(matrix.a), b(matrix.b), c(matrix.c), d(matrix.d), e(matrix.e), f(matrix.f)
{
}

Matrix::Matrix(const Transform& transform)
    : Matrix(transform.matrix())
{
}

Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

Matrix& Matrix::operator*=(const Matrix &matrix)
{
    return (*this = *this * matrix);
}

Matrix& Matrix::multiply(const Matrix& matrix)
{
    return (*this *= matrix);
}

Matrix& Matrix::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Matrix& Matrix::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Matrix& Matrix::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Matrix& Matrix::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Matrix Matrix::inverse() const
{
    return Transform(*this).inverse();
}

Matrix& Matrix::invert()
{
    return (*this = inverse());
}

void Matrix::reset()
{
    *this = Matrix(1, 0, 0, 1, 0, 0);
}

Matrix Matrix::translated(float tx, float ty)
{
    return Transform::translated(tx, ty);
}

Matrix Matrix::scaled(float sx, float sy)
{
    return Transform::scaled(sx, sy);
}

Matrix Matrix::rotated(float angle, float cx, float cy)
{
    return Transform::rotated(angle, cx, cy);
}

Matrix Matrix::sheared(float shx, float shy)
{
    return Transform::sheared(shx, shy);
}

} // namespace nova::vg
