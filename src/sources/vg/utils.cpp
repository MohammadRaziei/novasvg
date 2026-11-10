#include "utils.h"
#include <string_view>

namespace nova {
namespace vg {

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

    if(it >= end || (*it != '.' && !NOVA_IS_NUM(*it)))
        return false;
    if(NOVA_IS_NUM(*it)) {
        do {
            integer = 10.f * integer + (*it++ - '0');
        } while(it < end && NOVA_IS_NUM(*it));
    }

    if(it < end && *it == '.') {
        ++it;
        if(it >= end || !NOVA_IS_NUM(*it))
            return false;
        float divisor = 1.f;
        do {
            fraction = 10.f * fraction + (*it++ - '0');
            divisor *= 10.f;
        } while(it < end && NOVA_IS_NUM(*it));
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

        if(it >= end || !NOVA_IS_NUM(*it))
            return false;
        do {
            exponent = 10 * exponent + (*it++ - '0');
        } while(it < end && NOVA_IS_NUM(*it));
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
    while(it < end && NOVA_IS_WS(*it))
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
    const char* unit_start = it;
    while(it < end && ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || *it == '%'))
        ++it;
    if(it == unit_start)
        return;

    const std::string_view unit(unit_start, size_t(it - unit_start));
    if(unit == "rad")
        *number *= 180.f / 3.14159265f;
    else if(unit == "grad")
        *number *= 0.9f;
    else if(unit == "turn")
        *number *= 360.f;
    // "deg", "px", "pt", "%", and anything else: value already usable as-is.

    *begin = it;
}

// Decodes base64 text (image data: URIs and @font-face data) into a malloc'd buffer; the caller free()s it.
uint8_t* base64_decode(const char* data, int length_, size_t* out_length)
{
    // 0..63 = the sextet a character stands for; kWs = whitespace (skipped); kPad = '='; kBad = anything else.
    enum : uint8_t { k_ws = 0xFD, k_pad = 0xFE, k_bad = 0xFF };
    static const struct table {
        uint8_t v[256];
        table()
        {
            for(int i = 0; i < 256; ++i)
                v[i] = k_bad;
            for(int i = 0; i < 26; ++i) {
                v['A' + i] = uint8_t(i);
                v['a' + i] = uint8_t(26 + i);
            }
            for(int i = 0; i < 10; ++i)
                v['0' + i] = uint8_t(52 + i);
            v[uint8_t('+')] = 62;
            v[uint8_t('/')] = 63;
            v[uint8_t('=')] = k_pad;
            for(int c : {' ', '\t', '\n', '\r', '\f', '\v'})
                v[c] = k_ws;
        }
    } table_;

    *out_length = 0;
    if(length_ == -1)
        length_ = int(strlen(data));
    if(length_ <= 0)
        return NULL;
    // Every 4 input characters make 3 bytes, so `length` bytes is always enough room.
    auto* output_data = static_cast<uint8_t*>(malloc(size_t(length_)));
    if(output_data == NULL)
        return NULL;

    // Collect sextets and write finished bytes straight away. `acc` holds the bits not yet written.
    size_t didx = 0;
    size_t count = 0;        // sextets seen
    size_t pads = 0;         // '=' seen
    uint32_t acc = 0;
    int bits = 0;
    for(int i = 0; i < length_; ++i) {
        // Fast path: a clean group of four sextets (no padding or whitespace so far, bits aligned) is
        // three bytes, with nothing to track between them.
        if(bits == 0 && pads == 0 && i + 4 <= length_) {
            const uint8_t a = table_.v[uint8_t(data[i])], b = table_.v[uint8_t(data[i + 1])];
            const uint8_t c = table_.v[uint8_t(data[i + 2])], d = table_.v[uint8_t(data[i + 3])];
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

        const uint8_t v = table_.v[uint8_t(data[i])];
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
        } else if(v == k_pad) {
            ++pads;
        } else if(v == k_bad) {
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

} // namespace vg
} // namespace nova
