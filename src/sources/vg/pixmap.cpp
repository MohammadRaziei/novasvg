// Definitions declared in src/sources/vg/pixmap.h.
#include "pixmap.h"

namespace nova::vg {

pixmap::pixmap(int width, int height)
    : m_surface(surface_create(width, height))
{
}

pixmap::pixmap(uint8_t* data, int width, int height, int stride)
    : m_surface(surface_create_for_data(data, width, height, stride))
{
}

pixmap::pixmap(const pixmap& bitmap)
    : m_surface(surface_reference(bitmap.surface()))
{
}

pixmap::pixmap(pixmap&& bitmap)
    : m_surface(bitmap.release())
{
}

pixmap::~pixmap()
{
    surface_destroy(m_surface);
}

pixmap& pixmap::operator=(const pixmap& bitmap)
{
    pixmap(bitmap).swap(*this);
    return *this;
}

void pixmap::swap(pixmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

uint8_t* pixmap::data() const
{
    if(m_surface)
        return surface_get_data(m_surface);
    return nullptr;
}

int pixmap::width() const
{
    if(m_surface)
        return surface_get_width(m_surface);
    return 0;
}

int pixmap::height() const
{
    if(m_surface)
        return surface_get_height(m_surface);
    return 0;
}

int pixmap::stride() const
{
    if(m_surface)
        return surface_get_stride(m_surface);
    return 0;
}

void pixmap::clear(const stb::color& value)
{
    if(m_surface == nullptr)
        return;
    color_t color_;
    color_init_rgba32(&color_, value.value());
    surface_clear(m_surface, &color_);
}

void pixmap::convert_to_rgba()
{
    if(m_surface == nullptr)
        return;
    auto data = surface_get_data(m_surface);
    auto stride = size_t(surface_get_stride(m_surface));
    stb::premultiplied_argb_to_rgba(data, stride, data, stride, surface_get_width(m_surface), surface_get_height(m_surface));
}

pixmap& pixmap::operator=(pixmap&& bitmap)
{
    pixmap(std::move(bitmap)).swap(*this);
    return *this;
}

stb::image bitmap_to_rgba_image(const pixmap& bitmap)
{
    if(bitmap.is_null())
        return stb::image();
    const int width = bitmap.width();
    const int height = bitmap.height();
    const size_t row_bytes = size_t(width) * 4;
    std::shared_ptr<uint8_t> pixels(static_cast<uint8_t*>(malloc(row_bytes * size_t(height))), free);
    if(!pixels)
        return stb::image();
    stb::premultiplied_argb_to_rgba(pixels.get(), row_bytes, bitmap.data(), size_t(bitmap.stride()), width, height);
    uint8_t* data = pixels.get();
    return stb::image::wrap(width, height, stb::pixel_format::rgba, data, std::move(pixels));
}

stb::image pixmap_release_to_image(pixmap&& bitmap)
{
    if(bitmap.is_null())
        return stb::image();
    if(size_t(bitmap.stride()) != size_t(bitmap.width()) * 4)
        return bitmap_to_rgba_image(bitmap);
    bitmap.convert_to_rgba();
    const int width = bitmap.width();
    const int height = bitmap.height();
    auto keep = std::make_shared<pixmap>(std::move(bitmap));
    uint8_t* data = keep->data();
    return stb::image::wrap(width, height, stb::pixel_format::rgba, data, std::move(keep));
}

surface_t* pixmap::release()
{
    return std::exchange(m_surface, nullptr);
}

} // namespace nova::vg
