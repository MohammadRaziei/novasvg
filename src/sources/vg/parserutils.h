#ifndef NOVA_SVGPARSERUTILS_H
#define NOVA_SVGPARSERUTILS_H

#include <cmath>
#include <string_view>
#include <limits>

namespace nova::vg {

constexpr bool IS_NUM(int cc) { return cc >= '0' && cc <= '9'; }
constexpr bool IS_ALPHA(int cc) { return (cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z'); }
constexpr bool IS_WS(int cc) { return cc == ' ' || cc == '\t' || cc == '\n' || cc == '\r'; }
constexpr bool IS_CSS_STARTNAMECHAR(int c) { return IS_ALPHA(c) || c == '_' || c == '-'; }
constexpr bool IS_CSS_NAMECHAR(int c) { return IS_CSS_STARTNAMECHAR(c) || IS_NUM(c); }

constexpr void strip_leading_spaces(std::string_view& input)
{
    while(!input.empty() && IS_WS(input.front())) {
        input.remove_prefix(1);
    }
}

constexpr void strip_trailing_spaces(std::string_view& input)
{
    while(!input.empty() && IS_WS(input.back())) {
        input.remove_suffix(1);
    }
}

constexpr void strip_leading_and_trailing_spaces(std::string_view& input)
{
    strip_leading_spaces(input);
    strip_trailing_spaces(input);
}

constexpr bool skip_optional_spaces(std::string_view& input)
{
    while(!input.empty() && IS_WS(input.front()))
        input.remove_prefix(1);
    return !input.empty();
}

// Spaces, then at most one delimiter, then spaces. False when nothing is left or nothing was skipped; `has_delimiter`
// (if given) says whether the delimiter itself was there.
constexpr bool skip_optional_spaces_or_delimiter(std::string_view& input, char delimiter, bool* has_delimiter = nullptr)
{
    if(has_delimiter)
        *has_delimiter = false;
    if(!input.empty() && !IS_WS(input.front()) && delimiter != input.front())
        return false;
    if(skip_optional_spaces(input)) {
        if(delimiter == input.front()) {
            input.remove_prefix(1);
            if(has_delimiter)
                *has_delimiter = true;
            skip_optional_spaces(input);
        }
    }

    return !input.empty();
}

constexpr bool skip_optional_spaces_or_comma(std::string_view& input, bool* has_comma = nullptr)
{
    return skip_optional_spaces_or_delimiter(input, ',', has_comma);
}

constexpr bool skip_delimiter_and_optional_spaces(std::string_view& input, char delimiter)
{
    if(!input.empty() && input.front() == delimiter) {
        input.remove_prefix(1);
        skip_optional_spaces(input);
        return true;
    }

    return false;
}

constexpr bool skip_delimiter(std::string_view& input, char delimiter)
{
    if(!input.empty() && input.front() == delimiter) {
        input.remove_prefix(1);
        return true;
    }

    return false;
}

constexpr bool skip_string(std::string_view& input, std::string_view value)
{
    if(input.size() >= value.size() && value == input.substr(0, value.size())) {
        input.remove_prefix(value.size());
        return true;
    }

    return false;
}

constexpr bool is_integral_digit(char ch, int base)
{
    if(IS_NUM(ch))
        return ch - '0' < base;
    if(IS_ALPHA(ch))
        return (ch >= 'a' && ch < 'a' + base - 10) || (ch >= 'A' && ch < 'A' + base - 10);
    return false;
}

constexpr int to_integral_digit(char ch)
{
    if(IS_NUM(ch))
        return ch - '0';
    if(ch >= 'a')
        return ch - 'a' + 10;
    return ch - 'A' + 10;
}

template<typename T>
inline bool parse_integer(std::string_view& input, T& integer, int base = 10)
{
    constexpr bool is_signed = std::numeric_limits<T>::is_signed;
    constexpr T int_max = std::numeric_limits<T>::max();
    const T max_multiplier = int_max / static_cast<T>(base);

    T value = 0;
    bool is_negative = false;

    if(!input.empty() && input.front() == '+') {
        input.remove_prefix(1);
    } else if(!input.empty() && is_signed && input.front() == '-') {
        input.remove_prefix(1);
        is_negative = true;
    }

    if(input.empty() || !is_integral_digit(input.front(), base))
        return false;
    do {
        const int digit_value = to_integral_digit(input.front());
        if(value > max_multiplier || (value == max_multiplier && static_cast<T>(digit_value) > (int_max % static_cast<T>(base)) + is_negative))
            return false;
        value = static_cast<T>(base) * value + static_cast<T>(digit_value);
        input.remove_prefix(1);
    } while(!input.empty() && is_integral_digit(input.front(), base));

    using signed_type = typename std::make_signed<T>::type;
    if(is_negative)
        integer = -static_cast<signed_type>(value);
    else
        integer = value;
    return true;
}

template<typename T>
inline bool parse_number_unchecked(std::string_view& input, T& number)
{
    constexpr T max_value = std::numeric_limits<T>::max();
    T integer = 0;
    T fraction = 0;
    int exponent = 0;
    int sign = 1;
    int expsign = 1;

    if(!input.empty() && input.front() == '+') {
        input.remove_prefix(1);
    } else if(!input.empty() && input.front() == '-') {
        input.remove_prefix(1);
        sign = -1;
    }

    if(input.empty() || (!IS_NUM(input.front()) && input.front() != '.'))
        return false;
    if(IS_NUM(input.front())) {
        do {
            integer = static_cast<T>(10) * integer + (input.front() - '0');
            input.remove_prefix(1);
        } while(!input.empty() && IS_NUM(input.front()));
    }

    if(!input.empty() && input.front() == '.') {
        input.remove_prefix(1);
        if(input.empty() || !IS_NUM(input.front()))
            return false;
        T divisor = static_cast<T>(1);
        do {
            fraction = static_cast<T>(10) * fraction + (input.front() - '0');
            divisor *= static_cast<T>(10);
            input.remove_prefix(1);
        } while(!input.empty() && IS_NUM(input.front()));
        fraction /= divisor;
    }

    if(input.size() > 1 && (input[0] == 'e' || input[0] == 'E')
        && (input[1] != 'x' && input[1] != 'm'))
    {
        input.remove_prefix(1);
        if(!input.empty() && input.front() == '+')
            input.remove_prefix(1);
        else if(!input.empty() && input.front() == '-') {
            input.remove_prefix(1);
            expsign = -1;
        }

        if(input.empty() || !IS_NUM(input.front()))
            return false;
        do {
            exponent = 10 * exponent + (input.front() - '0');
            input.remove_prefix(1);
        } while(!input.empty() && IS_NUM(input.front()));
    }

    number = sign * (integer + fraction);
    if(exponent)
        number *= static_cast<T>(std::pow(10.0, expsign * exponent));
    return number >= -max_value && number <= max_value;
}

// The number at the start of `input` (SVG number syntax, an exponent only when no CSS unit starts with the e). On failure
// `input` and `number` are as they were.
template<typename T>
inline bool parse_number(std::string_view& input, T& number)
{
    std::string_view rest = input;
    T value = 0;
    if(!parse_number_unchecked(rest, value))
        return false;
    input = rest;
    number = value;
    return true;
}

} // namespace nova::vg

#endif // NOVA_SVGPARSERUTILS_H
