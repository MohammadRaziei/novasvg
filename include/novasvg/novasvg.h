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
#include <vector>

// True header-only linkage: every function/method defined in the detail/
// implementation headers is marked NOVASVG_INLINE (plain `inline`), so
// this whole library -- including its rendering backend -- can safely be
// #included from any number of translation units with no separate
// "#define NOVASVG_IMPLEMENTATION in exactly one .cpp" step required.
#ifndef NOVASVG_INLINE
#define NOVASVG_INLINE inline
#endif

#if defined(NOVASVG_BUILD_STATIC)
#define NOVASVG_EXPORT
#define NOVASVG_IMPORT
#elif (defined(_WIN32) || defined(__CYGWIN__))
#define NOVASVG_EXPORT __declspec(dllexport)
#define NOVASVG_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#define NOVASVG_EXPORT __attribute__((__visibility__("default")))
#define NOVASVG_IMPORT
#else
#define NOVASVG_EXPORT
#define NOVASVG_IMPORT
#endif

#ifdef NOVASVG_BUILD
#define NOVASVG_API NOVASVG_EXPORT
#else
#define NOVASVG_API NOVASVG_IMPORT
#endif

#define NOVASVG_VERSION_MAJOR 0
#define NOVASVG_VERSION_MINOR 3
#define NOVASVG_VERSION_PATCH 0

#define NOVASVG_VERSION_ENCODE(major, minor, patch) (((major) * 10000) + ((minor) * 100) + ((patch) * 1))
#define NOVASVG_VERSION NOVASVG_VERSION_ENCODE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

#define NOVASVG_VERSION_XSTRINGIZE(major, minor, patch) #major"."#minor"."#patch
#define NOVASVG_VERSION_STRINGIZE(major, minor, patch) NOVASVG_VERSION_XSTRINGIZE(major, minor, patch)
#define NOVASVG_VERSION_STRING NOVASVG_VERSION_STRINGIZE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

#include "detail/novacolor.h"

namespace novasvg {
namespace render {
struct surface;
struct matrix;
using surface_t = surface;
using matrix_t = matrix;
} // namespace render
} // namespace novasvg

/**
 * @brief Callback for cleaning up resources.
 *
 * This function is called to release resources associated with a specific operation.
 *
 * @param closure A user-defined pointer to the resource or context to be freed.
 */
typedef void (*novasvg_destroy_func_t)(void* closure);

/**
 * @brief A function pointer type for a write callback.
 * @param closure A pointer to user-defined data or context.
 * @param data A pointer to the data to be written.
 * @param size The size of the data in bytes.
 */
typedef void (*novasvg_write_func_t)(void* closure, void* data, int size);


namespace novasvg {
using namespace render;

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
std::string versionString();

/**
* @brief Add a font face from a file to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param filename The path to the font file.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename);

/**
* @brief Add a font face from memory to the cache.
* @param family The name of the font family. If an empty string is provided, the font will act as a fallback.
* @param bold Use `true` for bold, `false` otherwise.
* @param italic Use `true` for italic, `false` otherwise.
* @param data A pointer to the memory buffer containing the font data.
* @param length The size of the memory buffer in bytes.
* @param destroy_func Callback function to free the memory buffer when it is no longer needed.
* @param closure User-defined pointer passed to the `destroy_func` callback.
* @return `true` if the font face was successfully added to the cache, `false` otherwise.
*/
bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, novasvg_destroy_func_t destroy_func, void* closure);

/**
* @note Bitmap pixel format is ARGB32_Premultiplied.
*/
class NOVASVG_API Bitmap {
public:
    /**
     * @brief Constructs a null bitmap.
     */
    Bitmap() = default;

    /**
     * @brief Constructs a bitmap with the specified width and height.
     * @note A null bitmap will be returned if memory cannot be allocated.
     * @param width The width of the bitmap in pixels.
     * @param height The height of the bitmap in pixels.
     */
    Bitmap(int width, int height);

    /**
     * @brief Constructs a bitmap with the provided pixel data, width, height, and stride.
     *
     * @param data A pointer to the raw pixel data in ARGB32 Premultiplied format.
     * @param width The width of the bitmap in pixels.
     * @param height The height of the bitmap in pixels.
     * @param stride The number of bytes per row of pixel data (stride).
    */
    Bitmap(uint8_t* data, int width, int height, int stride);

    /**
     * @brief Copy constructor.
     * @param bitmap The bitmap to copy.
     */
    Bitmap(const Bitmap& bitmap);

    /**
     * @brief Move constructor.
     * @param bitmap The bitmap to move.
     */
    Bitmap(Bitmap&& bitmap);

    /**
     * @internal
     */
    Bitmap(surface_t* surface) : m_surface(surface) {}

    /**
     * @brief Cleans up any resources associated with the bitmap.
     */
    ~Bitmap();

    /**
     * @brief Copy assignment operator.
     * @param bitmap The bitmap to copy.
     * @return A reference to this bitmap.
     */
    Bitmap& operator=(const Bitmap& bitmap);

    /**
     * @brief Move assignment operator.
     * @param bitmap The bitmap to move.
     * @return A reference to this bitmap.
     */
    Bitmap& operator=(Bitmap&& bitmap);

    /**
     * @brief Swaps the content of this bitmap with another.
     * @param bitmap The bitmap to swap with.
     */
    void swap(Bitmap& bitmap);

    /**
     * @brief Gets the pointer to the raw pixel data.
     * @return A pointer to the raw pixel data.
     */
    uint8_t* data() const;

    /**
     * @brief Gets the width of the bitmap.
     * @return The width of the bitmap in pixels.
     */
    int width() const;

    /**
     * @brief Gets the height of the bitmap.
     * @return The height of the bitmap in pixels.
     */
    int height() const;

    /**
     * @brief Gets the stride of the bitmap.
     * @return The number of bytes per row of pixel data (stride).
     */
    int stride() const;

    /**
     * @brief Clears the bitmap with the specified color.
     * @param value The color to clear with.
     */
    void clear(const Color& value);

    /**
     * @brief Converts the bitmap pixel data from ARGB32 Premultiplied to RGBA Plain format in place.
     */
    void convertToRGBA();

    /**
     * @brief Checks if the bitmap is null.
     * @return True if the bitmap is null, false otherwise.
     */
    bool isNull() const { return m_surface == nullptr; }

    /**
     * @brief Checks if the bitmap is valid.
     * @deprecated This function has been deprecated. Use `isNull()` instead to check whether the bitmap is null.
     * @return True if the bitmap is valid, false otherwise.
     */
    bool valid() const { return !isNull(); }

    /**
     * @brief Writes the bitmap to a PNG file.
     * @param filename The name of the file to write.
     * @return True if the file was written successfully, false otherwise.
     */
    bool writeToPng(const std::string& filename) const;

    /**
     * @brief Writes the bitmap to a PNG stream.
     * @param callback Callback function for writing data.
     * @param closure User-defined data passed to the callback.
     * @return True if successful, false otherwise.
     */
    bool writeToPng(novasvg_write_func_t callback, void* closure) const;

    /**
     * @brief Writes the bitmap to a BMP file.
     */
    bool writeToBmp(const std::string& filename) const;

    /**
     * @brief Writes the bitmap to a BMP stream.
     */
    bool writeToBmp(novasvg_write_func_t callback, void* closure) const;

    /**
     * @brief Writes the bitmap to a TGA file.
     */
    bool writeToTga(const std::string& filename) const;

    /**
     * @brief Writes the bitmap to a TGA stream.
     */
    bool writeToTga(novasvg_write_func_t callback, void* closure) const;

    /**
     * @brief Writes the bitmap to a JPEG file.
     * @param quality JPEG quality, from 1 to 100.
     */
    bool writeToJpg(const std::string& filename, int quality = 80) const;

    /**
     * @brief Writes the bitmap to a JPEG stream.
     * @param quality JPEG quality, from 1 to 100.
     */
    bool writeToJpg(novasvg_write_func_t callback, void* closure, int quality = 80) const;

    /**
     * @internal
     */
    surface_t* surface() const { return m_surface; }

private:
    surface_t* release();
    surface_t* m_surface{nullptr};
};

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

class SVGNode;
class SVGTextNode;
class SVGElement;

class Element;
class TextNode;

class NOVASVG_API Node {
public:
    /**
     * @brief Constructs a null node.
     */
    Node() = default;

    /**
     * @brief Checks if the node is a text node.
     * @return True if the node is a text node, false otherwise.
     */
    bool isTextNode() const;

    /**
     * @brief Checks if the node is an element node.
     * @return True if the node is an element node, false otherwise.
     */
    bool isElement() const;

    /**
     * @brief Converts the node to a TextNode.
     * @return A TextNode or a null node if conversion is not possible.
     */
    TextNode toTextNode() const;

    /**
     * @brief Converts the node to an Element.
     * @return An Element or a null node if conversion is not possible.
     */
    Element toElement() const;

    /**
     * @brief Returns the parent element.
     * @return The parent element of this node. If this node has no parent, a null `Element` is returned.
     */
    Element parentElement() const;

    /**
     * @brief Checks if the node is null.
     * @return True if the node is null, false otherwise.
     */
    bool isNull() const { return m_node == nullptr; }

    /**
     * @brief Checks if the node is not null.
     * @return True if the node is not null, false otherwise.
     */
    operator bool() const { return !isNull(); }

    /**
     * @brief Checks if two nodes are equal.
     * @param element The node to compare.
     * @return True if equal, otherwise false.
     */
    bool operator==(const Node& node) const { return m_node == node.m_node; }

    /**
     * @brief Checks if two nodes are not equal.
     * @param element The node to compare.
     * @return True if not equal, otherwise false.
     */
    bool operator!=(const Node& node) const { return m_node != node.m_node; }

protected:
    Node(SVGNode* node);
    SVGNode* node() const { return m_node; }
    SVGNode* m_node{nullptr};
    friend class Element;
};

using NodeList = std::vector<Node>;

class NOVASVG_API TextNode : public Node {
public:
    /**
     * @brief Constructs a null text node.
     */
    TextNode() = default;

    /**
     * @brief Returns the text content of the node.
     * @return A string representing the text content.
     */
    const std::string& data() const;

    /**
     * @brief Sets the text content of the node.
     * @param data The new text content to set.
     */
    void setData(const std::string& data);

private:
    TextNode(SVGTextNode* text);
    SVGTextNode* text() const;
    friend class Node;
};

class NOVASVG_API Element : public Node {
public:
    /**
     * @brief Constructs a null element.
     */
    Element() = default;

    /**
     * @brief Checks if the element has a specific attribute.
     * @param name The name of the attribute to check.
     * @return True if the element has the specified attribute, false otherwise.
     */
    bool hasAttribute(const std::string& name) const;

    /**
     * @brief Retrieves the value of an attribute.
     * @param name The name of the attribute to retrieve.
     * @return The value of the attribute as a string.
     */
    const std::string& getAttribute(const std::string& name) const;

    /**
     * @brief Sets the value of an attribute.
     * @param name The name of the attribute to set.
     * @param value The value to assign to the attribute.
     */
    void setAttribute(const std::string& name, const std::string& value);

    /**
     * @brief Renders the element onto a bitmap using a transformation matrix.
     * @param bitmap The bitmap to render onto.
     * @param The root transformation matrix.
     */
    void render(Bitmap& bitmap, const Matrix& matrix = Matrix()) const;

    /**
     * @brief Renders the element to a bitmap with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return A Bitmap containing the raster representation of the element.
     */
    Bitmap renderToBitmap(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @brief Retrieves the local transformation matrix of the element.
     * @return The matrix that applies only to the element, relative to its parent.
     */
    Matrix getLocalMatrix() const;

    /**
     * @brief Retrieves the global transformation matrix of the element.
     * @return The matrix combining the element's local and all parent transformations.
     */
    Matrix getGlobalMatrix() const;

    /**
     * @brief Retrieves the local bounding box of the element.
     * @return A Box representing the bounding box after applying local transformations.
     */
    Box getLocalBoundingBox() const;

    /**
     * @brief Retrieves the global bounding box of the element.
     * @return A Box representing the bounding box after applying global transformations.
     */
    Box getGlobalBoundingBox() const;

    /**
     * @brief Retrieves the bounding box of the element without any transformations.
     * @return A Box representing the bounding box of the element without any transformations applied.
     */
    Box getBoundingBox() const;

    /**
     * @brief Returns the child nodes of this node.
     * @return A NodeList containing the child nodes.
     */
    NodeList children() const;

private:
    Element(SVGElement* element);
    SVGElement* element(bool layoutIfNeeded = false) const;
    friend class Node;
    friend class Document;
};

using ElementList = std::vector<Element>;

class SVGRootElement;

class NOVASVG_API Document {
public:
    /**
     * @brief Load an SVG document from a file.
     * @param filename The path to the SVG file.
     * @return A pointer to the loaded `Document`, or `nullptr` on failure.
     */
    static std::unique_ptr<Document> loadFromFile(const std::string& filename);

    /**
     * @brief Load an SVG document from a string.
     * @param string The SVG data as a string.
     * @return A pointer to the loaded `Document`, or `nullptr` on failure.
     */
    static std::unique_ptr<Document> loadFromData(const std::string& string);

    /**
     * @brief Load an SVG document from a null-terminated string.
     * @param data The string containing the SVG data.
     * @return A pointer to the loaded `Document`, or `nullptr` on failure.
     */
    static std::unique_ptr<Document> loadFromData(const char* data);

    /**
     * @brief Load an SVG document from a string with a specified length.
     * @param data The string containing the SVG data.
     * @param length The length of the string in bytes.
     * @return A pointer to the loaded `Document`, or `nullptr` on failure.
     */
    static std::unique_ptr<Document> loadFromData(const char* data, size_t length);

    /**
     * @brief Applies a CSS stylesheet to the document.
     * @param content A string containing the CSS rules to apply, with comments removed.
     */
    void applyStyleSheet(const std::string& content);

    /**
     * @brief Selects all elements that match the given CSS selector(s).
     * @param content A string containing the CSS selector(s) to match elements.
     * @return A list of elements matching the selector(s).
     */
    ElementList querySelectorAll(const std::string& content) const;

    /**
     * @brief Returns the intrinsic width of the document in pixels.
     * @return The width of the document.
     */
    float width() const;

    /**
     * @brief Returns the intrinsic height of the document in pixels.
     * @return The height of the document.
     */
    float height() const;

    /**
     * @brief Returns the smallest rectangle that encloses the document content.
     * @return A Box representing the bounding box of the document.
     */
    Box boundingBox() const;

    /**
     * @brief Updates the layout of the document if needed.
     */
    void updateLayout();

    /**
     * @brief Forces an immediate layout update.
     */
    void forceLayout();

    /**
     * @brief Renders the document onto a bitmap using a transformation matrix.
     * @param bitmap The bitmap to render onto.
     * @param The root transformation matrix.
     */
    void render(Bitmap& bitmap, const Matrix& matrix = Matrix()) const;

    /**
     * @brief Renders the document to a bitmap with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return A Bitmap containing the raster representation of the document.
     */
    Bitmap renderToBitmap(int width = -1, int height = -1, const Color& backgroundColor = Color(0, 0, 0, 0)) const;

    /**
     * @brief Returns the topmost element under the specified point.
     * @param x The x-coordinate in viewport space.
     * @param y The y-coordinate in viewport space.
     * @return The topmost Element at the given point, or a null `Element` if no match is found.
     */
    Element elementFromPoint(float x, float y) const;

    /**
     * @brief Retrieves an element by its ID.
     * @param id The ID of the element to retrieve.
     * @return The Element with the specified ID, or a null `Element` if not found.
     */
    Element getElementById(const std::string& id) const;

    /**
     * @brief Retrieves the document element.
     * @return The root Element of the document.
     */
    Element documentElement() const;

    Document(Document&&);
    Document& operator=(Document&&);
    ~Document();

private:
    Document();
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    SVGRootElement* rootElement(bool layoutIfNeeded = false) const;
    bool parse(const char* data, size_t length);
    std::unique_ptr<SVGRootElement> m_rootElement;
    friend class SVGURIReference;
    friend class SVGNode;
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

#include "detail/render/blend.h"
#include "detail/render/canvas.h"
#include "detail/render/font.h"
#include "detail/render/matrix.h"
#include "detail/render/paint.h"
#include "detail/render/path.h"
#include "detail/render/rasterize.h"
#include "detail/render/surface.h"

// Bitmap owns all image-writing (PNG/BMP/TGA/JPG) below; stb_image_write
// is only ever touched from here.
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "detail/render/vendor/stb_image_write.h"

#include "detail/graphics.h"
#include "detail/svgelement.h"
#include "detail/svgparser.hpp"

namespace novasvg {
using namespace render;

// ---------------------------------------------------------------
// Implementation (formerly detail/novasvg.hpp -- merged in since
// this header is fully NOVASVG_INLINE now, there's no reason left
// to keep the public declarations and their definitions apart).
// ---------------------------------------------------------------

using namespace render; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE int version()
{
    return NOVASVG_VERSION;
}

NOVASVG_INLINE std::string versionString()
{
    return NOVASVG_VERSION_STRING;
}

NOVASVG_INLINE bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(filename));
}

NOVASVG_INLINE bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, destroy_func_t destroy_func, void* closure)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(data, length, destroy_func, closure));
}

NOVASVG_INLINE Bitmap::Bitmap(int width, int height)
    : m_surface(surface_create(width, height))
{
}

NOVASVG_INLINE Bitmap::Bitmap(uint8_t* data, int width, int height, int stride)
    : m_surface(surface_create_for_data(data, width, height, stride))
{
}

NOVASVG_INLINE Bitmap::Bitmap(const Bitmap& bitmap)
    : m_surface(surface_reference(bitmap.surface()))
{
}

NOVASVG_INLINE Bitmap::Bitmap(Bitmap&& bitmap)
    : m_surface(bitmap.release())
{
}

NOVASVG_INLINE Bitmap::~Bitmap()
{
    surface_destroy(m_surface);
}

NOVASVG_INLINE Bitmap& Bitmap::operator=(const Bitmap& bitmap)
{
    Bitmap(bitmap).swap(*this);
    return *this;
}

NOVASVG_INLINE void Bitmap::swap(Bitmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

NOVASVG_INLINE uint8_t* Bitmap::data() const
{
    if(m_surface)
        return surface_get_data(m_surface);
    return nullptr;
}

NOVASVG_INLINE int Bitmap::width() const
{
    if(m_surface)
        return surface_get_width(m_surface);
    return 0;
}

NOVASVG_INLINE int Bitmap::height() const
{
    if(m_surface)
        return surface_get_height(m_surface);
    return 0;
}

NOVASVG_INLINE int Bitmap::stride() const
{
    if(m_surface)
        return surface_get_stride(m_surface);
    return 0;
}

NOVASVG_INLINE void Bitmap::clear(const Color& value)
{
    if(m_surface == nullptr)
        return;
    color_t color;
    color_init_rgba32(&color, value.value());
    surface_clear(m_surface, &color);
}

NOVASVG_INLINE void Bitmap::convertToRGBA()
{
    if(m_surface == nullptr)
        return;
    auto data = surface_get_data(m_surface);
    auto width = surface_get_width(m_surface);
    auto height = surface_get_height(m_surface);
    auto stride = surface_get_stride(m_surface);
    convert_argb_to_rgba(data, data, width, height, stride);
}

NOVASVG_INLINE Bitmap& Bitmap::operator=(Bitmap&& bitmap)
{
    Bitmap(std::move(bitmap)).swap(*this);
    return *this;
}

namespace {

// One-pass, unpremultiplied RGBA copy of a Bitmap's pixels -- lives only
// long enough to hand to stb_image_write. This is an implementation
// detail of Bitmap's write*() methods below, not a type of its own: no
// declaration in the class, nothing exported.
//
// Bitmap itself stays ARGB32-Premultiplied always -- it doubles as a
// drawable surface (Canvas can render onto an existing Bitmap) and as a
// texture source (canvas_set_texture reads bitmap.surface() directly for
// <image>/pattern fills), both of which need premultiplied alpha for
// `over`-compositing to be correct. Only encoding needs straight alpha,
// so only encoding pays for the one unavoidable unpremultiply pass, via
// this scratch buffer -- never a permanent, library-wide format change.
class BitmapRgbaScratch {
public:
    explicit BitmapRgbaScratch(const Bitmap& bitmap)
        : m_width(bitmap.width())
        , m_height(bitmap.height())
        , m_stride(m_width * 4)
        , m_data(bitmap.isNull() ? nullptr : static_cast<uint8_t*>(malloc(size_t(m_stride) * size_t(m_height))))
    {
        if(m_data != nullptr)
            convert_argb_to_rgba(m_data, bitmap.data(), m_width, m_height, bitmap.stride());
    }

    ~BitmapRgbaScratch() { free(m_data); }

    BitmapRgbaScratch(const BitmapRgbaScratch&) = delete;
    BitmapRgbaScratch& operator=(const BitmapRgbaScratch&) = delete;

    explicit operator bool() const { return m_data != nullptr; }
    uint8_t* data() const { return m_data; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int stride() const { return m_stride; }

private:
    int m_width;
    int m_height;
    int m_stride;
    uint8_t* m_data;
};

} // namespace

NOVASVG_INLINE bool Bitmap::writeToPng(const std::string& filename) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_png(filename.data(), rgba.width(), rgba.height(), 4, rgba.data(), rgba.stride()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToPng(novasvg_write_func_t callback, void* closure) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_png_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data(), rgba.stride()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToBmp(const std::string& filename) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_bmp(filename.data(), rgba.width(), rgba.height(), 4, rgba.data()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToBmp(novasvg_write_func_t callback, void* closure) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_bmp_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToTga(const std::string& filename) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_tga(filename.data(), rgba.width(), rgba.height(), 4, rgba.data()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToTga(novasvg_write_func_t callback, void* closure) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_tga_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data()) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToJpg(const std::string& filename, int quality) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_jpg(filename.data(), rgba.width(), rgba.height(), 4, rgba.data(), quality) != 0;
}

NOVASVG_INLINE bool Bitmap::writeToJpg(novasvg_write_func_t callback, void* closure, int quality) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;
    return stbi_write_jpg_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data(), quality) != 0;
}

NOVASVG_INLINE surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
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
    return Transform(matrix).mapRect(*this);
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

NOVASVG_INLINE Node::Node(SVGNode* node)
    : m_node(node)
{
}

NOVASVG_INLINE bool Node::isTextNode() const
{
    return m_node && m_node->isTextNode();
}

NOVASVG_INLINE bool Node::isElement() const
{
    return m_node && m_node->isElement();
}

NOVASVG_INLINE TextNode Node::toTextNode() const
{
    if(m_node && m_node->isTextNode())
        return static_cast<SVGTextNode*>(m_node);
    return TextNode();
}

NOVASVG_INLINE Element Node::toElement() const
{
    if(m_node && m_node->isElement())
        return static_cast<SVGElement*>(m_node);
    return Element();
}

NOVASVG_INLINE Element Node::parentElement() const
{
    if(m_node)
        return m_node->parentElement();
    return Element();
}

NOVASVG_INLINE TextNode::TextNode(SVGTextNode* text)
    : Node(text)
{
}

NOVASVG_INLINE const std::string& TextNode::data() const
{
    if(m_node)
        return text()->data();
    return emptyString;
}

NOVASVG_INLINE void TextNode::setData(const std::string& data)
{
    if(m_node) {
        text()->setData(data);
    }
}

NOVASVG_INLINE SVGTextNode* TextNode::text() const
{
    return static_cast<SVGTextNode*>(m_node);
}

NOVASVG_INLINE Element::Element(SVGElement* element)
    : Node(element)
{
}

NOVASVG_INLINE bool Element::hasAttribute(const std::string& name) const
{
    if(m_node)
        return element()->hasAttribute(name);
    return false;
}

NOVASVG_INLINE const std::string& Element::getAttribute(const std::string& name) const
{
    if(m_node)
        return element()->getAttribute(name);
    return emptyString;
}

NOVASVG_INLINE void Element::setAttribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        element()->setAttribute(name, value);
    }
}

NOVASVG_INLINE void Element::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(m_node == nullptr || bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    element(true)->render(state);
}

NOVASVG_INLINE Bitmap Element::renderToBitmap(int width, int height, const Color& backgroundColor) const
{
    if(m_node == nullptr)
        return Bitmap();
    auto elementBounds = element(true)->localTransform().mapRect(element()->paintBoundingBox());
    if(elementBounds.isEmpty())
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

NOVASVG_INLINE Matrix Element::getLocalMatrix() const
{
    if(m_node)
        return element(true)->localTransform();
    return Matrix();
}

NOVASVG_INLINE Matrix Element::getGlobalMatrix() const
{
    if(m_node == nullptr)
        return Matrix();
    auto transform = element(true)->localTransform();
    for(auto parent = element()->parentElement(); parent; parent = parent->parentElement())
        transform.postMultiply(parent->localTransform());
    return transform;
}

NOVASVG_INLINE Box Element::getLocalBoundingBox() const
{
    return getBoundingBox().transformed(getLocalMatrix());
}

NOVASVG_INLINE Box Element::getGlobalBoundingBox() const
{
    return getBoundingBox().transformed(getGlobalMatrix());
}

NOVASVG_INLINE Box Element::getBoundingBox() const
{
    if(m_node)
        return element(true)->paintBoundingBox();
    return Box();
}

NOVASVG_INLINE NodeList Element::children() const
{
    if(m_node == nullptr)
        return NodeList();
    NodeList children;
    for(const auto& child : element()->children())
        children.push_back(child.get());
    return children;
}

NOVASVG_INLINE SVGElement* Element::element(bool layoutIfNeeded) const
{
    auto element = static_cast<SVGElement*>(m_node);
    if(element && layoutIfNeeded)
        element->rootElement()->layoutIfNeeded();
    return element;
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromFile(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;
    std::string content;
    std::getline(fs, content, '\0');
    fs.close();
    return loadFromData(content);
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const std::string& string)
{
    return loadFromData(string.data(), string.size());
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const char* data)
{
    return loadFromData(data, std::strlen(data));
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const char* data, size_t length)
{
    std::unique_ptr<Document> document(new Document);
    if(!document->parse(data, length))
        return nullptr;
    return document;
}

NOVASVG_INLINE float Document::width() const
{
    return rootElement(true)->intrinsicWidth();
}

NOVASVG_INLINE float Document::height() const
{
    return rootElement(true)->intrinsicHeight();
}

NOVASVG_INLINE Box Document::boundingBox() const
{
    return rootElement(true)->localTransform().mapRect(rootElement()->paintBoundingBox());
}

NOVASVG_INLINE void Document::updateLayout()
{
    m_rootElement->layoutIfNeeded();
}

NOVASVG_INLINE void Document::forceLayout()
{
    m_rootElement->forceLayout();
}

NOVASVG_INLINE void Document::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    rootElement(true)->render(state);
}

NOVASVG_INLINE Bitmap Document::renderToBitmap(int width, int height, const Color& backgroundColor) const
{
    auto intrinsicWidth = rootElement(true)->intrinsicWidth();
    auto intrinsicHeight = rootElement()->intrinsicHeight();
    if(intrinsicWidth == 0.f || intrinsicHeight == 0.f)
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsicWidth));
        height = static_cast<int>(std::ceil(intrinsicHeight));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsicHeight / intrinsicWidth));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsicWidth / intrinsicHeight));
    }

    auto xScale = width / intrinsicWidth;
    auto yScale = height / intrinsicHeight;

    Matrix matrix(xScale, 0, 0, yScale, 0, 0);
    Bitmap bitmap(width, height);
    if(backgroundColor.value()) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

NOVASVG_INLINE Element Document::elementFromPoint(float x, float y) const
{
    return rootElement(true)->elementFromPoint(x, y);
}

NOVASVG_INLINE Element Document::getElementById(const std::string& id) const
{
    return m_rootElement->getElementById(id);
}

NOVASVG_INLINE Element Document::documentElement() const
{
    return m_rootElement.get();
}

NOVASVG_INLINE SVGRootElement* Document::rootElement(bool layoutIfNeeded) const
{
    if(layoutIfNeeded)
        m_rootElement->layoutIfNeeded();
    return m_rootElement.get();
}

NOVASVG_INLINE Document::Document(Document&&) = default;
NOVASVG_INLINE Document& Document::operator=(Document&&) = default;

NOVASVG_INLINE Document::Document() = default;
NOVASVG_INLINE Document::~Document() = default;

} // namespace novasvg

#endif // NOVASVG_H
