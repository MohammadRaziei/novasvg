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

#ifndef NOVASVG_RENDER_H
#define NOVASVG_RENDER_H

#include <stdbool.h>

// Pure C++ project: no separate C translation units link against this,
// so the extern "C" wrapper this file used to have (for use as a
// standalone C library) has been dropped along with the novasvg_ prefix
// below -- everything here now lives in namespace novasvg::render
// instead.
namespace novasvg {
namespace render {

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

#define NOVASVG_RENDER_VERSION_MAJOR 1
#define NOVASVG_RENDER_VERSION_MINOR 3
#define NOVASVG_RENDER_VERSION_MICRO 2

#define NOVASVG_RENDER_VERSION_ENCODE(major, minor, micro) (((major) * 10000) + ((minor) * 100) + ((micro) * 1))
#define NOVASVG_RENDER_VERSION NOVASVG_RENDER_VERSION_ENCODE(NOVASVG_RENDER_VERSION_MAJOR, NOVASVG_RENDER_VERSION_MINOR, NOVASVG_RENDER_VERSION_MICRO)

#define NOVASVG_RENDER_VERSION_XSTRINGIZE(major, minor, micro) #major"."#minor"."#micro
#define NOVASVG_RENDER_VERSION_STRINGIZE(major, minor, micro) NOVASVG_RENDER_VERSION_XSTRINGIZE(major, minor, micro)
#define NOVASVG_RENDER_VERSION_STRING NOVASVG_RENDER_VERSION_STRINGIZE(NOVASVG_RENDER_VERSION_MAJOR, NOVASVG_RENDER_VERSION_MINOR, NOVASVG_RENDER_VERSION_MICRO)

/**
 * @brief Gets the version of the novasvg rendering library.
 * @return An integer representing the version of the novasvg rendering library.
 */
NOVASVG_API int render_version(void);

/**
 * @brief Gets the version of the novasvg rendering library as a string.
 * @return A string representing the version of the novasvg rendering library.
 */
NOVASVG_API const char* render_version_string(void);

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
typedef void (*write_func_t)(void* closure, void* data, int size);

#define NOVASVG_PI      3.14159265358979323846f
#define NOVASVG_TWO_PI  6.28318530717958647693f
#define NOVASVG_HALF_PI 1.57079632679489661923f
#define NOVASVG_SQRT2   1.41421356237309504880f
#define NOVASVG_KAPPA   0.55228474983079339840f

#define NOVASVG_DEG2RAD(x) ((x) * (NOVASVG_PI / 180.0f))
#define NOVASVG_RAD2DEG(x) ((x) * (180.0f / NOVASVG_PI))

/**
 * @brief A structure representing a point in 2D space.
 */
typedef struct point {
    float x; ///< The x-coordinate of the point.
    float y; ///< The y-coordinate of the point.
} point_t;

#define NOVASVG_MAKE_POINT(x, y) ((point_t){x, y})
#define NOVASVG_EMPTY_POINT NOVASVG_MAKE_POINT(0, 0)

/**
 * @brief A structure representing a rectangle in 2D space.
 */
typedef struct rect {
    float x; ///< The x-coordinate of the top-left corner of the rectangle.
    float y; ///< The y-coordinate of the top-left corner of the rectangle.
    float w; ///< The width of the rectangle.
    float h; ///< The height of the rectangle.
} rect_t;

#define NOVASVG_MAKE_RECT(x, y, w, h) ((rect_t){x, y, w, h})
#define NOVASVG_EMPTY_RECT NOVASVG_MAKE_RECT(0, 0, 0, 0)

/**
 * @brief A structure representing a 2D transformation matrix.
 */
typedef struct matrix {
    float a; ///< The horizontal scaling factor.
    float b; ///< The vertical shearing factor.
    float c; ///< The horizontal shearing factor.
    float d; ///< The vertical scaling factor.
    float e; ///< The horizontal translation offset.
    float f; ///< The vertical translation offset.
} matrix_t;

#define NOVASVG_MAKE_MATRIX(a, b, c, d, e, f) ((matrix_t){a, b, c, d, e, f})
#define NOVASVG_MAKE_SCALE(x, y) NOVASVG_MAKE_MATRIX(x, 0, 0, y, 0, 0)
#define NOVASVG_MAKE_TRANSLATE(x, y) NOVASVG_MAKE_MATRIX(1, 0, 0, 1, x, y)
#define NOVASVG_IDENTITY_MATRIX NOVASVG_MAKE_MATRIX(1, 0, 0, 1, 0, 0)

/**
 * @brief Initializes a 2D transformation matrix.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 * @param a The horizontal scaling factor.
 * @param b The vertical shearing factor.
 * @param c The horizontal shearing factor.
 * @param d The vertical scaling factor.
 * @param e The horizontal translation offset.
 * @param f The vertical translation offset.
 */
NOVASVG_API void matrix_init(matrix_t* matrix, float a, float b, float c, float d, float e, float f);

/**
 * @brief Initializes a 2D transformation matrix to the identity matrix.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 */
NOVASVG_API void matrix_init_identity(matrix_t* matrix);

/**
 * @brief Initializes a 2D transformation matrix for translation.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 * @param tx The translation offset in the x-direction.
 * @param ty The translation offset in the y-direction.
 */
NOVASVG_API void matrix_init_translate(matrix_t* matrix, float tx, float ty);

/**
 * @brief Initializes a 2D transformation matrix for scaling.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 * @param sx The scaling factor in the x-direction.
 * @param sy The scaling factor in the y-direction.
 */
NOVASVG_API void matrix_init_scale(matrix_t* matrix, float sx, float sy);

/**
 * @brief Initializes a 2D transformation matrix for rotation.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 * @param angle The rotation angle in radians.
 */
NOVASVG_API void matrix_init_rotate(matrix_t* matrix, float angle);

/**
 * @brief Initializes a 2D transformation matrix for shearing.
 * @param matrix A pointer to the `matrix_t` object to be initialized.
 * @param shx The shearing factor in the x-direction.
 * @param shy The shearing factor in the y-direction.
 */
NOVASVG_API void matrix_init_shear(matrix_t* matrix, float shx, float shy);

/**
 * @brief Adds a translation with offsets `tx` and `ty` to the matrix.
 * @param matrix A pointer to the `matrix_t` object to be modified.
 * @param tx The translation offset in the x-direction.
 * @param ty The translation offset in the y-direction.
 */
NOVASVG_API void matrix_translate(matrix_t* matrix, float tx, float ty);

/**
 * @brief Scales the matrix by factors `sx` and `sy`
 * @param matrix A pointer to the `matrix_t` object to be modified.
 * @param sx The scaling factor in the x-direction.
 * @param sy The scaling factor in the y-direction.
 */
NOVASVG_API void matrix_scale(matrix_t* matrix, float sx, float sy);

/**
 * @brief Rotates the matrix by the specified angle (in radians).
 * @param matrix A pointer to the `matrix_t` object to be modified.
 * @param angle The rotation angle in radians.
 */
NOVASVG_API void matrix_rotate(matrix_t* matrix, float angle);

/**
 * @brief Shears the matrix by factors `shx` and `shy`.
 * @param matrix A pointer to the `matrix_t` object to be modified.
 * @param shx The shearing factor in the x-direction.
 * @param shy The shearing factor in the y-direction.
 */
NOVASVG_API void matrix_shear(matrix_t* matrix, float shx, float shy);

/**
 * @brief Multiplies `left` and `right` matrices and stores the result in `matrix`.
 * @note `matrix` can be identical to either `left` or `right`.
 * @param matrix A pointer to the `matrix_t` object to store the result.
 * @param left A pointer to the first `matrix_t` matrix.
 * @param right A pointer to the second `matrix_t` matrix.
 */
NOVASVG_API void matrix_multiply(matrix_t* matrix, const matrix_t* left, const matrix_t* right);

/**
 * @brief Calculates the inverse of `matrix` and stores it in `inverse`.
 *
 * If `inverse` is `NULL`, the function only checks if the matrix is invertible.
 *
 * @note `matrix` and `inverse` can be identical.
 * @param matrix A pointer to the `matrix_t` object to invert.
 * @param inverse A pointer to the `matrix_t` object to store the result, or `NULL`.
 * @return `true` if the matrix is invertible; `false` otherwise.
 */
NOVASVG_API bool matrix_invert(const matrix_t* matrix, matrix_t* inverse);

/**
 * @brief Transforms the point `(x, y)` using `matrix` and stores the result in `(xx, yy)`.
 * @param matrix A pointer to a `matrix_t` object.
 * @param x The x-coordinate of the point to transform.
 * @param y The y-coordinate of the point to transform.
 * @param xx A pointer to store the transformed x-coordinate.
 * @param yy A pointer to store the transformed y-coordinate.
 */
NOVASVG_API void matrix_map(const matrix_t* matrix, float x, float y, float* xx, float* yy);

/**
 * @brief Transforms the `src` point using `matrix` and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param matrix A pointer to a `matrix_t` object.
 * @param src A pointer to the `point_t` object to transform.
 * @param dst A pointer to the `point_t` to store the transformed point.
 */
NOVASVG_API void matrix_map_point(const matrix_t* matrix, const point_t* src, point_t* dst);

/**
 * @brief Transforms an array of `src` points using `matrix` and stores the results in `dst`.
 * @note `src` and `dst` can be identical.
 * @param matrix A pointer to a `matrix_t` object.
 * @param src A pointer to the array of `point_t` objects to transform.
 * @param dst A pointer to the array of `point_t` to store the transformed points.
 * @param count The number of points to transform.
 */
NOVASVG_API void matrix_map_points(const matrix_t* matrix, const point_t* src, point_t* dst, int count);

/**
 * @brief Transforms the `src` rectangle using `matrix` and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param matrix A pointer to a `matrix_t` object.
 * @param src A pointer to the `rect_t` object to transform.
 * @param dst A pointer to the `rect_t` to store the transformed rectangle.
 */
NOVASVG_API void matrix_map_rect(const matrix_t* matrix, const rect_t* src, rect_t* dst);

/**
 * @brief Parses an SVG transform string into a matrix.
 * 
 * @param matrix A pointer to a `matrix_t` object to store the result.
 * @param data Input SVG transform string.
 * @param length Length of the string, or `-1` if null-terminated.
 * 
 * @return `true` on success, `false` on failure.
 */
NOVASVG_API bool matrix_parse(matrix_t* matrix, const char* data, int length);

/**
 * @brief Represents a 2D path for drawing operations.
 */
typedef struct path path_t;

/**
 * @brief Enumeration defining path commands.
 */
typedef enum path_command {
    NOVASVG_PATH_COMMAND_MOVE_TO, ///< Moves the current point to a new position.
    NOVASVG_PATH_COMMAND_LINE_TO, ///< Draws a straight line to a new point.
    NOVASVG_PATH_COMMAND_CUBIC_TO, ///< Draws a cubic Bézier curve to a new point.
    NOVASVG_PATH_COMMAND_CLOSE ///< Closes the current path by drawing a line to the starting point.
} path_command_t;

/**
 * @brief Union representing a path element.
 *
 * A path element can be a command with a length or a coordinate point.
 * Each command type in the path element array is followed by a specific number of points:
 * - `NOVASVG_PATH_COMMAND_MOVE_TO`: 1 point
 * - `NOVASVG_PATH_COMMAND_LINE_TO`: 1 point
 * - `NOVASVG_PATH_COMMAND_CUBIC_TO`: 3 points
 * - `NOVASVG_PATH_COMMAND_CLOSE`: 1 point
 *
 * @example
 * const path_element_t* elements;
 * int count = path_get_elements(path, &elements);
 * for(int i = 0; i < count; i += elements[i].header.length) {
 *     path_command_t command = elements[i].header.command;
 *     switch(command) {
 *     case NOVASVG_PATH_COMMAND_MOVE_TO:
 *         printf("MoveTo: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     case NOVASVG_PATH_COMMAND_LINE_TO:
 *         printf("LineTo: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     case NOVASVG_PATH_COMMAND_CUBIC_TO:
 *         printf("CubicTo: %g %g %g %g %g %g\n",
 *                elements[i + 1].point.x, elements[i + 1].point.y,
 *                elements[i + 2].point.x, elements[i + 2].point.y,
 *                elements[i + 3].point.x, elements[i + 3].point.y);
 *         break;
 *     case NOVASVG_PATH_COMMAND_CLOSE:
 *         printf("Close: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     }
 * }
 */
typedef union path_element {
    struct {
        path_command_t command; ///< The path command.
        int length; ///< Number of elements including the header.
    } header; ///< Header for path commands.
    point_t point; ///< A coordinate point in the path.
} path_element_t;

/**
 * @brief Iterator for traversing path elements in a path.
 */
typedef struct path_iterator {
    const path_element_t* elements; ///< Pointer to the array of path elements.
    int size; ///< Total number of elements in the array.
    int index; ///< Current position in the array.
} path_iterator_t;

/**
 * @brief Initializes a path iterator for a given path.
 *
 * @param it The path iterator to initialize.
 * @param path The path to iterate over.
 */
NOVASVG_API void path_iterator_init(path_iterator_t* it, const path_t* path);

/**
 * @brief Checks if there are more elements to iterate over.
 *
 * @param it The path iterator.
 * @return `true` if there are more elements; otherwise, `false`.
 */
NOVASVG_API bool path_iterator_has_next(const path_iterator_t* it);

/**
 * @brief Retrieves the current command and its associated points, then advances the iterator.
 *
 * @param it The path iterator.
 * @param points An array to store the points for the current command.
 * @return The path command for the current element.
 */
NOVASVG_API path_command_t path_iterator_next(path_iterator_t* it, point_t points[3]);

/**
 * @brief Creates a new path object.
 *
 * @return A pointer to the newly created path object.
 */
NOVASVG_API path_t* path_create(void);

/**
 * @brief Increases the reference count of a path object.
 *
 * @param path A pointer to a `path_t` object.
 * @return A pointer to the same `path_t` object.
 */
NOVASVG_API path_t* path_reference(path_t* path);

/**
 * @brief Decreases the reference count of a path object.
 *
 * This function decrements the reference count of the given path object. If
 * the reference count reaches zero, the path object is destroyed and its
 * resources are freed.
 *
 * @param path A pointer to the `path_t` object.
 */
NOVASVG_API void path_destroy(path_t* path);

/**
 * @brief Retrieves the reference count of a path object.
 *
 * @param path A pointer to a `path_t` object.
 * @return The current reference count of the path object.
 */
NOVASVG_API int path_get_reference_count(const path_t* path);

/**
 * @brief Retrieves the elements of a path.
 *
 * Provides access to the array of path elements.
 *
 * @param path A pointer to a `path_t` object.
 * @param elements A pointer to a pointer that will be set to the array of path elements.
 * @return The number of elements in the path.
 */
NOVASVG_API int path_get_elements(const path_t* path, const path_element_t** elements);

/**
 * @brief Moves the current point to a new position.
 *
 * This function moves the current point to the specified coordinates without
 * drawing a line. This is equivalent to the `M` command in SVG path syntax.
 *
 * @param path A pointer to a `path_t` object.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 */
NOVASVG_API void path_move_to(path_t* path, float x, float y);

/**
 * @brief Adds a straight line segment to the path.
 *
 * This function adds a straight line segment from the current point to the
 * specified coordinates. This is equivalent to the `L` command in SVG path syntax.
 *
 * @param path A pointer to a `path_t` object.
 * @param x The x-coordinate of the end point of the line segment.
 * @param y The y-coordinate of the end point of the line segment.
 */
NOVASVG_API void path_line_to(path_t* path, float x, float y);

/**
 * @brief Adds a quadratic Bézier curve to the path.
 *
 * This function adds a quadratic Bézier curve segment from the current point
 * to the specified end point, using the given control point. This is equivalent
 * to the `Q` command in SVG path syntax.
 *
 * @param path A pointer to a `path_t` object.
 * @param x1 The x-coordinate of the control point.
 * @param y1 The y-coordinate of the control point.
 * @param x2 The x-coordinate of the end point of the curve.
 * @param y2 The y-coordinate of the end point of the curve.
 */
NOVASVG_API void path_quad_to(path_t* path, float x1, float y1, float x2, float y2);

/**
 * @brief Adds a cubic Bézier curve to the path.
 *
 * This function adds a cubic Bézier curve segment from the current point
 * to the specified end point, using the given two control points. This is
 * equivalent to the `C` command in SVG path syntax.
 *
 * @param path A pointer to a `path_t` object.
 * @param x1 The x-coordinate of the first control point.
 * @param y1 The y-coordinate of the first control point.
 * @param x2 The x-coordinate of the second control point.
 * @param y2 The y-coordinate of the second control point.
 * @param x3 The x-coordinate of the end point of the curve.
 * @param y3 The y-coordinate of the end point of the curve.
 */
NOVASVG_API void path_cubic_to(path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);

/**
 * @brief Adds an elliptical arc to the path.
 *
 * This function adds an elliptical arc segment from the current point to the
 * specified end point. The arc is defined by the radii, rotation angle, and
 * flags for large arc and sweep. This is equivalent to the `A` command in SVG
 * path syntax.
 *
 * @param path A pointer to a `path_t` object.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse in radians.
 * @param large_arc_flag If true, draw the large arc; otherwise, draw the small arc.
 * @param sweep_flag If true, draw the arc in the positive-angle direction; otherwise, in the negative-angle direction.
 * @param x The x-coordinate of the end point of the arc.
 * @param y The y-coordinate of the end point of the arc.
 */
NOVASVG_API void path_arc_to(path_t* path, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

/**
 * @brief Closes the current sub-path.
 *
 * This function closes the current sub-path by drawing a straight line back to
 * the start point of the sub-path. This is equivalent to the `Z` command in SVG
 * path syntax.
 *
 * @param path A pointer to a `path_t` object.
 */
NOVASVG_API void path_close(path_t* path);

/**
 * @brief Retrieves the current point of the path.
 *
 * Gets the current point's coordinates in the path. This point is the last
 * position used or the point where the path was last moved to.
 *
 * @param path A pointer to a `path_t` object.
 * @param x The x-coordinate of the current point.
 * @param y The y-coordinate of the current point.
 */
NOVASVG_API void path_get_current_point(const path_t* path, float* x, float* y);

/**
 * @brief Reserves space for path elements.
 *
 * Reserves space for a specified number of elements in the path. This helps optimize
 * memory allocation for future path operations.
 *
 * @param path A pointer to a `path_t` object.
 * @param count The number of path elements to reserve space for.
 */
NOVASVG_API void path_reserve(path_t* path, int count);

/**
 * @brief Resets the path.
 *
 * Clears all path data, effectively resetting the `path_t` object to its initial state.
 *
 * @param path A pointer to a `path_t` object.
 */
NOVASVG_API void path_reset(path_t* path);

/**
 * @brief Adds a rectangle to the path.
 *
 * Adds a rectangle defined by the top-left corner (x, y) and dimensions (w, h) to the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
NOVASVG_API void path_add_rect(path_t* path, float x, float y, float w, float h);

/**
 * @brief Adds a rounded rectangle to the path.
 *
 * Adds a rounded rectangle defined by the top-left corner (x, y), dimensions (w, h),
 * and corner radii (rx, ry) to the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param rx The x-radius of the rectangle's corners.
 * @param ry The y-radius of the rectangle's corners.
 */
NOVASVG_API void path_add_round_rect(path_t* path, float x, float y, float w, float h, float rx, float ry);

/**
 * @brief Adds an ellipse to the path.
 *
 * Adds an ellipse defined by the center (cx, cy) and radii (rx, ry) to the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param cx The x-coordinate of the ellipse's center.
 * @param cy The y-coordinate of the ellipse's center.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 */
NOVASVG_API void path_add_ellipse(path_t* path, float cx, float cy, float rx, float ry);

/**
 * @brief Adds a circle to the path.
 *
 * Adds a circle defined by its center (cx, cy) and radius (r) to the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param r The radius of the circle.
 */
NOVASVG_API void path_add_circle(path_t* path, float cx, float cy, float r);

/**
 * @brief Adds an arc to the path.
 *
 * Adds an arc defined by the center (cx, cy), radius (r), start angle (a0), end angle (a1),
 * and direction (ccw) to the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param cx The x-coordinate of the arc's center.
 * @param cy The y-coordinate of the arc's center.
 * @param r The radius of the arc.
 * @param a0 The start angle of the arc in radians.
 * @param a1 The end angle of the arc in radians.
 * @param ccw If true, the arc is drawn counter-clockwise; if false, clockwise.
 */
NOVASVG_API void path_add_arc(path_t* path, float cx, float cy, float r, float a0, float a1, bool ccw);

/**
 * @brief Adds a sub-path to the path.
 *
 * Adds all elements from another path (`source`) to the current path, optionally
 * applying a transformation matrix.
 *
 * @param path A pointer to a `path_t` object.
 * @param source A pointer to the `path_t` object to copy elements from.
 * @param matrix A pointer to a `matrix_t` object, or `NULL` to apply no transformation.
 */
NOVASVG_API void path_add_path(path_t* path, const path_t* source, const matrix_t* matrix);

/**
 * @brief Applies a transformation matrix to the path.
 *
 * Transforms the entire path using the provided transformation matrix.
 *
 * @param path A pointer to a `path_t` object.
 * @param matrix A pointer to a `matrix_t` object.
 */
NOVASVG_API void path_transform(path_t* path, const matrix_t* matrix);

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
typedef void (*path_traverse_func_t)(void* closure, path_command_t command, const point_t* points, int npoints);

/**
 * @brief Traverses the path and calls the callback for each element.
 *
 * @param path A pointer to a `path_t` object.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
NOVASVG_API void path_traverse(const path_t* path, path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Traverses the path with Bézier curves flattened to line segments.
 *
 * @param path A pointer to a `path_t` object.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
NOVASVG_API void path_traverse_flatten(const path_t* path, path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Traverses the path with a dashed pattern and calls the callback for each segment.
 *
 * @param path A pointer to a `path_t` object.
 * @param offset The starting offset into the dash pattern.
 * @param dashes An array of dash lengths.
 * @param ndashes The number of elements in the `dashes` array.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
NOVASVG_API void path_traverse_dashed(const path_t* path, float offset, const float* dashes, int ndashes, path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Creates a copy of the path.
 *
 * @param path A pointer to the `path_t` object to clone.
 * @return A pointer to the newly created path clone.
 */
NOVASVG_API path_t* path_clone(const path_t* path);

/**
 * @brief Creates a copy of the path with Bézier curves flattened to line segments.
 *
 * @param path A pointer to the `path_t` object to clone.
 * @return A pointer to the newly created path clone with flattened curves.
 */
NOVASVG_API path_t* path_clone_flatten(const path_t* path);

/**
 * @brief Creates a copy of the path with a dashed pattern applied.
 *
 * @param path A pointer to the `path_t` object to clone.
 * @param offset The starting offset into the dash pattern.
 * @param dashes An array of dash lengths.
 * @param ndashes The number of elements in the `dashes` array.
 * @return A pointer to the newly created path clone with dashed pattern.
 */
NOVASVG_API path_t* path_clone_dashed(const path_t* path, float offset, const float* dashes, int ndashes);

/**
 * @brief Computes the bounding box and total length of the path.
 *
 * @param path A pointer to a `path_t` object.
 * @param extents A pointer to a `rect_t` object to store the bounding box.
 * @param tight If `true`, computes a precise bounding box; otherwise, aligns to control points.
 * @return The total length of the path.
 */
NOVASVG_API float path_extents(const path_t* path, rect_t* extents, bool tight);

/**
 * @brief Calculates the total length of the path.
 *
 * @param path A pointer to a `path_t` object.
 * @return The total length of the path.
 */
NOVASVG_API float path_length(const path_t* path);

/**
 * @brief Parses SVG path data into a `path_t` object.
 *
 * @param path A pointer to the `path_t` object to populate.
 * @param data The SVG path data string.
 * @param length The length of `data`, or `-1` for null-terminated data.
 * @return `true` if successful; `false` otherwise.
 */
NOVASVG_API bool path_parse(path_t* path, const char* data, int length);

/**
 * @brief Text encodings used for converting text data to code points.
 */
typedef enum text_encoding {
    NOVASVG_TEXT_ENCODING_LATIN1, ///< Latin-1 encoding
    NOVASVG_TEXT_ENCODING_UTF8, ///< UTF-8 encoding
    NOVASVG_TEXT_ENCODING_UTF16, ///< UTF-16 encoding
    NOVASVG_TEXT_ENCODING_UTF32 ///< UTF-32 encoding
} text_encoding_t;

/**
 * @brief Iterator for traversing code points in text data.
 */
typedef struct text_iterator {
    const void* text; ///< Pointer to the text data.
    int length; ///< Length of the text data.
    text_encoding_t encoding; ///< Encoding format of the text data.
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
NOVASVG_API void text_iterator_init(text_iterator_t* it, const void* text, int length, text_encoding_t encoding);

/**
 * @brief Checks if there are more code points to iterate.
 *
 * @param it Pointer to the text iterator.
 * @return `true` if more code points are available; otherwise, `false`.
 */
NOVASVG_API bool text_iterator_has_next(const text_iterator_t* it);

/**
 * @brief Retrieves the next code point and advances the iterator.
 *
 * @param it Pointer to the text iterator.
 * @return The next code point.
 */
NOVASVG_API codepoint_t text_iterator_next(text_iterator_t* it);

/**
 * @brief Represents a font face.
 */
typedef struct font_face font_face_t;

/**
 * @brief Loads a font face from a file.
 *
 * @param filename Path to the font file.
 * @param ttcindex Index of the font face within a TrueType Collection (TTC).
 * @return A pointer to the loaded `font_face_t` object, or `NULL` on failure.
 */
NOVASVG_API font_face_t* font_face_load_from_file(const char* filename, int ttcindex);

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
NOVASVG_API font_face_t* font_face_load_from_data(const void* data, unsigned int length, int ttcindex, destroy_func_t destroy_func, void* closure);

/**
 * @brief Increments the reference count of a font face.
 *
 * @param face A pointer to a `font_face_t` object.
 * @return A pointer to the same `font_face_t` object with an incremented reference count.
 */
NOVASVG_API font_face_t* font_face_reference(font_face_t* face);

/**
 * @brief Decrements the reference count and potentially destroys the font face.
 *
 * @param face A pointer to a `font_face_t` object.
 */
NOVASVG_API void font_face_destroy(font_face_t* face);

/**
 * @brief Retrieves the current reference count of a font face.
 *
 * @param face A pointer to a `font_face_t` object.
 * @return The reference count of the font face.
 */
NOVASVG_API int font_face_get_reference_count(const font_face_t* face);

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
NOVASVG_API void font_face_get_metrics(const font_face_t* face, float size, float* ascent, float* descent, float* line_gap, rect_t* extents);

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
NOVASVG_API void font_face_get_glyph_metrics(font_face_t* face, float size, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents);

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
NOVASVG_API float font_face_get_glyph_path(font_face_t* face, float size, float x, float y, codepoint_t codepoint, path_t* path);

/**
 * @brief Traverses the path of a glyph and calls a callback for each path element.
 *
 * @param face A pointer to a `font_face_t` object.
 * @param size The font size in pixels.
 * @param x The x-coordinate for positioning the glyph.
 * @param y The y-coordinate for positioning the glyph.
 * @param codepoint The Unicode code point of the glyph.
 * @param traverse_func The callback function to be called for each path element.
 * @param closure User-defined data passed to the callback function.
 * @return The advance width of the glyph.
 */
NOVASVG_API float font_face_traverse_glyph_path(font_face_t* face, float size, float x, float y, codepoint_t codepoint, path_traverse_func_t traverse_func, void* closure);

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
NOVASVG_API float font_face_text_extents(font_face_t* face, float size, const void* text, int length, text_encoding_t encoding, rect_t* extents);

/**
 * @brief Represents a cache of loaded font faces.
 */
typedef struct font_face_cache font_face_cache_t;

/**
 * @brief Create a new, empty font‐face cache.
 *
 * @return Pointer to a newly allocated `font_face_cache_t` object.
 */
NOVASVG_API font_face_cache_t* font_face_cache_create(void);

/**
 * @brief Increments the reference count of a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return A pointer to the same `font_face_cache_t` object with an incremented reference count.
 */
NOVASVG_API font_face_cache_t* font_face_cache_reference(font_face_cache_t* cache);

/**
 * @brief Decrement the reference count of a font‐face cache and destroy it when it reaches zero.
 *
 * @param cache A pointer to a `font_face_cache_t` object to release.
 */
NOVASVG_API void font_face_cache_destroy(font_face_cache_t* cache);

/**
 * @brief Retrieve the current reference count of a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return The current reference count, or 0 if cache is NULL.
 */
NOVASVG_API int font_face_cache_reference_count(const font_face_cache_t* cache);

/**
 * @brief Remove all entries from a font‐face cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object to reset.
 */
NOVASVG_API void font_face_cache_reset(font_face_cache_t* cache);

/**
 * @brief Add a font face to the cache with the specified family and style.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param family The font family name.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param face A pointer to the `font_face_t` to add. The cache increments its reference count.
 */
NOVASVG_API void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, font_face_t* face);

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
NOVASVG_API bool font_face_cache_add_file(font_face_cache_t* cache, const char* family, bool bold, bool italic, const char* filename, int ttcindex);

/**
 * @brief Retrieve a font face from the cache by family and style.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param family The font family name.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @return A pointer to the matching `font_face_t` object, or NULL if not found. The returned face is owned by the cache and must not be destroyed by the caller.
 */
NOVASVG_API font_face_t* font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic);

/**
 * @brief Load all font faces from a file and add them to the cache.
 *
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param filename Path to the font file (TrueType, OpenType, or font collection).
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVASVG_API int font_face_cache_load_file(font_face_cache_t* cache, const char* filename);

/**
 * @brief Load all font faces from files in a directory recursively and add them to the cache.
 * 
 * This scans the specified directory recursively and loads all supported font files.
 * 
 * @param cache A pointer to a `font_face_cache_t` object.
 * @param dirname Path to the directory containing font files.
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVASVG_API int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname);

/**
 * @brief Load all available system font faces and add them to the cache.
 *
 * This scans standard system font directories recursively and loads all supported font files.
 * 
 * @param cache A pointer to a `font_face_cache_t` object.
 * @return The number of faces successfully loaded, or `-1` if font face cache loading is disabled.
 */
NOVASVG_API int font_face_cache_load_sys(font_face_cache_t* cache);

/**
 * @brief Represents a color with red, green, blue, and alpha components.
 */
typedef struct color {
    float r; ///< Red component (0 to 1).
    float g; ///< Green component (0 to 1).
    float b; ///< Blue component (0 to 1).
    float a; ///< Alpha (opacity) component (0 to 1).
} color_t;

#define NOVASVG_MAKE_COLOR(r, g, b, a) ((color_t){r, g, b, a})

#define NOVASVG_BLACK_COLOR   NOVASVG_MAKE_COLOR(0, 0, 0, 1)
#define NOVASVG_WHITE_COLOR   NOVASVG_MAKE_COLOR(1, 1, 1, 1)
#define NOVASVG_RED_COLOR     NOVASVG_MAKE_COLOR(1, 0, 0, 1)
#define NOVASVG_GREEN_COLOR   NOVASVG_MAKE_COLOR(0, 1, 0, 1)
#define NOVASVG_BLUE_COLOR    NOVASVG_MAKE_COLOR(0, 0, 1, 1)
#define NOVASVG_YELLOW_COLOR  NOVASVG_MAKE_COLOR(1, 1, 0, 1)
#define NOVASVG_CYAN_COLOR    NOVASVG_MAKE_COLOR(0, 1, 1, 1)
#define NOVASVG_MAGENTA_COLOR NOVASVG_MAKE_COLOR(1, 0, 1, 1)

/**
 * @brief Initializes a color using RGB components in the 0-1 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 1).
 * @param g Green component (0 to 1).
 * @param b Blue component (0 to 1).
 */
NOVASVG_API void color_init_rgb(color_t* color, float r, float g, float b);

/**
 * @brief Initializes a color using RGBA components in the 0-1 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 1).
 * @param g Green component (0 to 1).
 * @param b Blue component (0 to 1).
 * @param a Alpha component (0 to 1).
 */
NOVASVG_API void color_init_rgba(color_t* color, float r, float g, float b, float a);

/**
 * @brief Initializes a color using RGB components in the 0-255 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 255).
 * @param g Green component (0 to 255).
 * @param b Blue component (0 to 255).
 */
NOVASVG_API void color_init_rgb8(color_t* color, int r, int g, int b);

/**
 * @brief Initializes a color using RGBA components in the 0-255 range.
 * 
 * @param color A pointer to a `color_t` object.
 * @param r Red component (0 to 255).
 * @param g Green component (0 to 255).
 * @param b Blue component (0 to 255).
 * @param a Alpha component (0 to 255).
 */
NOVASVG_API void color_init_rgba8(color_t* color, int r, int g, int b, int a);

/**
 * @brief Initializes a color from a 32-bit unsigned RGBA value.
 * 
 * @param color A pointer to a `color_t` object.
 * @param value 32-bit unsigned RGBA value.
 */
NOVASVG_API void color_init_rgba32(color_t* color, unsigned int value);

/**
 * @brief Initializes a color from a 32-bit unsigned ARGB value.
 * 
 * @param color A pointer to a `color_t` object.
 * @param value 32-bit unsigned ARGB value.
 */
NOVASVG_API void color_init_argb32(color_t* color, unsigned int value);

/**
 * @brief Initializes a color with the specified HSL color values.
 *
 * @param color A pointer to a `color_t` object.
 * @param h Hue component in degrees (0 to 360).
 * @param s Saturation component (0 to 1).
 * @param l Lightness component (0 to 1).
 */
NOVASVG_API void color_init_hsl(color_t* color, float h, float s, float l);

/**
 * @brief Initializes a color with the specified HSLA color values.
 *
 * @param color A pointer to a `color_t` object.
 * @param h Hue component in degrees (0 to 360).
 * @param s Saturation component (0 to 1).
 * @param l Lightness component (0 to 1).
 * @param a Alpha component (0 to 1).
 */
NOVASVG_API void color_init_hsla(color_t* color, float h, float s, float l, float a);

/**
 * @brief Converts a color to a 32-bit unsigned RGBA value.
 * 
 * @param color A pointer to a `color_t` object.
 * 
 * @return 32-bit unsigned RGBA value.
 */
NOVASVG_API unsigned int color_to_rgba32(const color_t* color);

/**
 * @brief Converts a color to a 32-bit unsigned ARGB value.
 * 
 * @param color A pointer to a `color_t` object.
 * 
 * @return 32-bit unsigned ARGB value.
 */
NOVASVG_API unsigned int color_to_argb32(const color_t* color);

/**
 * @brief Parses a color from a string using CSS color syntax.
 * 
 * @param color A pointer to a `color_t` object to store the parsed color.
 * @param data A pointer to the input string containing the color data.
 * @param length The length of the input string in bytes, or `-1` if the string is null-terminated.
 * 
 * @return The number of characters consumed on success (including leading/trailing spaces), or 0 on failure.
 */
NOVASVG_API int color_parse(color_t* color, const char* data, int length);

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
NOVASVG_API surface_t* surface_create(int width, int height);

/**
 * @brief Creates an image surface using existing pixel data.
 *
 * @param data Pointer to the pixel data.
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 * @param stride The number of bytes per row in the pixel data.
 * @return A pointer to the newly created `surface_t` object.
 */
NOVASVG_API surface_t* surface_create_for_data(unsigned char* data, int width, int height, int stride);

/**
 * @brief Loads an image surface from a file.
 *
 * @param filename Path to the image file.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVASVG_API surface_t* surface_load_from_image_file(const char* filename);

/**
 * @brief Loads an image surface from raw image data.
 *
 * @param data Pointer to the image data.
 * @param length Length of the data in bytes.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVASVG_API surface_t* surface_load_from_image_data(const void* data, int length);

/**
 * @brief Loads an image surface from base64-encoded data.
 *
 * @param data Pointer to the base64-encoded image data.
 * @param length Length of the data in bytes, or `-1` if null-terminated.
 * @return Pointer to the surface, or `NULL` on failure.
 */
NOVASVG_API surface_t* surface_load_from_image_base64(const char* data, int length);

/**
 * @brief Increments the reference count for a surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Pointer to the `surface_t` object.
 */
NOVASVG_API surface_t* surface_reference(surface_t* surface);

/**
 * @brief Decrements the reference count and destroys the surface if the count reaches zero.
 *
 * @param surface Pointer to the `surface_t` object .
 */
NOVASVG_API void surface_destroy(surface_t* surface);

/**
 * @brief Gets the current reference count of a surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return The reference count of the surface.
 */
NOVASVG_API int surface_get_reference_count(const surface_t* surface);

/**
 * @brief Gets the pixel data of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Pointer to the pixel data.
 */
NOVASVG_API unsigned char* surface_get_data(const surface_t* surface);

/**
 * @brief Gets the width of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Width of the surface in pixels.
 */
NOVASVG_API int surface_get_width(const surface_t* surface);

/**
 * @brief Gets the height of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Height of the surface in pixels.
 */
NOVASVG_API int surface_get_height(const surface_t* surface);

/**
 * @brief Gets the stride of the surface.
 *
 * @param surface Pointer to the `surface_t` object.
 * @return Number of bytes per row.
 */
NOVASVG_API int surface_get_stride(const surface_t* surface);

/**
 * @brief Clears the entire surface with the specified color.
 *
 * @param surface Pointer to the target surface.
 * @param color Pointer to the color used for clearing.
 */
NOVASVG_API void surface_clear(surface_t* surface, const color_t* color);

/**
 * @brief Converts pixel data from premultiplied ARGB to RGBA format.
 *
 * Transforms pixel data from native-endian 32-bit ARGB premultiplied format
 * to a non-premultiplied RGBA byte sequence.
 *
 * @param dst Pointer to the destination buffer (can overlap with `src`).
 * @param src Pointer to the source buffer in ARGB premultiplied format.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Number of bytes per image row in the buffers.
 */
NOVASVG_API void convert_argb_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Converts pixel data from RGBA to premultiplied ARGB format.
 *
 * Transforms pixel data from a non-premultiplied RGBA byte sequence
 * to a native-endian 32-bit ARGB premultiplied format.
 *
 * @param dst Pointer to the destination buffer (can overlap with `src`).
 * @param src Pointer to the source buffer in RGBA format.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Number of bytes per image row in the buffers.
 */
NOVASVG_API void convert_rgba_to_argb(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Defines the type of texture, either plain or tiled.
 */
typedef enum {
    NOVASVG_TEXTURE_TYPE_PLAIN, ///< Plain texture.
    NOVASVG_TEXTURE_TYPE_TILED ///< Tiled texture.
} texture_type_t;

/**
 * @brief Defines the spread method for gradients.
 */
typedef enum {
    NOVASVG_SPREAD_METHOD_PAD, ///< Pad the gradient's edges.
    NOVASVG_SPREAD_METHOD_REFLECT, ///< Reflect the gradient beyond its bounds.
    NOVASVG_SPREAD_METHOD_REPEAT ///< Repeat the gradient pattern.
} spread_method_t;

/**
 * @brief Represents a gradient stop.
 */
typedef struct {
    float offset; ///< The offset of the gradient stop, as a value between 0 and 1.
    color_t color; ///< The color of the gradient stop.
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
NOVASVG_API paint_t* paint_create_rgb(float r, float g, float b);

/**
 * @brief Creates a solid RGBA paint.
 *
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @param a The alpha component (0 to 1).
 * @return A pointer to the created `paint_t` object.
 */
NOVASVG_API paint_t* paint_create_rgba(float r, float g, float b, float a);

/**
 * @brief Creates a solid color paint.
 *
 * @param color A pointer to the `color_t` object.
 * @return A pointer to the created `paint_t` object.
 */
NOVASVG_API paint_t* paint_create_color(const color_t* color);

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
NOVASVG_API paint_t* paint_create_linear_gradient(float x1, float y1, float x2, float y2,
    spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

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
NOVASVG_API paint_t* paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr,
    spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

/**
 * @brief Creates a texture paint from a surface.
 *
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `paint_t` object.
 */
NOVASVG_API paint_t* paint_create_texture(surface_t* surface, texture_type_t type, float opacity, const matrix_t* matrix);

/**
 * @brief Increments the reference count of a paint object.
 *
 * @param paint A pointer to the `paint_t` object.
 * @return A pointer to the referenced `paint_t` object.
 */
NOVASVG_API paint_t* paint_reference(paint_t* paint);

/**
 * @brief Decrements the reference count and destroys the paint if the count reaches zero.
 *
 * @param paint A pointer to the `paint_t` object.
 */
NOVASVG_API void paint_destroy(paint_t* paint);

/**
 * @brief Retrieves the reference count of a paint object.
 *
 * @param paint A pointer to the `paint_t` object.
 * @return The reference count of the `paint_t` object.
 */
NOVASVG_API int paint_get_reference_count(const paint_t* paint);

/**
 * @brief Defines fill rule types for filling paths.
 */
typedef enum {
    NOVASVG_FILL_RULE_NON_ZERO, ///< Non-zero winding fill rule.
    NOVASVG_FILL_RULE_EVEN_ODD ///< Even-odd fill rule.
} fill_rule_t;

/**
 * @brief Defines compositing operations.
 */
typedef enum {
    NOVASVG_OPERATOR_CLEAR,       ///< Clears the destination (resulting in a fully transparent image).
    NOVASVG_OPERATOR_SRC,         ///< Source replaces destination.
    NOVASVG_OPERATOR_DST,         ///< Destination is kept, source is ignored.
    NOVASVG_OPERATOR_SRC_OVER,    ///< Source is composited over destination.
    NOVASVG_OPERATOR_DST_OVER,    ///< Destination is composited over source.
    NOVASVG_OPERATOR_SRC_IN,      ///< Source within destination (only the overlapping part of source is shown).
    NOVASVG_OPERATOR_DST_IN,      ///< Destination within source.
    NOVASVG_OPERATOR_SRC_OUT,     ///< Source outside destination (non-overlapping part of source is shown).
    NOVASVG_OPERATOR_DST_OUT,     ///< Destination outside source.
    NOVASVG_OPERATOR_SRC_ATOP,    ///< Source atop destination (source shown over destination but only in the destination's bounds).
    NOVASVG_OPERATOR_DST_ATOP,    ///< Destination atop source (destination shown over source but only in the source's bounds).
    NOVASVG_OPERATOR_XOR          ///< Source and destination are combined, but their overlapping regions are cleared.
} operator_t;

/**
 * @brief Defines the shape used at the ends of open subpaths.
 */
typedef enum {
    NOVASVG_LINE_CAP_BUTT, ///< Flat edge at the end of the stroke.
    NOVASVG_LINE_CAP_ROUND, ///< Rounded ends at the end of the stroke.
    NOVASVG_LINE_CAP_SQUARE ///< Square ends at the end of the stroke.
} line_cap_t;

/**
 * @brief Defines the shape used at the corners of paths.
 */
typedef enum {
    NOVASVG_LINE_JOIN_MITER, ///< Miter join with sharp corners.
    NOVASVG_LINE_JOIN_ROUND, ///< Rounded join.
    NOVASVG_LINE_JOIN_BEVEL ///< Beveled join with a flattened corner.
} line_join_t;

/**
 * @brief Represents a drawing context.
 */
typedef struct canvas canvas_t;

/**
 * @brief Creates a drawing context on a surface.
 *
 * @param surface A pointer to a `surface_t` object.
 * @return A pointer to the newly created `canvas_t` object.
 */
NOVASVG_API canvas_t* canvas_create(surface_t* surface);

/**
 * @brief Increases the reference count of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The same pointer to the `canvas_t` object.
 */
NOVASVG_API canvas_t* canvas_reference(canvas_t* canvas);

/**
 * @brief Decreases the reference count and destroys the canvas when it reaches zero.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_destroy(canvas_t* canvas);

/**
 * @brief Retrieves the reference count of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current reference count.
 */
NOVASVG_API int canvas_get_reference_count(const canvas_t* canvas);

/**
 * @brief Gets the surface associated with the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to the `surface_t` object.
 */
NOVASVG_API surface_t* canvas_get_surface(const canvas_t* canvas);

/**
 * @brief Saves the current state of the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_save(canvas_t* canvas);

/**
 * @brief Restores the canvas to the most recently saved state.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_restore(canvas_t* canvas);

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
NOVASVG_API void canvas_set_rgb(canvas_t* canvas, float r, float g, float b);

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
NOVASVG_API void canvas_set_rgba(canvas_t* canvas, float r, float g, float b, float a);

/**
 * @brief Sets the current paint to a solid color.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param color A pointer to a `color_t` object.
 */
NOVASVG_API void canvas_set_color(canvas_t* canvas, const color_t* color);

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
NOVASVG_API void canvas_set_linear_gradient(canvas_t* canvas, float x1, float y1, float x2, float y2,
    spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

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
NOVASVG_API void canvas_set_radial_gradient(canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr,
    spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix);

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
NOVASVG_API void canvas_set_texture(canvas_t* canvas, surface_t* surface, texture_type_t type, float opacity, const matrix_t* matrix);

/**
 * @brief Sets the current paint.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param paint The paint to be used for subsequent drawing operations.
 */
NOVASVG_API void canvas_set_paint(canvas_t* canvas, paint_t* paint);

/**
 * @brief Retrieves the current paint.
 *
 * If not set, the default paint is opaque black color.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param color A pointer to a `color_t` object where the current color will be stored.
 * @return The current `paint_t` used for drawing operations. If no paint is set, `NULL` is returned.
 */
NOVASVG_API paint_t* canvas_get_paint(const canvas_t* canvas, color_t* color);

/**
 * @brief Assigns a font-face cache to the canvas for font management.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cache A pointer to a `font_face_cache_t` object, or NULL to unset the current cache.
 */
NOVASVG_API void canvas_set_font_face_cache(canvas_t* canvas, font_face_cache_t* cache);

/**
 * @brief Returns the font-face cache associated with the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to the associated `font_face_cache_t` object, or NULL if none is set.
 */
NOVASVG_API font_face_cache_t* canvas_get_font_face_cache(const canvas_t* canvas);

/**
 * @brief Add a font face to the canvas using the specified family and style.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param family The font family name to associate with the face.
 * @param bold Whether the font is bold.
 * @param italic Whether the font is italic.
 * @param face A pointer to the `font_face_t` object to add.
 */
NOVASVG_API void canvas_add_font_face(canvas_t* canvas, const char* family, bool bold, bool italic, font_face_t* face);

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
NOVASVG_API bool canvas_add_font_file(canvas_t* canvas, const char* family, bool bold, bool italic, const char* filename, int ttcindex);

/**
 * @brief Selects and sets the current font face on the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param family The font family name to select.
 * @param bold Whether to match a bold variant.
 * @param italic Whether to match an italic variant.
 * @return `true` if a matching font was found and set, `false` otherwise.
 */
NOVASVG_API bool canvas_select_font_face(canvas_t* canvas, const char* family, bool bold, bool italic);

/**
 * @brief Sets the font face and size for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`, and the default face size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param face A pointer to a `font_face_t` object representing the font face to use.
 * @param size The size of the font, in pixels. This determines the height of the rendered text.
 */
NOVASVG_API void canvas_set_font(canvas_t* canvas, font_face_t* face, float size);

/**
 * @brief Sets the font face for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param face A pointer to a `font_face_t` object representing the font face to use.
 */
NOVASVG_API void canvas_set_font_face(canvas_t* canvas, font_face_t* face);

/**
 * @brief Retrieves the current font face used for text rendering on the canvas.
 *
 * If not set, the default font face is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return A pointer to a `font_face_t` object representing the current font face.
 */
NOVASVG_API font_face_t* canvas_get_font_face(const canvas_t* canvas);

/**
 * @brief Sets the font size for text rendering on the canvas.
 *
 * If not set, the default font size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param size The size of the font, in pixels. This value defines the height of the rendered text.
 */
NOVASVG_API void canvas_set_font_size(canvas_t* canvas, float size);

/**
 * @brief Retrieves the current font size used for text rendering on the canvas.
 *
 * If not set, the default font size is 12.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current font size, in pixels. This value represents the height of the rendered text.
 */
NOVASVG_API float canvas_get_font_size(const canvas_t* canvas);

/**
 * @brief Sets the fill rule.
 *
 * If not set, the default fill rule is `NOVASVG_FILL_RULE_NON_ZERO`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param winding The fill rule.
 */
NOVASVG_API void canvas_set_fill_rule(canvas_t* canvas, fill_rule_t winding);

/**
 * @brief Retrieves the current fill rule.
 *
 * If not set, the default fill rule is `NOVASVG_FILL_RULE_NON_ZERO`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current fill rule.
 */
NOVASVG_API fill_rule_t canvas_get_fill_rule(const canvas_t* canvas);

/**
 * @brief Sets the compositing operator.
 *
 * If not set, the default compositing operator is `NOVASVG_OPERATOR_SRC_OVER`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param op The compositing operator.
 */
NOVASVG_API void canvas_set_operator(canvas_t* canvas, operator_t op);

/**
 * @brief Retrieves the current compositing operator.
 *
 * If not set, the default compositing operator is `NOVASVG_OPERATOR_SRC_OVER`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current compositing operator.
 */
NOVASVG_API operator_t canvas_get_operator(const canvas_t* canvas);

/**
 * @brief Sets the global opacity.
 *
 * If not set, the default global opacity is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param opacity The opacity value (0 to 1).
 */
NOVASVG_API void canvas_set_opacity(canvas_t* canvas, float opacity);

/**
 * @brief Retrieves the current global opacity.
 *
 * If not set, the default global opacity is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current opacity value.
 */
NOVASVG_API float canvas_get_opacity(const canvas_t* canvas);

/**
 * @brief Sets the line width.
 *
 * If not set, the default line width is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_width The width of the stroke.
 */
NOVASVG_API void canvas_set_line_width(canvas_t* canvas, float line_width);

/**
 * @brief Retrieves the current line width.
 *
 * If not set, the default line width is 1.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line width.
 */
NOVASVG_API float canvas_get_line_width(const canvas_t* canvas);

/**
 * @brief Sets the line cap style.
 *
 * If not set, the default line cap is `NOVASVG_LINE_CAP_BUTT`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_cap The line cap style.
 */
NOVASVG_API void canvas_set_line_cap(canvas_t* canvas, line_cap_t line_cap);

/**
 * @brief Retrieves the current line cap style.
 *
 * If not set, the default line cap is `NOVASVG_LINE_CAP_BUTT`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line cap style.
 */
NOVASVG_API line_cap_t canvas_get_line_cap(const canvas_t* canvas);

/**
 * @brief Sets the line join style.
 *
 * If not set, the default line join is `NOVASVG_LINE_JOIN_MITER`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param line_join The line join style.
 */
NOVASVG_API void canvas_set_line_join(canvas_t* canvas, line_join_t line_join);

/**
 * @brief Retrieves the current line join style.
 *
 * If not set, the default line join is `NOVASVG_LINE_JOIN_MITER`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current line join style.
 */
NOVASVG_API line_join_t canvas_get_line_join(const canvas_t* canvas);

/**
 * @brief Sets the miter limit.
 *
 * If not set, the default miter limit is 10.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param miter_limit The miter limit value.
 */
NOVASVG_API void canvas_set_miter_limit(canvas_t* canvas, float miter_limit);

/**
 * @brief Retrieves the current miter limit.
 *
 * If not set, the default miter limit is 10.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current miter limit value.
 */
NOVASVG_API float canvas_get_miter_limit(const canvas_t* canvas);

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
NOVASVG_API void canvas_set_dash(canvas_t* canvas, float offset, const float* dashes, int ndashes);

/**
 * @brief Sets the dash offset.
 *
 * If not set, the default dash offset is 0.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param offset The dash offset.
 */
NOVASVG_API void canvas_set_dash_offset(canvas_t* canvas, float offset);

/**
 * @brief Retrieves the current dash offset.
 *
 * If not set, the default dash offset is 0.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current dash offset.
 */
NOVASVG_API float canvas_get_dash_offset(const canvas_t* canvas);

/**
 * @brief Sets the dash pattern.
 *
 * If not set, the default dash array is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param dashes Array of dash lengths.
 * @param ndashes Number of dash lengths.
 */
NOVASVG_API void canvas_set_dash_array(canvas_t* canvas, const float* dashes, int ndashes);

/**
 * @brief Retrieves the current dash pattern.
 *
 * If not set, the default dash array is `NULL`.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param dashes Pointer to store the dash array.
 * @return The number of dash lengths.
 */
NOVASVG_API int canvas_get_dash_array(const canvas_t* canvas, const float** dashes);

/**
 * @brief Translates the current transformation matrix by offsets `tx` and `ty`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param tx The translation offset in the x-direction.
 * @param ty The translation offset in the y-direction.
 */
NOVASVG_API void canvas_translate(canvas_t* canvas, float tx, float ty);

/**
 * @brief Scales the current transformation matrix by factors `sx` and `sy`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param sx The scaling factor in the x-direction.
 * @param sy The scaling factor in the y-direction.
 */
NOVASVG_API void canvas_scale(canvas_t* canvas, float sx, float sy);

/**
 * @brief Shears the current transformation matrix by factors `shx` and `shy`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param shx The shearing factor in the x-direction.
 * @param shy The shearing factor in the y-direction.
 */
NOVASVG_API void canvas_shear(canvas_t* canvas, float shx, float shy);

/**
 * @brief Rotates the current transformation matrix by the specified angle (in radians).
 * @param canvas A pointer to a `canvas_t` object.
 * @param angle The rotation angle in radians.
 */
NOVASVG_API void canvas_rotate(canvas_t* canvas, float angle);

/**
 * @brief Multiplies the current transformation matrix with the specified `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param matrix A pointer to the `matrix_t` object.
 */
NOVASVG_API void canvas_transform(canvas_t* canvas, const matrix_t* matrix);

/**
 * @brief Resets the current transformation matrix to the identity matrix.
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_reset_matrix(canvas_t* canvas);

/**
 * @brief Resets the current transformation matrix to the specified `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param matrix A pointer to the `matrix_t` object.
 */
NOVASVG_API void canvas_set_matrix(canvas_t* canvas, const matrix_t* matrix);

/**
 * @brief Stores the current transformation matrix in `matrix`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param A pointer to the `matrix_t` to store the matrix.
 */
NOVASVG_API void canvas_get_matrix(const canvas_t* canvas, matrix_t* matrix);

/**
 * @brief Transforms the point `(x, y)` using the current transformation matrix and stores the result in `(xx, yy)`.
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the point to transform.
 * @param y The y-coordinate of the point to transform.
 * @param xx A pointer to store the transformed x-coordinate.
 * @param yy A pointer to store the transformed y-coordinate.
 */
NOVASVG_API void canvas_map(const canvas_t* canvas, float x, float y, float* xx, float* yy);

/**
 * @brief Transforms the `src` point using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `canvas_t` object.
 * @param src A pointer to the `point_t` point to transform.
 * @param dst A pointer to the `point_t` to store the transformed point.
 */
NOVASVG_API void canvas_map_point(const canvas_t* canvas, const point_t* src, point_t* dst);

/**
 * @brief Transforms the `src` rectangle using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `canvas_t` object.
 * @param src A pointer to the `rect_t` rectangle to transform.
 * @param dst A pointer to the `rect_t` to store the transformed rectangle.
 */
NOVASVG_API void canvas_map_rect(const canvas_t* canvas, const rect_t* src, rect_t* dst);

/**
 * @brief Moves the current point to a new position.
 *
 * Moves the current point to the specified coordinates without adding a line.
 * This operation is added to the current path. Equivalent to the SVG `M` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 */
NOVASVG_API void canvas_move_to(canvas_t* canvas, float x, float y);

/**
 * @brief Adds a straight line segment to the current path.
 *
 * Adds a straight line from the current point to the specified coordinates.
 * This segment is added to the current path. Equivalent to the SVG `L` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x The x-coordinate of the end point of the line.
 * @param y The y-coordinate of the end point of the line.
 */
NOVASVG_API void canvas_line_to(canvas_t* canvas, float x, float y);

/**
 * @brief Adds a quadratic Bézier curve to the current path.
 *
 * Adds a quadratic Bézier curve from the current point to the specified end point,
 * using the given control point. This curve is added to the current path. Equivalent to the SVG `Q` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x1 The x-coordinate of the control point.
 * @param y1 The y-coordinate of the control point.
 * @param x2 The x-coordinate of the end point of the curve.
 * @param y2 The y-coordinate of the end point of the curve.
 */
NOVASVG_API void canvas_quad_to(canvas_t* canvas, float x1, float y1, float x2, float y2);

/**
 * @brief Adds a cubic Bézier curve to the current path.
 *
 * Adds a cubic Bézier curve from the current point to the specified end point,
 * using the given control points. This curve is added to the current path. Equivalent to the SVG `C` command.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param x1 The x-coordinate of the first control point.
 * @param y1 The y-coordinate of the first control point.
 * @param x2 The x-coordinate of the second control point.
 * @param y2 The y-coordinate of the second control point.
 * @param x3 The x-coordinate of the end point of the curve.
 * @param y3 The y-coordinate of the end point of the curve.
 */
NOVASVG_API void canvas_cubic_to(canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3);

/**
 * @brief Adds an elliptical arc to the current path.
 *
 * Adds an elliptical arc from the current point to the specified end point,
 * defined by radii, rotation angle, and flags for arc type and direction.
 * This arc segment is added to the current path. Equivalent to the SVG `A` command.
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
NOVASVG_API void canvas_arc_to(canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

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
NOVASVG_API void canvas_rect(canvas_t* canvas, float x, float y, float w, float h);

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
NOVASVG_API void canvas_round_rect(canvas_t* canvas, float x, float y, float w, float h, float rx, float ry);

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
NOVASVG_API void canvas_ellipse(canvas_t* canvas, float cx, float cy, float rx, float ry);

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
NOVASVG_API void canvas_circle(canvas_t* canvas, float cx, float cy, float r);

/**
 * @brief Adds an arc to the current path.
 *
 * Adds an arc centered at the specified coordinates, with a given radius,
 * starting and ending at the specified angles. The direction of the arc is
 * determined by `ccw`. This arc segment is added to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param cx The x-coordinate of the arc's center.
 * @param cy The y-coordinate of the arc's center.
 * @param r The radius of the arc.
 * @param a0 The starting angle of the arc in radians.
 * @param a1 The ending angle of the arc in radians.
 * @param ccw If true, add the arc counter-clockwise; otherwise, clockwise.
 */
NOVASVG_API void canvas_arc(canvas_t* canvas, float cx, float cy, float r, float a0, float a1, bool ccw);

/**
 * @brief Adds a path to the current path.
 *
 * Appends the elements of the specified path to the current path.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param path A pointer to the `path_t` object to be added.
 */
NOVASVG_API void canvas_add_path(canvas_t* canvas, const path_t* path);

/**
 * @brief Starts a new path on the canvas.
 *
 * Begins a new path, clearing any existing path data. The new path starts with no commands.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_new_path(canvas_t* canvas);

/**
 * @brief Closes the current path.
 *
 * Closes the current path by adding a straight line back to the starting point.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_close_path(canvas_t* canvas);

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
NOVASVG_API void canvas_get_current_point(const canvas_t* canvas, float* x, float* y);

/**
 * @brief Gets the current path from the canvas.
 *
 * Retrieves the path object representing the sequence of path commands added to the canvas.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @return The current path.
 */
NOVASVG_API path_t* canvas_get_path(const canvas_t* canvas);

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
NOVASVG_API bool canvas_fill_contains(canvas_t* canvas, float x, float y);

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
NOVASVG_API bool canvas_stroke_contains(canvas_t* canvas, float x, float y);

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
NOVASVG_API bool canvas_clip_contains(canvas_t* canvas, float x, float y);

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
NOVASVG_API void canvas_fill_extents(canvas_t* canvas, rect_t* extents);

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
NOVASVG_API void canvas_stroke_extents(canvas_t* canvas, rect_t* extents);

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
NOVASVG_API void canvas_clip_extents(canvas_t* canvas, rect_t* extents);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_fill(canvas_t* canvas);

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_stroke(canvas_t* canvas);

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_clip(canvas_t* canvas);

/**
 * @brief A drawing operator that paints the current clipping region using the current paint.
 *
 * @note The current path will not be affected by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_paint(canvas_t* canvas);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_fill_preserve(canvas_t* canvas);

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_stroke_preserve(canvas_t* canvas);

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `canvas_t` object.
 */
NOVASVG_API void canvas_clip_preserve(canvas_t* canvas);

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
NOVASVG_API void canvas_fill_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Fills a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVASVG_API void canvas_fill_path(canvas_t* canvas, const path_t* path);

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
NOVASVG_API void canvas_stroke_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Strokes a path with the current stroke settings.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVASVG_API void canvas_stroke_path(canvas_t* canvas, const path_t* path);

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
NOVASVG_API void canvas_clip_rect(canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Intersects the current clipping region with a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `canvas_t` object.
 * @param path The `path_t` object.
 */
NOVASVG_API void canvas_clip_path(canvas_t* canvas, const path_t* path);

/**
 * @brief Adds a glyph to the current path at the specified origin.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The advance width of the glyph.
 */
NOVASVG_API float canvas_add_glyph(canvas_t* canvas, codepoint_t codepoint, float x, float y);

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
NOVASVG_API float canvas_add_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

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
NOVASVG_API float canvas_fill_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

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
NOVASVG_API float canvas_stroke_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

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
NOVASVG_API float canvas_clip_text(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, float x, float y);

/**
 * @brief Retrieves font metrics for the current font.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param ascent The ascent of the font.
 * @param descent The descent of the font.
 * @param line_gap The line gap of the font.
 * @param extents The bounding box of the font.
 */
NOVASVG_API void canvas_font_metrics(const canvas_t* canvas, float* ascent, float* descent, float* line_gap, rect_t* extents);

/**
 * @brief Retrieves metrics for a specific glyph.
 *
 * @param canvas A pointer to a `canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param advance_width The advance width of the glyph.
 * @param left_side_bearing The left side bearing of the glyph.
 * @param extents The bounding box of the glyph.
 */
NOVASVG_API void canvas_glyph_metrics(canvas_t* canvas, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents);

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
NOVASVG_API float canvas_text_extents(canvas_t* canvas, const void* text, int length, text_encoding_t encoding, rect_t* extents);

} // namespace render
} // namespace novasvg

#endif // NOVASVG_RENDER_H
