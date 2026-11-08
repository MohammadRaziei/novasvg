#pragma once

// Small CSS/text helpers shared by the core and the built-in foreignObject renderer: scanning a
// `property: value;` block, CSS colours, a number with an optional px unit, UTF-8 to UTF-32.

#include "svg_impl.h"

namespace nova::svg {
using namespace vg;

inline bool strip_important_flag(std::string& value)
{
    auto bang = value.rfind('!');
    if(bang == std::string::npos)
        return false;
    std::string_view flag(value);
    flag.remove_prefix(bang + 1);
    skip_optional_spaces(flag);
    strip_trailing_spaces(flag);
    if(flag.size() != 9)
        return false;
    for(size_t i = 0; i < flag.size(); ++i) {
        if(std::tolower(static_cast<unsigned char>(flag[i])) != "important"[i])
            return false;
    }

    value.erase(bang);
    while(!value.empty() && IS_WS(value.back()))
        value.pop_back();
    return true;
}


inline std::u32string utf8_to_u32(const std::string& text)
{
    std::u32string result;
    result.reserve(text.size());
    text_iterator_t it;
    text_iterator_init(&it, text.data(), (int)text.length(), text_encoding::utf8);
    while(text_iterator_has_next(&it))
        result.push_back(text_iterator_next(&it));
    return result;
}

// Pulls a "<property>: <value>" declaration's raw value text out of a CSS
// declaration block (the inside of a style="..." attribute, or the
// inside of a rule's { ... }). Not a general CSS-value parser -- just
// finds and trims the text between the property name and the next
// `;`/`}`/quote. `property` match is boundary-checked (not
// preceded/followed by identifier characters) so searching for "color"
// doesn't false-match inside "background-color", or "height" inside
// "line-height". Used by parseNumericDeclaration() below and by the
// white-space lookup.
inline std::optional<std::string_view> find_declaration_value(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto match_start = pos;
        auto after = pos + property.size();
        pos = after;
        if(match_start > 0) {
            auto prev = block[match_start - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue; // part of a longer property name
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue; // property name continues past what we matched, e.g. "line-height" vs "height"

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;
        auto end = block.find_first_of(";}\"'", after);
        auto value = block.substr(after, (end == std::string_view::npos ? block.size() : end) - after);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// Same idea as findDeclarationValue(), but scans to the next *top-level*
// `;`/`}` instead of stopping at the first quote -- needed for
// `@font-face`'s `src:` value, which legitimately contains both quotes
// and commas (`url(...) format("truetype"), url(...) format("woff2")`).
// Parens are tracked so a `;`/`}` inside a url()/format() call (neither
// occurs in valid CSS, but malformed input shouldn't cut the scan short)
// doesn't end the value early either.
inline std::optional<std::string_view> find_raw_declaration_value(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto match_start = pos;
        auto after = pos + property.size();
        pos = after;
        if(match_start > 0) {
            auto prev = block[match_start - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue;
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue;

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;

        auto value_start = after;
        auto end = after;
        int paren_depth = 0;
        while(end < block.size()) {
            auto ch = block[end];
            if(ch == '(') {
                // `url(data:...)` holds the whole embedded font. Nothing inside it ends the value, so when
                // the run up to the next `)` has no `(` in it, jump over it in one go.
                if(paren_depth == 0) {
                    const char* base = block.data() + end + 1;
                    const size_t remaining = block.size() - end - 1;
                    if(auto* close = static_cast<const char*>(std::memchr(base, ')', remaining))) {
                        if(std::memchr(base, '(', size_t(close - base)) == nullptr) {
                            end = size_t(close - block.data()) + 1;
                            continue;
                        }
                    }
                }
                ++paren_depth;
            } else if(ch == ')') {
                if(paren_depth > 0)
                    --paren_depth;
            } else if((ch == ';' || ch == '}') && paren_depth == 0) {
                break;
            }
            ++end;
        }

        auto value = block.substr(value_start, end - value_start);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// A CSS colour value (`#f9f`, `rgb(..)`, `hsl(..)`, `rgba(..)`, a name) -> stb::color; nullopt if it isn't one.
inline std::optional<stb::color> parse_css_color(std::string_view value)
{
    color_t color_;
    int length_ = color_parse(&color_, value.data(), (int)value.length());
    if(length_ == 0)
        return std::nullopt;
    auto argb = color_to_argb32(&color_);
    return stb::color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
}

// A parsed numeric CSS value: `raw` is the number itself, `isPixels`
// says whether it carried a "px" suffix (an absolute length) as opposed
// to a bare multiplier (e.g. line-height's "1.5"). Anything else
// (%, em, "normal", ...) isn't handled -- callers fall back to their own
// default for those, same as when the property is missing entirely.
struct numeric_declaration {
    float raw;
    bool is_pixels;
};

inline std::optional<numeric_declaration> parse_numeric_declaration(std::string_view block, std::string_view property)
{
    auto value = find_declaration_value(block, property);
    if(!value)
        return std::nullopt;

    char* end = nullptr;
    std::string value_str(*value);
    auto number = std::strtof(value_str.c_str(), &end);
    if(end == value_str.c_str())
        return std::nullopt; // no leading number at all, e.g. "normal"

    auto unit = value_str.c_str() + (end - value_str.c_str());
    while(*unit == ' ')
        ++unit;
    return numeric_declaration{number, std::string_view(unit) == "px"};
}


} // namespace nova::svg
