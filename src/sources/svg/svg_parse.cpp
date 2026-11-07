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
        {"clipPathUnits", property_id::ClipPathUnits},
        {"cx", property_id::cx},
        {"cy", property_id::cy},
        {"d", property_id::D},
        {"dx", property_id::dx},
        {"dy", property_id::dy},
        {"fx", property_id::fx},
        {"fy", property_id::fy},
        {"gradientTransform", property_id::GradientTransform},
        {"gradientUnits", property_id::GradientUnits},
        {"height", property_id::height},
        {"href", property_id::href},
        {"id", property_id::id},
        {"in", property_id::in},
        {"in2", property_id::in2},
        {"k1", property_id::K1},
        {"k2", property_id::K2},
        {"k3", property_id::K3},
        {"k4", property_id::K4},
        {"lengthAdjust", property_id::LengthAdjust},
        {"markerHeight", property_id::MarkerHeight},
        {"markerUnits", property_id::MarkerUnits},
        {"markerWidth", property_id::MarkerWidth},
        {"maskContentUnits", property_id::MaskContentUnits},
        {"maskUnits", property_id::MaskUnits},
        {"offset", property_id::offset},
        {"operator", property_id::operator_},
        {"orient", property_id::orient},
        {"patternContentUnits", property_id::PatternContentUnits},
        {"patternTransform", property_id::PatternTransform},
        {"patternUnits", property_id::PatternUnits},
        {"points", property_id::points},
        {"preserveAspectRatio", property_id::PreserveAspectRatio},
        {"r", property_id::R},
        {"refX", property_id::RefX},
        {"refY", property_id::RefY},
        {"requiredExtensions", property_id::required_extensions},
        {"result", property_id::result},
        {"rotate", property_id::rotate},
        {"rx", property_id::rx},
        {"ry", property_id::ry},
        {"spreadMethod", property_id::SpreadMethod},
        {"stdDeviation", property_id::std_deviation},
        {"style", property_id::style},
        {"systemLanguage", property_id::system_language},
        {"textLength", property_id::TextLength},
        {"transform", property_id::transform},
        {"viewBox", property_id::ViewBox},
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
        {"flood-color", property_id::Flood_Color},
        {"flood-opacity", property_id::Flood_Opacity},
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
        return state->color_();
    }

    color_t color_;
    int length_ = color_parse(&color_, input.data(), input.length());
    if(length_ == 0)
        return std::nullopt;
    input.remove_prefix(length_);
    // color_to_argb32() packs as 0xAARRGGBB (alpha high byte) -- a
    // different layout than stb::color::value()'s 0xRRGGBBAA, so this goes
    // through components, never through the raw packed value.
    auto argb = color_to_argb32(&color_);
    return stb::color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
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

length parse_length(std::string_view input, length_negative_mode mode, const length& default_value)
{
    length value;
    if(!value.parse(input, mode))
        value = default_value;
    return value;
}

baseline_shift parse_baseline_shift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return baseline_shift::type::baseline;
    if(input.compare("sub") == 0)
        return baseline_shift::type::sub;
    if(input.compare("super") == 0)
        return baseline_shift::type::super;
    return parse_length(input, length_negative_mode::allow, length(0.f, length_units::none));
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
        length value(0, length_units::none);
        if(!value.parse(input.substr(0, count), length_negative_mode::forbid))
            return length_list();
        input.remove_prefix(count);
        values.push_back(std::move(value));
    } while(skip_optional_spaces_or_comma(input));
    return values;
}

length parse_length_or_normal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return length(0, length_units::none);
    return parse_length(input, length_negative_mode::allow, length(0, length_units::none));
}

float parse_font_size(std::string_view input, const svg_layout_state* state)
{
    auto length_ = parse_length(input, length_negative_mode::forbid, length(12, length_units::none));
    if(length_.units_() == length_units::percent)
        return length_.value() * state->font_size() / 100.f;
    if(length_.units_() == length_units::ex)
        return length_.value() * state->font_size() / 2.f;
    if(length_.units_() == length_units::em)
        return length_.value() * state->font_size();
    return length_.value();
}

display parse_display(std::string_view input)
{
    static const svg_enumeration_entry<display> entries[] = {
        {display::inline_, "inline"},
        {display::none, "none"}
    };

    return parse_enum_value(input, entries, display::inline_);
}

visibility parse_visibility(std::string_view input)
{
    static const svg_enumeration_entry<visibility> entries[] = {
        {visibility::visible, "visible"},
        {visibility::hidden, "hidden"},
        {visibility::collapse, "collapse"}
    };

    return parse_enum_value(input, entries, visibility::visible);
}

overflow parse_overflow(std::string_view input)
{
    static const svg_enumeration_entry<overflow> entries[] = {
        {overflow::visible, "visible"},
        {overflow::hidden, "hidden"}
    };

    return parse_enum_value(input, entries, overflow::visible);
}

pointer_events parse_pointer_events(std::string_view input)
{
    static const svg_enumeration_entry<pointer_events> entries[] = {
        {pointer_events::none,"none"},
        {pointer_events::auto_,"auto"},
        {pointer_events::stroke,"stroke"},
        {pointer_events::fill,"fill"},
        {pointer_events::painted,"painted"},
        {pointer_events::visible,"visible"},
        {pointer_events::visible_stroke,"visibleStroke"},
        {pointer_events::visible_fill,"visibleFill"},
        {pointer_events::visible_painted,"visiblePainted"},
        {pointer_events::bounding_box,"bounding-box"},
        {pointer_events::all,"all"},
    };

    return parse_enum_value(input, entries, pointer_events::auto_);
}

font_weight parse_font_weight(std::string_view input)
{
    static const svg_enumeration_entry<font_weight> entries[] = {
        {font_weight::normal, "normal"},
        {font_weight::bold, "bold"},
        {font_weight::bold, "bolder"},
        {font_weight::normal, "lighter"},
        {font_weight::normal, "100"},
        {font_weight::normal, "200"},
        {font_weight::normal, "300"},
        {font_weight::normal, "400"},
        {font_weight::normal, "500"},
        {font_weight::bold, "600"},
        {font_weight::bold, "700"},
        {font_weight::bold, "800"},
        {font_weight::bold, "900"}
    };

    return parse_enum_value(input, entries, font_weight::normal);
}

font_style parse_font_style(std::string_view input)
{
    static const svg_enumeration_entry<font_style> entries[] = {
        {font_style::normal, "normal"},
        {font_style::italic, "italic"},
        {font_style::italic, "oblique"}
    };

    return parse_enum_value(input, entries, font_style::normal);
}

alignment_baseline parse_alignment_baseline(std::string_view input)
{
    static const svg_enumeration_entry<alignment_baseline> entries[] = {
        {alignment_baseline::auto_, "auto"},
        {alignment_baseline::baseline, "baseline"},
        {alignment_baseline::before_edge, "before-edge"},
        {alignment_baseline::text_before_edge, "text-before-edge"},
        {alignment_baseline::middle, "middle"},
        {alignment_baseline::central, "central"},
        {alignment_baseline::after_edge, "after-edge"},
        {alignment_baseline::text_after_edge, "text-after-edge"},
        {alignment_baseline::ideographic, "ideographic"},
        {alignment_baseline::alphabetic, "alphabetic"},
        {alignment_baseline::hanging, "hanging"},
        {alignment_baseline::mathematical, "mathematical"}
    };

    return parse_enum_value(input, entries, alignment_baseline::auto_);
}

dominant_baseline parse_dominant_baseline(std::string_view input)
{
    static const svg_enumeration_entry<dominant_baseline> entries[] = {
        {dominant_baseline::auto_, "auto"},
        {dominant_baseline::use_script, "use-script"},
        {dominant_baseline::no_change, "no-change"},
        {dominant_baseline::reset_size, "reset-size"},
        {dominant_baseline::ideographic, "ideographic"},
        {dominant_baseline::alphabetic, "alphabetic"},
        {dominant_baseline::hanging, "hanging"},
        {dominant_baseline::mathematical, "mathematical"},
        {dominant_baseline::central, "central"},
        {dominant_baseline::middle, "middle"},
        {dominant_baseline::text_after_edge, "text-after-edge"},
        {dominant_baseline::text_before_edge, "text-before-edge"}
    };

    return parse_enum_value(input, entries, dominant_baseline::auto_);
}

direction parse_direction(std::string_view input)
{
    static const svg_enumeration_entry<direction> entries[] = {
        {direction::ltr, "ltr"},
        {direction::rtl, "rtl"}
    };

    return parse_enum_value(input, entries, direction::ltr);
}

writing_mode parse_writing_mode(std::string_view input)
{
    static const svg_enumeration_entry<writing_mode> entries[] = {
        {writing_mode::horizontal, "horizontal-tb"},
        {writing_mode::vertical, "vertical-rl"},
        {writing_mode::vertical, "vertical-lr"},
        {writing_mode::horizontal, "lr-tb"},
        {writing_mode::horizontal, "lr"},
        {writing_mode::horizontal, "rl-tb"},
        {writing_mode::horizontal, "rl"},
        {writing_mode::vertical, "tb-rl"},
        {writing_mode::vertical, "tb"}
    };

    return parse_enum_value(input, entries, writing_mode::horizontal);
}

text_orientation parse_text_orientation(std::string_view input)
{
    static const svg_enumeration_entry<text_orientation> entries[] = {
        {text_orientation::mixed, "mixed"},
        {text_orientation::upright, "upright"}
    };

    return parse_enum_value(input, entries, text_orientation::mixed);
}

text_anchor parse_text_anchor(std::string_view input)
{
    static const svg_enumeration_entry<text_anchor> entries[] = {
        {text_anchor::start, "start"},
        {text_anchor::middle, "middle"},
        {text_anchor::end, "end"}
    };

    return parse_enum_value(input, entries, text_anchor::start);
}

white_space parse_white_space(std::string_view input)
{
    static const svg_enumeration_entry<white_space> entries[] = {
        {white_space::default_, "default"},
        {white_space::preserve, "preserve"},
        {white_space::default_, "normal"},
        {white_space::default_, "nowrap"},
        {white_space::default_, "pre-line"},
        {white_space::preserve, "pre-wrap"},
        {white_space::preserve, "pre"}
    };

    return parse_enum_value(input, entries, white_space::default_);
}

mix_blend_mode parse_mix_blend_mode(std::string_view input)
{
    static const svg_enumeration_entry<mix_blend_mode> entries[] = {
        {mix_blend_mode::normal, "normal"},
        {mix_blend_mode::multiply, "multiply"},
        {mix_blend_mode::screen, "screen"},
        {mix_blend_mode::darken, "darken"},
        {mix_blend_mode::lighten, "lighten"},
        {mix_blend_mode::difference, "difference"},
        {mix_blend_mode::exclusion, "exclusion"}
    };

    // CSS keywords are ASCII case-insensitive
    std::string keyword;
    for(char ch : input)
        keyword.push_back(char(std::tolower(static_cast<unsigned char>(ch))));
    return parse_enum_value(keyword, entries, mix_blend_mode::normal);
}

mask_type parse_mask_type(std::string_view input)
{
    static const svg_enumeration_entry<mask_type> entries[] = {
        {mask_type::luminance, "luminance"},
        {mask_type::alpha, "alpha"}
    };

    return parse_enum_value(input, entries, mask_type::luminance);
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
        {"foreignObject", element_id::ForeignObject},
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
