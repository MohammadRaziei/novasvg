#include "utils/text.h"
#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

property_id propertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        property_id value;
    } table_[] = {
        {"class", property_id::class_},
        {"clipPathUnits", property_id::clip_path_units},
        {"cx", property_id::cx},
        {"cy", property_id::cy},
        {"d", property_id::D},
        {"dx", property_id::dx},
        {"dy", property_id::dy},
        {"fx", property_id::fx},
        {"fy", property_id::fy},
        {"gradientTransform", property_id::gradient_transform},
        {"gradientUnits", property_id::gradient_units},
        {"height", property_id::height},
        {"href", property_id::href},
        {"id", property_id::id},
        {"in", property_id::in},
        {"in2", property_id::in2},
        {"k1", property_id::K1},
        {"k2", property_id::K2},
        {"k3", property_id::K3},
        {"k4", property_id::K4},
        {"lengthAdjust", property_id::length_adjust},
        {"markerHeight", property_id::marker_height},
        {"markerUnits", property_id::marker_units},
        {"markerWidth", property_id::marker_width},
        {"maskContentUnits", property_id::mask_content_units},
        {"maskUnits", property_id::mask_units},
        {"offset", property_id::offset},
        {"operator", property_id::operator_},
        {"orient", property_id::orient},
        {"patternContentUnits", property_id::pattern_content_units},
        {"patternTransform", property_id::pattern_transform},
        {"patternUnits", property_id::pattern_units},
        {"points", property_id::points},
        {"preserveAspectRatio", property_id::preserve_aspect_ratio},
        {"r", property_id::R},
        {"refX", property_id::ref_x},
        {"refY", property_id::ref_y},
        {"requiredExtensions", property_id::required_extensions},
        {"result", property_id::result},
        {"rotate", property_id::rotate},
        {"rx", property_id::rx},
        {"ry", property_id::ry},
        {"spreadMethod", property_id::spread_method},
        {"stdDeviation", property_id::std_deviation},
        {"style", property_id::style},
        {"systemLanguage", property_id::system_language},
        {"textLength", property_id::text_length},
        {"transform", property_id::transform},
        {"viewBox", property_id::view_box},
        {"width", property_id::width},
        {"x", property_id::X},
        {"x1", property_id::X1},
        {"x2", property_id::X2},
        {"xlink:href", property_id::href},
        {"xml:space", property_id::white_space_},
        {"y", property_id::Y},
        {"y1", property_id::Y1},
        {"y2", property_id::Y2}
    };

    auto it = std::lower_bound(table_, std::end(table_), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table_) || it->name != name)
        return csspropertyid(name);
    return it->value;
}

property_id csspropertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        property_id value;
    } table_[] = {
        {"alignment-baseline", property_id::alignment_baseline_},
        {"background-color", property_id::background_color},
        {"baseline-shift", property_id::baseline_shift_},
        {"clip-path", property_id::clip_path},
        {"clip-rule", property_id::clip_rule},
        {"color", property_id::color},
        {"direction", property_id::direction},
        {"display", property_id::display},
        {"dominant-baseline", property_id::dominant_baseline_},
        {"fill", property_id::fill},
        {"fill-opacity", property_id::fill_opacity},
        {"fill-rule", property_id::fill_rule_},
        {"filter", property_id::filter},
        {"flood-color", property_id::flood_color},
        {"flood-opacity", property_id::flood_opacity},
        {"font-family", property_id::font_family},
        {"font-size", property_id::font_size},
        {"font-style", property_id::font_style_},
        {"font-weight", property_id::font_weight_},
        {"letter-spacing", property_id::letter_spacing},
        {"marker-end", property_id::marker_end},
        {"marker-mid", property_id::marker_mid},
        {"marker-start", property_id::marker_start},
        {"mask", property_id::mask},
        {"mask-type", property_id::mask_type_},
        {"mix-blend-mode", property_id::mix_blend_mode_},
        {"opacity", property_id::opacity},
        {"overflow", property_id::overflow},
        {"pointer-events", property_id::pointer_events_},
        {"stop-color", property_id::stop_color},
        {"stop-opacity", property_id::stop_opacity},
        {"stroke", property_id::stroke},
        {"stroke-dasharray", property_id::stroke_dasharray},
        {"stroke-dashoffset", property_id::stroke_dashoffset},
        {"stroke-linecap", property_id::stroke_linecap},
        {"stroke-linejoin", property_id::stroke_linejoin},
        {"stroke-miterlimit", property_id::stroke_miterlimit},
        {"stroke-opacity", property_id::stroke_opacity},
        {"stroke-width", property_id::stroke_width},
        {"text-anchor", property_id::text_anchor_},
        {"text-orientation", property_id::text_orientation_},
        {"transform-box", property_id::transform_box},
        {"transform-origin", property_id::transform_origin},
        {"visibility", property_id::visibility},
        {"white-space", property_id::white_space_},
        {"word-spacing", property_id::word_spacing},
        {"writing-mode", property_id::writing_mode_}
    };

    auto it = std::lower_bound(table_, std::end(table_), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table_) || it->name != name)
        return property_id::unknown;
    return it->value;
}

std::optional<stb::color> parse_color_value(std::string_view& input, const svg_layout_state* state)
{
    if(skip_string(input, "currentColor")) {
        return state->color();
    }

    std::size_t used = 0;
    const auto color_ = stb::color::try_parse(input, &used);
    if(!color_)
        return std::nullopt;
    input.remove_prefix(used);
    input = nova::utils::ltrim(input);   // white space after a colour belongs to it
    return color_;
}

stb::color parse_color(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    auto color_ = parse_color_value(input, state);
    if(!color_ || !input.empty())
        color_ = default_value;
    return color_.value();
}

stb::color parse_color_or_none(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    if(input.compare("none") == 0)
        return stb::color::transparent();
    return parse_color(input, state, default_value);
}

bool parse_url_value(std::string_view& input, std::string& value)
{
    if(!skip_string(input, "url")
        || !skip_optional_spaces(input)
        || !skip_delimiter(input, '(')
        || !skip_optional_spaces(input)) {
        return false;
    }

    switch(input.front()) {
    case '\'':
    case '\"': {
        auto delim = input.front();
        input.remove_prefix(1);
        skip_optional_spaces(input);
        if(!skip_delimiter(input, '#'))
            return false;
        while(!input.empty() && input.front() != delim) {
            value += input.front();
            input.remove_prefix(1);
        }

        skip_optional_spaces(input);
        if(!skip_delimiter(input, delim))
            return false;
        break;
    } case '#': {
        input.remove_prefix(1);
        while(!input.empty() && input.front() != ')') {
            value += input.front();
            input.remove_prefix(1);
        }

        break;
    } default:
        return false;
    }

    return skip_optional_spaces(input) && skip_delimiter(input, ')');
}

std::string parse_url(std::string_view input)
{
    std::string value;
    if(!parse_url_value(input, value) || !input.empty())
        value.clear();
    return value;
}

paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    std::string id;
    if(!parse_url_value(input, id))
        return paint(parse_color_or_none(input, state, default_value));
    if(skip_optional_spaces(input))
        return paint(id, parse_color_or_none(input, state, default_value));
    return paint(id, stb::color::transparent());
}

float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value)
{
    float value;
    if(!parse_number(input, value))
        return default_value;
    if(allow_percentage) {
        if(skip_delimiter(input, '%'))
            value /= 100.f;
        value = std::clamp(value, 0.f, 1.f);
    }

    if(!input.empty())
        return default_value;
    return value;
}

css_length parse_length(std::string_view input, length_negative_mode mode, const css_length& default_value)
{
    css_length value;
    if(!value.parse(input, mode))
        value = default_value;
    return value;
}

css_baseline_shift parse_baseline_shift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return css_baseline_shift::shift_type::baseline;
    if(input.compare("sub") == 0)
        return css_baseline_shift::shift_type::sub;
    if(input.compare("super") == 0)
        return css_baseline_shift::shift_type::super;
    return parse_length(input, length_negative_mode::allow, css_length(0.f, length_units::none));
}

length_list parse_dash_array(std::string_view input)
{
    if(input.compare("none") == 0)
        return length_list();
    length_list values;
    do {
        size_t count = 0;
        while(count < input.length() && input[count] != ',' && !IS_WS(input[count]))
            ++count;
        css_length value(0, length_units::none);
        if(!value.parse(input.substr(0, count), length_negative_mode::forbid))
            return length_list();
        input.remove_prefix(count);
        values.push_back(std::move(value));
    } while(skip_optional_spaces_or_comma(input));
    return values;
}

css_length parse_length_or_normal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return css_length(0, length_units::none);
    return parse_length(input, length_negative_mode::allow, css_length(0, length_units::none));
}

float parse_font_size(std::string_view input, const svg_layout_state* state)
{
    auto length_ = parse_length(input, length_negative_mode::forbid, css_length(12, length_units::none));
    if(length_.units() == length_units::percent)
        return length_.value() * state->font_size() / 100.f;
    if(length_.units() == length_units::ex)
        return length_.value() * state->font_size() / 2.f;
    if(length_.units() == length_units::em)
        return length_.value() * state->font_size();
    return length_.value();
}

css_display parse_display(std::string_view input)
{
    static const svg_enumeration_entry<css_display> entries[] = {
        {css_display::inline_, "inline"},
        {css_display::none, "none"}
    };

    return parse_enum_value(input, entries, css_display::inline_);
}

css_visibility parse_visibility(std::string_view input)
{
    static const svg_enumeration_entry<css_visibility> entries[] = {
        {css_visibility::visible, "visible"},
        {css_visibility::hidden, "hidden"},
        {css_visibility::collapse, "collapse"}
    };

    return parse_enum_value(input, entries, css_visibility::visible);
}

css_overflow parse_overflow(std::string_view input)
{
    static const svg_enumeration_entry<css_overflow> entries[] = {
        {css_overflow::visible, "visible"},
        {css_overflow::hidden, "hidden"}
    };

    return parse_enum_value(input, entries, css_overflow::visible);
}

css_pointer_events parse_pointer_events(std::string_view input)
{
    static const svg_enumeration_entry<css_pointer_events> entries[] = {
        {css_pointer_events::none,"none"},
        {css_pointer_events::auto_,"auto"},
        {css_pointer_events::stroke,"stroke"},
        {css_pointer_events::fill,"fill"},
        {css_pointer_events::painted,"painted"},
        {css_pointer_events::visible,"visible"},
        {css_pointer_events::visible_stroke,"visibleStroke"},
        {css_pointer_events::visible_fill,"visibleFill"},
        {css_pointer_events::visible_painted,"visiblePainted"},
        {css_pointer_events::bounding_box,"bounding-box"},
        {css_pointer_events::all,"all"},
    };

    return parse_enum_value(input, entries, css_pointer_events::auto_);
}

css_font_weight parse_font_weight(std::string_view input)
{
    static const svg_enumeration_entry<css_font_weight> entries[] = {
        {css_font_weight::normal, "normal"},
        {css_font_weight::bold, "bold"},
        {css_font_weight::bold, "bolder"},
        {css_font_weight::normal, "lighter"},
        {css_font_weight::normal, "100"},
        {css_font_weight::normal, "200"},
        {css_font_weight::normal, "300"},
        {css_font_weight::normal, "400"},
        {css_font_weight::normal, "500"},
        {css_font_weight::bold, "600"},
        {css_font_weight::bold, "700"},
        {css_font_weight::bold, "800"},
        {css_font_weight::bold, "900"}
    };

    return parse_enum_value(input, entries, css_font_weight::normal);
}

css_font_style parse_font_style(std::string_view input)
{
    static const svg_enumeration_entry<css_font_style> entries[] = {
        {css_font_style::normal, "normal"},
        {css_font_style::italic, "italic"},
        {css_font_style::italic, "oblique"}
    };

    return parse_enum_value(input, entries, css_font_style::normal);
}

css_alignment_baseline parse_alignment_baseline(std::string_view input)
{
    static const svg_enumeration_entry<css_alignment_baseline> entries[] = {
        {css_alignment_baseline::auto_, "auto"},
        {css_alignment_baseline::baseline, "baseline"},
        {css_alignment_baseline::before_edge, "before-edge"},
        {css_alignment_baseline::text_before_edge, "text-before-edge"},
        {css_alignment_baseline::middle, "middle"},
        {css_alignment_baseline::central, "central"},
        {css_alignment_baseline::after_edge, "after-edge"},
        {css_alignment_baseline::text_after_edge, "text-after-edge"},
        {css_alignment_baseline::ideographic, "ideographic"},
        {css_alignment_baseline::alphabetic, "alphabetic"},
        {css_alignment_baseline::hanging, "hanging"},
        {css_alignment_baseline::mathematical, "mathematical"}
    };

    return parse_enum_value(input, entries, css_alignment_baseline::auto_);
}

css_dominant_baseline parse_dominant_baseline(std::string_view input)
{
    static const svg_enumeration_entry<css_dominant_baseline> entries[] = {
        {css_dominant_baseline::auto_, "auto"},
        {css_dominant_baseline::use_script, "use-script"},
        {css_dominant_baseline::no_change, "no-change"},
        {css_dominant_baseline::reset_size, "reset-size"},
        {css_dominant_baseline::ideographic, "ideographic"},
        {css_dominant_baseline::alphabetic, "alphabetic"},
        {css_dominant_baseline::hanging, "hanging"},
        {css_dominant_baseline::mathematical, "mathematical"},
        {css_dominant_baseline::central, "central"},
        {css_dominant_baseline::middle, "middle"},
        {css_dominant_baseline::text_after_edge, "text-after-edge"},
        {css_dominant_baseline::text_before_edge, "text-before-edge"}
    };

    return parse_enum_value(input, entries, css_dominant_baseline::auto_);
}

css_direction parse_direction(std::string_view input)
{
    static const svg_enumeration_entry<css_direction> entries[] = {
        {css_direction::ltr, "ltr"},
        {css_direction::rtl, "rtl"}
    };

    return parse_enum_value(input, entries, css_direction::ltr);
}

css_writing_mode parse_writing_mode(std::string_view input)
{
    static const svg_enumeration_entry<css_writing_mode> entries[] = {
        {css_writing_mode::horizontal, "horizontal-tb"},
        {css_writing_mode::vertical, "vertical-rl"},
        {css_writing_mode::vertical, "vertical-lr"},
        {css_writing_mode::horizontal, "lr-tb"},
        {css_writing_mode::horizontal, "lr"},
        {css_writing_mode::horizontal, "rl-tb"},
        {css_writing_mode::horizontal, "rl"},
        {css_writing_mode::vertical, "tb-rl"},
        {css_writing_mode::vertical, "tb"}
    };

    return parse_enum_value(input, entries, css_writing_mode::horizontal);
}

css_text_orientation parse_text_orientation(std::string_view input)
{
    static const svg_enumeration_entry<css_text_orientation> entries[] = {
        {css_text_orientation::mixed, "mixed"},
        {css_text_orientation::upright, "upright"}
    };

    return parse_enum_value(input, entries, css_text_orientation::mixed);
}

svg_text_anchor parse_text_anchor(std::string_view input)
{
    static const svg_enumeration_entry<svg_text_anchor> entries[] = {
        {svg_text_anchor::start, "start"},
        {svg_text_anchor::middle, "middle"},
        {svg_text_anchor::end, "end"}
    };

    return parse_enum_value(input, entries, svg_text_anchor::start);
}

css_white_space parse_white_space(std::string_view input)
{
    static const svg_enumeration_entry<css_white_space> entries[] = {
        {css_white_space::default_, "default"},
        {css_white_space::preserve, "preserve"},
        {css_white_space::default_, "normal"},
        {css_white_space::default_, "nowrap"},
        {css_white_space::default_, "pre-line"},
        {css_white_space::preserve, "pre-wrap"},
        {css_white_space::preserve, "pre"}
    };

    return parse_enum_value(input, entries, css_white_space::default_);
}

css_mix_blend_mode parse_mix_blend_mode(std::string_view input)
{
    static const svg_enumeration_entry<css_mix_blend_mode> entries[] = {
        {css_mix_blend_mode::normal, "normal"},
        {css_mix_blend_mode::multiply, "multiply"},
        {css_mix_blend_mode::screen, "screen"},
        {css_mix_blend_mode::darken, "darken"},
        {css_mix_blend_mode::lighten, "lighten"},
        {css_mix_blend_mode::difference, "difference"},
        {css_mix_blend_mode::exclusion, "exclusion"}
    };

    // CSS keywords are ASCII case-insensitive
    std::string keyword;
    for(char ch : input)
        keyword.push_back(utils::to_lower(ch));
    return parse_enum_value(keyword, entries, css_mix_blend_mode::normal);
}

css_mask_type parse_mask_type(std::string_view input)
{
    static const svg_enumeration_entry<css_mask_type> entries[] = {
        {css_mask_type::luminance, "luminance"},
        {css_mask_type::alpha, "alpha"}
    };

    return parse_enum_value(input, entries, css_mask_type::luminance);
}

fill_rule parse_fill_rule(std::string_view input)
{
    static const svg_enumeration_entry<fill_rule> entries[] = {
        {fill_rule::non_zero, "nonzero"},
        {fill_rule::even_odd, "evenodd"}
    };

    return parse_enum_value(input, entries, fill_rule::non_zero);
}

line_cap parse_line_cap(std::string_view input)
{
    static const svg_enumeration_entry<line_cap> entries[] = {
        {line_cap::butt, "butt"},
        {line_cap::round, "round"},
        {line_cap::square, "square"}
    };

    return parse_enum_value(input, entries, line_cap::butt);
}

line_join parse_line_join(std::string_view input)
{
    static const svg_enumeration_entry<line_join> entries[] = {
        {line_join::miter, "miter"},
        {line_join::round, "round"},
        {line_join::bevel, "bevel"}
    };

    return parse_enum_value(input, entries, line_join::miter);
}

element_id elementid(std::string_view name)
{
    static const struct {
        std::string_view name;
        element_id value;
    } table_[] = {
        {"a", element_id::G},
        {"circle", element_id::circle},
        {"clipPath", element_id::clip_path},
        {"defs", element_id::defs},
        {"ellipse", element_id::ellipse},
        {"feComposite", element_id::fe_composite},
        {"feDropShadow", element_id::fe_drop_shadow},
        {"feFlood", element_id::fe_flood},
        {"feGaussianBlur", element_id::fe_gaussian_blur},
        {"feMerge", element_id::fe_merge},
        {"feMergeNode", element_id::fe_merge_node},
        {"feOffset", element_id::fe_offset},
        {"filter", element_id::filter},
        {"foreignObject", element_id::foreign_object},
        {"g", element_id::G},
        {"image", element_id::image},
        {"line", element_id::line},
        {"linearGradient", element_id::linear_gradient},
        {"marker", element_id::marker},
        {"mask", element_id::mask},
        {"path", element_id::path},
        {"pattern", element_id::pattern},
        {"polygon", element_id::polygon},
        {"polyline", element_id::polyline},
        {"radialGradient", element_id::radial_gradient},
        {"rect", element_id::rect},
        {"stop", element_id::stop},
        {"style", element_id::style},
        {"svg", element_id::svg},
        {"switch", element_id::switch_},
        {"symbol", element_id::symbol},
        {"text", element_id::text},
        {"tspan", element_id::tspan},
        {"use", element_id::use}
    };

    auto it = std::lower_bound(table_, std::end(table_), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table_) || it->name != name)
        return element_id::unknown;
    return it->value;
}

} // namespace nova::svg
