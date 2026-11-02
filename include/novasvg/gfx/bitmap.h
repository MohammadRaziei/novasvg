#pragma once

// Bitmap (INTERNAL; the public pixel type is stb::image): a premultiplied-ARGB32 pixel buffer -- novasvg's
// drawable surface, texture source, and render target all in one. Self-contained:
// needs only Color and the low-level gfx::surface_t it wraps, nothing
// from the svg/Canvas/Font layers, so it can be used entirely on its own
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

namespace novasvg {
using namespace gfx;

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
    auto stride = size_t(surface_get_stride(m_surface));
    stb::premultiplied_argb_to_rgba(data, stride, data, stride, surface_get_width(m_surface), surface_get_height(m_surface));
}

NOVASVG_INLINE Bitmap& Bitmap::operator=(Bitmap&& bitmap)
{
    Bitmap(std::move(bitmap)).swap(*this);
    return *this;
}

// Bitmap is internal. Its pixels are premultiplied ARGB32 because that is what the rasterizer draws in and
// what canvas_set_texture samples (<image>, patterns): both need premultiplied alpha for `over` to be correct.
// The public pixel type is stb::image (straight RGBA); these two functions are the boundary.

// A straight-alpha RGBA copy of `bitmap` (one conversion pass, out of place). Works for any stride: a Bitmap
// over caller-owned memory may be wider than width * 4, an image never is.
NOVASVG_INLINE stb::image bitmap_to_rgba_image(const Bitmap& bitmap)
{
    if(bitmap.is_null())
        return stb::image();
    const int width = bitmap.width();
    const int height = bitmap.height();
    const size_t rowBytes = size_t(width) * 4;
    std::shared_ptr<uint8_t> pixels(static_cast<uint8_t*>(malloc(rowBytes * size_t(height))), free);
    if(!pixels)
        return stb::image();
    stb::premultiplied_argb_to_rgba(pixels.get(), rowBytes, bitmap.data(), size_t(bitmap.stride()), width, height);
    uint8_t* data = pixels.get();
    return stb::image::wrap(width, height, stb::pixel_format::rgba, data, std::move(pixels));
}

// The same image without a copy: the bitmap's own buffer is converted in place and the image keeps the
// bitmap alive. This is what render() hands to callers. A bitmap whose rows are padded (stride > width * 4)
// cannot be wrapped (stb::image has no stride), so that case takes the copying path above.
NOVASVG_INLINE stb::image bitmap_release_to_image(Bitmap&& bitmap)
{
    if(bitmap.is_null())
        return stb::image();
    if(size_t(bitmap.stride()) != size_t(bitmap.width()) * 4)
        return bitmap_to_rgba_image(bitmap);
    bitmap.convertToRGBA();
    const int width = bitmap.width();
    const int height = bitmap.height();
    auto keep = std::make_shared<Bitmap>(std::move(bitmap));
    uint8_t* data = keep->data();
    return stb::image::wrap(width, height, stb::pixel_format::rgba, data, std::move(keep));
}

NOVASVG_INLINE surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
}


} // namespace novasvg
