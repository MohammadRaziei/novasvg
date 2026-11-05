// Definitions declared in include/nova/vg/bitmap.h.
#include "nova/vg/bitmap.h"

namespace nova::vg {

Bitmap::Bitmap(int width, int height)
    : m_surface(surface_create(width, height))
{
}

Bitmap::Bitmap(uint8_t* data, int width, int height, int stride)
    : m_surface(surface_create_for_data(data, width, height, stride))
{
}

Bitmap::Bitmap(const Bitmap& bitmap)
    : m_surface(surface_reference(bitmap.surface()))
{
}

Bitmap::Bitmap(Bitmap&& bitmap)
    : m_surface(bitmap.release())
{
}

Bitmap::~Bitmap()
{
    surface_destroy(m_surface);
}

Bitmap& Bitmap::operator=(const Bitmap& bitmap)
{
    Bitmap(bitmap).swap(*this);
    return *this;
}

void Bitmap::swap(Bitmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

uint8_t* Bitmap::data() const
{
    if(m_surface)
        return surface_get_data(m_surface);
    return nullptr;
}

int Bitmap::width() const
{
    if(m_surface)
        return surface_get_width(m_surface);
    return 0;
}

int Bitmap::height() const
{
    if(m_surface)
        return surface_get_height(m_surface);
    return 0;
}

int Bitmap::stride() const
{
    if(m_surface)
        return surface_get_stride(m_surface);
    return 0;
}

void Bitmap::clear(const stb::color& value)
{
    if(m_surface == nullptr)
        return;
    color_t color;
    color_init_rgba32(&color, value.value());
    surface_clear(m_surface, &color);
}

void Bitmap::convertToRGBA()
{
    if(m_surface == nullptr)
        return;
    auto data = surface_get_data(m_surface);
    auto stride = size_t(surface_get_stride(m_surface));
    stb::premultiplied_argb_to_rgba(data, stride, data, stride, surface_get_width(m_surface), surface_get_height(m_surface));
}

Bitmap& Bitmap::operator=(Bitmap&& bitmap)
{
    Bitmap(std::move(bitmap)).swap(*this);
    return *this;
}

stb::image bitmap_to_rgba_image(const Bitmap& bitmap)
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

stb::image bitmap_release_to_image(Bitmap&& bitmap)
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

surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
}

} // namespace nova::vg
