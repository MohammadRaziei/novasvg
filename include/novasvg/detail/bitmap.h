#pragma once

// Bitmap: a premultiplied-ARGB32 pixel buffer -- novasvg's drawable
// surface, texture source, and render target all in one. Self-contained:
// needs only Color and the low-level render::surface_t it wraps, nothing
// from the SVG/Canvas/Font layers, so it can be used entirely on its own
// (e.g. to build up an image and write it out) without pulling in the
// rest of novasvg.

#include "color.h"
#include "render/blend.h"   // memfill32, used by surface_clear()
#include "render/paint.h"   // color_to_argb32 / color_init_rgba32, used by surface_clear()
#include "render/surface.h"

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
using namespace render;

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
     * @brief Writes the bitmap to a file, picking PNG/BMP/TGA/JPEG from
     * `filename`'s extension (.png/.bmp/.tga/.jpg/.jpeg; PNG if unrecognized
     * or missing). A convenience over calling writeToPng()/writeToBmp()/etc
     * yourself when the format doesn't need to be pinned down explicitly.
     * @param filename The name of the file to write.
     * @param jpgQuality JPEG quality (1-100), used only when the extension picks JPEG.
     * @return True if the file was written successfully, false otherwise.
     */
    bool write(const std::string& filename, int jpgQuality = 80) const;

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

// The bitmap's pixels as a straight-alpha RGBA stb::image, which is what every encoder wants.
//
// Bitmap itself stays ARGB32-Premultiplied always -- it doubles as a drawable surface (Canvas can
// render onto an existing Bitmap) and as a texture source (canvas_set_texture reads bitmap.surface()
// directly for <image>/pattern fills), both of which need premultiplied alpha for `over`-compositing
// to be correct. Only encoding needs straight alpha, so only encoding pays for this one unpremultiply
// pass. The image owns the buffer it is made from (wrap + keep_alive): there is no second copy.
stb::image bitmap_to_rgba_image(const Bitmap& bitmap)
{
    if(bitmap.isNull())
        return stb::image();
    const int width = bitmap.width();
    const int height = bitmap.height();
    const size_t rowBytes = size_t(width) * 4;
    std::shared_ptr<uint8_t> pixels(static_cast<uint8_t*>(malloc(rowBytes * size_t(height))), free);
    if(!pixels)
        return stb::image();
    // Row by row: a Bitmap over caller-owned memory may have a stride wider than width * 4, an image never does.
    for(int y = 0; y < height; ++y)
        convert_argb_to_rgba(pixels.get() + rowBytes * size_t(y), bitmap.data() + size_t(bitmap.stride()) * size_t(y),
                             width, 1, int(rowBytes));
    uint8_t* data = pixels.get();
    return stb::image::wrap(width, height, stb::pixel_format::rgba, data, std::move(pixels));
}

// Hands encoded bytes to a write callback, in pieces small enough for the callback's int size.
bool emit_encoded(const std::vector<uint8_t>& bytes, novasvg_write_func_t callback, void* closure)
{
    if(bytes.empty() || callback == nullptr)
        return false;
    size_t offset = 0;
    while(offset < bytes.size()) {
        const size_t piece = std::min(bytes.size() - offset, size_t(INT_MAX));
        callback(closure, const_cast<uint8_t*>(bytes.data()) + offset, int(piece));
        offset += piece;
    }
    return true;
}

// libstb reports a failed encode or write by exception; Bitmap's writeTo*() report it by returning false.
template<typename Encode>
bool encode_bitmap(const Bitmap& bitmap, Encode&& encode)
{
    try {
        const stb::image image = bitmap_to_rgba_image(bitmap);
        return !image.empty() && encode(image);
    } catch(const std::exception&) {
        return false;
    }
}

} // namespace

NOVASVG_INLINE bool Bitmap::writeToPng(const std::string& filename) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { image.write_png(filename); return true; });
}

NOVASVG_INLINE bool Bitmap::writeToPng(novasvg_write_func_t callback, void* closure) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { return emit_encoded(image.to_png(), callback, closure); });
}

NOVASVG_INLINE bool Bitmap::writeToBmp(const std::string& filename) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { image.write_bmp(filename); return true; });
}

NOVASVG_INLINE bool Bitmap::writeToBmp(novasvg_write_func_t callback, void* closure) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { return emit_encoded(image.to_bmp(), callback, closure); });
}

NOVASVG_INLINE bool Bitmap::writeToTga(const std::string& filename) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { image.write_tga(filename); return true; });
}

NOVASVG_INLINE bool Bitmap::writeToTga(novasvg_write_func_t callback, void* closure) const
{
    return encode_bitmap(*this, [&](const stb::image& image) { return emit_encoded(image.to_tga(), callback, closure); });
}

// Out-of-range qualities are clamped to 1..100, as the encoder this replaced did; libstb itself would throw.
NOVASVG_INLINE bool Bitmap::writeToJpg(const std::string& filename, int quality) const
{
    const int q = std::clamp(quality, 1, 100);
    return encode_bitmap(*this, [&](const stb::image& image) { image.write_jpg(filename, q); return true; });
}

NOVASVG_INLINE bool Bitmap::writeToJpg(novasvg_write_func_t callback, void* closure, int quality) const
{
    const int q = std::clamp(quality, 1, 100);
    return encode_bitmap(*this, [&](const stb::image& image) { return emit_encoded(image.to_jpg(q), callback, closure); });
}

NOVASVG_INLINE bool Bitmap::write(const std::string& filename, int jpgQuality) const
{
    // write() *is* the auto-detecting entry point -- it just delegates to
    // whichever writeTo*() the extension picks, rather than duplicating
    // their bodies.
    auto dot = filename.find_last_of('.');
    std::string ext = dot == std::string::npos ? std::string() : filename.substr(dot + 1);
    for(auto& c : ext)
        c = char(std::tolower(static_cast<unsigned char>(c)));

    if(ext == "bmp")
        return writeToBmp(filename);
    if(ext == "tga")
        return writeToTga(filename);
    if(ext == "jpg" || ext == "jpeg")
        return writeToJpg(filename, jpgQuality);
    return writeToPng(filename);
}

NOVASVG_INLINE surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
}


} // namespace novasvg
