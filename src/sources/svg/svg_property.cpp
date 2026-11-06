#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_property::svg_property(property_id id)
    : m_id(id)
{
}

bool svg_string::parse(std::string_view input)
{
    strip_leading_and_trailing_spaces(input);
    m_value.assign(input);
    return true;
}

bool svg_angle::parse(std::string_view input)
{
    strip_leading_and_trailing_spaces(input);
    if(input == "auto") {
        m_value = 0.f;
        m_orient_type = OrientType::Auto;
        return true;
    }

    if(input == "auto-start-reverse") {
        m_value = 0.f;
        m_orient_type = OrientType::auto_start_reverse;
        return true;
    }

    float value = 0.f;
    if(!parseNumber(input, value))
        return false;
    if(!input.empty()) {
        if(input == "rad")
            value *= 180.f / NOVA_PI;
        else if(input == "grad")
            value *= 360.f / 400.f;
        else if(input == "turn")
            value *= 360.f;
        else if(input != "deg") {
            return false;
        }
    }

    m_value = value;
    m_orient_type = OrientType::Angle;
    return true;
}

bool Length::parse(std::string_view input, length_negative_mode mode)
{
    float value = 0.f;
    strip_leading_and_trailing_spaces(input);
    if(!parseNumber(input, value))
        return false;
    if(value < 0.f && mode == length_negative_mode::forbid)
        return false;
    if(input.empty()) {
        m_value = value;
        m_units = length_units::None;
        return true;
    }

    constexpr auto dpi = 96.f;
    switch(input.front()) {
    case '%':
        m_value = value;
        m_units = length_units::percent;
        input.remove_prefix(1);
        break;
    case 'p':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'x')
            m_value = value;
        else if(input.front() == 'c')
            m_value = value * dpi / 6.f;
        else if(input.front() == 't')
            m_value = value * dpi / 72.f;
        else
            return false;
        m_units = length_units::Px;
        input.remove_prefix(1);
        break;
    case 'i':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'n')
            m_value = value * dpi;
        else
            return false;
        m_units = length_units::Px;
        input.remove_prefix(1);
        break;
    case 'c':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_value = value * dpi / 2.54f;
        else
            return false;
        m_units = length_units::Px;
        input.remove_prefix(1);
        break;
    case 'm':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_value = value * dpi / 25.4f;
        else
            return false;
        m_units = length_units::Px;
        input.remove_prefix(1);
        break;
    case 'e':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_units = length_units::em;
        else if(input.front() == 'x')
            m_units = length_units::ex;
        else
            return false;
        m_value = value;
        input.remove_prefix(1);
        break;
    default:
        return false;
    }

    return input.empty();
}

float LengthContext::value_for_length(const Length& length, length_direction direction) const
{
    if(length.units() == length_units::percent) {
        if(m_units == Units::user_space_on_use)
            return length.value() * viewport_dimension(direction) / 100.f;
        return length.value() / 100.f;
    }

    if(length.units() == length_units::ex)
        return length.value() * m_element->font_size() / 2.f;
    if(length.units() == length_units::em)
        return length.value() * m_element->font_size();
    return length.value();
}

float LengthContext::viewport_dimension(length_direction direction) const
{
    auto viewport_size = m_element->current_viewport_size();
    switch(direction) {
    case length_direction::Horizontal:
        return viewport_size.w;
    case length_direction::vertical:
        return viewport_size.h;
    default:
        return std::sqrt(viewport_size.w * viewport_size.w + viewport_size.h * viewport_size.h) / NOVA_SQRT2;
    }
}

bool svg_length::parse(std::string_view input)
{
    return m_value.parse(input, m_negative_mode);
}

bool svg_length_list::parse(std::string_view input)
{
    m_values.clear();
    while(!input.empty()) {
        size_t count = 0;
        while(count < input.length() && input[count] != ',' && !IS_WS(input[count]))
            ++count;
        if(count == 0)
            break;
        Length value(0, length_units::None);
        if(!value.parse(input.substr(0, count), m_negative_mode))
            return false;
        input.remove_prefix(count);
        skip_optional_spaces_or_comma(input);
        m_values.push_back(value);
    }

    return true;
}

bool svg_number::parse(std::string_view input)
{
    float value = 0.f;
    strip_leading_and_trailing_spaces(input);
    if(!parseNumber(input, value))
        return false;
    if(!input.empty())
        return false;
    m_value = value;
    return true;
}

bool svg_number_percentage::parse(std::string_view input)
{
    float value = 0.f;
    strip_leading_and_trailing_spaces(input);
    if(!parseNumber(input, value))
        return false;
    if(!input.empty() && input.front() == '%') {
        value /= 100.f;
        input.remove_prefix(1);
    }

    if(!input.empty())
        return false;
    m_value = std::clamp(value, 0.f, 1.f);
    return true;
}

bool svg_number_list::parse(std::string_view input)
{
    m_values.clear();
    strip_leading_spaces(input);
    while(!input.empty()) {
        float value = 0.f;
        if(!parseNumber(input, value))
            return false;
        skip_optional_spaces_or_comma(input);
        m_values.push_back(value);
    }

    return true;
}

bool svg_path::parse(std::string_view input)
{
    return parse_path(m_value, input.data(), input.length());
}

bool svg_point::parse(std::string_view input)
{
    Point value;
    strip_leading_and_trailing_spaces(input);
    if(!parseNumber(input, value.x)
        || !skip_optional_spaces(input)
        || !parseNumber(input, value.y)
        || !input.empty()) {
        return false;
    }

    m_value = value;
    return true;
}

bool svg_point_list::parse(std::string_view input)
{
    m_values.clear();
    strip_leading_spaces(input);
    while(!input.empty()) {
        Point value;
        if(!parseNumber(input, value.x)
            || !skip_optional_spaces_or_comma(input)
            || !parseNumber(input, value.y)) {
            return false;
        }

        m_values.push_back(value);
        skip_optional_spaces_or_comma(input);
    }

    return true;
}

bool svg_rect::parse(std::string_view input)
{
    Rect value;
    strip_leading_and_trailing_spaces(input);
    if(!parseNumber(input, value.x)
        || !skip_optional_spaces_or_comma(input)
        || !parseNumber(input, value.y)
        || !skip_optional_spaces_or_comma(input)
        || !parseNumber(input, value.w)
        || !skip_optional_spaces_or_comma(input)
        || !parseNumber(input, value.h)
        || !input.empty()) {
        return false;
    }

    if(value.w < 0.f || value.h < 0.f)
        return false;
    m_value = value;
    return true;
}

bool svg_transform::parse(std::string_view input)
{
    return m_value.parse(input.data(), input.length());
}

bool svg_transform_box::parse(std::string_view input)
{
    strip_leading_and_trailing_spaces(input);
    if(input == "fill-box" || input == "content-box") {
        m_value = transform_box_type::fill_box;
    } else if(input == "view-box" || input == "border-box" || input == "stroke-box") {
        // border-box/stroke-box aren't distinguished from view-box here (see
        // the class comment) -- treated as "not fill-box" for origin purposes.
        m_value = transform_box_type::ViewBox;
    } else {
        return false;
    }

    m_specified = true;
    return true;
}

bool svg_transform_origin::parse(std::string_view input)
{
    auto parse_component = [](std::string_view token, float& value, bool& is_percent) {
        if(token == "left" || token == "top") { value = 0.f; is_percent = true; return true; }
        if(token == "center") { value = 0.5f; is_percent = true; return true; }
        if(token == "right" || token == "bottom") { value = 1.f; is_percent = true; return true; }
        float number = 0.f;
        if(!parseNumber(token, number))
            return false;
        if(!token.empty() && token.front() == '%') {
            value = number / 100.f;
            is_percent = true;
            token.remove_prefix(1);
        } else {
            value = number;
            is_percent = false;
        }
        return token.empty();
    };

    std::vector<std::string_view> tokens;
    strip_leading_and_trailing_spaces(input);
    while(!input.empty()) {
        size_t count = 0;
        while(count < input.length() && !IS_WS(input[count]))
            ++count;
        tokens.push_back(input.substr(0, count));
        input.remove_prefix(count);
        skip_optional_spaces(input);
    }

    if(tokens.empty() || tokens.size() > 2)
        return false;

    float x = 0.5f, y = 0.5f;
    bool x_is_percent = true, y_is_percent = true;
    if(tokens.size() == 1) {
        if(tokens[0] == "top" || tokens[0] == "bottom") {
            if(!parse_component(tokens[0], y, y_is_percent))
                return false;
        } else if(!parse_component(tokens[0], x, x_is_percent)) {
            return false;
        }
    } else {
        if(!parse_component(tokens[0], x, x_is_percent) || !parse_component(tokens[1], y, y_is_percent))
            return false;
    }

    m_x = x;
    m_y = y;
    m_x_is_percent = x_is_percent;
    m_y_is_percent = y_is_percent;
    m_specified = true;
    return true;
}

bool svg_preserve_aspect_ratio::parse(std::string_view input)
{
    auto alignType = AlignType::x_mid_y_mid;
    strip_leading_spaces(input);
    if(skipString(input, "none"))
        alignType = AlignType::None;
    else if(skipString(input, "xMinYMin"))
        alignType = AlignType::x_min_y_min;
    else if(skipString(input, "xMidYMin"))
        alignType = AlignType::x_mid_y_min;
    else if(skipString(input, "xMaxYMin"))
        alignType = AlignType::x_max_y_min;
    else if(skipString(input, "xMinYMid"))
        alignType = AlignType::x_min_y_mid;
    else if(skipString(input, "xMidYMid"))
        alignType = AlignType::x_mid_y_mid;
    else if(skipString(input, "xMaxYMid"))
        alignType = AlignType::x_max_y_mid;
    else if(skipString(input, "xMinYMax"))
        alignType = AlignType::x_min_y_max;
    else if(skipString(input, "xMidYMax"))
        alignType = AlignType::x_mid_y_max;
    else if(skipString(input, "xMaxYMax"))
        alignType = AlignType::x_max_y_max;
    else {
        return false;
    }

    auto meetOrSlice = MeetOrSlice::meet;
    skip_optional_spaces(input);
    if(skipString(input, "meet")) {
        meetOrSlice = MeetOrSlice::meet;
    } else if(skipString(input, "slice")) {
        meetOrSlice = MeetOrSlice::slice;
    }

    if(alignType == AlignType::None)
        meetOrSlice = MeetOrSlice::meet;
    skip_optional_spaces(input);
    if(!input.empty())
        return false;
    m_align_type = alignType;
    m_meet_or_slice = meetOrSlice;
    return true;
}

Rect svg_preserve_aspect_ratio::get_clip_rect(const Rect& view_box_rect, const Size& viewport_size) const
{
    assert(!view_box_rect.empty() && !viewport_size.empty());
    auto x_scale = viewport_size.w / view_box_rect.w;
    auto y_scale = viewport_size.h / view_box_rect.h;
    if(m_align_type == AlignType::None) {
        return Rect(view_box_rect.x, view_box_rect.y, viewport_size.w / x_scale, viewport_size.h / y_scale);
    }

    auto scale = (m_meet_or_slice == MeetOrSlice::meet) ? STB_MIN(x_scale, y_scale) : STB_MAX(x_scale, y_scale);
    auto x_offset = -view_box_rect.x * scale;
    auto yOffset = -view_box_rect.y * scale;
    auto view_width = view_box_rect.w * scale;
    auto view_height = view_box_rect.h * scale;
    switch(m_align_type) {
    case AlignType::x_mid_y_min:
    case AlignType::x_mid_y_mid:
    case AlignType::x_mid_y_max:
        x_offset += (viewport_size.w - view_width) * 0.5f;
        break;
    case AlignType::x_max_y_min:
    case AlignType::x_max_y_mid:
    case AlignType::x_max_y_max:
        x_offset += (viewport_size.w - view_width);
        break;
    default:
        break;
    }

    switch(m_align_type) {
    case AlignType::x_min_y_mid:
    case AlignType::x_mid_y_mid:
    case AlignType::x_max_y_mid:
        yOffset += (viewport_size.h - view_height) * 0.5f;
        break;
    case AlignType::x_min_y_max:
    case AlignType::x_mid_y_max:
    case AlignType::x_max_y_max:
        yOffset += (viewport_size.h - view_height);
        break;
    default:
        break;
    }

    return Rect(-x_offset / scale, -yOffset / scale, viewport_size.w / scale, viewport_size.h / scale);
}

Transform svg_preserve_aspect_ratio::get_transform(const Rect& view_box_rect, const Size& viewport_size) const
{
    assert(!view_box_rect.empty() && !viewport_size.empty());
    auto x_scale = viewport_size.w / view_box_rect.w;
    auto y_scale = viewport_size.h / view_box_rect.h;
    if(m_align_type == AlignType::None) {
        return Transform(x_scale, 0, 0, y_scale, -view_box_rect.x * x_scale, -view_box_rect.y * y_scale);
    }

    auto scale = (m_meet_or_slice == MeetOrSlice::meet) ? STB_MIN(x_scale, y_scale) : STB_MAX(x_scale, y_scale);
    auto x_offset = -view_box_rect.x * scale;
    auto yOffset = -view_box_rect.y * scale;
    auto view_width = view_box_rect.w * scale;
    auto view_height = view_box_rect.h * scale;
    switch(m_align_type) {
    case AlignType::x_mid_y_min:
    case AlignType::x_mid_y_mid:
    case AlignType::x_mid_y_max:
        x_offset += (viewport_size.w - view_width) * 0.5f;
        break;
    case AlignType::x_max_y_min:
    case AlignType::x_max_y_mid:
    case AlignType::x_max_y_max:
        x_offset += (viewport_size.w - view_width);
        break;
    default:
        break;
    }

    switch(m_align_type) {
    case AlignType::x_min_y_mid:
    case AlignType::x_mid_y_mid:
    case AlignType::x_max_y_mid:
        yOffset += (viewport_size.h - view_height) * 0.5f;
        break;
    case AlignType::x_min_y_max:
    case AlignType::x_mid_y_max:
    case AlignType::x_max_y_max:
        yOffset += (viewport_size.h - view_height);
        break;
    default:
        break;
    }

    return Transform(scale, 0, 0, scale, x_offset, yOffset);
}

void svg_preserve_aspect_ratio::transform_rect(Rect& dstRect, Rect& srcRect) const
{
    if(m_align_type == AlignType::None)
        return;
    auto view_size = dstRect.size();
    auto image_size = srcRect.size();
    if(m_meet_or_slice == MeetOrSlice::meet) {
        auto scale = image_size.h / image_size.w;
        if(view_size.h > view_size.w * scale) {
            dstRect.h = view_size.w * scale;
            switch(m_align_type) {
            case AlignType::x_min_y_mid:
            case AlignType::x_mid_y_mid:
            case AlignType::x_max_y_mid:
                dstRect.y += (view_size.h - dstRect.h) * 0.5f;
                break;
            case AlignType::x_min_y_max:
            case AlignType::x_mid_y_max:
            case AlignType::x_max_y_max:
                dstRect.y += view_size.h - dstRect.h;
                break;
            default:
                break;
            }
        }

        if(view_size.w > view_size.h / scale) {
            dstRect.w = view_size.h / scale;
            switch(m_align_type) {
            case AlignType::x_mid_y_min:
            case AlignType::x_mid_y_mid:
            case AlignType::x_mid_y_max:
                dstRect.x += (view_size.w - dstRect.w) * 0.5f;
                break;
            case AlignType::x_max_y_min:
            case AlignType::x_max_y_mid:
            case AlignType::x_max_y_max:
                dstRect.x += view_size.w - dstRect.w;
                break;
            default:
                break;
            }
        }
    } else if(m_meet_or_slice == MeetOrSlice::slice) {
        auto scale = image_size.h / image_size.w;
        if(view_size.h < view_size.w * scale) {
            srcRect.h = view_size.h * (image_size.w / view_size.w);
            switch(m_align_type) {
            case AlignType::x_min_y_mid:
            case AlignType::x_mid_y_mid:
            case AlignType::x_max_y_mid:
                srcRect.y += (image_size.h - srcRect.h) * 0.5f;
                break;
            case AlignType::x_min_y_max:
            case AlignType::x_mid_y_max:
            case AlignType::x_max_y_max:
                srcRect.y += image_size.h - srcRect.h;
                break;
            default:
                break;
            }
        }

        if(view_size.w < view_size.h / scale) {
            srcRect.w = view_size.w * (image_size.h / view_size.h);
            switch(m_align_type) {
            case AlignType::x_mid_y_min:
            case AlignType::x_mid_y_mid:
            case AlignType::x_mid_y_max:
                srcRect.x += (image_size.w - srcRect.w) * 0.5f;
                break;
            case AlignType::x_max_y_min:
            case AlignType::x_max_y_mid:
            case AlignType::x_max_y_max:
                srcRect.x += image_size.w - srcRect.w;
                break;
            default:
                break;
            }
        }
    }
}

} // namespace nova::svg
