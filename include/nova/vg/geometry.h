#pragma once

// Box: the axis-aligned bounding box the svg API returns. Nothing here knows SVG.

#include "../config.h"



#include <stb/outline.hpp>  // stb::matrix

namespace nova::vg {

/**
 * @brief Represents a 2D axis-aligned bounding box.
 */
class NOVA_API box {
public:
    /**
     * @brief Constructs a box with zero dimensions.
     */
    box() = default;

    /**
     * @brief Constructs a box with the specified position and size.
     * @param x The x-coordinate of the box's origin.
     * @param y The y-coordinate of the box's origin.
     * @param w The width of the box.
     * @param h The height of the box.
     */
    box(float x, float y, float w, float h);

    /**
     * @brief Transforms the box using the specified matrix.
     * @param matrix The transformation matrix.
     * @return A reference to this box, modified by the transformation.
     */
    box& transform(const stb::matrix& matrix);

    /**
     * @brief Returns a new box transformed by the specified matrix.
     * @param matrix The transformation matrix.
     * @return A new box, transformed by the matrix.
     */
    box transformed(const stb::matrix& matrix) const;

    float x{0}; ///< The x-coordinate of the box's origin.
    float y{0}; ///< The y-coordinate of the box's origin.
    float w{0}; ///< The width of the box.
    float h{0}; ///< The height of the box.
};


} // namespace nova::vg
