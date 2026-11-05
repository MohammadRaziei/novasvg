#pragma once

// Bitmap (INTERNAL; the public pixel type is stb::image): a premultiplied-ARGB32 pixel buffer -- novasvg's
// drawable surface, texture source, and render target all in one. Self-contained:
// needs only stb::color and the low-level vg::surface_t it wraps, nothing
// from the svg/raster_canvas/Font layers, so it can be used entirely on its own
// (e.g. to build up an image and write it out) without pulling in the
// rest of novasvg.

#include "color.h"
#include "render.h"  // memfill32, color_to_argb32, color_init_rgba32 (surface_clear()), surface_t

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <climits>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <stb/image.hpp>

namespace nova::vg {

class NOVA_API Bitmap {
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
    void clear(const stb::color& value);

    /**
     * @brief Converts the bitmap pixel data from ARGB32 Premultiplied to RGBA Plain format in place.
     */
    void convertToRGBA();

    /**
     * @brief Checks if the bitmap is null.
     * @return True if the bitmap is null, false otherwise.
     */
    bool is_null() const { return m_surface == nullptr; }

    /**
     * @brief Checks if the bitmap is valid.
     * @deprecated This function has been deprecated. Use `is_null()` instead to check whether the bitmap is null.
     * @return True if the bitmap is valid, false otherwise.
     */
    bool valid() const { return !is_null(); }

    /**
     * @internal
     */
    surface_t* surface() const { return m_surface; }

private:
    surface_t* release();
    surface_t* m_surface{nullptr};
};















// Bitmap is internal. Its pixels are premultiplied ARGB32 because that is what the rasterizer draws in and
// what canvas_set_texture samples (<image>, patterns): both need premultiplied alpha for `over` to be correct.
// The public pixel type is stb::image (straight RGBA); these two functions are the boundary.

// A straight-alpha RGBA copy of `bitmap` (one conversion pass, out of place). Works for any stride: a Bitmap
// over caller-owned memory may be wider than width * 4, an image never is.

// The same image without a copy: the bitmap's own buffer is converted in place and the image keeps the
// bitmap alive. This is what render() hands to callers. A bitmap whose rows are padded (stride > width * 4)
// cannot be wrapped (stb::image has no stride), so that case takes the copying path above.
stb::image bitmap_to_rgba_image(const Bitmap& bitmap);
stb::image bitmap_release_to_image(Bitmap&& bitmap);

} // namespace nova::vg
