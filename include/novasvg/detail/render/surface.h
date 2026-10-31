#pragma once

#include "private.h"
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

namespace novasvg {
namespace render {

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

NOVASVG_INLINE surface_t* surface_create(int width, int height)
{
    surface_t* surface = surface_create_uninitialized(width, height);
    if(surface)
        memset(surface->data, 0, surface->height * surface->stride);
    return surface;
}

NOVASVG_INLINE surface_t* surface_create_for_data(unsigned char* data, int width, int height, int stride)
{
    surface_t* surface = static_cast<surface_t*>(malloc(sizeof(surface_t)));
    novasvg_init_reference(surface);
    surface->width = width;
    surface->height = height;
    surface->stride = stride;
    surface->data = data;
    return surface;
}

// libstb decodes to straight-alpha RGBA; the surface wants premultiplied ARGB32, so this is the one
// conversion pass between the two. Any decode problem (unknown format, corrupt data, too large) is
// reported by libstb as an exception and by this layer, as before, as a null surface.
static surface_t* surface_load_from_image(const stb::image& image)
{
    surface_t* surface = surface_create_uninitialized(image.width(), image.height());
    if(surface)
        convert_rgba_to_argb(surface->data, image.data(), surface->width, surface->height, surface->stride);
    return surface;
}

static stb::load_options surface_load_options()
{
    stb::load_options options;
    options.format = stb::pixel_format::rgba;
    return options;
}

NOVASVG_INLINE surface_t* surface_load_from_image_file(const char* filename)
{
    if(filename == NULL)
        return NULL;
    try {
        return surface_load_from_image(stb::image::open(filename, surface_load_options()));
    } catch(const std::exception&) {
        return NULL;
    }
}

NOVASVG_INLINE surface_t* surface_load_from_image_data(const void* data, int length)
{
    if(data == NULL || length <= 0)
        return NULL;
    try {
        return surface_load_from_image(stb::image::decode(data, size_t(length), surface_load_options()));
    } catch(const std::exception&) {
        return NULL;
    }
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
NOVASVG_INLINE uint8_t* base64_decode(const char* data, int length, size_t* out_length)
{
    // 0..63 = the sextet a character stands for; kWs = whitespace (skipped); kPad = '='; kBad = anything else.
    enum : uint8_t { kWs = 0xFD, kPad = 0xFE, kBad = 0xFF };
    static const struct Table {
        uint8_t v[256];
        Table()
        {
            for(int i = 0; i < 256; ++i)
                v[i] = kBad;
            for(int i = 0; i < 26; ++i) {
                v['A' + i] = uint8_t(i);
                v['a' + i] = uint8_t(26 + i);
            }
            for(int i = 0; i < 10; ++i)
                v['0' + i] = uint8_t(52 + i);
            v[uint8_t('+')] = 62;
            v[uint8_t('/')] = 63;
            v[uint8_t('=')] = kPad;
            for(int c : {' ', '\t', '\n', '\r', '\f', '\v'})
                v[c] = kWs;
        }
    } table;

    *out_length = 0;
    if(length == -1)
        length = int(strlen(data));
    if(length <= 0)
        return NULL;
    // Every 4 input characters make 3 bytes, so `length` bytes is always enough room.
    auto* output_data = static_cast<uint8_t*>(malloc(size_t(length)));
    if(output_data == NULL)
        return NULL;

    // Collect sextets and write finished bytes straight away. `acc` holds the bits not yet written.
    size_t didx = 0;
    size_t count = 0;        // sextets seen
    size_t pads = 0;         // '=' seen
    uint32_t acc = 0;
    int bits = 0;
    for(int i = 0; i < length; ++i) {
        // Fast path: a clean group of four sextets (no padding or whitespace so far, bits aligned) is
        // three bytes, with nothing to track between them.
        if(bits == 0 && pads == 0 && i + 4 <= length) {
            const uint8_t a = table.v[uint8_t(data[i])], b = table.v[uint8_t(data[i + 1])];
            const uint8_t c = table.v[uint8_t(data[i + 2])], d = table.v[uint8_t(data[i + 3])];
            if((a | b | c | d) < 64) {
                output_data[didx] = uint8_t((a << 2) | (b >> 4));
                output_data[didx + 1] = uint8_t((b << 4) | (c >> 2));
                output_data[didx + 2] = uint8_t((c << 6) | d);
                didx += 3;
                count += 4;
                i += 3;
                continue;
            }
        }

        const uint8_t v = table.v[uint8_t(data[i])];
        if(v < 64) {
            if(pads > 0)
                goto cleanup; // data after padding
            acc = (acc << 6) | v;
            bits += 6;
            ++count;
            if(bits >= 8) {
                bits -= 8;
                output_data[didx++] = uint8_t(acc >> bits);
            }
        } else if(v == kPad) {
            ++pads;
        } else if(v == kBad) {
            goto cleanup;
        }
    }

    if(count == 0 || pads > 2 || (count % 4) == 1)
        goto cleanup;
    if(didx == 0)
        goto cleanup;

    *out_length = didx;
    return output_data;
cleanup:
    free(output_data);
    return NULL;
}

NOVASVG_INLINE surface_t* surface_load_from_image_base64(const char* data, int length)
{
    size_t output_length = 0;
    uint8_t* output_data = base64_decode(data, length, &output_length);
    if(output_data == NULL)
        return NULL;
    surface_t* surface = surface_load_from_image_data(output_data, int(output_length));
    free(output_data);
    return surface;
}

NOVASVG_INLINE surface_t* surface_reference(surface_t* surface)
{
    novasvg_increment_reference(surface);
    return surface;
}

NOVASVG_INLINE void surface_destroy(surface_t* surface)
{
    if(novasvg_destroy_reference(surface)) {
        free(surface);
    }
}

NOVASVG_INLINE int surface_get_reference_count(const surface_t* surface)
{
    return novasvg_get_reference_count(surface);
}

NOVASVG_INLINE unsigned char* surface_get_data(const surface_t* surface)
{
    return surface->data;
}

NOVASVG_INLINE int surface_get_width(const surface_t* surface)
{
    return surface->width;
}

NOVASVG_INLINE int surface_get_height(const surface_t* surface)
{
    return surface->height;
}

NOVASVG_INLINE int surface_get_stride(const surface_t* surface)
{
    return surface->stride;
}

NOVASVG_INLINE void surface_clear(surface_t* surface, const color_t* color)
{
    uint32_t pixel = premultiply_argb(color_to_argb32(color));
    for(int y = 0; y < surface->height; y++) {
        uint32_t* pixels = (uint32_t*)(surface->data + surface->stride * y);
        memfill32(pixels, surface->width, pixel);
    }
}

NOVASVG_INLINE void convert_argb_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride)
{
    for(int y = 0; y < height; y++) {
        const uint32_t* src_row = (const uint32_t*)(src + stride * y);
        unsigned char* dst_row = dst + stride * y;
        for(int x = 0; x < width; x++) {
            uint32_t pixel = src_row[x];
            uint32_t a = (pixel >> 24) & 0xFF;
            if(a == 0) {
                *dst_row++ = 0;
                *dst_row++ = 0;
                *dst_row++ = 0;
                *dst_row++ = 0;
            } else {
                uint32_t r = (pixel >> 16) & 0xFF;
                uint32_t g = (pixel >> 8) & 0xFF;
                uint32_t b = (pixel >> 0) & 0xFF;
                if(a != 255) {
                    r = (r * 255) / a;
                    g = (g * 255) / a;
                    b = (b * 255) / a;
                }

                *dst_row++ = r;
                *dst_row++ = g;
                *dst_row++ = b;
                *dst_row++ = a;
            }
        }
    }
}

NOVASVG_INLINE void convert_rgba_to_argb(unsigned char* dst, const unsigned char* src, int width, int height, int stride)
{
    for(int y = 0; y < height; y++) {
        const unsigned char* src_row = src + stride * y;
        uint32_t* dst_row = (uint32_t*)(dst + stride * y);
        for(int x = 0; x < width; x++) {
            uint32_t a = src_row[4 * x + 3];
            if(a == 0) {
                dst_row[x] = 0x00000000;
            } else {
                uint32_t r = src_row[4 * x + 0];
                uint32_t g = src_row[4 * x + 1];
                uint32_t b = src_row[4 * x + 2];
                if(a != 255) {
                    r = (r * a) / 255;
                    g = (g * a) / 255;
                    b = (b * a) / 255;
                }

                dst_row[x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
}


} // namespace render
} // namespace novasvg