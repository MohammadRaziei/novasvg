#pragma once

#include <optional>
#include <string_view>

#include <stb/color.hpp>

#include "nova/svg/foreign_object.h"

namespace nova::svg {

/// What the CSS gives the HTML of one foreignObject: the text colour and the box behind it. Unset = the CSS
/// did not ask (the text is then black and no box is painted).
struct html_style {
    std::optional<stb::color> text_color;
    std::optional<stb::color> background_color;
};

html_style resolve_html_style(std::string_view markup, const css_query& css);

} // namespace nova::svg
