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
     * @brief Image formats Bitmap can encode itself as.
     */
    enum class Format {
        Auto,  ///< Detect from the filename's extension (write(filename, ...) only); Png if unrecognized or absent.
        Png,
        Bmp,
        Tga,
        Jpg
    };

    /**
     * @brief Writes the bitmap to a file, encoding it as `format`.
     * @param filename The name of the file to write. With `Format::Auto` (the
     * default), the format is picked from this name's extension (.png/.bmp/.tga/.jpg/.jpeg),
     * falling back to PNG if it's unrecognized or missing.
     * @param format Which format to encode as, or `Format::Auto` to detect it from `filename`.
     * @param jpgQuality JPEG quality (1-100), used only when writing as `Format::Jpg`.
     * @return True if the file was written successfully, false otherwise.
     */
    bool write(const std::string& filename, Format format = Format::Auto, int jpgQuality = 80) const;

    /**
     * @brief Writes the bitmap to a stream, encoding it as `format`.
     * @param callback Callback function for writing data.
     * @param closure User-defined data passed to the callback.
     * @param format Which format to encode as. `Format::Auto` has nothing to detect
     * from here, so it's treated the same as `Format::Png`.
     * @param jpgQuality JPEG quality (1-100), used only when writing as `Format::Jpg`.
     * @return True if successful, false otherwise.
     */
    bool write(novasvg_write_func_t callback, void* closure, Format format = Format::Png, int jpgQuality = 80) const;

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
// Image encoding (PNG, BMP, TGA, JPEG) for Bitmap::write(). Ported from
// stb_image_write v1.16 (Sean Barrett, public domain / MIT) with every
// identifier renamed to this project's naming and folded into
// novasvg::render. The single-header config-switch (extern vs static
// linkage) and standalone-include-guard machinery were dropped since this
// now only ever exists inlined here, always fully built in; the several
// `#ifndef NOVASVG_IW_NO_STDIO` blocks below are untouched from upstream
// (NOVASVG_IW_NO_STDIO is just never defined, same as upstream's own
// default) -- left alone deliberately rather than risk mismatching one of
// several differently-spelled #endif comments while unwrapping them.
// Upstream: https://github.com/nothings/stb

namespace novasvg {
namespace render {

#include <stdlib.h>

#ifndef NOVASVG_IW_NO_STDIO
NOVASVG_INLINE int write_png(char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
NOVASVG_INLINE int write_bmp(char const *filename, int w, int h, int comp, const void  *data);
NOVASVG_INLINE int write_tga(char const *filename, int w, int h, int comp, const void  *data);
NOVASVG_INLINE int write_hdr(char const *filename, int w, int h, int comp, const float *data);
NOVASVG_INLINE int write_jpg(char const *filename, int x, int y, int comp, const void  *data, int quality);

#ifdef NOVASVG_IW_WINDOWS_UTF8
NOVASVG_INLINE int convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input);
#endif
#endif

NOVASVG_INLINE int write_png_to_func(write_func_t func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
NOVASVG_INLINE int write_bmp_to_func(write_func_t func, void *context, int w, int h, int comp, const void  *data);
NOVASVG_INLINE int write_tga_to_func(write_func_t func, void *context, int w, int h, int comp, const void  *data);
NOVASVG_INLINE int write_hdr_to_func(write_func_t func, void *context, int w, int h, int comp, const float *data);
NOVASVG_INLINE int write_jpg_to_func(write_func_t func, void *context, int x, int y, int comp, const void  *data, int quality);

NOVASVG_INLINE void set_flip_vertically_on_write(int flip_boolean);


#ifdef _WIN32
   #ifndef _CRT_SECURE_NO_WARNINGS
   #define _CRT_SECURE_NO_WARNINGS
   #endif
   #ifndef _CRT_NONSTDC_NO_DEPRECATE
   #define _CRT_NONSTDC_NO_DEPRECATE
   #endif
#endif

#ifndef NOVASVG_IW_NO_STDIO
#include <stdio.h>
#endif // NOVASVG_IW_NO_STDIO

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(NOVASVG_IW_MALLOC) && defined(NOVASVG_IW_FREE) && (defined(NOVASVG_IW_REALLOC) || defined(NOVASVG_IW_REALLOC_SIZED))
// ok
#elif !defined(NOVASVG_IW_MALLOC) && !defined(NOVASVG_IW_FREE) && !defined(NOVASVG_IW_REALLOC) && !defined(NOVASVG_IW_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of NOVASVG_IW_MALLOC, NOVASVG_IW_FREE, and NOVASVG_IW_REALLOC (or NOVASVG_IW_REALLOC_SIZED)."
#endif

#ifndef NOVASVG_IW_MALLOC
#define NOVASVG_IW_MALLOC(sz)        malloc(sz)
#define NOVASVG_IW_REALLOC(p,newsz)  realloc(p,newsz)
#define NOVASVG_IW_FREE(p)           free(p)
#endif

#ifndef NOVASVG_IW_REALLOC_SIZED
#define NOVASVG_IW_REALLOC_SIZED(p,oldsz,newsz) NOVASVG_IW_REALLOC(p,newsz)
#endif


#ifndef NOVASVG_IW_MEMMOVE
#define NOVASVG_IW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif


#ifndef NOVASVG_IW_ASSERT
#include <assert.h>
#define NOVASVG_IW_ASSERT(x) assert(x)
#endif

#define NOVASVG_IW_UCHAR(x) (unsigned char) ((x) & 0xff)

static int g_png_compression_level = 8;
static int g_tga_with_rle = 1;
static int g_png_force_filter = -1;

static int g_flip_vertically_on_write = 0;

NOVASVG_INLINE void set_flip_vertically_on_write(int flag)
{
   g_flip_vertically_on_write = flag;
}

typedef struct
{
   write_func_t func;
   void *context;
   unsigned char buffer[64];
   int buf_used;
} write_context;

// initialize a callback-based context
static void write_start_callbacks(write_context *s, write_func_t c, void *context)
{
   s->func    = c;
   s->context = context;
}

#ifndef NOVASVG_IW_NO_STDIO

static void write_stdio_callback(void *context, void *data, int size)
{
   fwrite(data,1,size,(FILE*) context);
}

#if defined(_WIN32) && defined(NOVASVG_IW_WINDOWS_UTF8)
#ifdef __cplusplus
#define NOVASVG_IW_EXTERN extern "C"
#else
#define NOVASVG_IW_EXTERN extern
#endif
NOVASVG_IW_EXTERN __declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags, const char *str, int cbmb, wchar_t *widestr, int cchwide);
NOVASVG_IW_EXTERN __declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t *widestr, int cchwide, char *str, int cbmb, const char *defchar, int *used_default);

NOVASVG_INLINE int convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input)
{
   return WideCharToMultiByte(65001 /* UTF8 */, 0, input, -1, buffer, (int) bufferlen, NULL, NULL);
}
#endif

static FILE *open_file(char const *filename, char const *mode)
{
   FILE *f;
#if defined(_WIN32) && defined(NOVASVG_IW_WINDOWS_UTF8)
   wchar_t wMode[64];
   wchar_t wFilename[1024];
   if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, filename, -1, wFilename, sizeof(wFilename)/sizeof(*wFilename)))
      return 0;

   if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, mode, -1, wMode, sizeof(wMode)/sizeof(*wMode)))
      return 0;

#if defined(_MSC_VER) && _MSC_VER >= 1400
   if (0 != _wfopen_s(&f, wFilename, wMode))
      f = 0;
#else
   f = _wfopen(wFilename, wMode);
#endif

#elif defined(_MSC_VER) && _MSC_VER >= 1400
   if (0 != fopen_s(&f, filename, mode))
      f=0;
#else
   f = fopen(filename, mode);
#endif
   return f;
}

static int write_start_file(write_context *s, const char *filename)
{
   FILE *f = open_file(filename, "wb");
   write_start_callbacks(s, write_stdio_callback, (void *) f);
   return f != NULL;
}

static void write_end_file(write_context *s)
{
   fclose((FILE *)s->context);
}

#endif // !NOVASVG_IW_NO_STDIO

typedef unsigned int iw_uint32;
typedef int iw_uint32_size_check[sizeof(iw_uint32) == 4 ? 1 : -1];

static void write_formatted_v(write_context *s, const char *fmt, va_list v)
{
   while (*fmt) {
      switch (*fmt++) {
         case ' ': break;
         case '1': { unsigned char x = NOVASVG_IW_UCHAR(va_arg(v, int));
                     s->func(s->context,&x,1);
                     break; }
         case '2': { int x = va_arg(v,int);
                     unsigned char b[2];
                     b[0] = NOVASVG_IW_UCHAR(x);
                     b[1] = NOVASVG_IW_UCHAR(x>>8);
                     s->func(s->context,b,2);
                     break; }
         case '4': { iw_uint32 x = va_arg(v,int);
                     unsigned char b[4];
                     b[0]=NOVASVG_IW_UCHAR(x);
                     b[1]=NOVASVG_IW_UCHAR(x>>8);
                     b[2]=NOVASVG_IW_UCHAR(x>>16);
                     b[3]=NOVASVG_IW_UCHAR(x>>24);
                     s->func(s->context,b,4);
                     break; }
         default:
            NOVASVG_IW_ASSERT(0);
            return;
      }
   }
}

static void write_formatted(write_context *s, const char *fmt, ...)
{
   va_list v;
   va_start(v, fmt);
   write_formatted_v(s, fmt, v);
   va_end(v);
}

static void write_flush(write_context *s)
{
   if (s->buf_used) {
      s->func(s->context, &s->buffer, s->buf_used);
      s->buf_used = 0;
   }
}

static void write_putc(write_context *s, unsigned char c)
{
   s->func(s->context, &c, 1);
}

static void write_u8(write_context *s, unsigned char a)
{
   if ((size_t)s->buf_used + 1 > sizeof(s->buffer))
      write_flush(s);
   s->buffer[s->buf_used++] = a;
}

static void write_u8x3(write_context *s, unsigned char a, unsigned char b, unsigned char c)
{
   int n;
   if ((size_t)s->buf_used + 3 > sizeof(s->buffer))
      write_flush(s);
   n = s->buf_used;
   s->buf_used = n+3;
   s->buffer[n+0] = a;
   s->buffer[n+1] = b;
   s->buffer[n+2] = c;
}

static void write_pixel(write_context *s, int rgb_dir, int comp, int write_alpha, int expand_mono, unsigned char *d)
{
   unsigned char bg[3] = { 255, 0, 255}, px[3];
   int k;

   if (write_alpha < 0)
      write_u8(s, d[comp - 1]);

   switch (comp) {
      case 2: // 2 pixels = mono + alpha, alpha is written separately, so same as 1-channel case
      case 1:
         if (expand_mono)
            write_u8x3(s, d[0], d[0], d[0]); // monochrome bmp
         else
            write_u8(s, d[0]);  // monochrome TGA
         break;
      case 4:
         if (!write_alpha) {
            // composite against pink background
            for (k = 0; k < 3; ++k)
               px[k] = bg[k] + ((d[k] - bg[k]) * d[3]) / 255;
            write_u8x3(s, px[1 - rgb_dir], px[1], px[1 + rgb_dir]);
            break;
         }
         /* FALLTHROUGH */
      case 3:
         write_u8x3(s, d[1 - rgb_dir], d[1], d[1 + rgb_dir]);
         break;
   }
   if (write_alpha > 0)
      write_u8(s, d[comp - 1]);
}

static void write_pixels(write_context *s, int rgb_dir, int vdir, int x, int y, int comp, void *data, int write_alpha, int scanline_pad, int expand_mono)
{
   iw_uint32 zero = 0;
   int i,j, j_end;

   if (y <= 0)
      return;

   if (g_flip_vertically_on_write)
      vdir *= -1;

   if (vdir < 0) {
      j_end = -1; j = y-1;
   } else {
      j_end =  y; j = 0;
   }

   for (; j != j_end; j += vdir) {
      for (i=0; i < x; ++i) {
         unsigned char *d = (unsigned char *) data + (j*x+i)*comp;
         write_pixel(s, rgb_dir, comp, write_alpha, expand_mono, d);
      }
      write_flush(s);
      s->func(s->context, &zero, scanline_pad);
   }
}

static int outfile(write_context *s, int rgb_dir, int vdir, int x, int y, int comp, int expand_mono, void *data, int alpha, int pad, const char *fmt, ...)
{
   if (y < 0 || x < 0) {
      return 0;
   } else {
      va_list v;
      va_start(v, fmt);
      write_formatted_v(s, fmt, v);
      va_end(v);
      write_pixels(s,rgb_dir,vdir,x,y,comp,data,alpha,pad, expand_mono);
      return 1;
   }
}

static int write_bmp_core(write_context *s, int x, int y, int comp, const void *data)
{
   if (comp != 4) {
      // write RGB bitmap
      int pad = (-x*3) & 3;
      return outfile(s,-1,-1,x,y,comp,1,(void *) data,0,pad,
              "11 4 22 4" "4 44 22 444444",
              'B', 'M', 14+40+(x*3+pad)*y, 0,0, 14+40,  // file header
               40, x,y, 1,24, 0,0,0,0,0,0);             // bitmap header
   } else {
      // RGBA bitmaps need a v4 header
      // use BI_BITFIELDS mode with 32bpp and alpha mask
      // (straight BI_RGB with alpha mask doesn't work in most readers)
      return outfile(s,-1,-1,x,y,comp,1,(void *)data,1,0,
         "11 4 22 4" "4 44 22 444444 4444 4 444 444 444 444",
         'B', 'M', 14+108+x*y*4, 0, 0, 14+108, // file header
         108, x,y, 1,32, 3,0,0,0,0,0, 0xff0000,0xff00,0xff,0xff000000u, 0, 0,0,0, 0,0,0, 0,0,0, 0,0,0); // bitmap V4 header
   }
}

NOVASVG_INLINE int write_bmp_to_func(write_func_t func, void *context, int x, int y, int comp, const void *data)
{
   write_context s = { 0 };
   write_start_callbacks(&s, func, context);
   return write_bmp_core(&s, x, y, comp, data);
}

#ifndef NOVASVG_IW_NO_STDIO
NOVASVG_INLINE int write_bmp(char const *filename, int x, int y, int comp, const void *data)
{
   write_context s = { 0 };
   if (write_start_file(&s,filename)) {
      int r = write_bmp_core(&s, x, y, comp, data);
      write_end_file(&s);
      return r;
   } else
      return 0;
}
#endif //!NOVASVG_IW_NO_STDIO

static int write_tga_core(write_context *s, int x, int y, int comp, void *data)
{
   int has_alpha = (comp == 2 || comp == 4);
   int colorbytes = has_alpha ? comp-1 : comp;
   int format = colorbytes < 2 ? 3 : 2; // 3 color channels (RGB/RGBA) = 2, 1 color channel (Y/YA) = 3

   if (y < 0 || x < 0)
      return 0;

   if (!g_tga_with_rle) {
      return outfile(s, -1, -1, x, y, comp, 0, (void *) data, has_alpha, 0,
         "111 221 2222 11", 0, 0, format, 0, 0, 0, 0, 0, x, y, (colorbytes + has_alpha) * 8, has_alpha * 8);
   } else {
      int i,j,k;
      int jend, jdir;

      write_formatted(s, "111 221 2222 11", 0,0,format+8, 0,0,0, 0,0,x,y, (colorbytes + has_alpha) * 8, has_alpha * 8);

      if (g_flip_vertically_on_write) {
         j = 0;
         jend = y;
         jdir = 1;
      } else {
         j = y-1;
         jend = -1;
         jdir = -1;
      }
      for (; j != jend; j += jdir) {
         unsigned char *row = (unsigned char *) data + j * x * comp;
         int len;

         for (i = 0; i < x; i += len) {
            unsigned char *begin = row + i * comp;
            int diff = 1;
            len = 1;

            if (i < x - 1) {
               ++len;
               diff = memcmp(begin, row + (i + 1) * comp, comp);
               if (diff) {
                  const unsigned char *prev = begin;
                  for (k = i + 2; k < x && len < 128; ++k) {
                     if (memcmp(prev, row + k * comp, comp)) {
                        prev += comp;
                        ++len;
                     } else {
                        --len;
                        break;
                     }
                  }
               } else {
                  for (k = i + 2; k < x && len < 128; ++k) {
                     if (!memcmp(begin, row + k * comp, comp)) {
                        ++len;
                     } else {
                        break;
                     }
                  }
               }
            }

            if (diff) {
               unsigned char header = NOVASVG_IW_UCHAR(len - 1);
               write_u8(s, header);
               for (k = 0; k < len; ++k) {
                  write_pixel(s, -1, comp, has_alpha, 0, begin + k * comp);
               }
            } else {
               unsigned char header = NOVASVG_IW_UCHAR(len - 129);
               write_u8(s, header);
               write_pixel(s, -1, comp, has_alpha, 0, begin);
            }
         }
      }
      write_flush(s);
   }
   return 1;
}

NOVASVG_INLINE int write_tga_to_func(write_func_t func, void *context, int x, int y, int comp, const void *data)
{
   write_context s = { 0 };
   write_start_callbacks(&s, func, context);
   return write_tga_core(&s, x, y, comp, (void *) data);
}

#ifndef NOVASVG_IW_NO_STDIO
NOVASVG_INLINE int write_tga(char const *filename, int x, int y, int comp, const void *data)
{
   write_context s = { 0 };
   if (write_start_file(&s,filename)) {
      int r = write_tga_core(&s, x, y, comp, (void *) data);
      write_end_file(&s);
      return r;
   } else
      return 0;
}
#endif

// *************************************************************************************************
// Radiance RGBE HDR writer
// by Baldur Karlsson

#define iw_max(a, b)  ((a) > (b) ? (a) : (b))

#ifndef NOVASVG_IW_NO_STDIO

static void hdr_linear_to_rgbe(unsigned char *rgbe, float *linear)
{
   int exponent;
   float maxcomp = iw_max(linear[0], iw_max(linear[1], linear[2]));

   if (maxcomp < 1e-32f) {
      rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
   } else {
      float normalize = (float) frexp(maxcomp, &exponent) * 256.0f/maxcomp;

      rgbe[0] = (unsigned char)(linear[0] * normalize);
      rgbe[1] = (unsigned char)(linear[1] * normalize);
      rgbe[2] = (unsigned char)(linear[2] * normalize);
      rgbe[3] = (unsigned char)(exponent + 128);
   }
}

static void hdr_write_run_data(write_context *s, int length, unsigned char databyte)
{
   unsigned char lengthbyte = NOVASVG_IW_UCHAR(length+128);
   NOVASVG_IW_ASSERT(length+128 <= 255);
   s->func(s->context, &lengthbyte, 1);
   s->func(s->context, &databyte, 1);
}

static void hdr_write_dump_data(write_context *s, int length, unsigned char *data)
{
   unsigned char lengthbyte = NOVASVG_IW_UCHAR(length);
   NOVASVG_IW_ASSERT(length <= 128); // inconsistent with spec but consistent with official code
   s->func(s->context, &lengthbyte, 1);
   s->func(s->context, data, length);
}

static void hdr_write_scanline(write_context *s, int width, int ncomp, unsigned char *scratch, float *scanline)
{
   unsigned char scanlineheader[4] = { 2, 2, 0, 0 };
   unsigned char rgbe[4];
   float linear[3];
   int x;

   scanlineheader[2] = (width&0xff00)>>8;
   scanlineheader[3] = (width&0x00ff);

   /* skip RLE for images too small or large */
   if (width < 8 || width >= 32768) {
      for (x=0; x < width; x++) {
         switch (ncomp) {
            case 4: /* fallthrough */
            case 3: linear[2] = scanline[x*ncomp + 2];
                    linear[1] = scanline[x*ncomp + 1];
                    linear[0] = scanline[x*ncomp + 0];
                    break;
            default:
                    linear[0] = linear[1] = linear[2] = scanline[x*ncomp + 0];
                    break;
         }
         hdr_linear_to_rgbe(rgbe, linear);
         s->func(s->context, rgbe, 4);
      }
   } else {
      int c,r;
      /* encode into scratch buffer */
      for (x=0; x < width; x++) {
         switch(ncomp) {
            case 4: /* fallthrough */
            case 3: linear[2] = scanline[x*ncomp + 2];
                    linear[1] = scanline[x*ncomp + 1];
                    linear[0] = scanline[x*ncomp + 0];
                    break;
            default:
                    linear[0] = linear[1] = linear[2] = scanline[x*ncomp + 0];
                    break;
         }
         hdr_linear_to_rgbe(rgbe, linear);
         scratch[x + width*0] = rgbe[0];
         scratch[x + width*1] = rgbe[1];
         scratch[x + width*2] = rgbe[2];
         scratch[x + width*3] = rgbe[3];
      }

      s->func(s->context, scanlineheader, 4);

      /* RLE each component separately */
      for (c=0; c < 4; c++) {
         unsigned char *comp = &scratch[width*c];

         x = 0;
         while (x < width) {
            // find first run
            r = x;
            while (r+2 < width) {
               if (comp[r] == comp[r+1] && comp[r] == comp[r+2])
                  break;
               ++r;
            }
            if (r+2 >= width)
               r = width;
            // dump up to first run
            while (x < r) {
               int len = r-x;
               if (len > 128) len = 128;
               hdr_write_dump_data(s, len, &comp[x]);
               x += len;
            }
            // if there's a run, output it
            if (r+2 < width) { // same test as what we break out of in search loop, so only true if we break'd
               // find next byte after run
               while (r < width && comp[r] == comp[x])
                  ++r;
               // output run up to r
               while (x < r) {
                  int len = r-x;
                  if (len > 127) len = 127;
                  hdr_write_run_data(s, len, comp[x]);
                  x += len;
               }
            }
         }
      }
   }
}

static int write_hdr_core(write_context *s, int x, int y, int comp, float *data)
{
   if (y <= 0 || x <= 0 || data == NULL)
      return 0;
   else {
      // Each component is stored separately. Allocate scratch space for full output scanline.
      unsigned char *scratch = (unsigned char *) NOVASVG_IW_MALLOC(x*4);
      int i, len;
      char buffer[128];
      char header[] = "#?RADIANCE\n# Written by novasvg\nFORMAT=32-bit_rle_rgbe\n";
      s->func(s->context, header, sizeof(header)-1);

#ifdef __STDC_LIB_EXT1__
      len = sprintf_s(buffer, sizeof(buffer), "EXPOSURE=          1.0000000000000\n\n-Y %d +X %d\n", y, x);
#else
      len = snprintf(buffer, sizeof(buffer), "EXPOSURE=          1.0000000000000\n\n-Y %d +X %d\n", y, x);
#endif
      s->func(s->context, buffer, len);

      for(i=0; i < y; i++)
         hdr_write_scanline(s, x, comp, scratch, data + comp*x*(g_flip_vertically_on_write ? y-1-i : i));
      NOVASVG_IW_FREE(scratch);
      return 1;
   }
}

NOVASVG_INLINE int write_hdr_to_func(write_func_t func, void *context, int x, int y, int comp, const float *data)
{
   write_context s = { 0 };
   write_start_callbacks(&s, func, context);
   return write_hdr_core(&s, x, y, comp, (float *) data);
}

NOVASVG_INLINE int write_hdr(char const *filename, int x, int y, int comp, const float *data)
{
   write_context s = { 0 };
   if (write_start_file(&s,filename)) {
      int r = write_hdr_core(&s, x, y, comp, (float *) data);
      write_end_file(&s);
      return r;
   } else
      return 0;
}
#endif // NOVASVG_IW_NO_STDIO


//////////////////////////////////////////////////////////////////////////////
//
// PNG writer
//

#ifndef NOVASVG_IW_ZLIB_COMPRESS
// stretchy buffer; sb_push() == vector<>::push_back() -- sb_count() == vector<>::size()
#define sb_raw(a) ((int *) (void *) (a) - 2)
#define sb_m(a)   sb_raw(a)[0]
#define sb_n(a)   sb_raw(a)[1]

#define sb_needgrow(a,n)  ((a)==0 || sb_n(a)+n >= sb_m(a))
#define sb_maybegrow(a,n) (sb_needgrow(a,(n)) ? sb_grow(a,n) : 0)
#define sb_grow(a,n)  sb_growf((void **) &(a), (n), sizeof(*(a)))

#define sb_push(a, v)      (sb_maybegrow(a,1), (a)[sb_n(a)++] = (v))
#define sb_count(a)        ((a) ? sb_n(a) : 0)
#define sb_free(a)         ((a) ? NOVASVG_IW_FREE(sb_raw(a)),0 : 0)

static void *sb_growf(void **arr, int increment, int itemsize)
{
   int m = *arr ? 2*sb_m(*arr)+increment : increment+1;
   void *p = NOVASVG_IW_REALLOC_SIZED(*arr ? sb_raw(*arr) : 0, *arr ? (sb_m(*arr)*itemsize + sizeof(int)*2) : 0, itemsize * m + sizeof(int)*2);
   NOVASVG_IW_ASSERT(p);
   if (p) {
      if (!*arr) ((int *) p)[1] = 0;
      *arr = (void *) ((int *) p + 2);
      sb_m(*arr) = m;
   }
   return *arr;
}

static unsigned char *zlib_flushf(unsigned char *data, unsigned int *bitbuffer, int *bitcount)
{
   while (*bitcount >= 8) {
      sb_push(data, NOVASVG_IW_UCHAR(*bitbuffer));
      *bitbuffer >>= 8;
      *bitcount -= 8;
   }
   return data;
}

static int zlib_bitrev(int code, int codebits)
{
   int res=0;
   while (codebits--) {
      res = (res << 1) | (code & 1);
      code >>= 1;
   }
   return res;
}

static unsigned int zlib_countm(unsigned char *a, unsigned char *b, int limit)
{
   int i;
   for (i=0; i < limit && i < 258; ++i)
      if (a[i] != b[i]) break;
   return i;
}

static unsigned int g_zlib_hash_table(unsigned char *data)
{
   iw_uint32 hash = data[0] + (data[1] << 8) + (data[2] << 16);
   hash ^= hash << 3;
   hash += hash >> 5;
   hash ^= hash << 4;
   hash += hash >> 17;
   hash ^= hash << 25;
   hash += hash >> 6;
   return hash;
}

#define zlib_flush() (out = zlib_flushf(out, &bitbuf, &bitcount))
#define zlib_add(code,codebits) \
      (bitbuf |= (code) << bitcount, bitcount += (codebits), zlib_flush())
#define zlib_huffa(b,c)  zlib_add(zlib_bitrev(b,c),c)
// default huffman tables
#define zlib_huff1(n)  zlib_huffa(0x30 + (n), 8)
#define zlib_huff2(n)  zlib_huffa(0x190 + (n)-144, 9)
#define zlib_huff3(n)  zlib_huffa(0 + (n)-256,7)
#define zlib_huff4(n)  zlib_huffa(0xc0 + (n)-280,8)
#define zlib_huff(n)  ((n) <= 143 ? zlib_huff1(n) : (n) <= 255 ? zlib_huff2(n) : (n) <= 279 ? zlib_huff3(n) : zlib_huff4(n))
#define zlib_huffb(n) ((n) <= 143 ? zlib_huff1(n) : zlib_huff2(n))

#define ZLIB_HASH   16384

#endif // NOVASVG_IW_ZLIB_COMPRESS

NOVASVG_INLINE unsigned char * zlib_compress(unsigned char *data, int data_len, int *out_len, int quality)
{
#ifdef NOVASVG_IW_ZLIB_COMPRESS
   // user provided a zlib compress implementation, use that
   return NOVASVG_IW_ZLIB_COMPRESS(data, data_len, out_len, quality);
#else // use builtin
   static unsigned short lengthc[] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258, 259 };
   static unsigned char  lengtheb[]= { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
   static unsigned short distc[]   = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577, 32768 };
   static unsigned char  disteb[]  = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
   unsigned int bitbuf=0;
   int i,j, bitcount=0;
   unsigned char *out = NULL;
   unsigned char ***hash_table = (unsigned char***) NOVASVG_IW_MALLOC(ZLIB_HASH * sizeof(unsigned char**));
   if (hash_table == NULL)
      return NULL;
   if (quality < 5) quality = 5;

   sb_push(out, 0x78);   // DEFLATE 32K window
   sb_push(out, 0x5e);   // FLEVEL = 1
   zlib_add(1,1);  // BFINAL = 1
   zlib_add(1,2);  // BTYPE = 1 -- fixed huffman

   for (i=0; i < ZLIB_HASH; ++i)
      hash_table[i] = NULL;

   i=0;
   while (i < data_len-3) {
      // hash next 3 bytes of data to be compressed
      int h = g_zlib_hash_table(data+i)&(ZLIB_HASH-1), best=3;
      unsigned char *bestloc = 0;
      unsigned char **hlist = hash_table[h];
      int n = sb_count(hlist);
      for (j=0; j < n; ++j) {
         if (hlist[j]-data > i-32768) { // if entry lies within window
            int d = zlib_countm(hlist[j], data+i, data_len-i);
            if (d >= best) { best=d; bestloc=hlist[j]; }
         }
      }
      // when hash table entry is too long, delete half the entries
      if (hash_table[h] && sb_n(hash_table[h]) == 2*quality) {
         NOVASVG_IW_MEMMOVE(hash_table[h], hash_table[h]+quality, sizeof(hash_table[h][0])*quality);
         sb_n(hash_table[h]) = quality;
      }
      sb_push(hash_table[h],data+i);

      if (bestloc) {
         // "lazy matching" - check match at *next* byte, and if it's better, do cur byte as literal
         h = g_zlib_hash_table(data+i+1)&(ZLIB_HASH-1);
         hlist = hash_table[h];
         n = sb_count(hlist);
         for (j=0; j < n; ++j) {
            if (hlist[j]-data > i-32767) {
               int e = zlib_countm(hlist[j], data+i+1, data_len-i-1);
               if (e > best) { // if next match is better, bail on current match
                  bestloc = NULL;
                  break;
               }
            }
         }
      }

      if (bestloc) {
         int d = (int) (data+i - bestloc); // distance back
         NOVASVG_IW_ASSERT(d <= 32767 && best <= 258);
         for (j=0; best > lengthc[j+1]-1; ++j);
         zlib_huff(j+257);
         if (lengtheb[j]) zlib_add(best - lengthc[j], lengtheb[j]);
         for (j=0; d > distc[j+1]-1; ++j);
         zlib_add(zlib_bitrev(j,5),5);
         if (disteb[j]) zlib_add(d - distc[j], disteb[j]);
         i += best;
      } else {
         zlib_huffb(data[i]);
         ++i;
      }
   }
   // write out final bytes
   for (;i < data_len; ++i)
      zlib_huffb(data[i]);
   zlib_huff(256); // end of block
   // pad with 0 bits to byte boundary
   while (bitcount)
      zlib_add(0,1);

   for (i=0; i < ZLIB_HASH; ++i)
      (void) sb_free(hash_table[i]);
   NOVASVG_IW_FREE(hash_table);

   // store uncompressed instead if compression was worse
   if (sb_n(out) > data_len + 2 + ((data_len+32766)/32767)*5) {
      sb_n(out) = 2;  // truncate to DEFLATE 32K window and FLEVEL = 1
      for (j = 0; j < data_len;) {
         int blocklen = data_len - j;
         if (blocklen > 32767) blocklen = 32767;
         sb_push(out, data_len - j == blocklen); // BFINAL = ?, BTYPE = 0 -- no compression
         sb_push(out, NOVASVG_IW_UCHAR(blocklen)); // LEN
         sb_push(out, NOVASVG_IW_UCHAR(blocklen >> 8));
         sb_push(out, NOVASVG_IW_UCHAR(~blocklen)); // NLEN
         sb_push(out, NOVASVG_IW_UCHAR(~blocklen >> 8));
         memcpy(out+sb_n(out), data+j, blocklen);
         sb_n(out) += blocklen;
         j += blocklen;
      }
   }

   {
      // compute adler32 on input
      unsigned int s1=1, s2=0;
      int blocklen = (int) (data_len % 5552);
      j=0;
      while (j < data_len) {
         for (i=0; i < blocklen; ++i) { s1 += data[j+i]; s2 += s1; }
         s1 %= 65521; s2 %= 65521;
         j += blocklen;
         blocklen = 5552;
      }
      sb_push(out, NOVASVG_IW_UCHAR(s2 >> 8));
      sb_push(out, NOVASVG_IW_UCHAR(s2));
      sb_push(out, NOVASVG_IW_UCHAR(s1 >> 8));
      sb_push(out, NOVASVG_IW_UCHAR(s1));
   }
   *out_len = sb_n(out);
   // make returned pointer freeable
   NOVASVG_IW_MEMMOVE(sb_raw(out), out, *out_len);
   return (unsigned char *) sb_raw(out);
#endif // NOVASVG_IW_ZLIB_COMPRESS
}

static unsigned int png_crc32(unsigned char *buffer, int len)
{
#ifdef NOVASVG_IW_CRC32
    return NOVASVG_IW_CRC32(buffer, len);
#else
   static unsigned int crc_table[256] =
   {
      0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
      0x0eDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
      0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
      0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
      0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
      0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
      0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
      0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
      0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
      0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
      0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
      0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
      0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
      0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
      0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
      0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
      0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
      0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
      0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
      0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
      0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
      0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
      0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
      0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
      0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
      0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
      0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
      0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
      0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
      0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
      0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
      0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
   };

   unsigned int crc = ~0u;
   int i;
   for (i=0; i < len; ++i)
      crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
   return ~crc;
#endif
}

#define png_write_u32x4(o,a,b,c,d) ((o)[0]=NOVASVG_IW_UCHAR(a),(o)[1]=NOVASVG_IW_UCHAR(b),(o)[2]=NOVASVG_IW_UCHAR(c),(o)[3]=NOVASVG_IW_UCHAR(d),(o)+=4)
#define zlib_write_u32(data,v) png_write_u32x4(data, (v)>>24,(v)>>16,(v)>>8,(v));
#define png_write_tag(data,s) png_write_u32x4(data, s[0],s[1],s[2],s[3])

static void png_write_crc(unsigned char **data, int len)
{
   unsigned int crc = png_crc32(*data - len - 4, len+4);
   zlib_write_u32(*data, crc);
}

static unsigned char png_paeth_predictor(int a, int b, int c)
{
   int p = a + b - c, pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
   if (pa <= pb && pa <= pc) return NOVASVG_IW_UCHAR(a);
   if (pb <= pc) return NOVASVG_IW_UCHAR(b);
   return NOVASVG_IW_UCHAR(c);
}

// @OPTIMIZE: provide an option that always forces left-predict or paeth predict
static void png_encode_line(unsigned char *pixels, int stride_bytes, int width, int height, int y, int n, int filter_type, signed char *line_buffer)
{
   static int mapping[] = { 0,1,2,3,4 };
   static int firstmap[] = { 0,1,0,5,6 };
   int *mymap = (y != 0) ? mapping : firstmap;
   int i;
   int type = mymap[filter_type];
   unsigned char *z = pixels + stride_bytes * (g_flip_vertically_on_write ? height-1-y : y);
   int signed_stride = g_flip_vertically_on_write ? -stride_bytes : stride_bytes;

   if (type==0) {
      memcpy(line_buffer, z, width*n);
      return;
   }

   // first loop isn't optimized since it's just one pixel
   for (i = 0; i < n; ++i) {
      switch (type) {
         case 1: line_buffer[i] = z[i]; break;
         case 2: line_buffer[i] = z[i] - z[i-signed_stride]; break;
         case 3: line_buffer[i] = z[i] - (z[i-signed_stride]>>1); break;
         case 4: line_buffer[i] = (signed char) (z[i] - png_paeth_predictor(0,z[i-signed_stride],0)); break;
         case 5: line_buffer[i] = z[i]; break;
         case 6: line_buffer[i] = z[i]; break;
      }
   }
   switch (type) {
      case 1: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - z[i-n]; break;
      case 2: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - z[i-signed_stride]; break;
      case 3: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - ((z[i-n] + z[i-signed_stride])>>1); break;
      case 4: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - png_paeth_predictor(z[i-n], z[i-signed_stride], z[i-signed_stride-n]); break;
      case 5: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - (z[i-n]>>1); break;
      case 6: for (i=n; i < width*n; ++i) line_buffer[i] = z[i] - png_paeth_predictor(z[i-n], 0,0); break;
   }
}

NOVASVG_INLINE unsigned char *write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len)
{
   int force_filter = g_png_force_filter;
   int ctype[5] = { -1, 0, 4, 2, 6 };
   unsigned char sig[8] = { 137,80,78,71,13,10,26,10 };
   unsigned char *out,*o, *filt, *zlib;
   signed char *line_buffer;
   int j,zlen;

   if (stride_bytes == 0)
      stride_bytes = x * n;

   if (force_filter >= 5) {
      force_filter = -1;
   }

   filt = (unsigned char *) NOVASVG_IW_MALLOC((x*n+1) * y); if (!filt) return 0;
   line_buffer = (signed char *) NOVASVG_IW_MALLOC(x * n); if (!line_buffer) { NOVASVG_IW_FREE(filt); return 0; }
   for (j=0; j < y; ++j) {
      int filter_type;
      if (force_filter > -1) {
         filter_type = force_filter;
         png_encode_line((unsigned char*)(pixels), stride_bytes, x, y, j, n, force_filter, line_buffer);
      } else { // Estimate the best filter by running through all of them:
         int best_filter = 0, best_filter_val = 0x7fffffff, est, i;
         for (filter_type = 0; filter_type < 5; filter_type++) {
            png_encode_line((unsigned char*)(pixels), stride_bytes, x, y, j, n, filter_type, line_buffer);

            // Estimate the entropy of the line using this filter; the less, the better.
            est = 0;
            for (i = 0; i < x*n; ++i) {
               est += abs((signed char) line_buffer[i]);
            }
            if (est < best_filter_val) {
               best_filter_val = est;
               best_filter = filter_type;
            }
         }
         if (filter_type != best_filter) {  // If the last iteration already got us the best filter, don't redo it
            png_encode_line((unsigned char*)(pixels), stride_bytes, x, y, j, n, best_filter, line_buffer);
            filter_type = best_filter;
         }
      }
      // when we get here, filter_type contains the filter type, and line_buffer contains the data
      filt[j*(x*n+1)] = (unsigned char) filter_type;
      NOVASVG_IW_MEMMOVE(filt+j*(x*n+1)+1, line_buffer, x*n);
   }
   NOVASVG_IW_FREE(line_buffer);
   zlib = zlib_compress(filt, y*( x*n+1), &zlen, g_png_compression_level);
   NOVASVG_IW_FREE(filt);
   if (!zlib) return 0;

   // each tag requires 12 bytes of overhead
   out = (unsigned char *) NOVASVG_IW_MALLOC(8 + 12+13 + 12+zlen + 12);
   if (!out) return 0;
   *out_len = 8 + 12+13 + 12+zlen + 12;

   o=out;
   NOVASVG_IW_MEMMOVE(o,sig,8); o+= 8;
   zlib_write_u32(o, 13); // header length
   png_write_tag(o, "IHDR");
   zlib_write_u32(o, x);
   zlib_write_u32(o, y);
   *o++ = 8;
   *o++ = NOVASVG_IW_UCHAR(ctype[n]);
   *o++ = 0;
   *o++ = 0;
   *o++ = 0;
   png_write_crc(&o,13);

   zlib_write_u32(o, zlen);
   png_write_tag(o, "IDAT");
   NOVASVG_IW_MEMMOVE(o, zlib, zlen);
   o += zlen;
   NOVASVG_IW_FREE(zlib);
   png_write_crc(&o, zlen);

   zlib_write_u32(o,0);
   png_write_tag(o, "IEND");
   png_write_crc(&o,0);

   NOVASVG_IW_ASSERT(o == out + *out_len);

   return out;
}

#ifndef NOVASVG_IW_NO_STDIO
NOVASVG_INLINE int write_png(char const *filename, int x, int y, int comp, const void *data, int stride_bytes)
{
   FILE *f;
   int len;
   unsigned char *png = write_png_to_mem((const unsigned char *) data, stride_bytes, x, y, comp, &len);
   if (png == NULL) return 0;

   f = open_file(filename, "wb");
   if (!f) { NOVASVG_IW_FREE(png); return 0; }
   fwrite(png, 1, len, f);
   fclose(f);
   NOVASVG_IW_FREE(png);
   return 1;
}
#endif

NOVASVG_INLINE int write_png_to_func(write_func_t func, void *context, int x, int y, int comp, const void *data, int stride_bytes)
{
   int len;
   unsigned char *png = write_png_to_mem((const unsigned char *) data, stride_bytes, x, y, comp, &len);
   if (png == NULL) return 0;
   func(context, png, len);
   NOVASVG_IW_FREE(png);
   return 1;
}


/* ***************************************************************************
 *
 * JPEG writer
 *
 * This is based on Jon Olick's jo_jpeg.cpp:
 * public domain Simple, Minimalistic JPEG writer - http://www.jonolick.com/code.html
 */

static const unsigned char jpg_zig_zag[] = { 0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,3,8,12,17,25,30,41,43,9,11,18,
      24,31,40,44,53,10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63 };

static void jpg_write_bits(write_context *s, int *bitBufP, int *bitCntP, const unsigned short *bs) {
   int bitBuf = *bitBufP, bitCnt = *bitCntP;
   bitCnt += bs[1];
   bitBuf |= bs[0] << (24 - bitCnt);
   while(bitCnt >= 8) {
      unsigned char c = (bitBuf >> 16) & 255;
      write_putc(s, c);
      if(c == 255) {
         write_putc(s, 0);
      }
      bitBuf <<= 8;
      bitCnt -= 8;
   }
   *bitBufP = bitBuf;
   *bitCntP = bitCnt;
}

static void jpg_dct(float *d0p, float *d1p, float *d2p, float *d3p, float *d4p, float *d5p, float *d6p, float *d7p) {
   float d0 = *d0p, d1 = *d1p, d2 = *d2p, d3 = *d3p, d4 = *d4p, d5 = *d5p, d6 = *d6p, d7 = *d7p;
   float z1, z2, z3, z4, z5, z11, z13;

   float tmp0 = d0 + d7;
   float tmp7 = d0 - d7;
   float tmp1 = d1 + d6;
   float tmp6 = d1 - d6;
   float tmp2 = d2 + d5;
   float tmp5 = d2 - d5;
   float tmp3 = d3 + d4;
   float tmp4 = d3 - d4;

   // Even part
   float tmp10 = tmp0 + tmp3;   // phase 2
   float tmp13 = tmp0 - tmp3;
   float tmp11 = tmp1 + tmp2;
   float tmp12 = tmp1 - tmp2;

   d0 = tmp10 + tmp11;       // phase 3
   d4 = tmp10 - tmp11;

   z1 = (tmp12 + tmp13) * 0.707106781f; // c4
   d2 = tmp13 + z1;       // phase 5
   d6 = tmp13 - z1;

   // Odd part
   tmp10 = tmp4 + tmp5;       // phase 2
   tmp11 = tmp5 + tmp6;
   tmp12 = tmp6 + tmp7;

   // The rotator is modified from fig 4-8 to avoid extra negations.
   z5 = (tmp10 - tmp12) * 0.382683433f; // c6
   z2 = tmp10 * 0.541196100f + z5; // c2-c6
   z4 = tmp12 * 1.306562965f + z5; // c2+c6
   z3 = tmp11 * 0.707106781f; // c4

   z11 = tmp7 + z3;      // phase 5
   z13 = tmp7 - z3;

   *d5p = z13 + z2;         // phase 6
   *d3p = z13 - z2;
   *d1p = z11 + z4;
   *d7p = z11 - z4;

   *d0p = d0;  *d2p = d2;  *d4p = d4;  *d6p = d6;
}

static void jpg_calc_bits(int val, unsigned short bits[2]) {
   int tmp1 = val < 0 ? -val : val;
   val = val < 0 ? val-1 : val;
   bits[1] = 1;
   while(tmp1 >>= 1) {
      ++bits[1];
   }
   bits[0] = val & ((1<<bits[1])-1);
}

static int jpg_process_du(write_context *s, int *bitBuf, int *bitCnt, float *CDU, int du_stride, float *fdtbl, int DC, const unsigned short HTDC[256][2], const unsigned short HTAC[256][2]) {
   const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
   const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };
   int dataOff, i, j, n, diff, end0pos, x, y;
   int DU[64];

   // DCT rows
   for(dataOff=0, n=du_stride*8; dataOff<n; dataOff+=du_stride) {
      jpg_dct(&CDU[dataOff], &CDU[dataOff+1], &CDU[dataOff+2], &CDU[dataOff+3], &CDU[dataOff+4], &CDU[dataOff+5], &CDU[dataOff+6], &CDU[dataOff+7]);
   }
   // DCT columns
   for(dataOff=0; dataOff<8; ++dataOff) {
      jpg_dct(&CDU[dataOff], &CDU[dataOff+du_stride], &CDU[dataOff+du_stride*2], &CDU[dataOff+du_stride*3], &CDU[dataOff+du_stride*4],
                     &CDU[dataOff+du_stride*5], &CDU[dataOff+du_stride*6], &CDU[dataOff+du_stride*7]);
   }
   // Quantize/descale/zigzag the coefficients
   for(y = 0, j=0; y < 8; ++y) {
      for(x = 0; x < 8; ++x,++j) {
         float v;
         i = y*du_stride+x;
         v = CDU[i]*fdtbl[j];
         // DU[jpg_zig_zag[j]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
         // ceilf() and floorf() are C99, not C89, but I /think/ they're not needed here anyway?
         DU[jpg_zig_zag[j]] = (int)(v < 0 ? v - 0.5f : v + 0.5f);
      }
   }

   // Encode DC
   diff = DU[0] - DC;
   if (diff == 0) {
      jpg_write_bits(s, bitBuf, bitCnt, HTDC[0]);
   } else {
      unsigned short bits[2];
      jpg_calc_bits(diff, bits);
      jpg_write_bits(s, bitBuf, bitCnt, HTDC[bits[1]]);
      jpg_write_bits(s, bitBuf, bitCnt, bits);
   }
   // Encode ACs
   end0pos = 63;
   for(; (end0pos>0)&&(DU[end0pos]==0); --end0pos) {
   }
   // end0pos = first element in reverse order !=0
   if(end0pos == 0) {
      jpg_write_bits(s, bitBuf, bitCnt, EOB);
      return DU[0];
   }
   for(i = 1; i <= end0pos; ++i) {
      int startpos = i;
      int nrzeroes;
      unsigned short bits[2];
      for (; DU[i]==0 && i<=end0pos; ++i) {
      }
      nrzeroes = i-startpos;
      if ( nrzeroes >= 16 ) {
         int lng = nrzeroes>>4;
         int nrmarker;
         for (nrmarker=1; nrmarker <= lng; ++nrmarker)
            jpg_write_bits(s, bitBuf, bitCnt, M16zeroes);
         nrzeroes &= 15;
      }
      jpg_calc_bits(DU[i], bits);
      jpg_write_bits(s, bitBuf, bitCnt, HTAC[(nrzeroes<<4)+bits[1]]);
      jpg_write_bits(s, bitBuf, bitCnt, bits);
   }
   if(end0pos != 63) {
      jpg_write_bits(s, bitBuf, bitCnt, EOB);
   }
   return DU[0];
}

static int write_jpg_core(write_context *s, int width, int height, int comp, const void* data, int quality) {
   // Constants that don't pollute global namespace
   static const unsigned char std_dc_luminance_nrcodes[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
   static const unsigned char std_dc_luminance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
   static const unsigned char std_ac_luminance_nrcodes[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
   static const unsigned char std_ac_luminance_values[] = {
      0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
      0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
      0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
      0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
      0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
      0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
      0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
   };
   static const unsigned char std_dc_chrominance_nrcodes[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
   static const unsigned char std_dc_chrominance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
   static const unsigned char std_ac_chrominance_nrcodes[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
   static const unsigned char std_ac_chrominance_values[] = {
      0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
      0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
      0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
      0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
      0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
      0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
      0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
   };
   // Huffman tables
   static const unsigned short YDC_HT[256][2] = { {0,2},{2,3},{3,3},{4,3},{5,3},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9}};
   static const unsigned short UVDC_HT[256][2] = { {0,2},{1,2},{2,2},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9},{1022,10},{2046,11}};
   static const unsigned short YAC_HT[256][2] = {
      {10,4},{0,2},{1,2},{4,3},{11,4},{26,5},{120,7},{248,8},{1014,10},{65410,16},{65411,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {12,4},{27,5},{121,7},{502,9},{2038,11},{65412,16},{65413,16},{65414,16},{65415,16},{65416,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {28,5},{249,8},{1015,10},{4084,12},{65417,16},{65418,16},{65419,16},{65420,16},{65421,16},{65422,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {58,6},{503,9},{4085,12},{65423,16},{65424,16},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {59,6},{1016,10},{65430,16},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {122,7},{2039,11},{65438,16},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {123,7},{4086,12},{65446,16},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {250,8},{4087,12},{65454,16},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {504,9},{32704,15},{65462,16},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {505,9},{65470,16},{65471,16},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {506,9},{65479,16},{65480,16},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {1017,10},{65488,16},{65489,16},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {1018,10},{65497,16},{65498,16},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {2040,11},{65506,16},{65507,16},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {65515,16},{65516,16},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{0,0},{0,0},{0,0},{0,0},{0,0},
      {2041,11},{65525,16},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
   };
   static const unsigned short UVAC_HT[256][2] = {
      {0,2},{1,2},{4,3},{10,4},{24,5},{25,5},{56,6},{120,7},{500,9},{1014,10},{4084,12},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {11,4},{57,6},{246,8},{501,9},{2038,11},{4085,12},{65416,16},{65417,16},{65418,16},{65419,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {26,5},{247,8},{1015,10},{4086,12},{32706,15},{65420,16},{65421,16},{65422,16},{65423,16},{65424,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {27,5},{248,8},{1016,10},{4087,12},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{65430,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {58,6},{502,9},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{65438,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {59,6},{1017,10},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{65446,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {121,7},{2039,11},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{65454,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {122,7},{2040,11},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{65462,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {249,8},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{65470,16},{65471,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {503,9},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{65479,16},{65480,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {504,9},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{65488,16},{65489,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {505,9},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{65497,16},{65498,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {506,9},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{65506,16},{65507,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {2041,11},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{65515,16},{65516,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {16352,14},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{65525,16},{0,0},{0,0},{0,0},{0,0},{0,0},
      {1018,10},{32707,15},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
   };
   static const int YQT[] = {16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,
                             37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99};
   static const int UVQT[] = {17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,
                              99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99};
   static const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f,
                                 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };

   int row, col, i, k, subsample;
   float fdtbl_Y[64], fdtbl_UV[64];
   unsigned char YTable[64], UVTable[64];

   if(!data || !width || !height || comp > 4 || comp < 1) {
      return 0;
   }

   quality = quality ? quality : 90;
   subsample = quality <= 90 ? 1 : 0;
   quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
   quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

   for(i = 0; i < 64; ++i) {
      int uvti, yti = (YQT[i]*quality+50)/100;
      YTable[jpg_zig_zag[i]] = (unsigned char) (yti < 1 ? 1 : yti > 255 ? 255 : yti);
      uvti = (UVQT[i]*quality+50)/100;
      UVTable[jpg_zig_zag[i]] = (unsigned char) (uvti < 1 ? 1 : uvti > 255 ? 255 : uvti);
   }

   for(row = 0, k = 0; row < 8; ++row) {
      for(col = 0; col < 8; ++col, ++k) {
         fdtbl_Y[k]  = 1 / (YTable [jpg_zig_zag[k]] * aasf[row] * aasf[col]);
         fdtbl_UV[k] = 1 / (UVTable[jpg_zig_zag[k]] * aasf[row] * aasf[col]);
      }
   }

   // Write Headers
   {
      static const unsigned char head0[] = { 0xFF,0xD8,0xFF,0xE0,0,0x10,'J','F','I','F',0,1,1,0,0,1,0,1,0,0,0xFF,0xDB,0,0x84,0 };
      static const unsigned char head2[] = { 0xFF,0xDA,0,0xC,3,1,0,2,0x11,3,0x11,0,0x3F,0 };
      const unsigned char head1[] = { 0xFF,0xC0,0,0x11,8,(unsigned char)(height>>8),NOVASVG_IW_UCHAR(height),(unsigned char)(width>>8),NOVASVG_IW_UCHAR(width),
                                      3,1,(unsigned char)(subsample?0x22:0x11),0,2,0x11,1,3,0x11,1,0xFF,0xC4,0x01,0xA2,0 };
      s->func(s->context, (void*)head0, sizeof(head0));
      s->func(s->context, (void*)YTable, sizeof(YTable));
      write_putc(s, 1);
      s->func(s->context, UVTable, sizeof(UVTable));
      s->func(s->context, (void*)head1, sizeof(head1));
      s->func(s->context, (void*)(std_dc_luminance_nrcodes+1), sizeof(std_dc_luminance_nrcodes)-1);
      s->func(s->context, (void*)std_dc_luminance_values, sizeof(std_dc_luminance_values));
      write_putc(s, 0x10); // HTYACinfo
      s->func(s->context, (void*)(std_ac_luminance_nrcodes+1), sizeof(std_ac_luminance_nrcodes)-1);
      s->func(s->context, (void*)std_ac_luminance_values, sizeof(std_ac_luminance_values));
      write_putc(s, 1); // HTUDCinfo
      s->func(s->context, (void*)(std_dc_chrominance_nrcodes+1), sizeof(std_dc_chrominance_nrcodes)-1);
      s->func(s->context, (void*)std_dc_chrominance_values, sizeof(std_dc_chrominance_values));
      write_putc(s, 0x11); // HTUACinfo
      s->func(s->context, (void*)(std_ac_chrominance_nrcodes+1), sizeof(std_ac_chrominance_nrcodes)-1);
      s->func(s->context, (void*)std_ac_chrominance_values, sizeof(std_ac_chrominance_values));
      s->func(s->context, (void*)head2, sizeof(head2));
   }

   // Encode 8x8 macroblocks
   {
      static const unsigned short fillBits[] = {0x7F, 7};
      int DCY=0, DCU=0, DCV=0;
      int bitBuf=0, bitCnt=0;
      // comp == 2 is grey+alpha (alpha is ignored)
      int ofsG = comp > 2 ? 1 : 0, ofsB = comp > 2 ? 2 : 0;
      const unsigned char *dataR = (const unsigned char *)data;
      const unsigned char *dataG = dataR + ofsG;
      const unsigned char *dataB = dataR + ofsB;
      int x, y, pos;
      if(subsample) {
         for(y = 0; y < height; y += 16) {
            for(x = 0; x < width; x += 16) {
               float Y[256], U[256], V[256];
               for(row = y, pos = 0; row < y+16; ++row) {
                  // row >= height => use last input row
                  int clamped_row = (row < height) ? row : height - 1;
                  int base_p = (g_flip_vertically_on_write ? (height-1-clamped_row) : clamped_row)*width*comp;
                  for(col = x; col < x+16; ++col, ++pos) {
                     // if col >= width => use pixel from last input column
                     int p = base_p + ((col < width) ? col : (width-1))*comp;
                     float r = dataR[p], g = dataG[p], b = dataB[p];
                     Y[pos]= +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
                     U[pos]= -0.16874f*r - 0.33126f*g + 0.50000f*b;
                     V[pos]= +0.50000f*r - 0.41869f*g - 0.08131f*b;
                  }
               }
               DCY = jpg_process_du(s, &bitBuf, &bitCnt, Y+0,   16, fdtbl_Y, DCY, YDC_HT, YAC_HT);
               DCY = jpg_process_du(s, &bitBuf, &bitCnt, Y+8,   16, fdtbl_Y, DCY, YDC_HT, YAC_HT);
               DCY = jpg_process_du(s, &bitBuf, &bitCnt, Y+128, 16, fdtbl_Y, DCY, YDC_HT, YAC_HT);
               DCY = jpg_process_du(s, &bitBuf, &bitCnt, Y+136, 16, fdtbl_Y, DCY, YDC_HT, YAC_HT);

               // subsample U,V
               {
                  float subU[64], subV[64];
                  int yy, xx;
                  for(yy = 0, pos = 0; yy < 8; ++yy) {
                     for(xx = 0; xx < 8; ++xx, ++pos) {
                        int j = yy*32+xx*2;
                        subU[pos] = (U[j+0] + U[j+1] + U[j+16] + U[j+17]) * 0.25f;
                        subV[pos] = (V[j+0] + V[j+1] + V[j+16] + V[j+17]) * 0.25f;
                     }
                  }
                  DCU = jpg_process_du(s, &bitBuf, &bitCnt, subU, 8, fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
                  DCV = jpg_process_du(s, &bitBuf, &bitCnt, subV, 8, fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
               }
            }
         }
      } else {
         for(y = 0; y < height; y += 8) {
            for(x = 0; x < width; x += 8) {
               float Y[64], U[64], V[64];
               for(row = y, pos = 0; row < y+8; ++row) {
                  // row >= height => use last input row
                  int clamped_row = (row < height) ? row : height - 1;
                  int base_p = (g_flip_vertically_on_write ? (height-1-clamped_row) : clamped_row)*width*comp;
                  for(col = x; col < x+8; ++col, ++pos) {
                     // if col >= width => use pixel from last input column
                     int p = base_p + ((col < width) ? col : (width-1))*comp;
                     float r = dataR[p], g = dataG[p], b = dataB[p];
                     Y[pos]= +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
                     U[pos]= -0.16874f*r - 0.33126f*g + 0.50000f*b;
                     V[pos]= +0.50000f*r - 0.41869f*g - 0.08131f*b;
                  }
               }

               DCY = jpg_process_du(s, &bitBuf, &bitCnt, Y, 8, fdtbl_Y,  DCY, YDC_HT, YAC_HT);
               DCU = jpg_process_du(s, &bitBuf, &bitCnt, U, 8, fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
               DCV = jpg_process_du(s, &bitBuf, &bitCnt, V, 8, fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
            }
         }
      }

      // Do the bit alignment of the EOI marker
      jpg_write_bits(s, &bitBuf, &bitCnt, fillBits);
   }

   // EOI
   write_putc(s, 0xFF);
   write_putc(s, 0xD9);

   return 1;
}

NOVASVG_INLINE int write_jpg_to_func(write_func_t func, void *context, int x, int y, int comp, const void *data, int quality)
{
   write_context s = { 0 };
   write_start_callbacks(&s, func, context);
   return write_jpg_core(&s, x, y, comp, (void *) data, quality);
}


#ifndef NOVASVG_IW_NO_STDIO
NOVASVG_INLINE int write_jpg(char const *filename, int x, int y, int comp, const void *data, int quality)
{
   write_context s = { 0 };
   if (write_start_file(&s,filename)) {
      int r = write_jpg_core(&s, x, y, comp, data, quality);
      write_end_file(&s);
      return r;
   } else
      return 0;
}
#endif

} // namespace render
} // namespace novasvg

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
// long enough to hand to the encoders in render:: (below, in this same
// file). This is an implementation detail of Bitmap::write(), not a type
// of its own: no declaration in the class, nothing exported.
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

// Format::Auto, resolved from a filename's extension. Unrecognized or
// missing extensions fall back to Png -- write() always succeeds or
// fails on I/O/encoding grounds, never on "couldn't guess a format".
Bitmap::Format formatFromFilename(const std::string& filename)
{
    auto dot = filename.find_last_of('.');
    if(dot == std::string::npos)
        return Bitmap::Format::Png;

    auto ext = filename.substr(dot + 1);
    for(auto& c : ext)
        c = char(std::tolower(static_cast<unsigned char>(c)));

    if(ext == "bmp")
        return Bitmap::Format::Bmp;
    if(ext == "tga")
        return Bitmap::Format::Tga;
    if(ext == "jpg" || ext == "jpeg")
        return Bitmap::Format::Jpg;
    return Bitmap::Format::Png;
}

} // namespace

NOVASVG_INLINE bool Bitmap::write(const std::string& filename, Format format, int jpgQuality) const
{
    if(format == Format::Auto)
        format = formatFromFilename(filename);

    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;

    switch(format) {
    case Format::Bmp:
        return write_bmp(filename.data(), rgba.width(), rgba.height(), 4, rgba.data()) != 0;
    case Format::Tga:
        return write_tga(filename.data(), rgba.width(), rgba.height(), 4, rgba.data()) != 0;
    case Format::Jpg:
        return write_jpg(filename.data(), rgba.width(), rgba.height(), 4, rgba.data(), jpgQuality) != 0;
    case Format::Auto:
    case Format::Png:
    default:
        return write_png(filename.data(), rgba.width(), rgba.height(), 4, rgba.data(), rgba.stride()) != 0;
    }
}

NOVASVG_INLINE bool Bitmap::write(novasvg_write_func_t callback, void* closure, Format format, int jpgQuality) const
{
    BitmapRgbaScratch rgba(*this);
    if(!rgba)
        return false;

    switch(format) {
    case Format::Bmp:
        return write_bmp_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data()) != 0;
    case Format::Tga:
        return write_tga_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data()) != 0;
    case Format::Jpg:
        return write_jpg_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data(), jpgQuality) != 0;
    case Format::Auto:
    case Format::Png:
    default:
        return write_png_to_func(callback, closure, rgba.width(), rgba.height(), 4, rgba.data(), rgba.stride()) != 0;
    }
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
