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

void pixmap::clear(const color& value)
{
    if(m_surface)
        m_surface->fill(value);
}

void pixmap::convert_to_rgba()
{
    if(!m_surface)
        return;
    auto pixels = data();
    auto row_bytes = size_t(stride());
    stb::premultiplied_argb_to_rgba(pixels, row_bytes, pixels, row_bytes, width(), height());
}

image bitmap_to_rgba_image(const pixmap& bitmap)
{
    if(bitmap.is_null())
        return image();
    return bitmap.surface()->to_image();
}

pixmap pixmap_from_image(const image& image)
{
    if(image.empty() || image.width() >= max_side || image.height() >= max_side)
        return pixmap();
    return pixmap(std::make_shared<surface_t>(surface_t::from_image(image)));
}

image pixmap_release_to_image(pixmap&& bitmap)
{
    if(bitmap.is_null())
        return image();
    // The surface is converted in place and handed over when nobody else holds it (libstb decides whether it owns
    // its pixels); a shared one, or somebody else's buffer, is copied.
    std::shared_ptr<surface_t> surface = bitmap.surface();
    bitmap = pixmap();
    return surface.use_count() == 1 ? std::move(*surface).into_image() : surface->to_image();
}

} // namespace nova::vg
