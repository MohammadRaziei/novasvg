#pragma once

#include "private.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>

namespace novasvg {
namespace render {

#define COLOR_TABLE_SIZE 1024
typedef struct {
    matrix_t matrix;
    spread_method_t spread;
    uint32_t colortable[COLOR_TABLE_SIZE];
    union {
        struct {
            float x1, y1;
            float x2, y2;
        } linear;
        struct {
            float cx, cy, cr;
            float fx, fy, fr;
        } radial;
    } values;
} gradient_data_t;

typedef struct {
    matrix_t matrix;
    uint8_t* data;
    int width;
    int height;
    int stride;
    int const_alpha;
} texture_data_t;

typedef struct {
    float dx;
    float dy;
    float l;
    float off;
} linear_gradient_values_t;

typedef struct {
    float dx;
    float dy;
    float dr;
    float sqrfr;
    float a;
    bool extended;
} radial_gradient_values_t;

uint32_t premultiply_color_with_opacity(const color_t* color, float opacity);

uint32_t INTERPOLATE_PIXEL_255(uint32_t x, uint32_t a, uint32_t y, uint32_t b);

uint32_t INTERPOLATE_PIXEL_256(uint32_t x, uint32_t a, uint32_t y, uint32_t b);

uint32_t BYTE_MUL(uint32_t x, uint32_t a);

#ifdef __SSE2__

} // namespace render
} // namespace novasvg
#include <emmintrin.h>
namespace novasvg {
namespace render {

NOVASVG_INLINE void memfill32(unsigned int* dest, int length, unsigned int value)
{
    __m128i vector_data = _mm_set_epi32(value, value, value, value);
    while(length && ((uintptr_t)dest & 0xf)) {
        *dest++ = value;
        length--;
    }

    while(length >= 32) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);
        _mm_store_si128((__m128i*)(dest + 8), vector_data);
        _mm_store_si128((__m128i*)(dest + 12), vector_data);
        _mm_store_si128((__m128i*)(dest + 16), vector_data);
        _mm_store_si128((__m128i*)(dest + 20), vector_data);
        _mm_store_si128((__m128i*)(dest + 24), vector_data);
        _mm_store_si128((__m128i*)(dest + 28), vector_data);

        dest += 32;
        length -= 32;
    }

    if(length >= 16) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);
        _mm_store_si128((__m128i*)(dest + 8), vector_data);
        _mm_store_si128((__m128i*)(dest + 12), vector_data);

        dest += 16;
        length -= 16;
    }

    if(length >= 8) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);

        dest += 8;
        length -= 8;
    }

    if(length >= 4) {
        _mm_store_si128((__m128i*)(dest), vector_data);

        dest += 4;
        length -= 4;
    }

    while(length) {
        *dest++ = value;
        length--;
    }
}

#else

NOVASVG_INLINE void memfill32(unsigned int* dest, int length, unsigned int value)
{
    while(length--) {
        *dest++ = value;
    }
}

#endif // __SSE2__

int gradient_clamp(const gradient_data_t* gradient, int ipos);

#define FIXPT_BITS 8
#define FIXPT_SIZE (1 << FIXPT_BITS)
uint32_t gradient_pixel_fixed(const gradient_data_t* gradient, int fixed_pos);

uint32_t gradient_pixel(const gradient_data_t* gradient, float pos);

void fetch_linear_gradient(uint32_t* buffer, const linear_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length);

void fetch_radial_gradient(uint32_t* buffer, const radial_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length);

void composition_solid_clear(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_source(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_destination(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_source_over(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_destination_over(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_source_in(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_destination_in(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_source_out(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_destination_out(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_source_atop(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_destination_atop(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

void composition_solid_xor(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

// ---- CSS blend modes -------------------------------------------------------------------------------
// W3C Compositing and Blending, on premultiplied channels (all 0..255, "mul" = x*y/255 rounded):
//   result colour = Cs*(1-ab) + Cb*(1-as) + as*ab*B(Cb/ab, Cs/as)        result alpha = as + ab - as*ab
// which for each separable mode B simplifies to (Cs, Cb premultiplied; as, ab the two alphas):
//   multiply   Cs*(1-ab) + Cb*(1-as) + Cs*Cb
//   screen     Cs + Cb - Cs*Cb
//   darken     Cs + Cb - max(Cs*ab, Cb*as)
//   lighten    Cs + Cb - min(Cs*ab, Cb*as)
//   difference Cs + Cb - 2*min(Cs*ab, Cb*as)
//   exclusion  Cs + Cb - 2*Cs*Cb
int blend_mul255(int x, int y);

template<operator_t Mode>
NOVASVG_INLINE int blend_mode_channel(int cs, int cb, int as, int ab)
{
    int value;
    if(Mode == NOVASVG_OPERATOR_MULTIPLY)
        value = blend_mul255(cs, 255 - ab) + blend_mul255(cb, 255 - as) + blend_mul255(cs, cb);
    else if(Mode == NOVASVG_OPERATOR_SCREEN)
        value = cs + cb - blend_mul255(cs, cb);
    else if(Mode == NOVASVG_OPERATOR_DARKEN)
        value = cs + cb - NOVASVG_MAX(blend_mul255(cs, ab), blend_mul255(cb, as));
    else if(Mode == NOVASVG_OPERATOR_LIGHTEN)
        value = cs + cb - NOVASVG_MIN(blend_mul255(cs, ab), blend_mul255(cb, as));
    else if(Mode == NOVASVG_OPERATOR_DIFFERENCE)
        value = cs + cb - 2 * NOVASVG_MIN(blend_mul255(cs, ab), blend_mul255(cb, as));
    else
        value = cs + cb - 2 * blend_mul255(cs, cb);
    return value;
}

template<operator_t Mode>
NOVASVG_INLINE uint32_t blend_mode_pixel(uint32_t dest, uint32_t src)
{
    if(src == 0)
        return dest;
    const int as = novasvg_alpha(src);
    const int ab = novasvg_alpha(dest);
    const int ar = as + ab - blend_mul255(as, ab);
    uint32_t result = uint32_t(ar) << 24;
    for(int shift = 16; shift >= 0; shift -= 8) {
        int c = blend_mode_channel<Mode>(int((src >> shift) & 0xff), int((dest >> shift) & 0xff), as, ab);
        c = NOVASVG_MAX(0, NOVASVG_MIN(c, ar)); // a premultiplied channel never exceeds the alpha
        result |= uint32_t(c) << shift;
    }

    return result;
}

template<operator_t Mode>
NOVASVG_INLINE void composition_blend_mode(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha)
{
    for(int i = 0; i < length; i++) {
        uint32_t s = const_alpha == 255 ? src[i] : BYTE_MUL(src[i], const_alpha);
        dest[i] = blend_mode_pixel<Mode>(dest[i], s);
    }
}

template<operator_t Mode>
NOVASVG_INLINE void composition_solid_blend_mode(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha)
{
    if(const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    for(int i = 0; i < length; i++)
        dest[i] = blend_mode_pixel<Mode>(dest[i], color);
}

typedef void(*composition_solid_function_t)(uint32_t* dest, int length, uint32_t color, uint32_t const_alpha);

NOVASVG_INLINE const composition_solid_function_t composition_solid_table[] = {
    composition_solid_clear,
    composition_solid_source,
    composition_solid_destination,
    composition_solid_source_over,
    composition_solid_destination_over,
    composition_solid_source_in,
    composition_solid_destination_in,
    composition_solid_source_out,
    composition_solid_destination_out,
    composition_solid_source_atop,
    composition_solid_destination_atop,
    composition_solid_xor,
    composition_solid_blend_mode<NOVASVG_OPERATOR_MULTIPLY>,
    composition_solid_blend_mode<NOVASVG_OPERATOR_SCREEN>,
    composition_solid_blend_mode<NOVASVG_OPERATOR_DARKEN>,
    composition_solid_blend_mode<NOVASVG_OPERATOR_LIGHTEN>,
    composition_solid_blend_mode<NOVASVG_OPERATOR_DIFFERENCE>,
    composition_solid_blend_mode<NOVASVG_OPERATOR_EXCLUSION>
};

void composition_clear(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_source(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_destination(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_source_over(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_destination_over(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_source_in(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_destination_in(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_source_out(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_destination_out(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_source_atop(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_destination_atop(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

void composition_xor(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

typedef void(*composition_function_t)(uint32_t* dest, int length, const uint32_t* src, uint32_t const_alpha);

NOVASVG_INLINE const composition_function_t composition_table[] = {
    composition_clear,
    composition_source,
    composition_destination,
    composition_source_over,
    composition_destination_over,
    composition_source_in,
    composition_destination_in,
    composition_source_out,
    composition_destination_out,
    composition_source_atop,
    composition_destination_atop,
    composition_xor,
    composition_blend_mode<NOVASVG_OPERATOR_MULTIPLY>,
    composition_blend_mode<NOVASVG_OPERATOR_SCREEN>,
    composition_blend_mode<NOVASVG_OPERATOR_DARKEN>,
    composition_blend_mode<NOVASVG_OPERATOR_LIGHTEN>,
    composition_blend_mode<NOVASVG_OPERATOR_DIFFERENCE>,
    composition_blend_mode<NOVASVG_OPERATOR_EXCLUSION>
};

void blend_solid(surface_t* surface, operator_t op, uint32_t solid, const span_buffer_t* span_buffer);

#define BUFFER_SIZE 1024
void blend_linear_gradient(surface_t* surface, operator_t op, const gradient_data_t* gradient, const span_buffer_t* span_buffer);

void blend_radial_gradient(surface_t* surface, operator_t op, const gradient_data_t* gradient, const span_buffer_t* span_buffer);

void blend_untransformed_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer);

#define FIXED_SCALE (1 << 16)
void blend_transformed_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_untransformed_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_transformed_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer);

uint32_t interpolate_4_pixels(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br, uint32_t distx, uint32_t disty);

#define HALF_POINT (1 << 15)
void blend_transformed_bilinear_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer);

void blend_color(canvas_t* canvas, const color_t* color, const span_buffer_t* span_buffer);

void blend_gradient(canvas_t* canvas, const gradient_paint_t* gradient, const span_buffer_t* span_buffer);

void blend_texture(canvas_t* canvas, const texture_paint_t* texture, const span_buffer_t* span_buffer);

void blend(canvas_t* canvas, const span_buffer_t* span_buffer);


} // namespace render
} // namespace novasvg