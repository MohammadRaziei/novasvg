/*
 * MIT License
 *
 * Copyright (c) 2023-present Mohammad Raziei
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/


#ifndef NOVASVG_H
#define NOVASVG_H
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <cctype>
#include <vector>

#include "config.h"

#define NOVASVG_VERSION_MAJOR 0
#define NOVASVG_VERSION_MINOR 11
#define NOVASVG_VERSION_PATCH 4

#define NOVASVG_VERSION_ENCODE(major, minor, patch) (((major) * 10000) + ((minor) * 100) + ((patch) * 1))
#define NOVASVG_VERSION NOVASVG_VERSION_ENCODE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

#define NOVASVG_VERSION_XSTRINGIZE(major, minor, patch) #major"."#minor"."#patch
#define NOVASVG_VERSION_STRINGIZE(major, minor, patch) NOVASVG_VERSION_XSTRINGIZE(major, minor, patch)
#define NOVASVG_VERSION_STRING NOVASVG_VERSION_STRINGIZE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

#include "gfx/color.h"

namespace novasvg {
namespace gfx {
struct surface;
struct matrix;
using surface_t = surface;
using matrix_t = matrix;
} // namespace gfx
} // namespace novasvg

/**
 * @brief Callback for cleaning up resources.
 *
 * This function is called to release resources associated with a specific operation.
 *
 * @param closure A user-defined pointer to the resource or context to be freed.
 */
typedef void (*novasvg_destroy_func_t)(void* closure);



namespace novasvg {
using namespace gfx;

/**
 * @brief Returns the version of the novasvg library encoded in a single integer.
 *
 * Encodes the version of the novasvg library into a single integer for easier comparison.
 * The version is typically represented by combining major, minor, and patch numbers into one integer.
 *
 * @return The novasvg library version as a single integer.
 */
int version();

/**
 * @brief Returns the novasvg library version as a human-readable string in "X.Y.Z" format.
 *
 * Provides the version of the novasvg library as a human-readable string in the format "X.Y.Z",
 * where X represents the major version, Y the minor version, and Z the patch version.
 *
 * @return A pointer to a string containing the version in "X.Y.Z" format.
 */
std::string version_string();

} // namespace novasvg

#include "gfx/font.h"

#include "gfx/bitmap.h"

namespace novasvg {
using namespace gfx;

class Rect;
class Matrix;

/**
 * @brief Represents a 2D axis-aligned bounding box.
 */
class NOVASVG_API Box {
public:
    /**
     * @brief Constructs a box with zero dimensions.
     */
    Box() = default;

    /**
     * @brief Constructs a box with the specified position and size.
     * @param x The x-coordinate of the box's origin.
     * @param y The y-coordinate of the box's origin.
     * @param w The width of the box.
     * @param h The height of the box.
     */
    Box(float x, float y, float w, float h);

    /**
     * @internal
     */
    Box(const Rect& rect);

    /**
     * @brief Transforms the box using the specified matrix.
     * @param matrix The transformation matrix.
     * @return A reference to this box, modified by the transformation.
     */
    Box& transform(const Matrix& matrix);

    /**
     * @brief Returns a new box transformed by the specified matrix.
     * @param matrix The transformation matrix.
     * @return A new box, transformed by the matrix.
     */
    Box transformed(const Matrix& matrix) const;

    float x{0}; ///< The x-coordinate of the box's origin.
    float y{0}; ///< The y-coordinate of the box's origin.
    float w{0}; ///< The width of the box.
    float h{0}; ///< The height of the box.
};

class Transform;

/**
 * @brief Represents a 2D transformation matrix.
 */
class NOVASVG_API Matrix {
public:
    /**
     * @brief Initializes the matrix to the identity matrix.
     */
    Matrix() = default;

    /**
     * @brief Constructs a matrix with the specified values.
     * @param a The horizontal scaling factor.
     * @param b The vertical shearing factor.
     * @param c The horizontal shearing factor.
     * @param d The vertical scaling factor.
     * @param e The horizontal translation offset.
     * @param f The vertical translation offset.
     */
    Matrix(float a, float b, float c, float d, float e, float f);

    /**
     * @internal
     */
    Matrix(const matrix_t& matrix);

    /**
     * @internal
     */
    Matrix(const Transform& transform);

    /**
     * @brief Multiplies this matrix with another matrix.
     * @param matrix The matrix to multiply with.
     * @return A new matrix that is the result of the multiplication.
     */
    Matrix operator*(const Matrix& matrix) const;

    /**
     * @brief Multiplies this matrix with another matrix in place.
     * @param matrix The matrix to multiply with.
     * @return A reference to this matrix after multiplication.
     */
    Matrix& operator*=(const Matrix& matrix);

    /**
     * @brief Multiplies this matrix with another matrix.
     * @param matrix The matrix to multiply with.
     * @return A reference to this matrix after multiplication.
     */
    Matrix& multiply(const Matrix& matrix);

    /**
     * @brief Translates this matrix by the specified offsets.
     * @param tx The horizontal translation offset.
     * @param ty The vertical translation offset.
     * @return A reference to this matrix after translation.
     */
    Matrix& translate(float tx, float ty);

    /**
     * @brief Scales this matrix by the specified factors.
     * @param sx The horizontal scaling factor.
     * @param sy The vertical scaling factor.
     * @return A reference to this matrix after scaling.
     */
    Matrix& scale(float sx, float sy);

    /**
     * @brief Rotates this matrix by the specified angle around a point.
     * @param angle The rotation angle in degrees.
     * @param cx The x-coordinate of the center of rotation.
     * @param cy The y-coordinate of the center of rotation.
     * @return A reference to this matrix after rotation.
     */
    Matrix& rotate(float angle, float cx = 0.f, float cy = 0.f);

    /**
     * @brief Shears this matrix by the specified factors.
     * @param shx The horizontal shearing factor.
     * @param shy The vertical shearing factor.
     * @return A reference to this matrix after shearing.
     */
    Matrix& shear(float shx, float shy);

    /**
     * @brief Inverts this matrix.
     * @return A reference to this matrix after inversion.
     */
    Matrix& invert();

    /**
     * @brief Returns the inverse of this matrix.
     * @return A new matrix that is the inverse of this matrix.
     */
    Matrix inverse() const;

    /**
     * @brief Resets this matrix to the identity matrix.
     */
    void reset();

    /**
     * @brief Creates a translation matrix with the specified offsets.
     * @param tx The horizontal translation offset.
     * @param ty The vertical translation offset.
     * @return A new translation matrix.
     */
    static Matrix translated(float tx, float ty);

    /**
     * @brief Creates a scaling matrix with the specified factors.
     * @param sx The horizontal scaling factor.
     * @param sy The vertical scaling factor.
     * @return A new scaling matrix.
     */
    static Matrix scaled(float sx, float sy);

    /**
     * @brief Creates a rotation matrix with the specified angle around a point.
     * @param angle The rotation angle in degrees.
     * @param cx The x-coordinate of the center of rotation.
     * @param cy The y-coordinate of the center of rotation.
     * @return A new rotation matrix.
     */
    static Matrix rotated(float angle, float cx = 0.f, float cy = 0.f);

    /**
     * @brief Creates a shearing matrix with the specified factors.
     * @param shx The horizontal shearing factor.
     * @param shy The vertical shearing factor.
     * @return A new shearing matrix.
     */
    static Matrix sheared(float shx, float shy);

    float a{1}; ///< The horizontal scaling factor.
    float b{0}; ///< The vertical shearing factor.
    float c{0}; ///< The horizontal shearing factor.
    float d{1}; ///< The vertical scaling factor.
    float e{0}; ///< The horizontal translation offset.
    float f{0}; ///< The vertical translation offset.
};

class svg_node;
class svg_text_node;
class svg_element;

class element;
class text_node;

class NOVASVG_API node {
public:
    /**
     * @brief Constructs a null nd.
     */
    node() = default;

    /**
     * @brief Checks if the nd is a text nd.
     * @return True if the nd is a text nd, false otherwise.
     */
    bool is_text_node() const;

    /**
     * @brief Checks if the nd is an el nd.
     * @return True if the nd is an el nd, false otherwise.
     */
    bool is_element() const;

    /**
     * @brief Converts the nd to a text_node.
     * @return A text_node or a null nd if conversion is not possible.
     */
    text_node to_text_node() const;

    /**
     * @brief Converts the nd to an element.
     * @return An element or a null nd if conversion is not possible.
     */
    element to_element() const;

    /**
     * @brief Returns the parent el.
     * @return The parent el of this nd. If this nd has no parent, a null `element` is returned.
     */
    element parent_element() const;

    /**
     * @brief Checks if the nd is null.
     * @return True if the nd is null, false otherwise.
     */
    bool is_null() const { return m_node == nullptr; }

    /**
     * @brief Checks if the nd is not null.
     * @return True if the nd is not null, false otherwise.
     */
    operator bool() const { return !is_null(); }

    /**
     * @brief Checks if two nodes are equal.
     * @param el The nd to compare.
     * @return True if equal, otherwise false.
     */
    bool operator==(const node& nd) const { return m_node == nd.m_node; }

    /**
     * @brief Checks if two nodes are not equal.
     * @param el The nd to compare.
     * @return True if not equal, otherwise false.
     */
    bool operator!=(const node& nd) const { return m_node != nd.m_node; }

protected:
    node(svg_node* nd);
    svg_node* nd() const { return m_node; }
    svg_node* m_node{nullptr};
    friend class element;
};

using NodeList = std::vector<node>;

class NOVASVG_API text_node : public node {
public:
    /**
     * @brief Constructs a null text nd.
     */
    text_node() = default;

    /**
     * @brief Returns the text content of the nd.
     * @return A string representing the text content.
     */
    const std::string& data() const;

    /**
     * @brief Sets the text content of the nd.
     * @param data The new text content to set.
     */
    void set_data(const std::string& data);

private:
    text_node(svg_text_node* text);
    svg_text_node* text() const;
    friend class node;
};

class NOVASVG_API element : public node {
public:
    /**
     * @brief Constructs a null el.
     */
    element() = default;

    /**
     * @brief Checks if the el has a specific attribute.
     * @param name The name of the attribute to check.
     * @return True if the el has the specified attribute, false otherwise.
     */
    bool has_attribute(const std::string& name) const;

    /**
     * @brief Retrieves the value of an attribute.
     * @param name The name of the attribute to retrieve.
     * @return The value of the attribute as a string.
     */
    const std::string& attribute(const std::string& name) const;

    /**
     * @brief Sets the value of an attribute.
     * @param name The name of the attribute to set.
     * @param value The value to assign to the attribute.
     */
    void set_attribute(const std::string& name, const std::string& value);

    /**
     * @brief Renders the el onto an existing image, using a transformation matrix.
     *
     * The el is drawn on a transparent layer the size of `image` and the layer is composited over the
     * image's pixels with stb::image::blend (normal mode, "over" with straight alpha). Because the layer starts
     * transparent, `mix-blend-mode` inside the el blends against the layer, not against what was already in
     * `image`. Any pixel format is accepted; images larger than 32767 pixels on a side are left untouched.
     * @param image The image to render onto (changed in place).
     * @param matrix The root transformation matrix.
     */
    void render(stb::image& image, const Matrix& matrix = Matrix()) const;

    /**
     * @brief Renders the el to a new straight-alpha RGBA image with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return The rendered image, or an empty image if the el has nothing to draw or memory ran out.
     */
    stb::image render_to_image(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @internal Premultiplied-ARGB32 rendering target (Bitmap is not part of the public API; use the
     * stb::image overloads above).
     */
    void render(Bitmap& bitmap, const Matrix& matrix = Matrix()) const;

    /**
     * @internal See render(Bitmap&).
     */
    Bitmap render_to_bitmap(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @brief Retrieves the local transformation matrix of the el.
     * @return The matrix that applies only to the el, relative to its parent.
     */
    Matrix local_matrix() const;

    /**
     * @brief Retrieves the global transformation matrix of the el.
     * @return The matrix combining the el's local and all parent transformations.
     */
    Matrix global_matrix() const;

    /**
     * @brief Retrieves the local bounding box of the el.
     * @return A Box representing the bounding box after applying local transformations.
     */
    Box local_bounding_box() const;

    /**
     * @brief Retrieves the global bounding box of the el.
     * @return A Box representing the bounding box after applying global transformations.
     */
    Box global_bounding_box() const;

    /**
     * @brief Retrieves the bounding box of the el without any transformations.
     * @return A Box representing the bounding box of the el without any transformations applied.
     */
    Box bounding_box() const;

    /**
     * @brief Returns the child nodes of this nd.
     * @return A NodeList containing the child nodes.
     */
    NodeList children() const;

private:
    element(svg_element* el);
    svg_element* el(bool layout_if_needed = false) const;
    friend class node;
    friend class document;
};

using ElementList = std::vector<element>;

class svg_root_element;

class NOVASVG_API document {
public:
    /**
     * @brief Load an svg dom from a file.
     * @param filename The path to the svg file.
     * @return A pointer to the loaded `document`, or `nullptr` on failure.
     */
    static std::unique_ptr<document> load_file(const std::string& filename);

    /**
     * @brief Load an svg dom from a string.
     * @param string The svg data as a string.
     * @return A pointer to the loaded `document`, or `nullptr` on failure.
     */
    static std::unique_ptr<document> load_data(const std::string& string);

    /**
     * @brief Load an svg dom from a null-terminated string.
     * @param data The string containing the svg data.
     * @return A pointer to the loaded `document`, or `nullptr` on failure.
     */
    static std::unique_ptr<document> load_data(const char* data);

    /**
     * @brief Load an svg dom from a string with a specified length.
     * @param data The string containing the svg data.
     * @param length The length of the string in bytes.
     * @return A pointer to the loaded `document`, or `nullptr` on failure.
     */
    static std::unique_ptr<document> load_data(const char* data, size_t length);

    /**
     * @brief Applies a CSS stylesheet to the dom.
     * @param content A string containing the CSS rules to apply, with comments removed.
     */
    void apply_style_sheet(const std::string& content);

    /**
     * @brief Selects all elements that match the given CSS selector(s).
     * @param content A string containing the CSS selector(s) to match elements.
     * @return A list of elements matching the selector(s).
     */
    ElementList query_selector_all(const std::string& content) const;

    /**
     * @brief Returns the intrinsic width of the dom in pixels.
     * @return The width of the dom.
     */
    float width() const;

    /**
     * @brief Returns the intrinsic height of the dom in pixels.
     * @return The height of the dom.
     */
    float height() const;

    /**
     * @brief Returns the smallest rectangle that encloses the dom content.
     * @return A Box representing the bounding box of the dom.
     */
    Box bounding_box() const;

    /**
     * @brief Updates the layout of the dom if needed.
     */
    void update_layout();

    /**
     * @brief Forces an immediate layout update.
     */
    void force_layout();

    /**
     * @brief Renders the dom onto an existing image, using a transformation matrix.
     *
     * The dom is drawn on a transparent layer the size of `image` and the layer is composited over the
     * image's pixels with stb::image::blend (normal mode, "over" with straight alpha). Because the layer starts
     * transparent, `mix-blend-mode` inside the dom blends against the layer, not against what was already in
     * `image`. A root `background-color` is composited over the image too, instead of replacing it. Any pixel
     * format is accepted; images larger than 32767 pixels on a side are left untouched.
     * @param image The image to render onto (changed in place).
     * @param matrix The root transformation matrix.
     */
    void render(stb::image& image, const Matrix& matrix = Matrix()) const;

    /**
     * @brief Renders the dom to a new straight-alpha RGBA image with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return The rendered image, or an empty image if the dom has nothing to draw or memory ran out.
     */
    stb::image render_to_image(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @internal Premultiplied-ARGB32 rendering target (Bitmap is not part of the public API; use the
     * stb::image overloads above).
     */
    void render(Bitmap& bitmap, const Matrix& matrix = Matrix()) const;

    /**
     * @internal See render(Bitmap&).
     */
    Bitmap render_to_bitmap(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @brief Returns the topmost el under the specified point.
     * @param x The x-coordinate in viewport space.
     * @param y The y-coordinate in viewport space.
     * @return The topmost element at the given point, or a null `element` if no match is found.
     */
    element element_from_point(float x, float y) const;

    /**
     * @brief Retrieves an el by its ID.
     * @param id The ID of the el to retrieve.
     * @return The element with the specified ID, or a null `element` if not found.
     */
    element element_by_id(const std::string& id) const;

    /**
     * @brief Retrieves the dom el.
     * @return The root element of the dom.
     */
    element document_element() const;

    document(document&&);
    document& operator=(document&&);
    ~document();

private:
    document();
    document(const document&) = delete;
    document& operator=(const document&) = delete;
    svg_root_element* root_element(bool layout_if_needed = false) const;
    bool parse(const char* data, size_t length);
    std::unique_ptr<svg_root_element> m_rootElement;
    friend class svg_uri_reference;
    friend class svg_node;
};

} // namespace novasvg

// Always included -- see the NOVASVG_INLINE note above. Defining
// NOVASVG_IMPLEMENTATION is no longer required and has no effect; it is
// kept as a harmless no-op purely so existing code that still defines it
// keeps compiling unchanged.
//
// This used to be a separate "detail/novasvg_impl.h" file, but that file
// did nothing except forward this exact list of includes to the one
// place (here) that used it, so it's inlined directly instead.
//
// The render/ layer's own extern "C" wrapper is gone too: it's pure,
// header-only C++ (NOVASVG_INLINE / internal-linkage everywhere), nothing
// in this project links against it from actual C code, and the vendored
// STB headers already manage their own extern "C" internally when
// compiled as C++.

#include <cstring>
#include <fstream>
#include <cmath>

#include "gfx/canvas.h"
#include "svg/svgelement.h"
#include "svg/svgparser.hpp"

namespace novasvg {
using namespace gfx;

// ---------------------------------------------------------------
// Implementation (formerly detail/novasvg.hpp -- merged in since
// this header is fully NOVASVG_INLINE now, there's no reason left
// to keep the public declarations and their definitions apart).
// ---------------------------------------------------------------

using namespace gfx; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE int version()
{
    return NOVASVG_VERSION;
}

NOVASVG_INLINE std::string version_string()
{
    return NOVASVG_VERSION_STRING;
}


NOVASVG_INLINE Box::Box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

NOVASVG_INLINE Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

NOVASVG_INLINE Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

NOVASVG_INLINE Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).map_rect(*this);
}

NOVASVG_INLINE Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

NOVASVG_INLINE Matrix::Matrix(float a, float b, float c, float d, float e, float f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

NOVASVG_INLINE Matrix::Matrix(const matrix_t& matrix)
    : a(matrix.a), b(matrix.b), c(matrix.c), d(matrix.d), e(matrix.e), f(matrix.f)
{
}

NOVASVG_INLINE Matrix::Matrix(const Transform& transform)
    : Matrix(transform.matrix())
{
}

NOVASVG_INLINE Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

NOVASVG_INLINE Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

NOVASVG_INLINE Matrix& Matrix::operator*=(const Matrix &matrix)
{
    return (*this = *this * matrix);
}

NOVASVG_INLINE Matrix& Matrix::multiply(const Matrix& matrix)
{
    return (*this *= matrix);
}

NOVASVG_INLINE Matrix& Matrix::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

NOVASVG_INLINE Matrix& Matrix::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

NOVASVG_INLINE Matrix& Matrix::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

NOVASVG_INLINE Matrix& Matrix::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

NOVASVG_INLINE Matrix Matrix::inverse() const
{
    return Transform(*this).inverse();
}

NOVASVG_INLINE Matrix& Matrix::invert()
{
    return (*this = inverse());
}

NOVASVG_INLINE void Matrix::reset()
{
    *this = Matrix(1, 0, 0, 1, 0, 0);
}

NOVASVG_INLINE Matrix Matrix::translated(float tx, float ty)
{
    return Transform::translated(tx, ty);
}

NOVASVG_INLINE Matrix Matrix::scaled(float sx, float sy)
{
    return Transform::scaled(sx, sy);
}

NOVASVG_INLINE Matrix Matrix::rotated(float angle, float cx, float cy)
{
    return Transform::rotated(angle, cx, cy);
}

NOVASVG_INLINE Matrix Matrix::sheared(float shx, float shy)
{
    return Transform::sheared(shx, shy);
}

NOVASVG_INLINE node::node(svg_node* nd)
    : m_node(nd)
{
}

NOVASVG_INLINE bool node::is_text_node() const
{
    return m_node && m_node->is_text_node();
}

NOVASVG_INLINE bool node::is_element() const
{
    return m_node && m_node->is_element();
}

NOVASVG_INLINE text_node node::to_text_node() const
{
    if(m_node && m_node->is_text_node())
        return static_cast<svg_text_node*>(m_node);
    return text_node();
}

NOVASVG_INLINE element node::to_element() const
{
    if(m_node && m_node->is_element())
        return static_cast<svg_element*>(m_node);
    return element();
}

NOVASVG_INLINE element node::parent_element() const
{
    if(m_node)
        return m_node->parent_element();
    return element();
}

NOVASVG_INLINE text_node::text_node(svg_text_node* text)
    : node(text)
{
}

NOVASVG_INLINE const std::string& text_node::data() const
{
    if(m_node)
        return text()->data();
    return emptyString;
}

NOVASVG_INLINE void text_node::set_data(const std::string& data)
{
    if(m_node) {
        text()->set_data(data);
    }
}

NOVASVG_INLINE svg_text_node* text_node::text() const
{
    return static_cast<svg_text_node*>(m_node);
}

NOVASVG_INLINE element::element(svg_element* el)
    : node(el)
{
}

NOVASVG_INLINE bool element::has_attribute(const std::string& name) const
{
    if(m_node)
        return el()->has_attribute(name);
    return false;
}

NOVASVG_INLINE const std::string& element::attribute(const std::string& name) const
{
    if(m_node)
        return el()->attribute(name);
    return emptyString;
}

NOVASVG_INLINE void element::set_attribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        el()->set_attribute(name, value);
    }
}

NOVASVG_INLINE void element::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(m_node == nullptr || bitmap.is_null())
        return;
    auto canvas = Canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::Painting, canvas);
    el(true)->render(state);
}

NOVASVG_INLINE Bitmap element::render_to_bitmap(int width, int height, const Color& backgroundColor) const
{
    if(m_node == nullptr)
        return Bitmap();
    auto elementBounds = el(true)->local_transform().map_rect(el()->paint_bounding_box());
    if(elementBounds.empty())
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(elementBounds.w));
        height = static_cast<int>(std::ceil(elementBounds.h));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * elementBounds.h / elementBounds.w));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * elementBounds.w / elementBounds.h));
    }

    auto xScale = width / elementBounds.w;
    auto yScale = height / elementBounds.h;

    Matrix matrix(xScale, 0, 0, yScale, -elementBounds.x * xScale, -elementBounds.y * yScale);
    Bitmap bitmap(width, height);
    if(backgroundColor.value()) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

NOVASVG_INLINE void element::render(stb::image& image, const Matrix& matrix) const
{
    if(m_node == nullptr || image.empty())
        return;
    Bitmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = bitmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

NOVASVG_INLINE stb::image element::render_to_image(int width, int height, const Color& backgroundColor) const
{
    return bitmap_release_to_image(render_to_bitmap(width, height, backgroundColor));
}

NOVASVG_INLINE Matrix element::local_matrix() const
{
    if(m_node)
        return el(true)->local_transform();
    return Matrix();
}

NOVASVG_INLINE Matrix element::global_matrix() const
{
    if(m_node == nullptr)
        return Matrix();
    auto transform = el(true)->local_transform();
    for(auto parent = el()->parent_element(); parent; parent = parent->parent_element())
        transform.post_multiply(parent->local_transform());
    return transform;
}

NOVASVG_INLINE Box element::local_bounding_box() const
{
    return bounding_box().transformed(local_matrix());
}

NOVASVG_INLINE Box element::global_bounding_box() const
{
    return bounding_box().transformed(global_matrix());
}

NOVASVG_INLINE Box element::bounding_box() const
{
    if(m_node)
        return el(true)->paint_bounding_box();
    return Box();
}

NOVASVG_INLINE NodeList element::children() const
{
    if(m_node == nullptr)
        return NodeList();
    NodeList children;
    for(const auto& child : el()->children())
        children.push_back(child.get());
    return children;
}

NOVASVG_INLINE svg_element* element::el(bool layout_if_needed) const
{
    auto el = static_cast<svg_element*>(m_node);
    if(el && layout_if_needed)
        el->root_element()->layout_if_needed();
    return el;
}

NOVASVG_INLINE std::unique_ptr<document> document::load_file(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;
    std::string content;
    std::getline(fs, content, '\0');
    fs.close();
    return load_data(content);
}

NOVASVG_INLINE std::unique_ptr<document> document::load_data(const std::string& string)
{
    return load_data(string.data(), string.size());
}

NOVASVG_INLINE std::unique_ptr<document> document::load_data(const char* data)
{
    return load_data(data, std::strlen(data));
}

NOVASVG_INLINE std::unique_ptr<document> document::load_data(const char* data, size_t length)
{
    std::unique_ptr<document> dom(new document);
    if(!dom->parse(data, length))
        return nullptr;
    return dom;
}

NOVASVG_INLINE float document::width() const
{
    return root_element(true)->intrinsic_width();
}

NOVASVG_INLINE float document::height() const
{
    return root_element(true)->intrinsic_height();
}

NOVASVG_INLINE Box document::bounding_box() const
{
    return root_element(true)->local_transform().map_rect(root_element()->paint_bounding_box());
}

NOVASVG_INLINE void document::update_layout()
{
    m_rootElement->layout_if_needed();
}

NOVASVG_INLINE void document::force_layout()
{
    m_rootElement->force_layout();
}

NOVASVG_INLINE void document::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(bitmap.is_null())
        return;

    // A `background-color` in the root <svg>'s own inline style isn't a
    // real svg presentation property -- it's a CSS box-model property
    // that only means something when an svg is displayed as a standalone
    // dom/replaced el (which is exactly how a browser navigated
    // straight to a .svg file treats it, painting that color behind
    // everything). Most pure-vector svg renderers ignore it entirely, but
    // since generated svgs (Mermaid in particular) rely on it for a
    // correct-looking standalone render, honor it here the same way:
    // clear the bitmap to it before any content paints, so it sits under
    // everything and behind any transparency in the content itself. Takes
    // priority over render_to_bitmap()'s own `backgroundColor` argument
    // (this runs after that clear) since that argument means "what to
    // show through transparent content", and content here isn't actually
    // transparent -- it explicitly asked for this color.
    // A `background-color` in the root <svg>'s own style isn't a real svg
    // presentation property -- it's a CSS box-model property that only
    // means something when an svg is displayed as a standalone
    // dom/replaced el (exactly how a browser navigated straight
    // to a .svg file treats it, painting that color behind everything).
    // Most pure-vector svg renderers ignore it entirely, but since
    // generated svgs (Mermaid in particular) rely on it for a
    // correct-looking standalone render, honor it here the same way:
    // clear the bitmap to it before any content paints, so it sits under
    // everything and behind any transparency in the content itself. Takes
    // priority over render_to_bitmap()'s own `backgroundColor` argument
    // (this runs after that clear) since that argument means "what to
    // show through transparent content", and content here isn't actually
    // transparent -- it explicitly asked for this color.
    if(const auto& bg = root_element(true)->attribute(PropertyID::Background_Color); !bg.empty()) {
        color_t color;
        if(color_parse(&color, bg.data(), static_cast<int>(bg.length())) > 0) {
            auto argb = color_to_argb32(&color);
            // A fully transparent `background-color` (Mermaid emits `transparent`) paints nothing, so it must
            // not wipe the caller's `backgroundColor` argument -- that made every PNG come out transparent.
            // ponytail: a *semi*-transparent root color still replaces the argument instead of compositing
            // over it; upgrade path is to fill a rect through the canvas instead of clear().
            if((argb >> 24) & 0xff)
                bitmap.clear(Color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff));
        }
    }

    auto canvas = Canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::Painting, canvas);
    root_element(true)->render(state);
}

NOVASVG_INLINE Bitmap document::render_to_bitmap(int width, int height, const Color& backgroundColor) const
{
    auto intrinsic_width = root_element(true)->intrinsic_width();
    auto intrinsic_height = root_element()->intrinsic_height();
    if(intrinsic_width == 0.f || intrinsic_height == 0.f)
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsic_width));
        height = static_cast<int>(std::ceil(intrinsic_height));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsic_height / intrinsic_width));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsic_width / intrinsic_height));
    }

    auto xScale = width / intrinsic_width;
    auto yScale = height / intrinsic_height;

    Matrix matrix(xScale, 0, 0, yScale, 0, 0);
    Bitmap bitmap(width, height);
    if(backgroundColor.value()) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

NOVASVG_INLINE void document::render(stb::image& image, const Matrix& matrix) const
{
    if(image.empty())
        return;
    Bitmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = bitmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

NOVASVG_INLINE stb::image document::render_to_image(int width, int height, const Color& backgroundColor) const
{
    return bitmap_release_to_image(render_to_bitmap(width, height, backgroundColor));
}

NOVASVG_INLINE element document::element_from_point(float x, float y) const
{
    return root_element(true)->element_from_point(x, y);
}

NOVASVG_INLINE element document::element_by_id(const std::string& id) const
{
    return m_rootElement->element_by_id(id);
}

NOVASVG_INLINE element document::document_element() const
{
    return m_rootElement.get();
}

NOVASVG_INLINE svg_root_element* document::root_element(bool layout_if_needed) const
{
    if(layout_if_needed)
        m_rootElement->layout_if_needed();
    return m_rootElement.get();
}

NOVASVG_INLINE document::document(document&&) = default;
NOVASVG_INLINE document& document::operator=(document&&) = default;

NOVASVG_INLINE document::document() = default;
NOVASVG_INLINE document::~document() = default;

} // namespace novasvg

#endif // NOVASVG_H
