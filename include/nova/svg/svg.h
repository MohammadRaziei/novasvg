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


#ifndef NOVA_H
#define NOVA_H
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <cctype>
#include <vector>

#include "../config.h"

#define NOVA_VERSION_MAJOR 0
#define NOVA_VERSION_MINOR 11
#define NOVA_VERSION_PATCH 4

#define NOVA_VERSION_ENCODE(major, minor, patch) (((major) * 10000) + ((minor) * 100) + ((patch) * 1))
#define NOVA_VERSION NOVA_VERSION_ENCODE(NOVA_VERSION_MAJOR, NOVA_VERSION_MINOR, NOVA_VERSION_PATCH)

#define NOVA_VERSION_XSTRINGIZE(major, minor, patch) #major"."#minor"."#patch
#define NOVA_VERSION_STRINGIZE(major, minor, patch) NOVA_VERSION_XSTRINGIZE(major, minor, patch)
#define NOVA_VERSION_STRING NOVA_VERSION_STRINGIZE(NOVA_VERSION_MAJOR, NOVA_VERSION_MINOR, NOVA_VERSION_PATCH)

#include "../vg/color.h"


/**
 * @brief Callback for cleaning up resources.
 *
 * This function is called to release resources associated with a specific operation.
 *
 * @param closure A user-defined pointer to the resource or context to be freed.
 */



namespace nova::svg {
using namespace vg;

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

} // namespace nova::svg

#include "../vg/geometry.h"

namespace nova::svg {
using namespace vg;

class svg_node;
class svg_text_node;
class svg_element;

class element;
class text_node;

class NOVA_API node {
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

class NOVA_API text_node : public node {
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

class NOVA_API element : public node {
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
    void render(stb::image& image, const stb::matrix& matrix = stb::matrix()) const;

    /**
     * @brief Renders the el to a new straight-alpha RGBA image with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return The rendered image, or an empty image if the el has nothing to draw or memory ran out.
     */
    stb::image render_to_image(int width = -1, int height = -1, const stb::color& backgroundColor = stb::color(0, 0, 0, 0)) const;

    /**
     * @internal Premultiplied-ARGB32 rendering target (Bitmap is not part of the public API; use the
     * stb::image overloads above).
     */
    void render(Bitmap& bitmap, const stb::matrix& matrix = stb::matrix()) const;

    /**
     * @internal See render(Bitmap&).
     */
    Bitmap render_to_bitmap(int width = -1, int height = -1, const stb::color& backgroundColor = stb::color(0, 0, 0, 0)) const;

    /**
     * @brief Retrieves the local transformation matrix of the el.
     * @return The matrix that applies only to the el, relative to its parent.
     */
    stb::matrix local_matrix() const;

    /**
     * @brief Retrieves the global transformation matrix of the el.
     * @return The matrix combining the el's local and all parent transformations.
     */
    stb::matrix global_matrix() const;

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

class NOVA_API document {
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
    void render(stb::image& image, const stb::matrix& matrix = stb::matrix()) const;

    /**
     * @brief Renders the dom to a new straight-alpha RGBA image with specified dimensions.
     * @param width The desired width in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param height The desired height in pixels, or -1 to auto-scale based on the intrinsic size.
     * @param backgroundColor The background color to render onto.
     * @return The rendered image, or an empty image if the dom has nothing to draw or memory ran out.
     */
    stb::image render_to_image(int width = -1, int height = -1, const stb::color& backgroundColor = stb::color(0, 0, 0, 0)) const;

    /**
     * @internal Premultiplied-ARGB32 rendering target (Bitmap is not part of the public API; use the
     * stb::image overloads above).
     */
    void render(Bitmap& bitmap, const stb::matrix& matrix = stb::matrix()) const;

    /**
     * @internal See render(Bitmap&).
     */
    Bitmap render_to_bitmap(int width = -1, int height = -1, const stb::color& backgroundColor = stb::color(0, 0, 0, 0)) const;

    /**
     * @brief How render_to_pdf() writes the page.
     *
     * - vector: shapes, text (as outlines), gradients, clips, opacity groups and blend modes stay vector; what PDF
     *   cannot express (masks, clip masks, filters) and pattern tiles are rendered to bitmaps at `dpi` and placed in the
     *   same page, so the page is never worse than the raster mode and usually much smaller and sharper.
     * - raster: the whole page is one bitmap rendered at `dpi`. Every feature of the renderer works, nothing is vector.
     */
    enum class pdf_mode { vector, raster };

    /**
     * @brief Renders the document as a one-page PDF (page size = the document's size, 1 px = 0.75 pt).
     * @param mode See pdf_mode.
     * @param dpi Resolution of every bitmap in the file (96 is the SVG's own pixel size).
     * @return The PDF file's bytes, or empty when the document has no size.
     */
    std::vector<uint8_t> render_to_pdf(pdf_mode mode = pdf_mode::vector, float dpi = 192.f) const;

    /**
     * @brief render_to_pdf() written to a file.
     * @return false if nothing could be rendered or the file could not be written.
     */
    bool save_pdf(const std::string& filename, pdf_mode mode = pdf_mode::vector, float dpi = 192.f) const;

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

} // namespace nova::svg

#endif // NOVA_H
