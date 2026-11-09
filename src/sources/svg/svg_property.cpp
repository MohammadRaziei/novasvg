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
        m_orient_type = orient_type_value::auto_;
        return true;
    }

    if(input == "auto-start-reverse") {
        m_value = 0.f;
        m_orient_type = orient_type_value::auto_start_reverse;
        return true;
    }

    float value = 0.f;
    if(!parse_number(input, value))
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
    m_orient_type = orient_type_value::angle;
    return true;
}

bool css_length::parse(std::string_view input, length_negative_mode mode)
{
    float value = 0.f;
    strip_leading_and_trailing_spaces(input);
    if(!parse_number(input, value))
        return false;
    if(value < 0.f && mode == length_negative_mode::forbid)
        return false;
    if(input.empty()) {
        m_value = value;
        m_units = length_units::none;
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
        m_units = length_units::px;
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
        m_units = length_units::px;
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
        m_units = length_units::px;
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
        m_units = length_units::px;
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

float length_context::value_for_length(const css_length& length_, length_direction direction_) const
{
    if(length_.units() == length_units::percent) {
        if(m_units == units::user_space_on_use)
            return length_.value() * viewport_dimension(direction_) / 100.f;
        return length_.value() / 100.f;
    }

    if(length_.units() == length_units::ex)
        return length_.value() * m_element->font_size() / 2.f;
    if(length_.units() == length_units::em)
        return length_.value() * m_element->font_size();
    return length_.value();
}

float length_context::viewport_dimension(length_direction direction_) const
{
    auto viewport_size = m_element->current_viewport_size();
    switch(direction_) {
    case length_direction::horizontal:
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
        css_length value(0, length_units::none);
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
    if(!parse_number(input, value))
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
    if(!parse_number(input, value))
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
        if(!parse_number(input, value))
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
    point value;
    strip_leading_and_trailing_spaces(input);
    if(!parse_number(input, value.x)
        || !skip_optional_spaces(input)
        || !parse_number(input, value.y)
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
        point value;
        if(!parse_number(input, value.x)
            || !skip_optional_spaces_or_comma(input)
            || !parse_number(input, value.y)) {
            return false;
        }

        m_values.push_back(value);
        skip_optional_spaces_or_comma(input);
    }

    return true;
}

bool svg_rect::parse(std::string_view input)
{
    rect value;
    strip_leading_and_trailing_spaces(input);
    if(!parse_number(input, value.x)
        || !skip_optional_spaces_or_comma(input)
        || !parse_number(input, value.y)
        || !skip_optional_spaces_or_comma(input)
        || !parse_number(input, value.w)
        || !skip_optional_spaces_or_comma(input)
        || !parse_number(input, value.h)
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
        m_value = transform_box_type::view_box;
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
        if(!parse_number(token, number))
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
    auto align_type_ = align_type_value::x_mid_y_mid;
    strip_leading_spaces(input);
    if(skip_string(input, "none"))
        align_type_ = align_type_value::none;
    else if(skip_string(input, "xMinYMin"))
        align_type_ = align_type_value::x_min_y_min;
    else if(skip_string(input, "xMidYMin"))
        align_type_ = align_type_value::x_mid_y_min;
    else if(skip_string(input, "xMaxYMin"))
        align_type_ = align_type_value::x_max_y_min;
    else if(skip_string(input, "xMinYMid"))
        align_type_ = align_type_value::x_min_y_mid;
    else if(skip_string(input, "xMidYMid"))
        align_type_ = align_type_value::x_mid_y_mid;
    else if(skip_string(input, "xMaxYMid"))
        align_type_ = align_type_value::x_max_y_mid;
    else if(skip_string(input, "xMinYMax"))
        align_type_ = align_type_value::x_min_y_max;
    else if(skip_string(input, "xMidYMax"))
        align_type_ = align_type_value::x_mid_y_max;
    else if(skip_string(input, "xMaxYMax"))
        align_type_ = align_type_value::x_max_y_max;
    else {
        return false;
    }

    auto meet_or_slice_ = meet_or_slice_value::meet;
    skip_optional_spaces(input);
    if(skip_string(input, "meet")) {
        meet_or_slice_ = meet_or_slice_value::meet;
    } else if(skip_string(input, "slice")) {
        meet_or_slice_ = meet_or_slice_value::slice;
    }

    if(align_type_ == align_type_value::none)
        meet_or_slice_ = meet_or_slice_value::meet;
    skip_optional_spaces(input);
    if(!input.empty())
        return false;
    m_align_type = align_type_;
    m_meet_or_slice = meet_or_slice_;
    return true;
}

rect svg_preserve_aspect_ratio::get_clip_rect(const rect& view_box_rect, const size& viewport_size) const
{
    assert(!view_box_rect.empty() && !viewport_size.empty());
    auto x_scale = viewport_size.w / view_box_rect.w;
    auto y_scale = viewport_size.h / view_box_rect.h;
    if(m_align_type == align_type_value::none) {
        return rect(view_box_rect.x, view_box_rect.y, viewport_size.w / x_scale, viewport_size.h / y_scale);
    }

    auto scale = (m_meet_or_slice == meet_or_slice_value::meet) ? STB_MIN(x_scale, y_scale) : STB_MAX(x_scale, y_scale);
    auto x_offset = -view_box_rect.x * scale;
    auto y_offset = -view_box_rect.y * scale;
    auto view_width = view_box_rect.w * scale;
    auto view_height = view_box_rect.h * scale;
    switch(m_align_type) {
    case align_type_value::x_mid_y_min:
    case align_type_value::x_mid_y_mid:
    case align_type_value::x_mid_y_max:
        x_offset += (viewport_size.w - view_width) * 0.5f;
        break;
    case align_type_value::x_max_y_min:
    case align_type_value::x_max_y_mid:
    case align_type_value::x_max_y_max:
        x_offset += (viewport_size.w - view_width);
        break;
    default:
        break;
    }

    switch(m_align_type) {
    case align_type_value::x_min_y_mid:
    case align_type_value::x_mid_y_mid:
    case align_type_value::x_max_y_mid:
        y_offset += (viewport_size.h - view_height) * 0.5f;
        break;
    case align_type_value::x_min_y_max:
    case align_type_value::x_mid_y_max:
    case align_type_value::x_max_y_max:
        y_offset += (viewport_size.h - view_height);
        break;
    default:
        break;
    }

    return rect(-x_offset / scale, -y_offset / scale, viewport_size.w / scale, viewport_size.h / scale);
}

transform svg_preserve_aspect_ratio::get_transform(const rect& view_box_rect, const size& viewport_size) const
{
    assert(!view_box_rect.empty() && !viewport_size.empty());
    auto x_scale = viewport_size.w / view_box_rect.w;
    auto y_scale = viewport_size.h / view_box_rect.h;
    if(m_align_type == align_type_value::none) {
        return transform(x_scale, 0, 0, y_scale, -view_box_rect.x * x_scale, -view_box_rect.y * y_scale);
    }

    auto scale = (m_meet_or_slice == meet_or_slice_value::meet) ? STB_MIN(x_scale, y_scale) : STB_MAX(x_scale, y_scale);
    auto x_offset = -view_box_rect.x * scale;
    auto y_offset = -view_box_rect.y * scale;
    auto view_width = view_box_rect.w * scale;
    auto view_height = view_box_rect.h * scale;
    switch(m_align_type) {
    case align_type_value::x_mid_y_min:
    case align_type_value::x_mid_y_mid:
    case align_type_value::x_mid_y_max:
        x_offset += (viewport_size.w - view_width) * 0.5f;
        break;
    case align_type_value::x_max_y_min:
    case align_type_value::x_max_y_mid:
    case align_type_value::x_max_y_max:
        x_offset += (viewport_size.w - view_width);
        break;
    default:
        break;
    }

    switch(m_align_type) {
    case align_type_value::x_min_y_mid:
    case align_type_value::x_mid_y_mid:
    case align_type_value::x_max_y_mid:
        y_offset += (viewport_size.h - view_height) * 0.5f;
        break;
    case align_type_value::x_min_y_max:
    case align_type_value::x_mid_y_max:
    case align_type_value::x_max_y_max:
        y_offset += (viewport_size.h - view_height);
        break;
    default:
        break;
    }

    return transform(scale, 0, 0, scale, x_offset, y_offset);
}

void svg_preserve_aspect_ratio::transform_rect(rect& dst_rect, rect& src_rect) const
{
    if(m_align_type == align_type_value::none)
        return;
    auto view_size = dst_rect.size();
    auto image_size = src_rect.size();
    if(m_meet_or_slice == meet_or_slice_value::meet) {
        auto scale = image_size.h / image_size.w;
        if(view_size.h > view_size.w * scale) {
            dst_rect.h = view_size.w * scale;
            switch(m_align_type) {
            case align_type_value::x_min_y_mid:
            case align_type_value::x_mid_y_mid:
            case align_type_value::x_max_y_mid:
                dst_rect.y += (view_size.h - dst_rect.h) * 0.5f;
                break;
            case align_type_value::x_min_y_max:
            case align_type_value::x_mid_y_max:
            case align_type_value::x_max_y_max:
                dst_rect.y += view_size.h - dst_rect.h;
                break;
            default:
                break;
            }
        }

        if(view_size.w > view_size.h / scale) {
            dst_rect.w = view_size.h / scale;
            switch(m_align_type) {
            case align_type_value::x_mid_y_min:
            case align_type_value::x_mid_y_mid:
            case align_type_value::x_mid_y_max:
                dst_rect.x += (view_size.w - dst_rect.w) * 0.5f;
                break;
            case align_type_value::x_max_y_min:
            case align_type_value::x_max_y_mid:
            case align_type_value::x_max_y_max:
                dst_rect.x += view_size.w - dst_rect.w;
                break;
            default:
                break;
            }
        }
    } else if(m_meet_or_slice == meet_or_slice_value::slice) {
        auto scale = image_size.h / image_size.w;
        if(view_size.h < view_size.w * scale) {
            src_rect.h = view_size.h * (image_size.w / view_size.w);
            switch(m_align_type) {
            case align_type_value::x_min_y_mid:
            case align_type_value::x_mid_y_mid:
            case align_type_value::x_max_y_mid:
                src_rect.y += (image_size.h - src_rect.h) * 0.5f;
                break;
            case align_type_value::x_min_y_max:
            case align_type_value::x_mid_y_max:
            case align_type_value::x_max_y_max:
                src_rect.y += image_size.h - src_rect.h;
                break;
            default:
                break;
            }
        }

        if(view_size.w < view_size.h / scale) {
            src_rect.w = view_size.w * (image_size.h / view_size.h);
            switch(m_align_type) {
            case align_type_value::x_mid_y_min:
            case align_type_value::x_mid_y_mid:
            case align_type_value::x_mid_y_max:
                src_rect.x += (image_size.w - src_rect.w) * 0.5f;
                break;
            case align_type_value::x_max_y_min:
            case align_type_value::x_max_y_mid:
            case align_type_value::x_max_y_max:
                src_rect.x += image_size.w - src_rect.w;
                break;
            default:
                break;
            }
        }
    }
}

} // namespace nova::svg
