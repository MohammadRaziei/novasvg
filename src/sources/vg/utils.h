#pragma once

#ifndef NOVA_UTILS_H
#define NOVA_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <algorithm>

#include "nova/config.h"

namespace nova {
namespace vg {

#define NOVA_IS_NUM(c) ((c) >= '0' && (c) <= '9')
#define NOVA_IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define NOVA_IS_ALNUM(c) (NOVA_IS_ALPHA(c) || NOVA_IS_NUM(c))
#define NOVA_IS_WS(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

// min/max/clamp were plain macros here (novasvg_min/max/clamp) -- pure
// numeric helpers with an exact STL equivalent, so they're gone in favor
// of std::min/std::max/std::clamp at every call site instead of being
// reimplemented. What's left below (alpha/red/green/blue channel
// extraction, the dynamic array, the mutex wrapper, the refcounting
// helpers) has no STL equivalent doing the same job, so those stay as
// genuinely-own, C-style, prefixed macros/functions.

#define novasvg_alpha(c) (((c) >> 24) & 0xff)
#define novasvg_red(c) (((c) >> 16) & 0xff)
#define novasvg_green(c) (((c) >> 8) & 0xff)
#define novasvg_blue(c) (((c) >> 0) & 0xff)

#define novasvg_array_init(array) \
    do { \
        (array).data = NULL; \
        (array).size = 0; \
        (array).capacity = 0; \
    } while(0)

#ifdef __cplusplus
#define NOVA_ARRAY_CAST(array, value) static_cast<decltype((array).data)>(value)
#else
#define NOVA_ARRAY_CAST(array, value) (value)
#endif

#define novasvg_array_ensure(array, count) \
    do { \
        if((array).size + (count) > (array).capacity) { \
            int capacity = (array).size + (count); \
            int newcapacity = (array).capacity == 0 ? 8 : (array).capacity; \
            while(newcapacity < capacity) { newcapacity *= 2; } \
            (array).data = NOVA_ARRAY_CAST(array, realloc((array).data, newcapacity * sizeof((array).data[0]))); \
            (array).capacity = newcapacity; \
        } \
    } while(0)

#define novasvg_array_append_data(array, newdata, count) \
    do { \
        if(newdata && count > 0) { \
            novasvg_array_ensure(array, count); \
            memcpy((array).data + (array).size, newdata, (count) * sizeof((newdata)[0])); \
            (array).size += count; \
        } \
    } while(0)

#define novasvg_array_append(array, other) novasvg_array_append_data(array, (other).data, (other).size)
#define novasvg_array_clear(array) ((array).size = 0)
#define novasvg_array_destroy(array) free((array).data)

uint32_t premultiply_argb(uint32_t color);

bool parse_number(const char** begin, const char* end, float* number);

bool skip_delim(const char** begin, const char* end, const char delim);

bool skip_string(const char** begin, const char* end, const char* data);

bool skip_ws(const char** begin, const char* end);

bool skip_ws_and_delim(const char** begin, const char* end, char delim);

bool skip_ws_and_comma(const char** begin, const char* end);

bool skip_ws_or_delim(const char** begin, const char* end, char delim, bool* has_delim);

bool skip_ws_or_comma(const char** begin, const char* end, bool* has_comma);

// ponytail: CSS transform values (e.g. `rotate(15deg)`, set via a CSS
// class/style rather than the plain svg presentation attribute) carry a
// unit immediately after the number, which the svg attribute grammar
// above doesn't expect. Rather than reject the whole transform list on
// the first such token (the previous behavior), convert angle units to
// degrees in place and just skip length units (px and friends -- treating
// them as raw user-units is an acceptable simplification for translate).
// Upgrade path: consult the target el's viewport/font-size for true
// unit conversion (%, em, ...) if that ever matters in practice.
void skip_css_unit(const char** begin, const char* end, float* number);

} // namespace vg
} // namespace nova

#endif // NOVA_UTILS_H
