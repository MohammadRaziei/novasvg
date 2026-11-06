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
    } table[] = {
        {"class", property_id::Class},
        {"clipPathUnits", property_id::ClipPathUnits},
        {"cx", property_id::Cx},
        {"cy", property_id::Cy},
        {"d", property_id::D},
        {"dx", property_id::Dx},
        {"dy", property_id::Dy},
        {"fx", property_id::Fx},
        {"fy", property_id::Fy},
        {"gradientTransform", property_id::GradientTransform},
        {"gradientUnits", property_id::GradientUnits},
        {"height", property_id::Height},
        {"href", property_id::Href},
        {"id", property_id::Id},
        {"in", property_id::In},
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
        {"offset", property_id::Offset},
        {"operator", property_id::Operator},
        {"orient", property_id::Orient},
        {"patternContentUnits", property_id::PatternContentUnits},
        {"patternTransform", property_id::PatternTransform},
        {"patternUnits", property_id::PatternUnits},
        {"points", property_id::Points},
        {"preserveAspectRatio", property_id::PreserveAspectRatio},
        {"r", property_id::R},
        {"refX", property_id::RefX},
        {"refY", property_id::RefY},
        {"requiredExtensions", property_id::required_extensions},
        {"result", property_id::Result},
        {"rotate", property_id::Rotate},
        {"rx", property_id::Rx},
        {"ry", property_id::Ry},
        {"spreadMethod", property_id::SpreadMethod},
        {"stdDeviation", property_id::std_deviation},
        {"style", property_id::Style},
        {"systemLanguage", property_id::system_language},
        {"textLength", property_id::TextLength},
        {"transform", property_id::Transform},
        {"viewBox", property_id::ViewBox},
        {"width", property_id::Width},
        {"x", property_id::X},
        {"x1", property_id::X1},
        {"x2", property_id::X2},
        {"xlink:href", property_id::Href},
        {"xml:space", property_id::White_Space},
        {"y", property_id::Y},
        {"y1", property_id::Y1},
        {"y2", property_id::Y2}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return csspropertyid(name);
    return it->value;
}

property_id csspropertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        property_id value;
    } table[] = {
        {"alignment-baseline", property_id::Alignment_Baseline},
        {"background-color", property_id::Background_Color},
        {"baseline-shift", property_id::Baseline_Shift},
        {"clip-path", property_id::Clip_Path},
        {"clip-rule", property_id::Clip_Rule},
        {"color", property_id::Color},
        {"direction", property_id::Direction},
        {"display", property_id::Display},
        {"dominant-baseline", property_id::Dominant_Baseline},
        {"fill", property_id::Fill},
        {"fill-opacity", property_id::Fill_Opacity},
        {"fill-rule", property_id::Fill_Rule},
        {"filter", property_id::Filter},
        {"flood-color", property_id::Flood_Color},
        {"flood-opacity", property_id::Flood_Opacity},
        {"font-family", property_id::Font_Family},
        {"font-size", property_id::Font_Size},
        {"font-style", property_id::Font_Style},
        {"font-weight", property_id::Font_Weight},
        {"letter-spacing", property_id::Letter_Spacing},
        {"marker-end", property_id::Marker_End},
        {"marker-mid", property_id::Marker_Mid},
        {"marker-start", property_id::Marker_Start},
        {"mask", property_id::Mask},
        {"mask-type", property_id::Mask_Type},
        {"mix-blend-mode", property_id::Mix_Blend_Mode},
        {"opacity", property_id::Opacity},
        {"overflow", property_id::Overflow},
        {"pointer-events", property_id::Pointer_Events},
        {"stop-color", property_id::Stop_Color},
        {"stop-opacity", property_id::Stop_Opacity},
        {"stroke", property_id::Stroke},
        {"stroke-dasharray", property_id::Stroke_Dasharray},
        {"stroke-dashoffset", property_id::Stroke_Dashoffset},
        {"stroke-linecap", property_id::Stroke_Linecap},
        {"stroke-linejoin", property_id::Stroke_Linejoin},
        {"stroke-miterlimit", property_id::Stroke_Miterlimit},
        {"stroke-opacity", property_id::Stroke_Opacity},
        {"stroke-width", property_id::Stroke_Width},
        {"text-anchor", property_id::Text_Anchor},
        {"text-orientation", property_id::Text_Orientation},
        {"transform-box", property_id::transform_box},
        {"transform-origin", property_id::transform_origin},
        {"visibility", property_id::Visibility},
        {"white-space", property_id::White_Space},
        {"word-spacing", property_id::Word_Spacing},
        {"writing-mode", property_id::Writing_Mode}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return property_id::unknown;
    return it->value;
}

std::optional<stb::color> parse_color_value(std::string_view& input, const svg_layout_state* state)
{
    if(skipString(input, "currentColor")) {
        return state->color();
    }

    color_t color;
    int length = color_parse(&color, input.data(), input.length());
    if(length == 0)
        return std::nullopt;
    input.remove_prefix(length);
    // color_to_argb32() packs as 0xAARRGGBB (alpha high byte) -- a
    // different layout than stb::color::value()'s 0xRRGGBBAA, so this goes
    // through components, never through the raw packed value.
    auto argb = color_to_argb32(&color);
    return stb::color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
}

stb::color parse_color(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    auto color = parse_color_value(input, state);
    if(!color || !input.empty())
        color = default_value;
    return color.value();
}

stb::color parse_color_or_none(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    if(input.compare("none") == 0)
        return stb::color::transparent();
    return parse_color(input, state, default_value);
}

bool parse_url_value(std::string_view& input, std::string& value)
{
    if(!skipString(input, "url")
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

Paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value)
{
    std::string id;
    if(!parse_url_value(input, id))
        return Paint(parse_color_or_none(input, state, default_value));
    if(skip_optional_spaces(input))
        return Paint(id, parse_color_or_none(input, state, default_value));
    return Paint(id, stb::color::transparent());
}

float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value)
{
    float value;
    if(!parseNumber(input, value))
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

Length parse_length(std::string_view input, length_negative_mode mode, const Length& default_value)
{
    Length value;
    if(!value.parse(input, mode))
        value = default_value;
    return value;
}

BaselineShift parse_baseline_shift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return BaselineShift::Type::Baseline;
    if(input.compare("sub") == 0)
        return BaselineShift::Type::sub;
    if(input.compare("super") == 0)
        return BaselineShift::Type::super;
    return parse_length(input, length_negative_mode::allow, Length(0.f, length_units::None));
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
        Length value(0, length_units::None);
        if(!value.parse(input.substr(0, count), length_negative_mode::forbid))
            return length_list();
        input.remove_prefix(count);
        values.push_back(std::move(value));
    } while(skip_optional_spaces_or_comma(input));
    return values;
}

Length parse_length_or_normal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return Length(0, length_units::None);
    return parse_length(input, length_negative_mode::allow, Length(0, length_units::None));
}

float parse_font_size(std::string_view input, const svg_layout_state* state)
{
    auto length = parse_length(input, length_negative_mode::forbid, Length(12, length_units::None));
    if(length.units() == length_units::percent)
        return length.value() * state->font_size() / 100.f;
    if(length.units() == length_units::ex)
        return length.value() * state->font_size() / 2.f;
    if(length.units() == length_units::em)
        return length.value() * state->font_size();
    return length.value();
}

Display parse_display(std::string_view input)
{
    static const svg_enumeration_entry<Display> entries[] = {
        {Display::Inline, "inline"},
        {Display::None, "none"}
    };

    return parse_enum_value(input, entries, Display::Inline);
}

Visibility parse_visibility(std::string_view input)
{
    static const svg_enumeration_entry<Visibility> entries[] = {
        {Visibility::visible, "visible"},
        {Visibility::hidden, "hidden"},
        {Visibility::collapse, "collapse"}
    };

    return parse_enum_value(input, entries, Visibility::visible);
}

Overflow parse_overflow(std::string_view input)
{
    static const svg_enumeration_entry<Overflow> entries[] = {
        {Overflow::visible, "visible"},
        {Overflow::hidden, "hidden"}
    };

    return parse_enum_value(input, entries, Overflow::visible);
}

PointerEvents parse_pointer_events(std::string_view input)
{
    static const svg_enumeration_entry<PointerEvents> entries[] = {
        {PointerEvents::None,"none"},
        {PointerEvents::Auto,"auto"},
        {PointerEvents::Stroke,"stroke"},
        {PointerEvents::Fill,"fill"},
        {PointerEvents::painted,"painted"},
        {PointerEvents::visible,"visible"},
        {PointerEvents::visible_stroke,"visibleStroke"},
        {PointerEvents::visible_fill,"visibleFill"},
        {PointerEvents::visible_painted,"visiblePainted"},
        {PointerEvents::BoundingBox,"bounding-box"},
        {PointerEvents::All,"all"},
    };

    return parse_enum_value(input, entries, PointerEvents::Auto);
}

FontWeight parse_font_weight(std::string_view input)
{
    static const svg_enumeration_entry<FontWeight> entries[] = {
        {FontWeight::normal, "normal"},
        {FontWeight::Bold, "bold"},
        {FontWeight::Bold, "bolder"},
        {FontWeight::normal, "lighter"},
        {FontWeight::normal, "100"},
        {FontWeight::normal, "200"},
        {FontWeight::normal, "300"},
        {FontWeight::normal, "400"},
        {FontWeight::normal, "500"},
        {FontWeight::Bold, "600"},
        {FontWeight::Bold, "700"},
        {FontWeight::Bold, "800"},
        {FontWeight::Bold, "900"}
    };

    return parse_enum_value(input, entries, FontWeight::normal);
}

FontStyle parse_font_style(std::string_view input)
{
    static const svg_enumeration_entry<FontStyle> entries[] = {
        {FontStyle::normal, "normal"},
        {FontStyle::Italic, "italic"},
        {FontStyle::Italic, "oblique"}
    };

    return parse_enum_value(input, entries, FontStyle::normal);
}

AlignmentBaseline parse_alignment_baseline(std::string_view input)
{
    static const svg_enumeration_entry<AlignmentBaseline> entries[] = {
        {AlignmentBaseline::Auto, "auto"},
        {AlignmentBaseline::Baseline, "baseline"},
        {AlignmentBaseline::before_edge, "before-edge"},
        {AlignmentBaseline::text_before_edge, "text-before-edge"},
        {AlignmentBaseline::middle, "middle"},
        {AlignmentBaseline::central, "central"},
        {AlignmentBaseline::after_edge, "after-edge"},
        {AlignmentBaseline::text_after_edge, "text-after-edge"},
        {AlignmentBaseline::ideographic, "ideographic"},
        {AlignmentBaseline::alphabetic, "alphabetic"},
        {AlignmentBaseline::hanging, "hanging"},
        {AlignmentBaseline::mathematical, "mathematical"}
    };

    return parse_enum_value(input, entries, AlignmentBaseline::Auto);
}

DominantBaseline parse_dominant_baseline(std::string_view input)
{
    static const svg_enumeration_entry<DominantBaseline> entries[] = {
        {DominantBaseline::Auto, "auto"},
        {DominantBaseline::use_script, "use-script"},
        {DominantBaseline::no_change, "no-change"},
        {DominantBaseline::reset_size, "reset-size"},
        {DominantBaseline::ideographic, "ideographic"},
        {DominantBaseline::alphabetic, "alphabetic"},
        {DominantBaseline::hanging, "hanging"},
        {DominantBaseline::mathematical, "mathematical"},
        {DominantBaseline::central, "central"},
        {DominantBaseline::middle, "middle"},
        {DominantBaseline::text_after_edge, "text-after-edge"},
        {DominantBaseline::text_before_edge, "text-before-edge"}
    };

    return parse_enum_value(input, entries, DominantBaseline::Auto);
}

Direction parse_direction(std::string_view input)
{
    static const svg_enumeration_entry<Direction> entries[] = {
        {Direction::ltr, "ltr"},
        {Direction::rtl, "rtl"}
    };

    return parse_enum_value(input, entries, Direction::ltr);
}

WritingMode parse_writing_mode(std::string_view input)
{
    static const svg_enumeration_entry<WritingMode> entries[] = {
        {WritingMode::Horizontal, "horizontal-tb"},
        {WritingMode::vertical, "vertical-rl"},
        {WritingMode::vertical, "vertical-lr"},
        {WritingMode::Horizontal, "lr-tb"},
        {WritingMode::Horizontal, "lr"},
        {WritingMode::Horizontal, "rl-tb"},
        {WritingMode::Horizontal, "rl"},
        {WritingMode::vertical, "tb-rl"},
        {WritingMode::vertical, "tb"}
    };

    return parse_enum_value(input, entries, WritingMode::Horizontal);
}

TextOrientation parse_text_orientation(std::string_view input)
{
    static const svg_enumeration_entry<TextOrientation> entries[] = {
        {TextOrientation::mixed, "mixed"},
        {TextOrientation::upright, "upright"}
    };

    return parse_enum_value(input, entries, TextOrientation::mixed);
}

TextAnchor parse_text_anchor(std::string_view input)
{
    static const svg_enumeration_entry<TextAnchor> entries[] = {
        {TextAnchor::Start, "start"},
        {TextAnchor::middle, "middle"},
        {TextAnchor::End, "end"}
    };

    return parse_enum_value(input, entries, TextAnchor::Start);
}

WhiteSpace parse_white_space(std::string_view input)
{
    static const svg_enumeration_entry<WhiteSpace> entries[] = {
        {WhiteSpace::Default, "default"},
        {WhiteSpace::preserve, "preserve"},
        {WhiteSpace::Default, "normal"},
        {WhiteSpace::Default, "nowrap"},
        {WhiteSpace::Default, "pre-line"},
        {WhiteSpace::preserve, "pre-wrap"},
        {WhiteSpace::preserve, "pre"}
    };

    return parse_enum_value(input, entries, WhiteSpace::Default);
}

MixBlendMode parse_mix_blend_mode(std::string_view input)
{
    static const svg_enumeration_entry<MixBlendMode> entries[] = {
        {MixBlendMode::normal, "normal"},
        {MixBlendMode::Multiply, "multiply"},
        {MixBlendMode::screen, "screen"},
        {MixBlendMode::darken, "darken"},
        {MixBlendMode::lighten, "lighten"},
        {MixBlendMode::difference, "difference"},
        {MixBlendMode::exclusion, "exclusion"}
    };

    // CSS keywords are ASCII case-insensitive
    std::string keyword;
    for(char ch : input)
        keyword.push_back(char(std::tolower(static_cast<unsigned char>(ch))));
    return parse_enum_value(keyword, entries, MixBlendMode::normal);
}

MaskType parse_mask_type(std::string_view input)
{
    static const svg_enumeration_entry<MaskType> entries[] = {
        {MaskType::luminance, "luminance"},
        {MaskType::Alpha, "alpha"}
    };

    return parse_enum_value(input, entries, MaskType::luminance);
}

FillRule parse_fill_rule(std::string_view input)
{
    static const svg_enumeration_entry<FillRule> entries[] = {
        {FillRule::NonZero, "nonzero"},
        {FillRule::EvenOdd, "evenodd"}
    };

    return parse_enum_value(input, entries, FillRule::NonZero);
}

LineCap parse_line_cap(std::string_view input)
{
    static const svg_enumeration_entry<LineCap> entries[] = {
        {LineCap::Butt, "butt"},
        {LineCap::Round, "round"},
        {LineCap::Square, "square"}
    };

    return parse_enum_value(input, entries, LineCap::Butt);
}

LineJoin parse_line_join(std::string_view input)
{
    static const svg_enumeration_entry<LineJoin> entries[] = {
        {LineJoin::Miter, "miter"},
        {LineJoin::Round, "round"},
        {LineJoin::Bevel, "bevel"}
    };

    return parse_enum_value(input, entries, LineJoin::Miter);
}

element_id elementid(std::string_view name)
{
    static const struct {
        std::string_view name;
        element_id value;
    } table[] = {
        {"a", element_id::G},
        {"circle", element_id::Circle},
        {"clipPath", element_id::ClipPath},
        {"defs", element_id::Defs},
        {"ellipse", element_id::ellipse},
        {"feComposite", element_id::fe_composite},
        {"feDropShadow", element_id::fe_drop_shadow},
        {"feFlood", element_id::fe_flood},
        {"feGaussianBlur", element_id::fe_gaussian_blur},
        {"feMerge", element_id::fe_merge},
        {"feMergeNode", element_id::fe_merge_node},
        {"feOffset", element_id::fe_offset},
        {"filter", element_id::Filter},
        {"foreignObject", element_id::ForeignObject},
        {"g", element_id::G},
        {"image", element_id::Image},
        {"line", element_id::Line},
        {"linearGradient", element_id::linear_gradient},
        {"marker", element_id::marker},
        {"mask", element_id::Mask},
        {"path", element_id::Path},
        {"pattern", element_id::Pattern},
        {"polygon", element_id::polygon},
        {"polyline", element_id::polyline},
        {"radialGradient", element_id::radial_gradient},
        {"rect", element_id::Rect},
        {"stop", element_id::Stop},
        {"style", element_id::Style},
        {"svg", element_id::Svg},
        {"switch", element_id::Switch},
        {"symbol", element_id::symbol},
        {"text", element_id::Text},
        {"tspan", element_id::tspan},
        {"use", element_id::use}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return element_id::unknown;
    return it->value;
}

} // namespace nova::svg
