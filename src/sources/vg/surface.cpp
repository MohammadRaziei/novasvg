#include "surface.h"

namespace nova {
namespace vg {

surface_t* surface_create(int width, int height)
{
    surface_t* surface = surface_create_uninitialized(width, height);
    if(surface)
        memset(surface->data, 0, surface->height * surface->stride);
    return surface;
}

surface_t* surface_create_for_data(unsigned char* data, int width, int height, int stride)
{
    surface_t* surface = static_cast<surface_t*>(malloc(sizeof(surface_t)));
    novasvg_init_reference(surface);
    surface->width = width;
    surface->height = height;
    surface->stride = stride;
    surface->data = data;
    return surface;
}

surface_t* surface_load_from_image_file(const char* filename)
{
    if(filename == NULL)
        return NULL;
    try {
        return surface_load_from_image(stb::image::open(filename, surface_load_options()));
    } catch(const std::exception&) {
        return NULL;
    }
}

surface_t* surface_load_from_image_data(const void* data, int length)
{
    if(data == NULL || length <= 0)
        return NULL;
    try {
        return surface_load_from_image(stb::image::decode(data, size_t(length), surface_load_options()));
    } catch(const std::exception&) {
        return NULL;
    }
}

uint8_t* base64_decode(const char* data, int length, size_t* out_length)
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

surface_t* surface_load_from_image_base64(const char* data, int length)
{
    size_t output_length = 0;
    uint8_t* output_data = base64_decode(data, length, &output_length);
    if(output_data == NULL)
        return NULL;
    surface_t* surface = surface_load_from_image_data(output_data, int(output_length));
    free(output_data);
    return surface;
}

surface_t* surface_reference(surface_t* surface)
{
    novasvg_increment_reference(surface);
    return surface;
}

void surface_destroy(surface_t* surface)
{
    if(novasvg_destroy_reference(surface)) {
        free(surface);
    }
}

int surface_get_reference_count(const surface_t* surface)
{
    return novasvg_get_reference_count(surface);
}

unsigned char* surface_get_data(const surface_t* surface)
{
    return surface->data;
}

int surface_get_width(const surface_t* surface)
{
    return surface->width;
}

int surface_get_height(const surface_t* surface)
{
    return surface->height;
}

int surface_get_stride(const surface_t* surface)
{
    return surface->stride;
}

void surface_clear(surface_t* surface, const color_t* color)
{
    uint32_t pixel = premultiply_argb(color_to_argb32(color));
    for(int y = 0; y < surface->height; y++) {
        uint32_t* pixels = (uint32_t*)(surface->data + surface->stride * y);
        memfill32(pixels, surface->width, pixel);
    }
}

void convert_argb_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride)
{
    stb::premultiplied_argb_to_rgba(dst, size_t(stride), src, size_t(stride), width, height);
}

void convert_rgba_to_argb(unsigned char* dst, const unsigned char* src, int width, int height, int stride)
{
    stb::rgba_to_premultiplied_argb(dst, size_t(stride), src, size_t(stride), width, height);
}

} // namespace vg
} // namespace nova
