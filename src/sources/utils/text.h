#pragma once

// Small text helpers shared by the svg and vg layers (ASCII, locale independent).

#include <cctype>
#include <string>
#include <string_view>

namespace nova::utils {

inline bool is_space(char c) noexcept
{
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

inline std::string_view ltrim(std::string_view v) noexcept
{
    while(!v.empty() && is_space(v.front()))
        v.remove_prefix(1);
    return v;
}

inline std::string_view rtrim(std::string_view v) noexcept
{
    while(!v.empty() && is_space(v.back()))
        v.remove_suffix(1);
    return v;
}

inline std::string_view trim(std::string_view v) noexcept
{
    return ltrim(rtrim(v));
}

inline char to_lower(char c) noexcept
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

inline std::string to_lower(std::string_view v)
{
    std::string result(v);
    for(char& c : result)
        c = to_lower(c);
    return result;
}

} // namespace nova::utils
