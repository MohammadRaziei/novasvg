#pragma once

#include "internal.h"
#include "utils.h"

// Image decoding is libstb's (stb::image); this layer only rasterizes and turns decoded pixels into a
// premultiplied surface. Writing images is Bitmap's job (it asks libstb too).
#include <stb/image.hpp>

// The standard headers the old inlined stb_image used to pull in for the rest of this file.
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

namespace nova {
namespace vg {

static surface_t* surface_create_uninitialized(int width, int height)
{
    static const int kMaxSize = 1 << 15;
    if(width <= 0 || height <= 0 || width >= kMaxSize || height >= kMaxSize)
        return NULL;
    const size_t size = width * height * 4;
    surface_t* surface = static_cast<surface_t*>(malloc(size + sizeof(surface_t)));
    if(surface == NULL)
        return NULL;
    novasvg_init_reference(surface);
    surface->width = width;
    surface->height = height;
    surface->stride = width * 4;
    surface->data = (uint8_t*)(surface + 1);
    return surface;
}

// libstb decodes to straight-alpha RGBA; the surface wants premultiplied ARGB32, so this is the one
// conversion pass between the two. Any decode problem (unknown format, corrupt data, too large) is
// reported by libstb as an exception and by this layer, as before, as a null surface.
static surface_t* surface_load_from_image(const stb::image& image)
{
    surface_t* surface = surface_create_uninitialized(image.width(), image.height());
    if(surface)
        stb::rgba_to_premultiplied_argb(surface->data, size_t(surface->stride), image.data(), image.stride(),
                                        surface->width, surface->height);
    return surface;
}

static stb::load_options surface_load_options()
{
    stb::load_options options;
    options.format = stb::pixel_format::rgba;
    return options;
}

static const uint8_t base64_table[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Decodes a base64 payload into a malloc'd buffer (caller must free()).
// Extracted so both image data URIs (below) and @font-face embedded font
// data URIs (svgparser.hpp) can share one implementation. Returns NULL
// on any malformed input (bad character, wrong padding, ...).

} // namespace vg
} // namespace nova
