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
#include <algorithm>
#include <cmath>
#include <stb/color.hpp>
#include <stb/image.hpp>
#include <memory>
#include <string_view>
#include <stb/raster.hpp>
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

// Fill rule, line cap and line join are libstb's (stb/raster.hpp).
using stb::fill_rule;
using stb::line_cap;
using stb::line_join;

// The rest of libstb's vocabulary, under its own names (matrix and point are not here: vg has its own `point` and
// `transform::matrix()`).
using stb::blend_mode;
using stb::color;
using stb::composite_op;
using stb::image;
using stb::outline;
using stb::outline_bounds;
using stb::outline_op;
using stb::span_set;


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

inline constexpr float pi = 3.14159265358979323846f;
inline constexpr float sqrt2 = 1.41421356237309504880f;

constexpr float deg_to_rad(float degrees) { return degrees * (pi / 180.0f); }
constexpr float rad_to_deg(float radians) { return radians * (180.0f / pi); }

/**
 * @brief A structure representing a rectangle in 2D space.
 */
typedef struct {
    float x; ///< The x-coordinate of the top-left corner of the rectangle.
    float y; ///< The y-coordinate of the top-left corner of the rectangle.
    float w; ///< The width of the rectangle.
    float h; ///< The height of the rectangle.
} rect_t;

// Matrices are stb::matrix and points are stb::point (<stb/outline.hpp>); mapping points and boxes is `stb::matrix::map` / `map_rect`; the helper below is what libstb does not have.

/// Parses an SVG `transform` list (`matrix(...) translate(...) ...`) into `matrix`. Returns false on a syntax error.
/// `length` is the byte length of `data`, or -1 for a NUL-terminated string.
bool parse_matrix(stb::matrix* matrix, const char* data, int length);

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
typedef void (*path_traverse_func_t)(void* closure, outline_op command, const stb::point* points, int npoints);

/**
 * @brief The run a piece of text is shaped in (ADR-0012).
 *
 * `text` passed next to it must be a view into `run` (UTF-32). The piece is then shaped as a part of the whole run:
 * kerning, ligatures and joined letters across its edges are those of the run. An empty `run` shapes the piece alone.
 * `ligatures` false drops the optional ligatures (`letter-spacing` needs that).
 */
struct shaping_context {
    std::u32string_view run;
    bool ligatures = true;
};

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
class font_face_data;

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
 * @param face A pointer to the `font_face_data` to add. The cache increments its reference count.
 */
NOVA_API void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, font_face_data* face);

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
 * @return A pointer to the matching `font_face_data` object, or NULL if not found. The returned face is owned by the cache and must not be destroyed by the caller.
 */
NOVA_API font_face_data* font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic);

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

/// A colour with float channels (0 to 1, clamped) rounded to the 8-bit stb::color.
inline color color_from_floats(float r, float g, float b, float a = 1.f)
{
    auto channel = [](float v) { return static_cast<uint8_t>(lroundf(std::clamp(v, 0.f, 1.f) * 255.f)); };
    return color(channel(r), channel(g), channel(b), channel(a));
}

/// The colour packed as 0xAARRGGBB (alpha in the high byte; stb::color::value() is 0xRRGGBBAA).
inline uint32_t color_to_argb32(const color& c)
{
    return (uint32_t(c.a()) << 24) | (uint32_t(c.r()) << 16) | (uint32_t(c.g()) << 8) | uint32_t(c.b());
}

/**
 * @brief The drawing surface: 32-bit premultiplied ARGB pixels (0xAARRGGBB), libstb's type.
 *
 * Pixmaps, canvases and texture paints share one through std::shared_ptr.
 */
using surface_t = stb::premultiplied_image;

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
enum class spread_method_value : uint8_t {
    pad, ///< Pad the gradient's edges.
    reflect, ///< Reflect the gradient beyond its bounds.
    repeat ///< Repeat the gradient pattern.
};

/**
 * @brief Represents a gradient stop.
 */
typedef struct {
    float offset; ///< The offset of the gradient stop, as a value between 0 and 1.
    color color_; ///< The color of the gradient stop.
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
 * @param color A pointer to the `color` object.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_color(const color* color_);

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
    spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

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
    spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

/**
 * @brief Creates a texture paint from a surface.
 *
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `paint_t` object.
 */
NOVA_API paint_t* paint_create_texture(std::shared_ptr<surface_t> surface, texture_type type_, float opacity, const stb::matrix* matrix);

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
 * @brief Represents a drawing context.
 */
typedef struct canvas_context canvas_t;

/**
 * @brief Creates a drawing context on a surface.
 *
 * @param surface A pointer to a `surface_t` object.
 * @return A pointer to the newly created `canvas_t` object.
 */
NOVA_API canvas_t* canvas_create(std::shared_ptr<surface_t> surface);

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
 * @param color A pointer to a `color` object.
 */
NOVA_API void canvas_set_color(canvas_t* canvas, const color* color_);

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
    spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

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
    spread_method_value spread, const gradient_stop_t* stops, int nstops, const stb::matrix* matrix);

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
NOVA_API void canvas_set_texture(canvas_t* canvas, std::shared_ptr<surface_t> surface, texture_type type_, float opacity, const stb::matrix* matrix);

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
 * @param color A pointer to a `color` object where the current color will be stored.
 * @return The current `paint_t` used for drawing operations. If no paint is set, `NULL` is returned.
 */
NOVA_API paint_t* canvas_get_paint(const canvas_t* canvas, color* color_);

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
 * @param face A pointer to the `font_face_data` object to add.
 */
NOVA_API void canvas_add_font_face(canvas_t* canvas, const char* family, bool bold, bool italic, font_face_data* face);

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
 * @param face A pointer to a `font_face_data` object representing the font face to use.
 * @param size The size of the font, in pixels. This determines the height of the rendered text.
 */
NOVA_API void canvas_set_font(canvas_t* canvas, font_face_data* face, float size_);

/**
 * @brief Sets the font face for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param face A pointer to a `font_face_data` object representing the font face to use.
 */
NOVA_API void canvas_set_font_face(canvas_t* canvas, font_face_data* face);

/**
 * @brief Retrieves the current font face used for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to a `font_face_data` object representing the current font face.
 */
NOVA_API font_face_data* canvas_get_font_face(const canvas_t* canvas);

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
 * @brief Sets the compositing operator and blend mode.
 *
 * `op` is a Porter-Duff operator and `mode` how the colours mix (libstb's types; a blend mode other than `normal`
 * with an operator other than `source_over` is allowed, see `stb::composite_row`). If not set, the default is
 * `composite_op::source_over` with `blend_mode::normal`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param op The compositing operator.
 * @param mode The blend mode.
 */
NOVA_API void canvas_set_operator(canvas_t* canvas, composite_op op, blend_mode mode = blend_mode::normal);

/**
 * @brief Retrieves the current compositing operator.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current compositing operator.
 */
NOVA_API composite_op canvas_get_operator(const canvas_t* canvas);

/**
 * @brief Retrieves the current blend mode.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current blend mode.
 */
NOVA_API blend_mode canvas_get_blend_mode(const canvas_t* canvas);

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
NOVA_API void canvas_add_path(canvas_t* canvas, const outline& path_);

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
NOVA_API void canvas_fill_path(canvas_t* canvas, const outline& path_);

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
NOVA_API void canvas_stroke_path(canvas_t* canvas, const outline& path_);

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
NOVA_API void canvas_clip_path(canvas_t* canvas, const outline& path_);

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
NOVA_API float canvas_add_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y, const shaping_context& context = {});

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
NOVA_API float canvas_fill_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y, const shaping_context& context = {});

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
NOVA_API float canvas_stroke_text(canvas_t* canvas, const void* text, int length_, text_encoding encoding, float x, float y, const shaping_context& context = {});

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
