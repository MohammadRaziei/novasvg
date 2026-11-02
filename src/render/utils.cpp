#include "novasvg/detail/render/utils.h"

namespace novasvg {
namespace render {

uint32_t premultiply_argb(uint32_t color)
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

bool parse_number(const char** begin, const char* end, float* number)
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

bool skip_delim(const char** begin, const char* end, const char delim)
{
    const char* it = *begin;
    if(it < end && *it == delim) {
        *begin = it + 1;
        return true;
    }

    return false;
}

bool skip_string(const char** begin, const char* end, const char* data)
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

bool skip_ws(const char** begin, const char* end)
{
    const char* it = *begin;
    while(it < end && NOVASVG_IS_WS(*it))
        ++it;
    *begin = it;
    return it < end;
}

bool skip_ws_and_delim(const char** begin, const char* end, char delim)
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

bool skip_ws_and_comma(const char** begin, const char* end)
{
    return skip_ws_and_delim(begin, end, ',');
}

bool skip_ws_or_delim(const char** begin, const char* end, char delim, bool* has_delim)
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

bool skip_ws_or_comma(const char** begin, const char* end, bool* has_comma)
{
    return skip_ws_or_delim(begin, end, ',', has_comma);
}

void skip_css_unit(const char** begin, const char* end, float* number)
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
