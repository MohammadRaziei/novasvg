#pragma once

#include "private.h"
#include "utils.h"

#include <ctype.h>

namespace novasvg {
namespace render {

NOVASVG_INLINE void color_init_rgb(color_t* color, float r, float g, float b)
{
    color_init_rgba(color, r, g, b, 1.f);
}

NOVASVG_INLINE void color_init_rgba(color_t* color, float r, float g, float b, float a)
{
    color->r = std::clamp(r, 0.f, 1.f);
    color->g = std::clamp(g, 0.f, 1.f);
    color->b = std::clamp(b, 0.f, 1.f);
    color->a = std::clamp(a, 0.f, 1.f);
}

NOVASVG_INLINE void color_init_rgb8(color_t* color, int r, int g, int b)
{
    color_init_rgba8(color, r, g, b, 255);
}

NOVASVG_INLINE void color_init_rgba8(color_t* color, int r, int g, int b, int a)
{
    color_init_rgba(color, r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

NOVASVG_INLINE void color_init_rgba32(color_t* color, unsigned int value)
{
    uint8_t r = (value >> 24) & 0xFF;
    uint8_t g = (value >> 16) & 0xFF;
    uint8_t b = (value >>  8) & 0xFF;
    uint8_t a = (value >>  0) & 0xFF;
    color_init_rgba8(color, r, g, b, a);
}

NOVASVG_INLINE void color_init_argb32(color_t* color, unsigned int value)
{
    uint8_t a = (value >> 24) & 0xFF;
    uint8_t r = (value >> 16) & 0xFF;
    uint8_t g = (value >>  8) & 0xFF;
    uint8_t b = (value >>  0) & 0xFF;
    color_init_rgba8(color, r, g, b, a);
}

NOVASVG_INLINE void color_init_hsl(color_t* color, float h, float s, float l)
{
    color_init_hsla(color, h, s, l, 1.f);
}

static inline float hsl_component(float h, float s, float l, float n)
{
    const float k = fmodf(n + h / 30.f, 12.f);
    const float a = s * std::min(l, 1.f - l);
    return l - a * std::max(-1.f, std::min(1.f, std::min(k - 3.f, 9.f - k)));
}

NOVASVG_INLINE void color_init_hsla(color_t* color, float h, float s, float l, float a)
{
    h = fmodf(h, 360.f);
    if(h < 0.f) { h += 360.f; }

    float r = hsl_component(h, s, l, 0);
    float g = hsl_component(h, s, l, 8);
    float b = hsl_component(h, s, l, 4);
    color_init_rgba(color, r, g, b, a);
}

NOVASVG_INLINE unsigned int color_to_rgba32(const color_t* color)
{
    uint32_t r = lroundf(color->r * 255);
    uint32_t g = lroundf(color->g * 255);
    uint32_t b = lroundf(color->b * 255);
    uint32_t a = lroundf(color->a * 255);
    return (r << 24) | (g << 16) | (b << 8) | (a);
}

NOVASVG_INLINE unsigned int color_to_argb32(const color_t* color)
{
    uint32_t a = lroundf(color->a * 255);
    uint32_t r = lroundf(color->r * 255);
    uint32_t g = lroundf(color->g * 255);
    uint32_t b = lroundf(color->b * 255);
    return (a << 24) | (r << 16) | (g << 8) | (b);
}

static inline uint8_t hex_digit(uint8_t c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return 10 + c - 'A';
}

static inline uint8_t hex_byte(uint8_t c1, uint8_t c2)
{
    uint8_t h1 = hex_digit(c1);
    uint8_t h2 = hex_digit(c2);
    return (h1 << 4) | h2;
}

#define MAX_NAME 20
typedef struct {
    const char* name;
    uint32_t value;
} color_entry_t;

static int color_entry_compare(const void* a, const void* b)
{
    const char* name = static_cast<const char*>(a);
    const color_entry_t* entry = static_cast<const color_entry_t*>(b);
    return strcmp(name, entry->name);
}

static bool parse_rgb_component(const char** begin, const char* end, float* component)
{
    float value = 0;
    if(!parse_number(begin, end, &value))
        return false;
    if(skip_delim(begin, end, '%'))
        value *= 2.55f;
    *component = std::clamp(value, 0.f, 255.f) / 255.f;
    return true;
}

static bool parse_alpha_component(const char** begin, const char* end, float* component)
{
    float value = 0;
    if(!parse_number(begin, end, &value))
        return false;
    if(skip_delim(begin, end, '%'))
        value /= 100.f;
    *component = std::clamp(value, 0.f, 1.f);
    return true;
}

NOVASVG_INLINE int color_parse(color_t* color, const char* data, int length)
{
    if(length == -1)
        length = strlen(data);
    const char* it = data;
    const char* end = it + length;
    skip_ws(&it, end);
    if(skip_delim(&it, end, '#')) {
        int r, g, b, a = 255;
        const char* begin = it;
        while(it < end && isxdigit(*it))
            ++it;
        int count = it - begin;
        if(count == 3 || count == 4) {
            r = hex_byte(begin[0], begin[0]);
            g = hex_byte(begin[1], begin[1]);
            b = hex_byte(begin[2], begin[2]);
            if(count == 4) {
                a = hex_byte(begin[3], begin[3]);
            }
        } else if(count == 6 || count == 8) {
            r = hex_byte(begin[0], begin[1]);
            g = hex_byte(begin[2], begin[3]);
            b = hex_byte(begin[4], begin[5]);
            if(count == 8) {
                a = hex_byte(begin[6], begin[7]);
            }
        } else {
            return 0;
        }

        color_init_rgba8(color, r, g, b, a);
    } else {
        int name_length = 0;
        char name[MAX_NAME + 1];
        while(it < end && name_length < MAX_NAME && isalpha(*it))
            name[name_length++] = tolower(*it++);
        name[name_length] = '\0';

        if(strcmp(name, "transparent") == 0) {
            color_init_rgba(color, 0, 0, 0, 0);
        } else if(strcmp(name, "rgb") == 0 || strcmp(name, "rgba") == 0) {
            if(!skip_ws_and_delim(&it, end, '('))
                return 0;
            float r, g, b, a = 1.f;
            if(!parse_rgb_component(&it, end, &r)
                || !skip_ws_and_comma(&it, end)
                || !parse_rgb_component(&it, end, &g)
                || !skip_ws_and_comma(&it, end)
                || !parse_rgb_component(&it, end, &b)) {
                return 0;
            }

            if(skip_ws_and_comma(&it, end)
                && !parse_alpha_component(&it, end, &a)) {
                return 0;
            }

            skip_ws(&it, end);
            if(!skip_delim(&it, end, ')'))
                return 0;
            color_init_rgba(color, r, g, b, a);
        } else if(strcmp(name, "hsl") == 0 || strcmp(name, "hsla") == 0) {
            if(!skip_ws_and_delim(&it, end, '('))
                return 0;
            float h, s, l, a = 1.f;
            if(!parse_number(&it, end, &h)
                || !skip_ws_and_comma(&it, end)
                || !parse_alpha_component(&it, end, &s)
                || !skip_ws_and_comma(&it, end)
                || !parse_alpha_component(&it, end, &l)) {
                return 0;
            }

            if(skip_ws_and_comma(&it, end)
                && !parse_alpha_component(&it, end, &a)) {
                return 0;
            }

            skip_ws(&it, end);
            if(!skip_delim(&it, end, ')'))
                return 0;
            color_init_hsla(color, h, s, l, a);
        } else {
            static const color_entry_t colormap[] = {
                {"aliceblue", 0xF0F8FF},
                {"antiquewhite", 0xFAEBD7},
                {"aqua", 0x00FFFF},
                {"aquamarine", 0x7FFFD4},
                {"azure", 0xF0FFFF},
                {"beige", 0xF5F5DC},
                {"bisque", 0xFFE4C4},
                {"black", 0x000000},
                {"blanchedalmond", 0xFFEBCD},
                {"blue", 0x0000FF},
                {"blueviolet", 0x8A2BE2},
                {"brown", 0xA52A2A},
                {"burlywood", 0xDEB887},
                {"cadetblue", 0x5F9EA0},
                {"chartreuse", 0x7FFF00},
                {"chocolate", 0xD2691E},
                {"coral", 0xFF7F50},
                {"cornflowerblue", 0x6495ED},
                {"cornsilk", 0xFFF8DC},
                {"crimson", 0xDC143C},
                {"cyan", 0x00FFFF},
                {"darkblue", 0x00008B},
                {"darkcyan", 0x008B8B},
                {"darkgoldenrod", 0xB8860B},
                {"darkgray", 0xA9A9A9},
                {"darkgreen", 0x006400},
                {"darkgrey", 0xA9A9A9},
                {"darkkhaki", 0xBDB76B},
                {"darkmagenta", 0x8B008B},
                {"darkolivegreen", 0x556B2F},
                {"darkorange", 0xFF8C00},
                {"darkorchid", 0x9932CC},
                {"darkred", 0x8B0000},
                {"darksalmon", 0xE9967A},
                {"darkseagreen", 0x8FBC8F},
                {"darkslateblue", 0x483D8B},
                {"darkslategray", 0x2F4F4F},
                {"darkslategrey", 0x2F4F4F},
                {"darkturquoise", 0x00CED1},
                {"darkviolet", 0x9400D3},
                {"deeppink", 0xFF1493},
                {"deepskyblue", 0x00BFFF},
                {"dimgray", 0x696969},
                {"dimgrey", 0x696969},
                {"dodgerblue", 0x1E90FF},
                {"firebrick", 0xB22222},
                {"floralwhite", 0xFFFAF0},
                {"forestgreen", 0x228B22},
                {"fuchsia", 0xFF00FF},
                {"gainsboro", 0xDCDCDC},
                {"ghostwhite", 0xF8F8FF},
                {"gold", 0xFFD700},
                {"goldenrod", 0xDAA520},
                {"gray", 0x808080},
                {"green", 0x008000},
                {"greenyellow", 0xADFF2F},
                {"grey", 0x808080},
                {"honeydew", 0xF0FFF0},
                {"hotpink", 0xFF69B4},
                {"indianred", 0xCD5C5C},
                {"indigo", 0x4B0082},
                {"ivory", 0xFFFFF0},
                {"khaki", 0xF0E68C},
                {"lavender", 0xE6E6FA},
                {"lavenderblush", 0xFFF0F5},
                {"lawngreen", 0x7CFC00},
                {"lemonchiffon", 0xFFFACD},
                {"lightblue", 0xADD8E6},
                {"lightcoral", 0xF08080},
                {"lightcyan", 0xE0FFFF},
                {"lightgoldenrodyellow", 0xFAFAD2},
                {"lightgray", 0xD3D3D3},
                {"lightgreen", 0x90EE90},
                {"lightgrey", 0xD3D3D3},
                {"lightpink", 0xFFB6C1},
                {"lightsalmon", 0xFFA07A},
                {"lightseagreen", 0x20B2AA},
                {"lightskyblue", 0x87CEFA},
                {"lightslategray", 0x778899},
                {"lightslategrey", 0x778899},
                {"lightsteelblue", 0xB0C4DE},
                {"lightyellow", 0xFFFFE0},
                {"lime", 0x00FF00},
                {"limegreen", 0x32CD32},
                {"linen", 0xFAF0E6},
                {"magenta", 0xFF00FF},
                {"maroon", 0x800000},
                {"mediumaquamarine", 0x66CDAA},
                {"mediumblue", 0x0000CD},
                {"mediumorchid", 0xBA55D3},
                {"mediumpurple", 0x9370DB},
                {"mediumseagreen", 0x3CB371},
                {"mediumslateblue", 0x7B68EE},
                {"mediumspringgreen", 0x00FA9A},
                {"mediumturquoise", 0x48D1CC},
                {"mediumvioletred", 0xC71585},
                {"midnightblue", 0x191970},
                {"mintcream", 0xF5FFFA},
                {"mistyrose", 0xFFE4E1},
                {"moccasin", 0xFFE4B5},
                {"navajowhite", 0xFFDEAD},
                {"navy", 0x000080},
                {"oldlace", 0xFDF5E6},
                {"olive", 0x808000},
                {"olivedrab", 0x6B8E23},
                {"orange", 0xFFA500},
                {"orangered", 0xFF4500},
                {"orchid", 0xDA70D6},
                {"palegoldenrod", 0xEEE8AA},
                {"palegreen", 0x98FB98},
                {"paleturquoise", 0xAFEEEE},
                {"palevioletred", 0xDB7093},
                {"papayawhip", 0xFFEFD5},
                {"peachpuff", 0xFFDAB9},
                {"peru", 0xCD853F},
                {"pink", 0xFFC0CB},
                {"plum", 0xDDA0DD},
                {"powderblue", 0xB0E0E6},
                {"purple", 0x800080},
                {"rebeccapurple", 0x663399},
                {"red", 0xFF0000},
                {"rosybrown", 0xBC8F8F},
                {"royalblue", 0x4169E1},
                {"saddlebrown", 0x8B4513},
                {"salmon", 0xFA8072},
                {"sandybrown", 0xF4A460},
                {"seagreen", 0x2E8B57},
                {"seashell", 0xFFF5EE},
                {"sienna", 0xA0522D},
                {"silver", 0xC0C0C0},
                {"skyblue", 0x87CEEB},
                {"slateblue", 0x6A5ACD},
                {"slategray", 0x708090},
                {"slategrey", 0x708090},
                {"snow", 0xFFFAFA},
                {"springgreen", 0x00FF7F},
                {"steelblue", 0x4682B4},
                {"tan", 0xD2B48C},
                {"teal", 0x008080},
                {"thistle", 0xD8BFD8},
                {"tomato", 0xFF6347},
                {"turquoise", 0x40E0D0},
                {"violet", 0xEE82EE},
                {"wheat", 0xF5DEB3},
                {"white", 0xFFFFFF},
                {"whitesmoke", 0xF5F5F5},
                {"yellow", 0xFFFF00},
                {"yellowgreen", 0x9ACD32}
            };

            const color_entry_t* entry = static_cast<const color_entry_t*>(bsearch(
                name,
                colormap,
                sizeof(colormap) / sizeof(color_entry_t),
                sizeof(color_entry_t),
                color_entry_compare
            ));
            if(entry == NULL)
                return 0;
            color_init_argb32(color, 0xFF000000 | entry->value);
        }
    }

    skip_ws(&it, end);
    return it - data;
}

static void* paint_create(paint_type_t type, size_t size)
{
    paint_t* paint = static_cast<paint_t*>(malloc(size));
    novasvg_init_reference(paint);
    paint->type = type;
    return paint;
}

NOVASVG_INLINE paint_t* paint_create_rgb(float r, float g, float b)
{
    return paint_create_rgba(r, g, b, 1.f);
}

NOVASVG_INLINE paint_t* paint_create_rgba(float r, float g, float b, float a)
{
    solid_paint_t* solid = static_cast<solid_paint_t*>(paint_create(
        NOVASVG_PAINT_TYPE_COLOR,
        sizeof(solid_paint_t)
    ));
    solid->color.r = std::clamp(r, 0.f, 1.f);
    solid->color.g = std::clamp(g, 0.f, 1.f);
    solid->color.b = std::clamp(b, 0.f, 1.f);
    solid->color.a = std::clamp(a, 0.f, 1.f);
    return &solid->base;
}

NOVASVG_INLINE paint_t* paint_create_color(const color_t* color)
{
    return paint_create_rgba(color->r, color->g, color->b, color->a);
}

static gradient_paint_t* gradient_create(gradient_type_t type, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    gradient_paint_t* gradient = static_cast<gradient_paint_t*>(paint_create(
        NOVASVG_PAINT_TYPE_GRADIENT,
        sizeof(gradient_paint_t) + nstops * sizeof(gradient_stop_t)
    ));
    gradient->type = type;
    gradient->spread = spread;
    gradient->matrix = matrix ? *matrix : NOVASVG_IDENTITY_MATRIX;
    gradient->stops = (gradient_stop_t*)(gradient + 1);
    gradient->nstops = nstops;

    float prev_offset = 0.f;
    for(int i = 0; i < nstops; ++i) {
        const gradient_stop_t* stop = stops + i;
        gradient->stops[i].offset = std::max(prev_offset, std::clamp(stop->offset, 0.f, 1.f));
        gradient->stops[i].color.r = std::clamp(stop->color.r, 0.f, 1.f);
        gradient->stops[i].color.g = std::clamp(stop->color.g, 0.f, 1.f);
        gradient->stops[i].color.b = std::clamp(stop->color.b, 0.f, 1.f);
        gradient->stops[i].color.a = std::clamp(stop->color.a, 0.f, 1.f);
        prev_offset = gradient->stops[i].offset;
    }

    return gradient;
}

NOVASVG_INLINE paint_t* paint_create_linear_gradient(float x1, float y1, float x2, float y2, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    gradient_paint_t* gradient = gradient_create(NOVASVG_GRADIENT_TYPE_LINEAR, spread, stops, nstops, matrix);
    gradient->values[0] = x1;
    gradient->values[1] = y1;
    gradient->values[2] = x2;
    gradient->values[3] = y2;
    return &gradient->base;
}

NOVASVG_INLINE paint_t* paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr, spread_method_t spread, const gradient_stop_t* stops, int nstops, const matrix_t* matrix)
{
    gradient_paint_t* gradient = gradient_create(NOVASVG_GRADIENT_TYPE_RADIAL, spread, stops, nstops, matrix);
    gradient->values[0] = cx;
    gradient->values[1] = cy;
    gradient->values[2] = cr;
    gradient->values[3] = fx;
    gradient->values[4] = fy;
    gradient->values[5] = fr;
    return &gradient->base;
}

NOVASVG_INLINE paint_t* paint_create_texture(surface_t* surface, texture_type_t type, float opacity, const matrix_t* matrix)
{
    texture_paint_t* texture = static_cast<texture_paint_t*>(paint_create(
        NOVASVG_PAINT_TYPE_TEXTURE,
        sizeof(texture_paint_t)
    ));
    texture->type = type;
    texture->opacity = std::clamp(opacity, 0.f, 1.f);
    texture->matrix = matrix ? *matrix : NOVASVG_IDENTITY_MATRIX;
    texture->surface = surface_reference(surface);
    return &texture->base;
}

NOVASVG_INLINE paint_t* paint_reference(paint_t* paint)
{
    novasvg_increment_reference(paint);
    return paint;
}

NOVASVG_INLINE void paint_destroy(paint_t* paint)
{
    if(novasvg_destroy_reference(paint)) {
        if(paint->type == NOVASVG_PAINT_TYPE_TEXTURE) {
            texture_paint_t* texture = (texture_paint_t*)(paint);
            surface_destroy(texture->surface);
        }

        free(paint);
    }
}

NOVASVG_INLINE int paint_get_reference_count(const paint_t* paint)
{
    return novasvg_get_reference_count(paint);
}


} // namespace render
} // namespace novasvg