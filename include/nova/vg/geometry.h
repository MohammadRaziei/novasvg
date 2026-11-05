#pragma once

// nova::vg geometry that the svg layer and the engine share: Box, and the forward declarations of the engine's
// own Rect / Transform / surface / matrix. Nothing here knows SVG.

#include "../config.h"
#include "color.h"

namespace nova {
namespace vg {
struct surface;
struct matrix;
using surface_t = surface;
using matrix_t = matrix;
} // namespace vg
} // namespace nova

#include "bitmap.h"

#include <stb/outline.hpp>  // stb::matrix

namespace nova::vg {

class Rect;

/**
 * @brief Represents a 2D axis-aligned bounding box.
 */
class NOVA_API Box {
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
    Box& transform(const stb::matrix& matrix);

    /**
     * @brief Returns a new box transformed by the specified matrix.
     * @param matrix The transformation matrix.
     * @return A new box, transformed by the matrix.
     */
    Box transformed(const stb::matrix& matrix) const;

    float x{0}; ///< The x-coordinate of the box's origin.
    float y{0}; ///< The y-coordinate of the box's origin.
    float w{0}; ///< The width of the box.
    float h{0}; ///< The height of the box.
};

class Transform;

} // namespace nova::vg
