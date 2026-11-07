#include "blend.h"

namespace nova {
namespace vg {

uint32_t premultiply_color_with_opacity(const color_t* color_, float opacity)
{
    uint32_t alpha = lroundf(color_->a * opacity * 255);
    uint32_t pr = lroundf(color_->r * alpha);
    uint32_t pg = lroundf(color_->g * alpha);
    uint32_t pb = lroundf(color_->b * alpha);
    return (alpha << 24) | (pr << 16) | (pg << 8) | (pb);
}

uint32_t INTERPOLATE_PIXEL_255(uint32_t x, uint32_t a, uint32_t y, uint32_t b)
{
    uint32_t t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

uint32_t INTERPOLATE_PIXEL_256(uint32_t x, uint32_t a, uint32_t y, uint32_t b)
{
    uint32_t t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
    t >>= 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
    x &= 0xff00ff00;
    x |= t;
    return x;
}

uint32_t BYTE_MUL(uint32_t x, uint32_t a)
{
    uint32_t t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

int gradient_clamp(const gradient_data_t* gradient, int ipos)
{
    if(gradient->spread == NOVA_SPREAD_METHOD_REPEAT) {
        ipos = ipos % COLOR_TABLE_SIZE;
        ipos = ipos < 0 ? COLOR_TABLE_SIZE + ipos : ipos;
    } else if(gradient->spread == NOVA_SPREAD_METHOD_REFLECT) {
        const int limit = COLOR_TABLE_SIZE * 2;
        ipos = ipos % limit;
        ipos = ipos < 0 ? limit + ipos : ipos;
        ipos = ipos >= COLOR_TABLE_SIZE ? limit - 1 - ipos : ipos;
    } else {
        if(ipos < 0) {
            ipos = 0;
        } else if(ipos >= COLOR_TABLE_SIZE) {
            ipos = COLOR_TABLE_SIZE - 1;
        }
    }

    return ipos;
}

uint32_t gradient_pixel_fixed(const gradient_data_t* gradient, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return gradient->colortable[gradient_clamp(gradient, ipos)];
}

uint32_t gradient_pixel(const gradient_data_t* gradient, float pos)
{
    int ipos = (int)(pos * (COLOR_TABLE_SIZE - 1) + 0.5f);
    return gradient->colortable[gradient_clamp(gradient, ipos)];
}

void fetch_linear_gradient(uint32_t* buffer, const linear_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length_)
{
    float t, inc;
    float rx = 0, ry = 0;

    if(v->l == 0.f) {
        t = inc = 0;
    } else {
        rx = gradient->matrix.c * (y + 0.5f) + gradient->matrix.a * (x + 0.5f) + gradient->matrix.e;
        ry = gradient->matrix.d * (y + 0.5f) + gradient->matrix.b * (x + 0.5f) + gradient->matrix.f;
        t = v->dx * rx + v->dy * ry + v->off;
        inc = v->dx * gradient->matrix.a + v->dy * gradient->matrix.b;
        t *= (COLOR_TABLE_SIZE - 1);
        inc *= (COLOR_TABLE_SIZE - 1);
    }

    const uint32_t* end = buffer + length_;
    if(inc > -1e-5f && inc < 1e-5f) {
        memfill32(buffer, length_, gradient_pixel_fixed(gradient, (int)(t * FIXPT_SIZE)));
    } else {
        if(t + inc * length_ < (float)(INT_MAX >> (FIXPT_BITS + 1)) && t + inc * length_ > (float)(INT_MIN >> (FIXPT_BITS + 1))) {
            int t_fixed = (int)(t * FIXPT_SIZE);
            int inc_fixed = (int)(inc * FIXPT_SIZE);
            while(buffer < end) {
                *buffer = gradient_pixel_fixed(gradient, t_fixed);
                t_fixed += inc_fixed;
                ++buffer;
            }
        } else {
            while(buffer < end) {
                *buffer = gradient_pixel(gradient, t / COLOR_TABLE_SIZE);
                t += inc;
                ++buffer;
            }
        }
    }
}

void fetch_radial_gradient(uint32_t* buffer, const radial_gradient_values_t* v, const gradient_data_t* gradient, int y, int x, int length_)
{
    if(v->a == 0.f) {
        memfill32(buffer, length_, 0);
        return;
    }

    float rx = gradient->matrix.c * (y + 0.5f) + gradient->matrix.e + gradient->matrix.a * (x + 0.5f);
    float ry = gradient->matrix.d * (y + 0.5f) + gradient->matrix.f + gradient->matrix.b * (x + 0.5f);

    rx -= gradient->values.radial.fx;
    ry -= gradient->values.radial.fy;

    float inv_a = 1.f / (2.f * v->a);
    float delta_rx = gradient->matrix.a;
    float delta_ry = gradient->matrix.b;

    float b = 2 * (v->dr * gradient->values.radial.fr + rx * v->dx + ry * v->dy);
    float delta_b = 2 * (delta_rx * v->dx + delta_ry * v->dy);
    float b_delta_b = 2 * b * delta_b;
    float delta_b_delta_b = 2 * delta_b * delta_b;

    float bb = b * b;
    float delta_bb = delta_b * delta_b;

    b *= inv_a;
    delta_b *= inv_a;

    float rxrxryry = rx * rx + ry * ry;
    float delta_rxrxryry = delta_rx * delta_rx + delta_ry * delta_ry;
    float rx_plus_ry = 2 * (rx * delta_rx + ry * delta_ry);
    float delta_rx_plus_ry = 2 * delta_rxrxryry;

    inv_a *= inv_a;

    float det = (bb - 4 * v->a * (v->sqrfr - rxrxryry)) * inv_a;
    float delta_det = (b_delta_b + delta_bb + 4 * v->a * (rx_plus_ry + delta_rxrxryry)) * inv_a;
    float delta_delta_det = (delta_b_delta_b + 4 * v->a * delta_rx_plus_ry) * inv_a;

    const uint32_t* end = buffer + length_;
    if(v->extended) {
        while(buffer < end) {
            uint32_t result = 0;
            if(det >= 0) {
                float w = sqrtf(det) - b;
                if(gradient->values.radial.fr + v->dr * w >= 0) {
                    result = gradient_pixel(gradient, w);
                }
            }

            *buffer = result;
            det += delta_det;
            delta_det += delta_delta_det;
            b += delta_b;
            ++buffer;
        }
    } else {
        while(buffer < end) {
            *buffer++ = gradient_pixel(gradient, sqrtf(det) - b);
            det += delta_det;
            delta_det += delta_delta_det;
            b += delta_b;
        }
    }
}

void composition_solid_clear(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        memfill32(dest, length_, 0);
    } else {
        uint32_t ialpha = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(dest[i], ialpha);
        }
    }
}

void composition_solid_source(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        memfill32(dest, length_, color_);
    } else {
        uint32_t ialpha = 255 - const_alpha;
        color_ = BYTE_MUL(color_, const_alpha);
        for(int i = 0; i < length_; i++) {
            dest[i] = color_ + BYTE_MUL(dest[i], ialpha);
        }
    }
}

void composition_solid_destination(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
}

void composition_solid_source_over(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha != 255)
        color_ = BYTE_MUL(color_, const_alpha);
    uint32_t ialpha = 255 - novasvg_alpha(color_);
    for(int i = 0; i < length_; i++) {
        dest[i] = color_ + BYTE_MUL(dest[i], ialpha);
    }
}

void composition_solid_destination_over(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha != 255)
        color_ = BYTE_MUL(color_, const_alpha);
    for(int i = 0; i < length_; i++) {
        uint32_t d = dest[i];
        dest[i] = d + BYTE_MUL(color_, novasvg_alpha(~d));
    }
}

void composition_solid_source_in(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(color_, novasvg_alpha(dest[i]));
        }
    } else {
        color_ = BYTE_MUL(color_, const_alpha);
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color_, novasvg_alpha(d), d, cia);
        }
    }
}

void composition_solid_destination_in(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    uint32_t a = novasvg_alpha(color_);
    if(const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    for(int i = 0; i < length_; i++) {
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void composition_solid_source_out(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(color_, novasvg_alpha(~dest[i]));
        }
    } else {
        color_ = BYTE_MUL(color_, const_alpha);
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color_, novasvg_alpha(~d), d, cia);
        }
    }
}

void composition_solid_destination_out(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    uint32_t a = novasvg_alpha(~color_);
    if(const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    for(int i = 0; i < length_; i++) {
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void composition_solid_source_atop(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha != 255)
        color_ = BYTE_MUL(color_, const_alpha);
    uint32_t sia = novasvg_alpha(~color_);
    for(int i = 0; i < length_; i++) {
        uint32_t d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color_, novasvg_alpha(d), d, sia);
    }
}

void composition_solid_destination_atop(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    uint32_t a = novasvg_alpha(color_);
    if(const_alpha != 255) {
        color_ = BYTE_MUL(color_, const_alpha);
        a = novasvg_alpha(color_) + 255 - const_alpha;
    }

    for(int i = 0; i < length_; i++) {
        uint32_t d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(d, a, color_, novasvg_alpha(~d));
    }
}

void composition_solid_xor(uint32_t* dest, int length_, uint32_t color_, uint32_t const_alpha)
{
    if(const_alpha != 255)
        color_ = BYTE_MUL(color_, const_alpha);
    uint32_t sia = novasvg_alpha(~color_);
    for(int i = 0; i < length_; i++) {
        uint32_t d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color_, novasvg_alpha(~d), d, sia);
    }
}

int blend_mul255(int x, int y)
{
    return (x * y + 127) / 255;
}

void composition_clear(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        memfill32(dest, length_, 0);
    } else {
        uint32_t ialpha = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(dest[i], ialpha);
        }
    }
}

void composition_source(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        memcpy(dest, src, length_ * sizeof(uint32_t));
    } else {
        uint32_t ialpha = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            dest[i] = INTERPOLATE_PIXEL_255(src[i], const_alpha, dest[i], ialpha);
        }
    }
}

void composition_destination(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
}

void composition_source_over(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            uint32_t s = src[i];
            if(s >= 0xff000000) {
                dest[i] = s;
            } else if(s != 0) {
                dest[i] = s + BYTE_MUL(dest[i], novasvg_alpha(~s));
            }
        }
    } else {
        for(int i = 0; i < length_; i++) {
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            dest[i] = s + BYTE_MUL(dest[i], novasvg_alpha(~s));
        }
    }
}

void composition_destination_over(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            dest[i] = d + BYTE_MUL(src[i], novasvg_alpha(~d));
        }
    } else {
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            dest[i] = d + BYTE_MUL(s, novasvg_alpha(~d));
        }
    }
}

void composition_source_in(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(src[i], novasvg_alpha(dest[i]));
        }
    } else {
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(d), d, cia);
        }
    }
}

void composition_destination_in(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(dest[i], novasvg_alpha(src[i]));
        }
    } else {
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t a = BYTE_MUL(novasvg_alpha(src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

void composition_source_out(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(src[i], novasvg_alpha(~dest[i]));
        }
    } else {
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(~d), d, cia);
        }
    }
}

void composition_destination_out(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            dest[i] = BYTE_MUL(dest[i], novasvg_alpha(~src[i]));
        }
    } else {
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t sia = BYTE_MUL(novasvg_alpha(~src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

void composition_source_atop(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            uint32_t s = src[i];
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(d), d, novasvg_alpha(~s));
        }
    } else {
        for(int i = 0; i < length_; i++) {
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(d), d, novasvg_alpha(~s));
        }
    }
}

void composition_destination_atop(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            uint32_t s = src[i];
            uint32_t d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(d, novasvg_alpha(s), s, novasvg_alpha(~d));
        }
    } else {
        uint32_t cia = 255 - const_alpha;
        for(int i = 0; i < length_; i++) {
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            uint32_t d = dest[i];
            uint32_t a = novasvg_alpha(s) + cia;
            dest[i] = INTERPOLATE_PIXEL_255(d, a, s, novasvg_alpha(~d));
        }
    }
}

void composition_xor(uint32_t* dest, int length_, const uint32_t* src, uint32_t const_alpha)
{
    if(const_alpha == 255) {
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            uint32_t s = src[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(~d), d, novasvg_alpha(~s));
        }
    } else {
        for(int i = 0; i < length_; i++) {
            uint32_t d = dest[i];
            uint32_t s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, novasvg_alpha(~d), d, novasvg_alpha(~s));
        }
    }
}

void blend_solid(surface_t* surface, operator_t op, uint32_t solid, const span_buffer_t* span_buffer)
{
    composition_solid_function_t func = composition_solid_table[op];
    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + spans->x;
        func(target, spans->len, solid, spans->coverage);
        ++spans;
    }
}

void blend_linear_gradient(surface_t* surface, operator_t op, const gradient_data_t* gradient, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];
    unsigned int buffer[BUFFER_SIZE];

    linear_gradient_values_t v;
    v.dx = gradient->values.linear.x2 - gradient->values.linear.x1;
    v.dy = gradient->values.linear.y2 - gradient->values.linear.y1;
    v.l = v.dx * v.dx + v.dy * v.dy;
    v.off = 0.f;
    if(v.l != 0.f) {
        v.dx /= v.l;
        v.dy /= v.l;
        v.off = -v.dx * gradient->values.linear.x1 - v.dy * gradient->values.linear.y1;
    }

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        int length_ = spans->len;
        int x = spans->x;
        while(length_) {
            int l = STB_MIN(length_, BUFFER_SIZE);
            fetch_linear_gradient(buffer, &v, gradient, spans->y, x, l);
            uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + x;
            func(target, l, buffer, spans->coverage);
            x += l;
            length_ -= l;
        }

        ++spans;
    }
}

void blend_radial_gradient(surface_t* surface, operator_t op, const gradient_data_t* gradient, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];
    unsigned int buffer[BUFFER_SIZE];

    radial_gradient_values_t v;
    v.dx = gradient->values.radial.cx - gradient->values.radial.fx;
    v.dy = gradient->values.radial.cy - gradient->values.radial.fy;
    v.dr = gradient->values.radial.cr - gradient->values.radial.fr;
    v.sqrfr = gradient->values.radial.fr * gradient->values.radial.fr;
    v.a = v.dr * v.dr - v.dx * v.dx - v.dy * v.dy;
    v.extended = gradient->values.radial.fr != 0.f || v.a <= 0.f;

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        int length_ = spans->len;
        int x = spans->x;
        while(length_) {
            int l = STB_MIN(length_, BUFFER_SIZE);
            fetch_radial_gradient(buffer, &v, gradient, spans->y, x, l);
            uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + x;
            func(target, l, buffer, spans->coverage);
            x += l;
            length_ -= l;
        }

        ++spans;
    }
}

void blend_untransformed_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];

    const int image_width = texture->width;
    const int image_height = texture->height;

    int xoff = (int)(texture->matrix.e);
    int yoff = (int)(texture->matrix.f);

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        int x = spans->x;
        int length_ = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if(sy >= 0 && sy < image_height && sx < image_width) {
            if(sx < 0) {
                x -= sx;
                length_ += sx;
                sx = 0;
            }

            if(sx + length_ > image_width)
                length_ = image_width - sx;
            if(length_ > 0) {
                const int coverage = (spans->coverage * texture->const_alpha) >> 8;
                const uint32_t* src = (const uint32_t*)(texture->data + sy * texture->stride) + sx;
                uint32_t* dest = (uint32_t*)(surface->data + spans->y * surface->stride) + x;
                func(dest, length_, src, coverage);
            }
        }

        ++spans;
    }
}

void blend_transformed_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];
    uint32_t buffer[BUFFER_SIZE];

    int image_width = texture->width;
    int image_height = texture->height;

    int fdx = (int)(texture->matrix.a * FIXED_SCALE);
    int fdy = (int)(texture->matrix.b * FIXED_SCALE);

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + spans->x;

        const float cx = spans->x + 0.5f;
        const float cy = spans->y + 0.5f;

        int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx + texture->matrix.e) * FIXED_SCALE);
        int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx + texture->matrix.f) * FIXED_SCALE);

        int length_ = spans->len;
        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        while(length_) {
            int l = STB_MIN(length_, BUFFER_SIZE);
            const uint32_t* end = buffer + l;
            uint32_t* b = buffer;
            while(b < end) {
                int px = x >> 16;
                int py = y >> 16;
                if((px < 0) || (px >= image_width) || (py < 0) || (py >= image_height)) {
                    *b = 0x00000000;
                } else {
                    *b = ((const uint32_t*)(texture->data + py * texture->stride))[px];
                }

                x += fdx;
                y += fdy;
                ++b;
            }

            func(target, l, buffer, coverage);
            target += l;
            length_ -= l;
        }

        ++spans;
    }
}

void blend_untransformed_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];

    int image_width = texture->width;
    int image_height = texture->height;

    int xoff = (int)(texture->matrix.e) % image_width;
    int yoff = (int)(texture->matrix.f) % image_height;

    if(xoff < 0)
        xoff += image_width;
    if(yoff < 0) {
        yoff += image_height;
    }

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        int x = spans->x;
        int length_ = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if(sx < 0)
            sx += image_width;
        if(sy < 0) {
            sy += image_height;
        }

        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        while(length_) {
            int l = STB_MIN(image_width - sx, length_);
            if(BUFFER_SIZE < l)
                l = BUFFER_SIZE;
            const uint32_t* src = (const uint32_t*)(texture->data + sy * texture->stride) + sx;
            uint32_t* dest = (uint32_t*)(surface->data + spans->y * surface->stride) + x;
            func(dest, l, src, coverage);
            x += l;
            sx += l;
            length_ -= l;
            if(sx >= image_width) {
                sx = 0;
            }
        }

        ++spans;
    }
}

void blend_transformed_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];
    uint32_t buffer[BUFFER_SIZE];

    int image_width = texture->width;
    int image_height = texture->height;
    const int scanline_offset = texture->stride / 4;

    int fdx = (int)(texture->matrix.a * FIXED_SCALE);
    int fdy = (int)(texture->matrix.b * FIXED_SCALE);

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + spans->x;
        const uint32_t* image_bits = (const uint32_t*)texture->data;

        const float cx = spans->x + 0.5f;
        const float cy = spans->y + 0.5f;

        int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx + texture->matrix.e) * FIXED_SCALE);
        int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx + texture->matrix.f) * FIXED_SCALE);

        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        int length_ = spans->len;
        while(length_) {
            int l = STB_MIN(length_, BUFFER_SIZE);
            const uint32_t* end = buffer + l;
            uint32_t* b = buffer;
            while(b < end) {
                int px = x >> 16;
                int py = y >> 16;
                px %= image_width;
                py %= image_height;
                if(px < 0) px += image_width;
                if(py < 0) py += image_height;
                int y_offset = py * scanline_offset;

                assert(px >= 0 && px < image_width);
                assert(py >= 0 && py < image_height);

                *b = image_bits[y_offset + px];
                x += fdx;
                y += fdy;
                ++b;
            }

            func(target, l, buffer, coverage);
            target += l;
            length_ -= l;
        }

        ++spans;
    }
}

uint32_t interpolate_4_pixels(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br, uint32_t distx, uint32_t disty)
{
    uint32_t idistx = 256 - distx;
    uint32_t idisty = 256 - disty;
    uint32_t xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
    uint32_t xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
    return INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
}

void blend_transformed_bilinear_tiled_argb(surface_t* surface, operator_t op, const texture_data_t* texture, const span_buffer_t* span_buffer)
{
    composition_function_t func = composition_table[op];
    uint32_t buffer[BUFFER_SIZE];

    int image_width = texture->width;
    int image_height = texture->height;

    int fdx = (int)(texture->matrix.a * FIXED_SCALE);
    int fdy = (int)(texture->matrix.b * FIXED_SCALE);

    int count = span_buffer->spans.size_;
    const span_t* spans = span_buffer->spans.data;
    while(count--) {
        uint32_t* target = (uint32_t*)(surface->data + spans->y * surface->stride) + spans->x;

        const float cx = spans->x + 0.5f;
        const float cy = spans->y + 0.5f;

        int fx = (int)((texture->matrix.c * cy + texture->matrix.a * cx + texture->matrix.e) * FIXED_SCALE);
        int fy = (int)((texture->matrix.d * cy + texture->matrix.b * cx + texture->matrix.f) * FIXED_SCALE);

        fx -= HALF_POINT;
        fy -= HALF_POINT;

        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        int length_ = spans->len;
        while(length_) {
            int l = STB_MIN(length_, BUFFER_SIZE);
            const uint32_t* end = buffer + l;
            uint32_t* b = buffer;
            while (b < end) {
                int x1 = (fx >> 16) % image_width;
                int y1 = (fy >> 16) % image_height;

                if(x1 < 0) x1 += image_width;
                if(y1 < 0) y1 += image_height;

                int x2 = (x1 + 1) % image_width;
                int y2 = (y1 + 1) % image_height;

                const uint32_t* s1 = (const uint32_t*)(texture->data + y1 * texture->stride);
                const uint32_t* s2 = (const uint32_t*)(texture->data + y2 * texture->stride);

                uint32_t tl = s1[x1];
                uint32_t tr = s1[x2];
                uint32_t bl = s2[x1];
                uint32_t br = s2[x2];

                int distx = (fx & 0x0000ffff) >> 8;
                int disty = (fy & 0x0000ffff) >> 8;
                *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

                fx += fdx;
                fy += fdy;
                ++b;
            }

            func(target, l, buffer, coverage);
            target += l;
            length_ -= l;
        }

        ++spans;
    }
}

void blend_color(canvas_t* canvas, const color_t* color_, const span_buffer_t* span_buffer)
{
    state_t* state = canvas->state;
    uint32_t solid = premultiply_color_with_opacity(color_, state->opacity);
    uint32_t alpha = novasvg_alpha(solid);

    if(alpha == 255 && state->op == NOVA_OPERATOR_SRC_OVER) {
        blend_solid(canvas->surface, NOVA_OPERATOR_SRC, solid, span_buffer);
    } else {
        blend_solid(canvas->surface, state->op, solid, span_buffer);
    }
}

void blend_gradient(canvas_t* canvas, const gradient_paint_t* gradient, const span_buffer_t* span_buffer)
{
    if(gradient->nstops == 0)
        return;
    state_t* state = canvas->state;
    gradient_data_t data;
    data.spread = gradient->spread;
    data.matrix = gradient->matrix;
    matrix_multiply(&data.matrix, &data.matrix, &state->matrix);
    if(!matrix_invert(&data.matrix, &data.matrix))
        return;
    int i, pos = 0, nstops = gradient->nstops;
    const gradient_stop_t *curr, *next, *start, *last;
    uint32_t curr_color, next_color, last_color;
    uint32_t dist, idist;
    float delta, t, incr, fpos;
    float opacity = state->opacity;

    start = gradient->stops;
    curr = start;
    curr_color = premultiply_color_with_opacity(&curr->color_, opacity);

    data.colortable[pos++] = curr_color;
    incr = 1.0f / COLOR_TABLE_SIZE;
    fpos = 1.5f * incr;

    while(fpos <= curr->offset) {
        data.colortable[pos] = data.colortable[pos - 1];
        ++pos;
        fpos += incr;
    }

    for(i = 0; i < nstops - 1; i++) {
        curr = (start + i);
        next = (start + i + 1);
        if(curr->offset == next->offset)
            continue;
        delta = 1.f / (next->offset - curr->offset);
        next_color = premultiply_color_with_opacity(&next->color_, opacity);
        while(fpos < next->offset && pos < COLOR_TABLE_SIZE) {
            t = (fpos - curr->offset) * delta;
            dist = (uint32_t)(255 * t);
            idist = 255 - dist;
            data.colortable[pos] = INTERPOLATE_PIXEL_255(curr_color, idist, next_color, dist);
            ++pos;
            fpos += incr;
        }

        curr_color = next_color;
    }

    last = start + nstops - 1;
    last_color = premultiply_color_with_opacity(&last->color_, opacity);
    for(; pos < COLOR_TABLE_SIZE; ++pos) {
        data.colortable[pos] = last_color;
    }

    if(gradient->type_ == NOVA_GRADIENT_TYPE_LINEAR) {
        data.values.linear.x1 = gradient->values[0];
        data.values.linear.y1 = gradient->values[1];
        data.values.linear.x2 = gradient->values[2];
        data.values.linear.y2 = gradient->values[3];
        blend_linear_gradient(canvas->surface, state->op, &data, span_buffer);
    } else {
        data.values.radial.cx = gradient->values[0];
        data.values.radial.cy = gradient->values[1];
        data.values.radial.cr = gradient->values[2];
        data.values.radial.fx = gradient->values[3];
        data.values.radial.fy = gradient->values[4];
        data.values.radial.fr = gradient->values[5];
        blend_radial_gradient(canvas->surface, state->op, &data, span_buffer);
    }
}

void blend_texture(canvas_t* canvas, const texture_paint_t* texture, const span_buffer_t* span_buffer)
{
    if(texture->surface == NULL)
        return;
    state_t* state = canvas->state;
    texture_data_t data;
    data.matrix = texture->matrix;
    data.data = texture->surface->data;
    data.width = texture->surface->width;
    data.height = texture->surface->height;
    data.stride = texture->surface->stride;
    data.const_alpha = lroundf(state->opacity * texture->opacity * 256);

    matrix_multiply(&data.matrix, &data.matrix, &state->matrix);
    if(!matrix_invert(&data.matrix, &data.matrix))
        return;
    const matrix_t* matrix = &data.matrix;
    if(matrix->a == 1 && matrix->b == 0 && matrix->c == 0 && matrix->d == 1) {
        if(texture->type_ == NOVA_TEXTURE_TYPE_PLAIN) {
            blend_untransformed_argb(canvas->surface, state->op, &data, span_buffer);
        } else {
            blend_untransformed_tiled_argb(canvas->surface, state->op, &data, span_buffer);
        }
    } else {
        if(texture->type_ == NOVA_TEXTURE_TYPE_PLAIN) {
            blend_transformed_argb(canvas->surface, state->op, &data, span_buffer);
        } else if(fabsf(matrix->b) > 1e-6f || fabsf(matrix->c) > 1e-6f) {
            blend_transformed_bilinear_tiled_argb(canvas->surface, state->op, &data, span_buffer);
        } else {
            blend_transformed_tiled_argb(canvas->surface, state->op, &data, span_buffer);
        }
    }
}

void blend(canvas_t* canvas, const span_buffer_t* span_buffer)
{
    if(span_buffer->spans.size_ == 0)
        return;
    if(canvas->state->paint_ == NULL) {
        blend_color(canvas, &canvas->state->color_, span_buffer);
        return;
    }

    paint_t* paint_ = canvas->state->paint_;
    if(paint_->type_ == NOVA_PAINT_TYPE_COLOR) {
        solid_paint_t* solid = (solid_paint_t*)(paint_);
        blend_color(canvas, &solid->color_, span_buffer);
    } else if(paint_->type_ == NOVA_PAINT_TYPE_GRADIENT) {
        gradient_paint_t* gradient = (gradient_paint_t*)(paint_);
        blend_gradient(canvas, gradient, span_buffer);
    } else {
        texture_paint_t* texture = (texture_paint_t*)(paint_);
        blend_texture(canvas, texture, span_buffer);
    }
}

} // namespace vg
} // namespace nova
