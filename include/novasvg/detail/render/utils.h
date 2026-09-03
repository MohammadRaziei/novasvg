#pragma once

#ifndef NOVASVG_UTILS_H
#define NOVASVG_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <algorithm>

namespace novasvg {
namespace render {

#define NOVASVG_IS_NUM(c) ((c) >= '0' && (c) <= '9')
#define NOVASVG_IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define NOVASVG_IS_ALNUM(c) (NOVASVG_IS_ALPHA(c) || NOVASVG_IS_NUM(c))
#define NOVASVG_IS_WS(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

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
#define NOVASVG_ARRAY_CAST(array, value) static_cast<decltype((array).data)>(value)
#else
#define NOVASVG_ARRAY_CAST(array, value) (value)
#endif

#define novasvg_array_ensure(array, count) \
    do { \
        if((array).size + (count) > (array).capacity) { \
            int capacity = (array).size + (count); \
            int newcapacity = (array).capacity == 0 ? 8 : (array).capacity; \
            while(newcapacity < capacity) { newcapacity *= 2; } \
            (array).data = NOVASVG_ARRAY_CAST(array, realloc((array).data, newcapacity * sizeof((array).data[0]))); \
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

NOVASVG_INLINE uint32_t premultiply_argb(uint32_t color)
{
    uint32_t a = novasvg_alpha(color);
    uint32_t r = novasvg_red(color);
    uint32_t g = novasvg_green(color);
    uint32_t b = novasvg_blue(color);
    if(a != 255) {
        r = (r * a) / 255;
        g = (g * a) / 255;
        b = (b * a) / 255;
    }

    return (a << 24) | (r << 16) | (g << 8) | (b);
}

NOVASVG_INLINE bool parse_number(const char** begin, const char* end, float* number)
{
    const char* it = *begin;
    float integer = 0;
    float fraction = 0;
    float exponent = 0;
    int sign = 1;
    int expsign = 1;

    if(it < end && *it == '+') {
        ++it;
    } else if(it < end && *it == '-') {
        ++it;
        sign = -1;
    }

    if(it >= end || (*it != '.' && !NOVASVG_IS_NUM(*it)))
        return false;
    if(NOVASVG_IS_NUM(*it)) {
        do {
            integer = 10.f * integer + (*it++ - '0');
        } while(it < end && NOVASVG_IS_NUM(*it));
    }

    if(it < end && *it == '.') {
        ++it;
        if(it >= end || !NOVASVG_IS_NUM(*it))
            return false;
        float divisor = 1.f;
        do {
            fraction = 10.f * fraction + (*it++ - '0');
            divisor *= 10.f;
        } while(it < end && NOVASVG_IS_NUM(*it));
        fraction /= divisor;
    }

    if(it < end && (*it == 'e' || *it == 'E')) {
        ++it;
        if(it < end && *it == '+') {
            ++it;
        } else if(it < end && *it == '-') {
            ++it;
            expsign = -1;
        }

        if(it >= end || !NOVASVG_IS_NUM(*it))
            return false;
        do {
            exponent = 10 * exponent + (*it++ - '0');
        } while(it < end && NOVASVG_IS_NUM(*it));
    }

    *begin = it;
    *number = sign * (integer + fraction);
    if(exponent)
        *number *= powf(10.f, expsign * exponent);
    return *number >= -FLT_MAX && *number <= FLT_MAX;
}

NOVASVG_INLINE bool skip_delim(const char** begin, const char* end, const char delim)
{
    const char* it = *begin;
    if(it < end && *it == delim) {
        *begin = it + 1;
        return true;
    }

    return false;
}

NOVASVG_INLINE bool skip_string(const char** begin, const char* end, const char* data)
{
    const char* it = *begin;
    while(it < end && *data && *it == *data) {
        ++data;
        ++it;
    }

    if(*data == '\0') {
        *begin = it;
        return true;
    }

    return false;
}

NOVASVG_INLINE bool skip_ws(const char** begin, const char* end)
{
    const char* it = *begin;
    while(it < end && NOVASVG_IS_WS(*it))
        ++it;
    *begin = it;
    return it < end;
}

NOVASVG_INLINE bool skip_ws_and_delim(const char** begin, const char* end, char delim)
{
    const char* it = *begin;
    if(skip_ws(&it, end)) {
        if(!skip_delim(&it, end, delim))
            return false;
        skip_ws(&it, end);
    } else {
        return false;
    }

    *begin = it;
    return it < end;
}

NOVASVG_INLINE bool skip_ws_and_comma(const char** begin, const char* end)
{
    return skip_ws_and_delim(begin, end, ',');
}

NOVASVG_INLINE bool skip_ws_or_delim(const char** begin, const char* end, char delim, bool* has_delim)
{
    const char* it = *begin;
    if(has_delim)
        *has_delim = false;
    if(skip_ws(&it, end)) {
        if(skip_delim(&it, end, delim)) {
            if(has_delim)
                *has_delim = true;
            skip_ws(&it, end);
        }
    }

    if(it == *begin)
        return false;
    *begin = it;
    return it < end;
}

NOVASVG_INLINE bool skip_ws_or_comma(const char** begin, const char* end, bool* has_comma)
{
    return skip_ws_or_delim(begin, end, ',', has_comma);
}

// ponytail: CSS transform values (e.g. `rotate(15deg)`, set via a CSS
// class/style rather than the plain SVG presentation attribute) carry a
// unit immediately after the number, which the SVG attribute grammar
// above doesn't expect. Rather than reject the whole transform list on
// the first such token (the previous behavior), convert angle units to
// degrees in place and just skip length units (px and friends -- treating
// them as raw user-units is an acceptable simplification for translate).
// Upgrade path: consult the target element's viewport/font-size for true
// unit conversion (%, em, ...) if that ever matters in practice.
NOVASVG_INLINE void skip_css_unit(const char** begin, const char* end, float* number)
{
    const char* it = *begin;
    const char* unitStart = it;
    while(it < end && ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || *it == '%'))
        ++it;
    if(it == unitStart)
        return;

    auto length = size_t(it - unitStart);
    auto matches = [&](const char* unit) {
        auto unitLength = strlen(unit);
        return length == unitLength && strncmp(unitStart, unit, unitLength) == 0;
    };

    if(matches("rad"))
        *number *= 180.f / 3.14159265f;
    else if(matches("grad"))
        *number *= 0.9f;
    else if(matches("turn"))
        *number *= 360.f;
    // "deg", "px", "pt", "%", and anything else: value already usable as-is.

    *begin = it;
}

} // namespace render
} // namespace novasvg

#endif // NOVASVG_UTILS_H
