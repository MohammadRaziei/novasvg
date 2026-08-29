#pragma once

#include "private.h"
#include "utils.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

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

static surface_t* surface_load_from_image(stbi_uc* image, int width, int height)
{
    surface_t* surface = surface_create_uninitialized(width, height);
    if(surface)
        convert_rgba_to_argb(surface->data, image, surface->width, surface->height, surface->stride);
    stbi_image_free(image);
    return surface;
}

NOVASVG_INLINE surface_t* surface_load_from_image_file(const char* filename)
{
    int width, height, channels;
    stbi_uc* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if(image == NULL)
        return NULL;
    return surface_load_from_image(image, width, height);
}

NOVASVG_INLINE surface_t* surface_load_from_image_data(const void* data, int length)
{
    int width, height, channels;
    stbi_uc* image = stbi_load_from_memory(static_cast<const stbi_uc*>(data), length, &width, &height, &channels, STBI_rgb_alpha);
    if(image == NULL)
        return NULL;
    return surface_load_from_image(image, width, height);
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

NOVASVG_INLINE surface_t* surface_load_from_image_base64(const char* data, int length)
{
    surface_t* surface = NULL;
    uint8_t* output_data = NULL;
    size_t output_length = 0;

    size_t equals_sign_count = 0;
    size_t sidx = 0;
    size_t didx = 0;

    if(length == -1)
        length = strlen(data);
    output_data = static_cast<decltype(output_data)>(malloc(length));
    if(output_data == NULL)
        return NULL;
    for(int i = 0; i < length; ++i) {
        uint8_t cc = data[i];
        if(cc == '=') {
            ++equals_sign_count;
        } else if(cc == '+' || cc == '/' || NOVASVG_IS_ALNUM(cc)) {
            if(equals_sign_count > 0)
                goto cleanup;
            output_data[output_length++] = base64_table[cc];
        } else if(!NOVASVG_IS_WS(cc)) {
            goto cleanup;
        }
    }

    if(output_length == 0 || equals_sign_count > 2 || (output_length % 4) == 1)
        goto cleanup;
    output_length -= (output_length + 3) / 4;
    if(output_length == 0) {
        goto cleanup;
    }

    if(output_length > 1) {
        while(didx < output_length - 2) {
            output_data[didx + 0] = (((output_data[sidx + 0] << 2) & 255) | ((output_data[sidx + 1] >> 4) & 003));
            output_data[didx + 1] = (((output_data[sidx + 1] << 4) & 255) | ((output_data[sidx + 2] >> 2) & 017));
            output_data[didx + 2] = (((output_data[sidx + 2] << 6) & 255) | ((output_data[sidx + 3] >> 0) & 077));
            sidx += 4;
            didx += 3;
        }
    }

    if(didx < output_length)
        output_data[didx] = (((output_data[sidx + 0] << 2) & 255) | ((output_data[sidx + 1] >> 4) & 003));
    if(++didx < output_length) {
        output_data[didx] = (((output_data[sidx + 1] << 4) & 255) | ((output_data[sidx + 2] >> 2) & 017));
    }

    surface = surface_load_from_image_data(output_data, output_length);
cleanup:
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

// A short-lived, unpremultiplied RGBA copy of a surface's pixels, used
// only for encoding. `surface`'s own premultiplied-ARGB buffer is the
// one every compositing operator in blend.h assumes, so encoding must
// not touch it -- this makes surface_write_to_* true read-only
// operations instead of the old convert -> encode -> convert-back
// dance, which did two full unpremultiply-divide passes over every
// pixel for a net no-op. One pass is the least this can cost: the
// divide-by-alpha is unavoidable (PNG/JPEG want straight alpha, the
// rasterizer needs premultiplied for `over` to be correct), so this
// does it exactly once, into a buffer sized to match `surface` exactly
// and freed as soon as encoding is done.
class SurfaceRGBACopy {
public:
    explicit SurfaceRGBACopy(const surface_t* surface)
        : m_data(static_cast<unsigned char*>(malloc(size_t(surface->stride) * size_t(surface->height))))
    {
        if(m_data != nullptr)
            convert_argb_to_rgba(m_data, surface->data, surface->width, surface->height, surface->stride);
    }

    ~SurfaceRGBACopy() { free(m_data); }

    SurfaceRGBACopy(const SurfaceRGBACopy&) = delete;
    SurfaceRGBACopy& operator=(const SurfaceRGBACopy&) = delete;

    unsigned char* data() const { return m_data; }
    explicit operator bool() const { return m_data != nullptr; }

private:
    unsigned char* m_data;
};

NOVASVG_INLINE bool surface_write_to_png(const surface_t* surface, const char* filename)
{
    SurfaceRGBACopy copy(surface);
    if(!copy)
        return false;
    return stbi_write_png(filename, surface->width, surface->height, 4, copy.data(), surface->stride) != 0;
}

NOVASVG_INLINE bool surface_write_to_jpg(const surface_t* surface, const char* filename, int quality)
{
    SurfaceRGBACopy copy(surface);
    if(!copy)
        return false;
    return stbi_write_jpg(filename, surface->width, surface->height, 4, copy.data(), quality) != 0;
}

NOVASVG_INLINE bool surface_write_to_png_stream(const surface_t* surface, write_func_t write_func, void* closure)
{
    SurfaceRGBACopy copy(surface);
    if(!copy)
        return false;
    return stbi_write_png_to_func(write_func, closure, surface->width, surface->height, 4, copy.data(), surface->stride) != 0;
}

NOVASVG_INLINE bool surface_write_to_jpg_stream(const surface_t* surface, write_func_t write_func, void* closure, int quality)
{
    SurfaceRGBACopy copy(surface);
    if(!copy)
        return false;
    return stbi_write_jpg_to_func(write_func, closure, surface->width, surface->height, 4, copy.data(), quality) != 0;
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