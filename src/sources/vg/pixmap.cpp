// Definitions declared in src/sources/vg/pixmap.h.
#include "pixmap.h"

namespace nova::vg {

// The longest side a pixmap can have (the python binding checks the same limit).
static constexpr int max_side = 1 << 15;

pixmap::pixmap(int width, int height)
{
    if(width <= 0 || height <= 0 || width >= max_side || height >= max_side)
        return;
    try {
        m_surface = std::make_shared<surface_t>(width, height);
    } catch(const std::exception&) {
    }
}

pixmap::pixmap(uint8_t* data, int width, int height, int stride)
{
    try {
        m_surface = std::make_shared<surface_t>(surface_t::wrap(width, height, data, size_t(stride)));
    } catch(const std::exception&) {
    }
}

void pixmap::swap(pixmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

uint8_t* pixmap::data() const
{
    return m_surface ? m_surface->data() : nullptr;
}

int pixmap::width() const
{
    return m_surface ? m_surface->width() : 0;
}

int pixmap::height() const
{
    return m_surface ? m_surface->height() : 0;
}

int pixmap::stride() const
{
    return m_surface ? int(m_surface->stride()) : 0;
}

void pixmap::clear(const stb::color& value)
{
    if(!m_surface)
        return;
    // One pixel through libstb's straight-RGBA -> premultiplied-ARGB32 conversion (c * a / 255, packed 0xAARRGGBB).
    const uint8_t rgba[4] = {value.r(), value.g(), value.b(), value.a()};
    uint32_t pixel = 0;
    stb::rgba_to_premultiplied_argb(reinterpret_cast<uint8_t*>(&pixel), sizeof(pixel), rgba, sizeof(rgba), 1, 1);
    for(int y = 0; y < height(); y++) {
        uint32_t* row = reinterpret_cast<uint32_t*>(data() + size_t(stride()) * size_t(y));
        std::fill_n(row, width(), pixel);
    }
}

void pixmap::convert_to_rgba()
{
    if(!m_surface)
        return;
    auto pixels = data();
    auto row_bytes = size_t(stride());
    stb::premultiplied_argb_to_rgba(pixels, row_bytes, pixels, row_bytes, width(), height());
}

stb::image bitmap_to_rgba_image(const pixmap& bitmap)
{
    if(bitmap.is_null())
        return stb::image();
    return bitmap.surface()->to_image();
}

pixmap pixmap_from_image(const stb::image& image)
{
    if(image.empty() || image.width() >= max_side || image.height() >= max_side)
        return pixmap();
    return pixmap(std::make_shared<surface_t>(surface_t::from_image(image)));
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

} // namespace nova::vg
