/*
 * Copyright (c) 2020-2025 Samuel Ugochukwu <sammycageagle@gmail.com>
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

#ifndef NOVA_RENDER_H
#define NOVA_RENDER_H

#include <stdbool.h>
#include <stb/outline.hpp>
#include <stdint.h>
#include <stddef.h>

#include "nova/config.h"   // NOVA_API, STB_MAX/MIN (one definition, shared with novasvg.h)

// STB_MAX/STB_MIN are defined in detail/config.h, which #includes
// (transitively, via canvas.h) this header -- always available by the
// time any of the macros below are actually used at a call site.

// Pure C++ project: no separate C translation units link against this,
// so the extern "C" wrapper this file used to have (for use as a
// standalone C library) has been dropped along with the novasvg_ prefix
// below -- everything here now lives in namespace nova::vg
// instead.
namespace nova {
namespace vg {

#define NOVA_RENDER_VERSION_MAJOR 1
#define NOVA_RENDER_VERSION_MINOR 3
#define NOVA_RENDER_VERSION_MICRO 2

#define NOVA_RENDER_VERSION_ENCODE(major, minor, micro) (((major) * 10000) + ((minor) * 100) + ((micro) * 1))
#define NOVA_RENDER_VERSION NOVA_RENDER_VERSION_ENCODE(NOVA_RENDER_VERSION_MAJOR, NOVA_RENDER_VERSION_MINOR, NOVA_RENDER_VERSION_MICRO)

#define NOVA_RENDER_VERSION_XSTRINGIZE(major, minor, micro) #major"."#minor"."#micro
#define NOVA_RENDER_VERSION_STRINGIZE(major, minor, micro) NOVA_RENDER_VERSION_XSTRINGIZE(major, minor, micro)
#define NOVA_RENDER_VERSION_STRING NOVA_RENDER_VERSION_STRINGIZE(NOVA_RENDER_VERSION_MAJOR, NOVA_RENDER_VERSION_MINOR, NOVA_RENDER_VERSION_MICRO)

/**
 * @brief Gets the version of the novasvg rendering library.
 * @return An integer representing the version of the novasvg rendering library.
 */
NOVA_API int render_version(void);

/**
 * @brief Gets the version of the novasvg rendering library as a string.
 * @return A string representing the version of the novasvg rendering library.
 */
NOVA_API const char* render_version_string(void);

/**
 * @brief A function pointer type for a cleanup callback.
 * @param closure A pointer to the resource to be cleaned up.
 */
typedef void (*destroy_func_t)(void* closure);

/**
 * @brief A function pointer type for a write callback.
 * @param closure A pointer to user-defined data or context.
 * @param data A pointer to the data to be written.
 * @param size The size of the data in bytes.
 */
typedef void (*write_func_t)(void* closure, void* data, int size_);

#define NOVA_PI      3.14159265358979323846f
#define NOVA_TWO_PI  6.28318530717958647693f
#define NOVA_HALF_PI 1.57079632679489661923f
#define NOVA_SQRT2   1.41421356237309504880f
#define NOVA_KAPPA   0.55228474983079339840f

#define NOVA_DEG2RAD(x) ((x) * (NOVA_PI / 180.0f))
#define NOVA_RAD2DEG(x) ((x) * (180.0f / NOVA_PI))

/**
 * @brief A structure representing a rectangle in 2D space.
 */
typedef struct {
    float x; ///< The x-coordinate of the top-left corner of the rectangle.
    float y; ///< The y-coordinate of the top-left corner of the rectangle.
    float w; ///< The width of the rectangle.
    float h; ///< The height of the rectangle.
} rect_t;

#define NOVA_MAKE_RECT(x, y, w, h) (rect_t{x, y, w, h})
#define NOVA_EMPTY_RECT NOVA_MAKE_RECT(0, 0, 0, 0)

// Matrices are stb::matrix and points are stb::point (<stb/outline.hpp>); the helpers below are what libstb does not have.

/// Maps `count` points through `matrix` (`src` and `dst` may be the same array).
void map_points(const stb::matrix& matrix, const stb::point* src, stb::point* dst, int count);

/// The bounding rectangle of `src` after `matrix` (`src` and `dst` may be the same rectangle).
void mapped_bounds(const stb::matrix& matrix, const rect_t* src, rect_t* dst);

/// Parses an SVG `transform` list (`matrix(...) translate(...) ...`) into `matrix`. Returns false on a syntax error.
/// `length` is the byte length of `data`, or -1 for a NUL-terminated string.
bool parse_matrix(stb::matrix* matrix, const char* data, int length);

/**
 * @brief Enumeration defining path commands.
 */
enum class path_command : int {
    move_to, ///< Moves the current point to a new position.
    line_to, ///< Draws a straight line to a new point.
    cubic_to, ///< Draws a cubic Bézier curve to a new point.
    close ///< Closes the current path by drawing a line to the starting point.
};

/**
 * @brief Callback function type for traversing a path.
 *
 * This function type defines a callback used to traverse path elements.
 *
 * @param closure A pointer to user-defined data passed to the callback.
 * @param command The current path command.
 * @param points An array of points associated with the command.
 * @param npoints The number of points in the array.
 */
typedef void (*path_traverse_func_t)(void* closure, path_command command, const stb::point* points, int npoints);

/**
 * @brief Text encodings used for converting text data to code points.
 */
enum class text_encoding : int {
    latin1, ///< Latin-1 encoding
    utf8, ///< UTF-8 encoding
    utf16, ///< UTF-16 encoding
    utf32 ///< UTF-32 encoding
};

/**
 * @brief Iterator for traversing code points in text data.
 */
typedef struct text_iterator {
    const void* text; ///< Pointer to the text data.
    int length_; ///< Length of the text data.
    text_encoding encoding; ///< Encoding format of the text data.
    int index; ///< Current position in the text data.
} text_iterator_t;

/**
 * @brief Represents a Unicode code point.
 */
typedef unsigned int codepoint_t;

/**
 * @brief Initializes a text iterator.
 *
 * @param it Pointer to the text iterator.
 * @param text Pointer to the text data.
 * @param length Length of the text data, or -1 if the data is null-terminated.
 * @param encoding Encoding of the text data.
 */
NOVA_API void text_iterator_init(text_iterator_t* it, const void* text, int length_, text_encoding encoding);

/**
 * @brief Checks if there are more code points to iterate.
 *
 * @param it Pointer to the text iterator.
 * @return `true` if more code points are available; otherwise, `false`.
 */
NOVA_API bool text_iterator_has_next(const text_iterator_t* it);

/**
 * @brief Retrieves the next code point and advances the iterator.
 *
 * @param it Pointer to the text iterator.
 * @return The next code point.
 */
NOVA_API codepoint_t text_iterator_next(text_iterator_t* it);

/**
 * @brief Represents a font face.
 */
typedef struct font_face_impl font_face_t;

/**
 * @brief Loads a font face from a file.
 *
 * @param filename Path to the font file.
 * @param ttcindex Index of the font face within a TrueType Collection (TTC).
 * @return A pointer to the loaded `font_face_t` object, or `NULL` on failure.
 */
NOVA_API font_face_t* font_face_load_from_file(const char* filename, int ttcindex);

/**
 * @brief Loads a font face from memory.
 *
 * @param data Pointer to the font data.
 * @param length Length of the font data.
 * @param ttcindex Index of the font face within a TrueType Collection (TTC).
 * @param destroy_func Function to free the font data when no longer needed.
 * @param closure User-defined data passed to `destroy_func`.
 * @return A pointer to the loaded `font_face_t` object, or `NULL` on failure.
 */
NOVA_API font_face_t* font_face_load_from_data(const void* data, unsigned int length_, int ttcindex, destroy_func_t destroy_func, void* closure);

/**
 * @brief Increments the reference count of a font face.
 *
 * @param face A pointer to a `font_face_t` object.
 * @return A pointer to the same `font_face_t` object with an incremented reference count.
 */
NOVA_API font_face_t* font_face_reference(font_face_t* face);

/**
 * @brief Decrements the reference count and potentially destroys the font face.
 *
 * @param face A pointer to a `font_face_t` object.
 */
NOVA_API void font_face_destroy(font_face_t* face);

/**
 * @brief Retrieves the current reference count of a font face.
 *
 * @param face A pointer to a `font_face_t` object.
 * @return The reference count of the font face.
 */
NOVA_API int font_face_get_reference_count(const font_face_t* face);

/**
 * @brief Retrieves metrics for a font face at a specified size.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param ascent Pointer to store the ascent metric.
 * @param descent Pointer to store the descent metric.
 * @param line_gap Pointer to store the line gap metric.
 * @param extents Pointer to a `rect_t` object to store the font bounding box.
 */
NOVA_API void font_face_get_metrics(const font_face_t* face, float size_, float* ascent, float* descent, float* line_gap, rect_t* extents);

/**
 * @brief Retrieves metrics for a specified glyph at a given size.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param codepoint The Unicode code point of the glyph.
 * @param advance_width Pointer to store the advance width of the glyph.
 * @param left_side_bearing Pointer to store the left side bearing of the glyph.
 * @param extents Pointer to a `rect_t` object to store the glyph bounding box.
 */
NOVA_API void font_face_get_glyph_metrics(font_face_t* face, float size_, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents);

/**
 * @brief Retrieves the path of a glyph and its advance width.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param x The x-coordinate for positioning the glyph.
 * @param y The y-coordinate for positioning the glyph.
 * @param codepoint The Unicode code point of the glyph.
 * @param path Pointer to a `path_t` object to store the glyph path.
 * @return The advance width of the glyph.
 */
NOVA_API float font_face_get_glyph_path(font_face_t* face, float size_, float x, float y, codepoint_t codepoint, stb::outline& path_);

/**
 * @brief Traverses the path of a glyph and calls a callback for each path el.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param x The x-coordinate for positioning the glyph.
 * @param y The y-coordinate for positioning the glyph.
 * @param codepoint The Unicode code point of the glyph.
 * @param traverse_func The callback function to be called for each path el.
 * @param closure User-defined data passed to the callback function.
 * @return The advance width of the glyph.
 */
NOVA_API float font_face_traverse_glyph_path(font_face_t* face, float size_, float x, float y, codepoint_t codepoint, path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Computes the bounding box of a text string and its advance width.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param text Pointer to the text data.
 * @param length Length of the text data, or -1 if null-terminated.
 * @param encoding Encoding of the text data.
 * @param extents Pointer to a `rect_t` object to store the bounding box of the text.
 * @return The total advance width of the text.
 */
NOVA_API float font_face_text_extents(font_face_t* face, float size_, const void* text, int length_, text_encoding encoding, rect_t* extents);

/**
 * @brief Represents a cache of loaded font faces.
 */
typedef struct font_face_cache_impl font_face_cache_t;

/**
 * @brief Create a new, empty font‐face cache.
 *
 * @return Pointer to a newly allocated `font_face_cache_t` object.
 */
NOVA_API font_face_cache_t* font_face_cache_create(void);

/**
 * @brief Increments the reference count of a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return A pointer to the same `font_face_cache_t` object with an incremented reference count.
 */
NOVA_API font_face_cache_t* font_face_cache_reference(font_face_cache_t* cache);

/**
 * @brief Decrement the reference count of a font‐face cache and destroy it when it reaches zero.
 *
 * @param cache A pointer to a `font_face_cache_t` object to release.
 */
NOVA_API void font_face_cache_destroy(font_face_cache_t* cache);

/**
 * @brief Retrieve the current reference count of a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return The current reference count, or 0 if cache is NULL.
 */
NOVA_API int font_face_cache_reference_count(const font_face_cache_t* cache);

/**
 * @brief Remove all entries from a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object to reset.
 */
NOVA_API void font_face_cache_reset(font_face_cache_t* cache);

/**
 * @brief Add a font face to the cache with the specified family and style.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param family The font family name.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param face A pointer to the `font_face_t` to add. The cache increments its reference count.
 */
NOVA_API void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, font_face_t* face);

/**
 * @brief Load a font face from a file and add it to the cache with the specified family and style.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param family The font family name to associate with the face.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param filename Path to the font file.
 * @param ttcindex Index of the face in a TrueType collection (use 0 for non-TTC fonts).
 * @return `true` on success, `false` if the file could not be loaded.
 */
NOVA_API bool font_face_cache_add_file(font_face_cache_t* cache, const char* family, bool bold, bool italic, const char* filename, int ttcindex);

/**
 * @brief Retrieve a font face from the cache by family and style.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param family The font family name.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @return A pointer to the matching `font_face_t` object, or NULL if not found. The returned face is owned by the cache and must not be destroyed by the caller.
 */
NOVA_API font_face_t* font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic);

/**
 * @brief Load all font faces from a file and add them to the cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param filename Path to the font file (TrueType, OpenType, or font collection).
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVA_API int font_face_cache_load_file(font_face_cache_t* cache, const char* filename);

/**
 * @brief Load all font faces from files in a directory recursively and add them to the cache.
 * 
 * This scans the specified directory recursively and loads all supported font files.
 * 
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param dirname Path to the directory containing font files.
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVA_API int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname);

/**
 * @brief Load all available system font faces and add them to the cache.
 *
 * This scans standard system font directories recursively and loads all supported font files.
 * 
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVA_API int font_face_cache_load_sys(font_face_cache_t* cache);

/**
 * @brief Represents a color with red, green, blue, and alpha components.
 */
typedef struct color {
    float r; ///< Red component (0 to 1).
    float g; ///< Green component (0 to 1).
    float b; ///< Blue component (0 to 1).
    float a; ///< Alpha (opacity) component (0 to 1).
} color_t;

#define NOVA_MAKE_COLOR(r, g, b, a) (color_t{r, g, b, a})

#define NOVA_BLACK_COLOR   NOVA_MAKE_COLOR(0, 0, 0, 1)
#define NOVA_WHITE_COLOR   NOVA_MAKE_COLOR(1, 1, 1, 1)
#define NOVA_RED_COLOR     NOVA_MAKE_COLOR(1, 0, 0, 1)
#define NOVA_GREEN_COLOR   NOVA_MAKE_COLOR(0, 1, 0, 1)
#define NOVA_BLUE_COLOR    NOVA_MAKE_COLOR(0, 0, 1, 1)
#define NOVA_YELLOW_COLOR  NOVA_MAKE_COLOR(1, 1, 0, 1)
#define NOVA_CYAN_COLOR    NOVA_MAKE_COLOR(0, 1, 1, 1)
#define NOVA_MAGENTA_COLOR NOVA_MAKE_COLOR(1, 0, 1, 1)

/**
 * @brief Initializes a color using RGB components in the 0-1 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 1).
 * @param g Green component (0 to 1).
 * @param b Blue component (0 to 1).
 */
NOVA_API void color_init_rgb(color_t* color_, float r, float g, float b);

/**
 * @brief Initializes a color using RGBA components in the 0-1 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 1).
 * @param g Green component (0 to 1).
 * @param b Blue component (0 to 1).
 * @param a Alpha component (0 to 1).
 */
NOVA_API void color_init_rgba(color_t* color_, float r, float g, float b, float a);

/**
 * @brief Initializes a color using RGB components in the 0-255 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 255).
 * @param g Green component (0 to 255).
 * @param b Blue component (0 to 255).
 */
NOVA_API void color_init_rgb8(color_t* color_, int r, int g, int b);

/**
 * @brief Initializes a color using RGBA components in the 0-255 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 255).
 * @param g Green component (0 to 255).
 * @param b Blue component (0 to 255).
 * @param a Alpha component (0 to 255).
 */
NOVA_API void color_init_rgba8(color_t* color_, int r, int g, int b, int a);

/**
 * @brief Initializes a color from a 32-bit unsigned RGBA value.
 * 
 * @param color A pointer to a `color_t` object.
 * @param value 32-bit unsigned RGBA value.
 */
NOVA_API void color_init_rgba32(color_t* color_, unsigned int value);

/**
 * @brief Initializes a color from a 32-bit unsigned ARGB value.
 * 
 * @param color A pointer to a `color_t` object.
 * @param value 32-bit unsigned ARGB value.
 */
NOVA_API void color_init_argb32(color_t* color_, unsigned int value);

/**
 * @brief Initializes a color with the specified HSL color values.
 *
 * @param color A pointer to a `color_t` object.
 * @param h Hue component in degrees (0 to 360).
 * @param s Saturation component (0 to 1).
 * @param l Lightness component (0 to 1).
 */
NOVA_API void color_init_hsl(color_t* color_, float h, float s, float l);

/**
 * @brief Initializes a color with the specified HSLA color values.
 *
 * @param color A pointer to a `color_t` object.
 * @param h Hue component in degrees (0 to 360).
 * @param s Saturation component (0 to 1).
 * @param l Lightness component (0 to 1).
 * @param a Alpha component (0 to 1).
 */
NOVA_API void color_init_hsla(color_t* color_, float h, float s, float l, float a);

/**
 * @brief Converts a color to a 32-bit unsigned RGBA value.
 * 
 * @param color A pointer to a `color_t` object.
 * 
 * @return 32-bit unsigned RGBA value.
 */
NOVA_API unsigned int color_to_rgba32(const color_t* color_);

/**
 * @brief Converts a color to a 32-bit unsigned ARGB value.
 * 
 * @param color A pointer to a `color_t` object.
 * 
 * @return 32-bit unsigned ARGB value.
 */
NOVA_API unsigned int color_to_argb32(const color_t* color_);

/**
 * @brief Parses a color from a string using CSS color syntax.
 * 
 * @param color A pointer to a `color_t` object to store the parsed color.
 * @param data A pointer to the input string containing the color data.
 * @param length The length of the input string in bytes, or `-1` if the string is null-terminated.
 * 
 * @return The number of characters consumed on success (including leading/trailing spaces), or 0 on failure.
 */
NOVA_API int color_parse(color_t* color_, const char* data, int length_);

/**
 * @brief Represents an image surface for drawing operations.
 *
 * Stores pixel data in a 32-bit premultiplied ARGB format (0xAARRGGBB),
 * where red, green, and blue channels are multiplied by the alpha channel
 * and divided by 255.
 */
typedef struct surface surface_t;

/**
 * @brief Creates a new image surface with the specified dimensions.
 *
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 * @return A pointer to the newly created `surface_t` object.
 */
/** @brief Decodes base64 text (used for data: URIs). Returns a malloc'd buffer the caller frees, or NULL. */
NOVA_API uint8_t* base64_decode(const char* data, int length_, size_t* out_length);

NOVA_API surface_t* surface_create(int width, int height);

/**
 * @brief Creates an image surface using existing pixel data.
 *
 * @param data Pointer to the pixel data.
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 * @param stride The number of bytes per row in the pixel data.
 * @return A pointer to the newly created `surface_t` object.
 */
NOVA_API surface_t* surface_create_for_data(unsigned char* data, int width, int height, int stride);

/**
 * @brief Loads an image surface from a file.
 *
 * @param filename Path to the image file.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVA_API surface_t* surface_load_from_image_file(const char* filename);

/**
 * @brief Loads an image surface from raw image data.
 *
 * @param data Pointer to the image data.
 * @param length Length of the data in bytes.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVA_API surface_t* surface_load_from_image_data(const void* data, int length_);

/**
 * @brief Loads an image surface from base64-encoded data.
 *
 * @param data Pointer to the base64-encoded image data.
 * @param length Length of the data in bytes, or `-1` if null-terminated.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVA_API surface_t* surface_load_from_image_base64(const char* data, int length_);

/**
 * @brief Increments the reference count for a surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Pointer to the `surface_t` object.
 */
NOVA_API surface_t* surface_reference(surface_t* surface);

/**
 * @brief Decrements the reference count and destroys the surface if the count reaches zero.
 *
 * @param surface Pointer to the `surface_t` object .
 */
NOVA_API void surface_destroy(surface_t* surface);

/**
 * @brief Gets the current reference count of a surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return The reference count of the surface.
 */
NOVA_API int surface_get_reference_count(const surface_t* surface);

/**
 * @brief Gets the pixel data of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Pointer to the pixel data.
 */
NOVA_API unsigned char* surface_get_data(const surface_t* surface);

/**
 * @brief Gets the width of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Width of the surface in pixels.
 */
NOVA_API int surface_get_width(const surface_t* surface);

/**
 * @brief Gets the height of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Height of the surface in pixels.
 */
NOVA_API int surface_get_height(const surface_t* surface);

/**
 * @brief Gets the stride of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Number of bytes per row.
 */
NOVA_API int surface_get_stride(const surface_t* surface);

/**
 * @brief Clears the entire surface with the specified color.
 *
 * @param surface Pointer to the target surface.
 * @param color Pointer to the color used for clearing.
 */
NOVA_API void surface_clear(surface_t* surface, const color_t* color_);

/**
 * @brief Converts pixel data from premultiplied ARGB to RGBA format.
 *
 * Transforms pixel data from native-endian 32-bit ARGB premultiplied format
 * to a non-premultiplied RGBA byte sequence.
 *
 * @param dst Pointer to the destination buffer (the same buffer as `src` for in-place use, otherwise not overlapping it).
 * @param src Pointer to the source buffer in ARGB premultiplied format.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Number of bytes per image row in the buffers.
 */
NOVA_API void convert_argb_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Converts pixel data from RGBA to premultiplied ARGB format.
 *
 * Transforms pixel data from a non-premultiplied RGBA byte sequence
 * to a native-endian 32-bit ARGB premultiplied format.
 *
 * @param dst Pointer to the destination buffer (the same buffer as `src` for in-place use, otherwise not overlapping it).
 * @param src Pointer to the source buffer in RGBA format.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Number of bytes per image row in the buffers.
 */
NOVA_API void convert_rgba_to_argb(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Defines the type of texture, either plain or tiled.
 */
enum class texture_type : int {
    plain, ///< Plain texture.
    tiled ///< Tiled texture.
};

/**
 * @brief Defines the spread method for gradients.
 */
enum class spread_method : uint8_t {
    pad, ///< Pad the gradient's edges.
    reflect, ///< Reflect the gradient beyond its bounds.
    repeat ///< Repeat the gradient pattern.
};

/**
 * @brief Represents a gradient stop.
 */
typedef struct {
    float offset; ///< The offset of the gradient stop, as a value between 0 and 1.
    color_t color_; ///< The color of the gradient stop.
} gradient_stop_t;

/**
 * @brief Represents a paint object used for drawing operations.
 */
typedef struct paint paint_t;

/**
 * @brief Creates a solid RGB paint.
 *
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_rgb(float r, float g, float b);

/**
 * @brief Creates a solid RGBA paint.
 *
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @param a The alpha component (0 to 1).
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_rgba(float r, float g, float b, float a);

/**
 * @brief Creates a solid color paint.
 *
 * @param color A pointer to the `color_t` object.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_color(const color_t* color_);

/**
 * @brief Creates a linear gradient paint.
 *
 * @param x1 The x coordinate of the gradient start.
 * @param y1 The y coordinate of the gradient start.
 * @param x2 The x coordinate of the gradient end.
 * @param y2 The y coordinate of the gradient end.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_linear_gradient(float x1, float y1, float x2, float y2,
    spread_method spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

/**
 * @brief Creates a radial gradient paint.
 *
 * @param cx The x coordinate of the gradient center.
 * @param cy The y coordinate of the gradient center.
 * @param cr The radius of the gradient.
 * @param fx The x coordinate of the focal point.
 * @param fy The y coordinate of the focal point.
 * @param fr The radius of the focal point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr,
    spread_method spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

/**
 * @brief Creates a texture paint from a surface.
 *
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_texture(surface_t* surface, texture_type type_, float opacity, const stb::matrix* matrix);

/**
 * @brief Increments the reference count of a paint object.
 *
 * @param paint A pointer to the `paint_t` object.
 * @return A pointer to the referenced `paint_t` object.
 */
NOVA_API paint_t* paint_reference(paint_t* paint_);

/**
 * @brief Decrements the reference count and destroys the paint if the count reaches zero.
 *
 * @param paint A pointer to the `paint_t` object.
 */
NOVA_API void paint_destroy(paint_t* paint_);

/**
 * @brief Retrieves the reference count of a paint object.
 *
 * @param paint A pointer to the `paint_t` object.
 * @return The reference count of the `paint_t` object.
 */
NOVA_API int paint_get_reference_count(const paint_t* paint_);

/**
 * @brief Defines fill rule types for filling paths.
 */
enum class fill_rule : uint8_t {
    non_zero, ///< Non-zero winding fill rule.
    even_odd ///< Even-odd fill rule.
};

/**
 * @brief Defines compositing operations.
 */
enum class blend_mode : int {
    clear,       ///< Clears the destination (resulting in a fully transparent image).
    src,         ///< Source replaces destination.
    dst,         ///< Destination is kept, source is ignored.
    src_over,    ///< Source is composited over destination.
    dst_over,    ///< Destination is composited over source.
    src_in,      ///< Source within destination (only the overlapping part of source is shown).
    dst_in,      ///< Destination within source.
    src_out,     ///< Source outside destination (non-overlapping part of source is shown).
    dst_out,     ///< Destination outside source.
    src_atop,    ///< Source atop destination (source shown over destination but only in the destination's bounds).
    dst_atop,    ///< Destination atop source (destination shown over source but only in the source's bounds).
    exclusive_or,        ///< Source and destination are combined, but their overlapping regions are cleared.
    // CSS blend modes (W3C Compositing and Blending): the source is blended with the backdrop's colour,
    // then composited over it. Appended after the Porter-Duff operators; same order as the tables in blend.h.
    multiply,    ///< Backdrop * source: darkens, white is neutral.
    screen,      ///< Inverse of multiply: lightens, black is neutral.
    darken,      ///< The darker of backdrop and source, per channel.
    lighten,     ///< The lighter of backdrop and source, per channel.
    difference,  ///< Absolute difference of backdrop and source, per channel.
    exclusion    ///< Like difference, with lower contrast.
};

/**
 * @brief Defines the shape used at the ends of open subpaths.
 */
enum class line_cap : uint8_t {
    butt, ///< Flat edge at the end of the stroke.
    round, ///< Rounded ends at the end of the stroke.
    square ///< Square ends at the end of the stroke.
};

/**
 * @brief Defines the shape used at the corners of paths.
 */
enum class line_join : uint8_t {
    miter, ///< Miter join with sharp corners.
    round, ///< Rounded join.
    bevel ///< Beveled join with a flattened corner.
};

/**
 * @brief Represents a drawing context.
 */
typedef struct canvas_context canvas_t;

/**
 * @brief Creates a drawing context on a surface.
 *
 * @param surface A pointer to a `surface_t` object.
 * @return A pointer to the newly created `canvas_t` object.
 */
NOVA_API canvas_t* canvas_create(surface_t* surface);

/**
 * @brief Increases the reference count of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The same pointer to the `canvas_t` object.
 */
NOVA_API canvas_t* canvas_reference(canvas_t* canvas);

/**
 * @brief Decreases the reference count and destroys the canvas when it reaches zero.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_destroy(canvas_t* canvas);

/**
 * @brief Retrieves the reference count of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current reference count.
 */
NOVA_API int canvas_get_reference_count(const canvas_t* canvas);

/**
 * @brief Gets the surface associated with the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to the `surface_t` object.
 */
NOVA_API surface_t* canvas_get_surface(const canvas_t* canvas);

/**
 * @brief Saves the current state of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_save(canvas_t* canvas);

/**
 * @brief Restores the canvas to the most recently saved state.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_restore(canvas_t* canvas);

/**
 * @brief Sets the current paint to a solid color.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 */
NOVA_API void canvas_set_rgb(canvas_t* canvas, float r, float g, float b);

/**
 * @brief Sets the current paint to a solid color.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @param a The alpha component (0 to 1).
 */
NOVA_API void canvas_set_rgba(canvas_t* canvas, float r, float g, float b, float a);

/**
 * @brief Sets the current paint to a solid color.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param color A pointer to a `color_t` object.
 */
NOVA_API void canvas_set_color(canvas_t* canvas, const color_t* color_);

/**
 * @brief Sets the current paint to a linear gradient.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x1 The x coordinate of the start point.
 * @param y1 The y coordinate of the start point.
 * @param x2 The x coordinate of the end point.
 * @param y2 The y coordinate of the end point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 */
NOVA_API void canvas_set_linear_gradient(canvas_t* canvas, float x1, float y1, float x2, float y2,
    spread_method spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

/**
 * @brief Sets the current paint to a radial gradient.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cx The x coordinate of the center.
 * @param cy The y coordinate of the center.
 * @param cr The radius of the gradient.
 * @param fx The x coordinate of the focal point.
 * @param fy The y coordinate of the focal point.
 * @param fr The radius of the focal point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 */
NOVA_API void canvas_set_radial_gradient(canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr,
    spread_method spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

/**
 * @brief Sets the current paint to a texture.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 */
NOVA_API void canvas_set_texture(canvas_t* canvas, surface_t* surface, texture_type type_, float opacity, const stb::matrix* matrix);

/**
 * @brief Sets the current paint.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param paint The paint to be used for subsequent drawing operations.
 */
NOVA_API void canvas_set_paint(canvas_t* canvas, paint_t* paint_);

/**
 * @brief Retrieves the current paint.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param color A pointer to a `color_t` object where the current color will be stored.
 * @return The current `paint_t` used for drawing operations. If no paint is set, `NULL` is returned.
 */
NOVA_API paint_t* canvas_get_paint(const canvas_t* canvas, color_t* color_);

/**
 * @brief Assigns a font-face cache to the canvas for font management.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cache A pointer to a `font_face_cache_t` object, or NULL to unset the current cache.
 */
NOVA_API void canvas_set_font_face_cache(canvas_t* canvas, font_face_cache_t* cache);

/**
 * @brief Returns the font-face cache associated with the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to the associated `font_face_cache_t` object, or NULL if none is set.
 */
NOVA_API font_face_cache_t* canvas_get_font_face_cache(const canvas_t* canvas);

/**
 * @brief Add a font face to the canvas using the specified family and style.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param family The font family name to associate with the face.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param face A pointer to the `font_face_t` object to add.
 */
NOVA_API void canvas_add_font_face(canvas_t* canvas, const char* family, bool bold, bool italic, font_face_t* face);

/**
 * @brief Load a font face from a file and add it to the canvas using the specified family and style.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param family The font family name to associate with the face.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param filename Path to the font file.
 * @param ttcindex Index within a TrueType Collection (use 0 for regular font files).
 * @return `true` on success, or `false` if the font could not be loaded.
 */
NOVA_API bool canvas_add_font_file(canvas_t* canvas, const char* family, bool bold, bool italic, const char* filename, int ttcindex);

/**
 * @brief Selects and sets the current font face on the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param family The font family name to select.
 * @param bold Whether to match a bold variant.
 * @param italic Whether to match an italic variant.
 * @return `true` if a matching font was found and set, `false` otherwise.
 */
NOVA_API bool canvas_select_font_face(canvas_t* canvas, const char* family, bool bold, bool italic);

/**
 * @brief Sets the font face and size for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`, and the default face size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param face A pointer to a `font_face_t` object representing the font face to use.
 * @param size The size of the font, in pixels. This determines the height of the rendered text.
 */
NOVA_API void canvas_set_font(canvas_t* canvas, font_face_t* face, float size_);

/**
 * @brief Sets the font face for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param face A pointer to a `font_face_t` object representing the font face to use.
 */
NOVA_API void canvas_set_font_face(canvas_t* canvas, font_face_t* face);

/**
 * @brief Retrieves the current font face used for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to a `font_face_t` object representing the current font face.
 */
NOVA_API font_face_t* canvas_get_font_face(const canvas_t* canvas);

/**
 * @brief Sets the font size for text rendering on the canvas.
 *
 * If not set, the default font size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param size The size of the font, in pixels. This value defines the height of the rendered text.
 */
NOVA_API void canvas_set_font_size(canvas_t* canvas, float size_);

/**
 * @brief Retrieves the current font size used for text rendering on the canvas.
 *
 * If not set, the default font size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current font size, in pixels. This value represents the height of the rendered text.
 */
NOVA_API float canvas_get_font_size(const canvas_t* canvas);

/**
 * @brief Sets the fill rule.
 *
 * If not set, the default fill rule is `fill_rule::non_zero`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param winding The fill rule.
 */
NOVA_API void canvas_set_fill_rule(canvas_t* canvas, fill_rule winding);

/**
 * @brief Retrieves the current fill rule.
 *
 * If not set, the default fill rule is `fill_rule::non_zero`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current fill rule.
 */
NOVA_API fill_rule canvas_get_fill_rule(const canvas_t* canvas);

/**
 * @brief Sets the compositing operator.
 *
 * If not set, the default compositing operator is `blend_mode::src_over`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param op The compositing operator.
 */
NOVA_API void canvas_set_operator(canvas_t* canvas, blend_mode op);

/**
 * @brief Retrieves the current compositing operator.
 *
 * If not set, the default compositing operator is `blend_mode::src_over`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current compositing operator.
 */
NOVA_API blend_mode canvas_get_operator(const canvas_t* canvas);

/**
 * @brief Sets the global opacity.
 *
 * If not set, the default global opacity is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param opacity The opacity value (0 to 1).
 */
NOVA_API void canvas_set_opacity(canvas_t* canvas, float opacity);

/**
 * @brief Retrieves the current global opacity.
 *
 * If not set, the default global opacity is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current opacity value.
 */
NOVA_API float canvas_get_opacity(const canvas_t* canvas);

/**
 * @brief Sets the line width.
 *
 * If not set, the default line width is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_width The width of the stroke.
 */
NOVA_API void canvas_set_line_width(canvas_t* canvas, float line_width);

/**
 * @brief Retrieves the current line width.
 *
 * If not set, the default line width is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line width.
 */
NOVA_API float canvas_get_line_width(const canvas_t* canvas);

/**
 * @brief Sets the line cap style.
 *
 * If not set, the default line cap is `line_cap::butt`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_cap The line cap style.
 */
NOVA_API void canvas_set_line_cap(canvas_t* canvas, line_cap line_cap_);

/**
 * @brief Retrieves the current line cap style.
 *
 * If not set, the default line cap is `line_cap::butt`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line cap style.
 */
NOVA_API line_cap canvas_get_line_cap(const canvas_t* canvas);

/**
 * @brief Sets the line join style.
 *
 * If not set, the default line join is `line_join::miter`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_join The line join style.
 */
NOVA_API void canvas_set_line_join(canvas_t* canvas, line_join line_join_);

/**
 * @brief Retrieves the current line join style.
 *
 * If not set, the default line join is `line_join::miter`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line join style.
 */
NOVA_API line_join canvas_get_line_join(const canvas_t* canvas);

/**
 * @brief Sets the miter limit.
 *
 * If not set, the default miter limit is 10.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param miter_limit The miter limit value.
 */
NOVA_API void canvas_set_miter_limit(canvas_t* canvas, float miter_limit);

/**
 * @brief Retrieves the current miter limit.
 *
 * If not set, the default miter limit is 10.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current miter limit value.
 */
NOVA_API float canvas_get_miter_limit(const canvas_t* canvas);

/**
 * @brief Sets the dash pattern.
 *
 * If not set, the default dash offset is 0, and the default dash array is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param offset The dash offset.
 * @param dashes Array of dash lengths.
 * @param ndashes Number of dash lengths.
 */
NOVA_API void canvas_set_dash(canvas_t* canvas, float offset, const float* dashes, int ndashes);

/**
 * @brief Sets the dash offset.
 *
 * If not set, the default dash offset is 0.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param offset The dash offset.
 */
NOVA_API void canvas_set_dash_offset(canvas_t* canvas, float offset);

/**
 * @brief Retrieves the current dash offset.
 *
 * If not set, the default dash offset is 0.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current dash offset.
 */
NOVA_API float canvas_get_dash_offset(const canvas_t* canvas);

/**
 * @brief Sets the dash pattern.
 *
 * If not set, the default dash array is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param dashes Array of dash lengths.
 * @param ndashes Number of dash lengths.
 */
NOVA_API void canvas_set_dash_array(canvas_t* canvas, const float* dashes, int ndashes);

/**
 * @brief Retrieves the current dash pattern.
 *
 * If not set, the default dash array is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param dashes Pointer to store the dash array.
 * @return The number of dash lengths.
 */
NOVA_API int canvas_get_dash_array(const canvas_t* canvas, const float** dashes);

/**
 * @brief Translates the current transformation matrix by offsets `tx` and `ty`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param tx The translation offset in the x-direction.
 * @param ty The translation offset in the y-direction.
 */
NOVA_API void canvas_translate(canvas_t* canvas, float tx, float ty);

/**
 * @brief Scales the current transformation matrix by factors `sx` and `sy`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param sx The scaling factor in the x-direction.
 * @param sy The scaling factor in the y-direction.
 */
NOVA_API void canvas_scale(canvas_t* canvas, float sx, float sy);

/**
 * @brief Shears the current transformation matrix by factors `shx` and `shy`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param shx The shearing factor in the x-direction.
 * @param shy The shearing factor in the y-direction.
 */
NOVA_API void canvas_shear(canvas_t* canvas, float shx, float shy);

/**
 * @brief Rotates the current transformation matrix by the specified angle (in radians).
 * @param canvas A pointer to a `canvas_t` object.
 * @param angle The rotation angle in radians.
 */
NOVA_API void canvas_rotate(canvas_t* canvas, float angle);

/**
 * @brief Multiplies the current transformation matrix with the specified `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param matrix A pointer to the `stb::matrix` object.
 */
NOVA_API void canvas_transform(canvas_t* canvas, const stb::matrix* matrix);

/**
 * @brief Resets the current transformation matrix to the identity matrix.
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_reset_matrix(canvas_t* canvas);

/**
 * @brief Resets the current transformation matrix to the specified `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param matrix A pointer to the `stb::matrix` object.
 */
NOVA_API void canvas_set_matrix(canvas_t* canvas, const stb::matrix* matrix);

/**
 * @brief Stores the current transformation matrix in `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param A pointer to the `stb::matrix` to store the matrix.
 */
NOVA_API void canvas_get_matrix(const canvas_t* canvas, stb::matrix* matrix);

/**
 * @brief Transforms the point `(x, y)` using the current transformation matrix and stores the result in `(xx, yy)`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the point to transform.
 * @param y The y-coordinate of the point to transform.
 * @param xx A pointer to store the transformed x-coordinate.
 * @param yy A pointer to store the transformed y-coordinate.
 */
NOVA_API void canvas_map(const canvas_t* canvas, float x, float y, float* xx, float* yy);

/**
 * @brief Transforms the `src` point using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `canvas_t` object.
 * @param src A pointer to the `stb::point` point to transform.
 * @param dst A pointer to the `stb::point` to store the transformed point.
 */
NOVA_API void canvas_map_point(const canvas_t* canvas, const stb::point* src, stb::point* dst);

/**
 * @brief Transforms the `src` rectangle using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `canvas_t` object.
 * @param src A pointer to the `rect_t` rectangle to transform.
 * @param dst A pointer to the `rect_t` to store the transformed rectangle.
 */
NOVA_API void canvas_map_rect(const canvas_t* canvas, const rect_t* src, rect_t* dst);

/**
 * @brief Moves the current point to a new position.
 *
 * Moves the current point to the specified coordinates without adding a line.
 * This operation is added to the current path. Equivalent to the svg `M` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 */
NOVA_API void canvas_move_to(canvas_t* canvas, float x, float y);

/**
 * @brief Adds a straight line segment to the current path.
 *
 * Adds a straight line from the current point to the specified coordinates.
 * This segment is added to the current path. Equivalent to the svg `L` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the end point of the line.
 * @param y The y-coordinate of the end point of the line.
 */
NOVA_API void canvas_line_to(canvas_t* canvas, float x, float y);

/**
 * @brief Adds a quadratic Bézier curve to the current path.
 *
 * Adds a quadratic Bézier curve from the current point to the specified end point,
 * using the given control point. This curve is added to the current path. Equivalent to the svg `Q` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x1 The x-coordinate of the control point.
 * @param y1 The y-coordinate of the control point.
 * @param x2 The x-coordinate of the end point of the curve.
 * @param y2 The y-coordinate of the end point of the curve.
 */
NOVA_API void canvas_quad_to(canvas_t* canvas, float x1, float y1, float x2, float y2);

/**
 * @brief Adds a cubic Bézier curve to the current path.
 *
 * Adds a cubic Bézier curve from the current point to the specified end point,
 * using the given control points. This curve is added to the current path. Equivalent to the svg `C` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x1 The x-coordinate of the first control point.
 * @param y1 The y-coordinate of the first control point.
 * @param x2 The x-coordinate of the second control point.
 * @param y2 The y-coordinate of the second control point.
 * @param x3 The x-coordinate of the end point of the curve.
 * @param y3 The y-coordinate of the end point of the curve.
 */
NOVA_API void canvas_cubic_to(canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3);

/**
 * @brief Adds an elliptical arc to the current path.
 *
 * Adds an elliptical arc from the current point to the specified end point,
 * defined by radii, rotation angle, and flags for arc type and direction.
 * This arc segment is added to the current path. Equivalent to the svg `A` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse in degrees.
 * @param large_arc_flag If true, add the large arc; otherwise, add the small arc.
 * @param sweep_flag If true, add the arc in the positive-angle direction; otherwise, in the negative-angle direction.
 * @param x The x-coordinate of the end point.
 * @param y The y-coordinate of the end point.
 */
NOVA_API void canvas_arc_to(canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

/**
 * @brief Adds a rectangle to the current path.
 *
 * Adds a rectangle with the specified position and dimensions to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
NOVA_API void canvas_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Adds a rounded rectangle to the current path.
 *
 * Adds a rectangle with rounded corners defined by the specified position,
 * dimensions, and corner radii to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param rx The x-radius of the corners.
 * @param ry The y-radius of the corners.
 */
NOVA_API void canvas_round_rect(canvas_t* canvas, float x, float y, float w, float h, float rx, float ry);

/**
 * @brief Adds an ellipse to the current path.
 *
 * Adds an ellipse centered at the specified coordinates with the given radii to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cx The x-coordinate of the ellipse's center.
 * @param cy The y-coordinate of the ellipse's center.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 */
NOVA_API void canvas_ellipse(canvas_t* canvas, float cx, float cy, float rx, float ry);

/**
 * @brief Adds a circle to the current path.
 *
 * Adds a circle centered at the specified coordinates with the given radius to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param r The radius of the circle.
 */
NOVA_API void canvas_circle(canvas_t* canvas, float cx, float cy, float r);

/**
 * @brief Adds a path to the current path.
 *
 * Appends the elements of the specified path to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param path A pointer to the `path_t` object to be added.
 */
NOVA_API void canvas_add_path(canvas_t* canvas, const stb::outline& path_);

/**
 * @brief Starts a new path on the canvas.
 *
 * Begins a new path, clearing any existing path data. The new path starts with no commands.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_new_path(canvas_t* canvas);

/**
 * @brief Closes the current path.
 *
 * Closes the current path by adding a straight line back to the starting point.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_close_path(canvas_t* canvas);

/**
 * @brief Retrieves the current point of the canvas.
 *
 * Gets the coordinates of the current point in the canvas, which is the last point
 * added or moved to in the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the current point.
 * @param y The y-coordinate of the current point.
 */
NOVA_API void canvas_get_current_point(const canvas_t* canvas, float* x, float* y);

/**
 * @brief Tests whether a point lies within the current fill region.
 *
 * Determines whether the point at coordinates `(x, y)` falls within the area
 * that would be filled by a `canvas_fill()` operation, given the current path,
 * fill rule, and transformation state.
 *
 * @note Clipping and surface dimensions are not considered in this test.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The X coordinate of the point, in user space.
 * @param y The Y coordinate of the point, in user space.
 * @return `true` if the point is within the fill region, `false` otherwise.
 */
NOVA_API bool canvas_fill_contains(canvas_t* canvas, float x, float y);

/**
 * @brief Tests whether a point lies within the current stroke region.
 *
 * Determines whether the point at coordinates `(x, y)` falls within the area
 * that would be stroked by a `canvas_stroke()` operation, given the current path,
 * stroke width, joins, caps, miter limit, dash pattern, and transformation state.
 *
 * @note Clipping and surface dimensions are not considered in this test.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The X coordinate of the point, in user space.
 * @param y The Y coordinate of the point, in user space.
 * @return `true` if the point is within the stroke region, `false` otherwise.
 */
NOVA_API bool canvas_stroke_contains(canvas_t* canvas, float x, float y);

/**
 * @brief Tests whether a point lies within the current clipping region.
 *
 * Determines whether the point at coordinates `(x, y)` falls within the active clipping
 * region on the canvas.
 *
 * If no clipping is applied, the default clipping region covers the entire canvas
 * area starting at `(0, 0)` with width and height equal to the canvas dimensions.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The X coordinate of the point, in user space.
 * @param y The Y coordinate of the point, in user space.
 * @return `true` if the point is within the clipping region, `false` otherwise.
 */
NOVA_API bool canvas_clip_contains(canvas_t* canvas, float x, float y);

/**
 * @brief Computes the bounding box of the area that would be affected by a fill operation.
 *
 * Computes an axis-aligned bounding box in user space that encloses the area
 * which would be affected by a fill operation (`canvas_fill()`) given the current path,
 * fill rule, and transformation state.
 * 
 * @note Clipping and surface dimensions are not considered in this calculation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param extents A pointer to a `rect_t` structure that receives the bounding box.
 */
NOVA_API void canvas_fill_extents(canvas_t* canvas, rect_t* extents);

/**
 * @brief Computes the bounding box of the area that would be affected by a stroke operation.
 *
 * Computes an axis-aligned bounding box in user space that encloses the area
 * which would be affected by a stroke operation (`canvas_stroke()`) given the current path,
 * stroke width, joins, caps, miter limit, dash pattern, and transformation state.
 *
 * @note Clipping and surface dimensions are not considered in this calculation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param extents A pointer to a `rect_t` structure that receives the bounding box.
 */
NOVA_API void canvas_stroke_extents(canvas_t* canvas, rect_t* extents);

/**
 * @brief Gets the bounding box of the current clipping region.
 *
 * Computes an axis-aligned bounding box in user space that encloses the currently active
 * clipping region on the canvas.
 *
 * If no clip is applied, the returned rectangle covers the entire canvas area,
 * starting at `(0, 0)` with width and height equal to the canvas dimensions.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param extents A pointer to a `rect_t` structure that receives the bounding box.
 */
NOVA_API void canvas_clip_extents(canvas_t* canvas, rect_t* extents);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_fill(canvas_t* canvas);

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_stroke(canvas_t* canvas);

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_clip(canvas_t* canvas);

/**
 * @brief A drawing operator that paints the current clipping region using the current paint.
 *
 * @note The current path will not be affected by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_paint(canvas_t* canvas);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_fill_preserve(canvas_t* canvas);

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_stroke_preserve(canvas_t* canvas);

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVA_API void canvas_clip_preserve(canvas_t* canvas);

/**
 * @brief Fills a rectangle according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
NOVA_API void canvas_fill_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Fills a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVA_API void canvas_fill_path(canvas_t* canvas, const stb::outline& path_);

/**
 * @brief Strokes a rectangle with the current stroke settings.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
NOVA_API void canvas_stroke_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Strokes a path with the current stroke settings.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVA_API void canvas_stroke_path(canvas_t* canvas, const stb::outline& path_);

/**
 * @brief Intersects the current clipping region with a rectangle according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
NOVA_API void canvas_clip_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Intersects the current clipping region with a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVA_API void canvas_clip_path(canvas_t* canvas, const stb::outline& path_);

/**
 * @brief Adds a glyph to the current path at the specified origin.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The advance width of the glyph.
 */
NOVA_API float canvas_add_glyph(canvas_t* canvas, codepoint_t codepoint, float x, float y);

/**
 * @brief Adds text to the current path at the specified origin.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
NOVA_API float canvas_add_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y);

/**
 * @brief Fills a text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
NOVA_API float canvas_fill_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y);

/**
 * @brief Strokes a text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
NOVA_API float canvas_stroke_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y);

/**
 * @brief Intersects the current clipping region with text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
NOVA_API float canvas_clip_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y);

/**
 * @brief Retrieves font metrics for the current font.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param ascent The ascent of the font.
 * @param descent The descent of the font.
 * @param line_gap The line gap of the font.
 * @param extents The bounding box of the font.
 */
NOVA_API void canvas_font_metrics(const canvas_t* canvas, float* ascent, float* descent, float* line_gap, rect_t* extents);

/**
 * @brief Retrieves metrics for a specific glyph.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param advance_width The advance width of the glyph.
 * @param left_side_bearing The left side bearing of the glyph.
 * @param extents The bounding box of the glyph.
 */
NOVA_API void canvas_glyph_metrics(canvas_t* canvas, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents);

/**
 * @brief Retrieves the extents of a text.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param extents The bounding box of the text.
 * @return The total advance width of the text.
 */
NOVA_API float canvas_text_extents(canvas_t* canvas, const void* text, int length_, text_encoding encoding, rect_t* extents);

} // namespace vg
} // namespace nova

#endif // NOVA_RENDER_H
