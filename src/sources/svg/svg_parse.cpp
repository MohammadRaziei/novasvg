#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

PropertyID propertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        PropertyID value;
    } table[] = {
        {"class", PropertyID::Class},
        {"clipPathUnits", PropertyID::ClipPathUnits},
        {"cx", PropertyID::Cx},
        {"cy", PropertyID::Cy},
        {"d", PropertyID::D},
        {"dx", PropertyID::Dx},
        {"dy", PropertyID::Dy},
        {"fx", PropertyID::Fx},
        {"fy", PropertyID::Fy},
        {"gradientTransform", PropertyID::GradientTransform},
        {"gradientUnits", PropertyID::GradientUnits},
        {"height", PropertyID::Height},
        {"href", PropertyID::Href},
        {"id", PropertyID::Id},
        {"in", PropertyID::In},
        {"in2", PropertyID::In2},
        {"k1", PropertyID::K1},
        {"k2", PropertyID::K2},
        {"k3", PropertyID::K3},
        {"k4", PropertyID::K4},
        {"lengthAdjust", PropertyID::LengthAdjust},
        {"markerHeight", PropertyID::MarkerHeight},
        {"markerUnits", PropertyID::MarkerUnits},
        {"markerWidth", PropertyID::MarkerWidth},
        {"maskContentUnits", PropertyID::MaskContentUnits},
        {"maskUnits", PropertyID::MaskUnits},
        {"offset", PropertyID::Offset},
        {"operator", PropertyID::Operator},
        {"orient", PropertyID::Orient},
        {"patternContentUnits", PropertyID::PatternContentUnits},
        {"patternTransform", PropertyID::PatternTransform},
        {"patternUnits", PropertyID::PatternUnits},
        {"points", PropertyID::Points},
        {"preserveAspectRatio", PropertyID::PreserveAspectRatio},
        {"r", PropertyID::R},
        {"refX", PropertyID::RefX},
        {"refY", PropertyID::RefY},
        {"requiredExtensions", PropertyID::RequiredExtensions},
        {"result", PropertyID::Result},
        {"rotate", PropertyID::Rotate},
        {"rx", PropertyID::Rx},
        {"ry", PropertyID::Ry},
        {"spreadMethod", PropertyID::SpreadMethod},
        {"stdDeviation", PropertyID::StdDeviation},
        {"style", PropertyID::Style},
        {"systemLanguage", PropertyID::SystemLanguage},
        {"textLength", PropertyID::TextLength},
        {"transform", PropertyID::Transform},
        {"viewBox", PropertyID::ViewBox},
        {"width", PropertyID::Width},
        {"x", PropertyID::X},
        {"x1", PropertyID::X1},
        {"x2", PropertyID::X2},
        {"xlink:href", PropertyID::Href},
        {"xml:space", PropertyID::White_Space},
        {"y", PropertyID::Y},
        {"y1", PropertyID::Y1},
        {"y2", PropertyID::Y2}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return csspropertyid(name);
    return it->value;
}

PropertyID csspropertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        PropertyID value;
    } table[] = {
        {"alignment-baseline", PropertyID::Alignment_Baseline},
        {"background-color", PropertyID::Background_Color},
        {"baseline-shift", PropertyID::Baseline_Shift},
        {"clip-path", PropertyID::Clip_Path},
        {"clip-rule", PropertyID::Clip_Rule},
        {"color", PropertyID::Color},
        {"direction", PropertyID::Direction},
        {"display", PropertyID::Display},
        {"dominant-baseline", PropertyID::Dominant_Baseline},
        {"fill", PropertyID::Fill},
        {"fill-opacity", PropertyID::Fill_Opacity},
        {"fill-rule", PropertyID::Fill_Rule},
        {"filter", PropertyID::Filter},
        {"flood-color", PropertyID::Flood_Color},
        {"flood-opacity", PropertyID::Flood_Opacity},
        {"font-family", PropertyID::Font_Family},
        {"font-size", PropertyID::Font_Size},
        {"font-style", PropertyID::Font_Style},
        {"font-weight", PropertyID::Font_Weight},
        {"letter-spacing", PropertyID::Letter_Spacing},
        {"marker-end", PropertyID::Marker_End},
        {"marker-mid", PropertyID::Marker_Mid},
        {"marker-start", PropertyID::Marker_Start},
        {"mask", PropertyID::Mask},
        {"mask-type", PropertyID::Mask_Type},
        {"mix-blend-mode", PropertyID::Mix_Blend_Mode},
        {"opacity", PropertyID::Opacity},
        {"overflow", PropertyID::Overflow},
        {"pointer-events", PropertyID::Pointer_Events},
        {"stop-color", PropertyID::Stop_Color},
        {"stop-opacity", PropertyID::Stop_Opacity},
        {"stroke", PropertyID::Stroke},
        {"stroke-dasharray", PropertyID::Stroke_Dasharray},
        {"stroke-dashoffset", PropertyID::Stroke_Dashoffset},
        {"stroke-linecap", PropertyID::Stroke_Linecap},
        {"stroke-linejoin", PropertyID::Stroke_Linejoin},
        {"stroke-miterlimit", PropertyID::Stroke_Miterlimit},
        {"stroke-opacity", PropertyID::Stroke_Opacity},
        {"stroke-width", PropertyID::Stroke_Width},
        {"text-anchor", PropertyID::Text_Anchor},
        {"text-orientation", PropertyID::Text_Orientation},
        {"transform-box", PropertyID::Transform_Box},
        {"transform-origin", PropertyID::Transform_Origin},
        {"visibility", PropertyID::Visibility},
        {"white-space", PropertyID::White_Space},
        {"word-spacing", PropertyID::Word_Spacing},
        {"writing-mode", PropertyID::Writing_Mode}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return PropertyID::Unknown;
    return it->value;
}

std::optional<stb::color> parseColorValue(std::string_view& input, const svg_layout_state* state)
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

stb::color parseColor(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue)
{
    auto color = parseColorValue(input, state);
    if(!color || !input.empty())
        color = defaultValue;
    return color.value();
}

stb::color parseColorOrNone(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue)
{
    if(input.compare("none") == 0)
        return stb::color::transparent();
    return parseColor(input, state, defaultValue);
}

bool parseUrlValue(std::string_view& input, std::string& value)
{
    if(!skipString(input, "url")
        || !skipOptionalSpaces(input)
        || !skipDelimiter(input, '(')
        || !skipOptionalSpaces(input)) {
        return false;
    }

    switch(input.front()) {
    case '\'':
    case '\"': {
        auto delim = input.front();
        input.remove_prefix(1);
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, '#'))
            return false;
        while(!input.empty() && input.front() != delim) {
            value += input.front();
            input.remove_prefix(1);
        }

        skipOptionalSpaces(input);
        if(!skipDelimiter(input, delim))
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

    return skipOptionalSpaces(input) && skipDelimiter(input, ')');
}

std::string parseUrl(std::string_view input)
{
    std::string value;
    if(!parseUrlValue(input, value) || !input.empty())
        value.clear();
    return value;
}

Paint parsePaint(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue)
{
    std::string id;
    if(!parseUrlValue(input, id))
        return Paint(parseColorOrNone(input, state, defaultValue));
    if(skipOptionalSpaces(input))
        return Paint(id, parseColorOrNone(input, state, defaultValue));
    return Paint(id, stb::color::transparent());
}

float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue)
{
    float value;
    if(!parseNumber(input, value))
        return defaultValue;
    if(allowPercentage) {
        if(skipDelimiter(input, '%'))
            value /= 100.f;
        value = std::clamp(value, 0.f, 1.f);
    }

    if(!input.empty())
        return defaultValue;
    return value;
}

Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue)
{
    Length value;
    if(!value.parse(input, mode))
        value = defaultValue;
    return value;
}

BaselineShift parseBaselineShift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return BaselineShift::Type::Baseline;
    if(input.compare("sub") == 0)
        return BaselineShift::Type::Sub;
    if(input.compare("super") == 0)
        return BaselineShift::Type::Super;
    return parseLength(input, LengthNegativeMode::Allow, Length(0.f, LengthUnits::None));
}

LengthList parseDashArray(std::string_view input)
{
    if(input.compare("none") == 0)
        return LengthList();
    LengthList values;
    do {
        size_t count = 0;
        while(count < input.length() && input[count] != ',' && !IS_WS(input[count]))
            ++count;
        Length value(0, LengthUnits::None);
        if(!value.parse(input.substr(0, count), LengthNegativeMode::Forbid))
            return LengthList();
        input.remove_prefix(count);
        values.push_back(std::move(value));
    } while(skipOptionalSpacesOrComma(input));
    return values;
}

Length parseLengthOrNormal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return Length(0, LengthUnits::None);
    return parseLength(input, LengthNegativeMode::Allow, Length(0, LengthUnits::None));
}

float parseFontSize(std::string_view input, const svg_layout_state* state)
{
    auto length = parseLength(input, LengthNegativeMode::Forbid, Length(12, LengthUnits::None));
    if(length.units() == LengthUnits::Percent)
        return length.value() * state->font_size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * state->font_size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * state->font_size();
    return length.value();
}

Display parseDisplay(std::string_view input)
{
    static const svg_enumeration_entry<Display> entries[] = {
        {Display::Inline, "inline"},
        {Display::None, "none"}
    };

    return parseEnumValue(input, entries, Display::Inline);
}

Visibility parseVisibility(std::string_view input)
{
    static const svg_enumeration_entry<Visibility> entries[] = {
        {Visibility::Visible, "visible"},
        {Visibility::Hidden, "hidden"},
        {Visibility::Collapse, "collapse"}
    };

    return parseEnumValue(input, entries, Visibility::Visible);
}

Overflow parseOverflow(std::string_view input)
{
    static const svg_enumeration_entry<Overflow> entries[] = {
        {Overflow::Visible, "visible"},
        {Overflow::Hidden, "hidden"}
    };

    return parseEnumValue(input, entries, Overflow::Visible);
}

PointerEvents parsePointerEvents(std::string_view input)
{
    static const svg_enumeration_entry<PointerEvents> entries[] = {
        {PointerEvents::None,"none"},
        {PointerEvents::Auto,"auto"},
        {PointerEvents::Stroke,"stroke"},
        {PointerEvents::Fill,"fill"},
        {PointerEvents::Painted,"painted"},
        {PointerEvents::Visible,"visible"},
        {PointerEvents::VisibleStroke,"visibleStroke"},
        {PointerEvents::VisibleFill,"visibleFill"},
        {PointerEvents::VisiblePainted,"visiblePainted"},
        {PointerEvents::BoundingBox,"bounding-box"},
        {PointerEvents::All,"all"},
    };

    return parseEnumValue(input, entries, PointerEvents::Auto);
}

FontWeight parseFontWeight(std::string_view input)
{
    static const svg_enumeration_entry<FontWeight> entries[] = {
        {FontWeight::Normal, "normal"},
        {FontWeight::Bold, "bold"},
        {FontWeight::Bold, "bolder"},
        {FontWeight::Normal, "lighter"},
        {FontWeight::Normal, "100"},
        {FontWeight::Normal, "200"},
        {FontWeight::Normal, "300"},
        {FontWeight::Normal, "400"},
        {FontWeight::Normal, "500"},
        {FontWeight::Bold, "600"},
        {FontWeight::Bold, "700"},
        {FontWeight::Bold, "800"},
        {FontWeight::Bold, "900"}
    };

    return parseEnumValue(input, entries, FontWeight::Normal);
}

FontStyle parseFontStyle(std::string_view input)
{
    static const svg_enumeration_entry<FontStyle> entries[] = {
        {FontStyle::Normal, "normal"},
        {FontStyle::Italic, "italic"},
        {FontStyle::Italic, "oblique"}
    };

    return parseEnumValue(input, entries, FontStyle::Normal);
}

AlignmentBaseline parseAlignmentBaseline(std::string_view input)
{
    static const svg_enumeration_entry<AlignmentBaseline> entries[] = {
        {AlignmentBaseline::Auto, "auto"},
        {AlignmentBaseline::Baseline, "baseline"},
        {AlignmentBaseline::BeforeEdge, "before-edge"},
        {AlignmentBaseline::TextBeforeEdge, "text-before-edge"},
        {AlignmentBaseline::Middle, "middle"},
        {AlignmentBaseline::Central, "central"},
        {AlignmentBaseline::AfterEdge, "after-edge"},
        {AlignmentBaseline::TextAfterEdge, "text-after-edge"},
        {AlignmentBaseline::Ideographic, "ideographic"},
        {AlignmentBaseline::Alphabetic, "alphabetic"},
        {AlignmentBaseline::Hanging, "hanging"},
        {AlignmentBaseline::Mathematical, "mathematical"}
    };

    return parseEnumValue(input, entries, AlignmentBaseline::Auto);
}

DominantBaseline parseDominantBaseline(std::string_view input)
{
    static const svg_enumeration_entry<DominantBaseline> entries[] = {
        {DominantBaseline::Auto, "auto"},
        {DominantBaseline::UseScript, "use-script"},
        {DominantBaseline::NoChange, "no-change"},
        {DominantBaseline::ResetSize, "reset-size"},
        {DominantBaseline::Ideographic, "ideographic"},
        {DominantBaseline::Alphabetic, "alphabetic"},
        {DominantBaseline::Hanging, "hanging"},
        {DominantBaseline::Mathematical, "mathematical"},
        {DominantBaseline::Central, "central"},
        {DominantBaseline::Middle, "middle"},
        {DominantBaseline::TextAfterEdge, "text-after-edge"},
        {DominantBaseline::TextBeforeEdge, "text-before-edge"}
    };

    return parseEnumValue(input, entries, DominantBaseline::Auto);
}

Direction parseDirection(std::string_view input)
{
    static const svg_enumeration_entry<Direction> entries[] = {
        {Direction::Ltr, "ltr"},
        {Direction::Rtl, "rtl"}
    };

    return parseEnumValue(input, entries, Direction::Ltr);
}

WritingMode parseWritingMode(std::string_view input)
{
    static const svg_enumeration_entry<WritingMode> entries[] = {
        {WritingMode::Horizontal, "horizontal-tb"},
        {WritingMode::Vertical, "vertical-rl"},
        {WritingMode::Vertical, "vertical-lr"},
        {WritingMode::Horizontal, "lr-tb"},
        {WritingMode::Horizontal, "lr"},
        {WritingMode::Horizontal, "rl-tb"},
        {WritingMode::Horizontal, "rl"},
        {WritingMode::Vertical, "tb-rl"},
        {WritingMode::Vertical, "tb"}
    };

    return parseEnumValue(input, entries, WritingMode::Horizontal);
}

TextOrientation parseTextOrientation(std::string_view input)
{
    static const svg_enumeration_entry<TextOrientation> entries[] = {
        {TextOrientation::Mixed, "mixed"},
        {TextOrientation::Upright, "upright"}
    };

    return parseEnumValue(input, entries, TextOrientation::Mixed);
}

TextAnchor parseTextAnchor(std::string_view input)
{
    static const svg_enumeration_entry<TextAnchor> entries[] = {
        {TextAnchor::Start, "start"},
        {TextAnchor::Middle, "middle"},
        {TextAnchor::End, "end"}
    };

    return parseEnumValue(input, entries, TextAnchor::Start);
}

WhiteSpace parseWhiteSpace(std::string_view input)
{
    static const svg_enumeration_entry<WhiteSpace> entries[] = {
        {WhiteSpace::Default, "default"},
        {WhiteSpace::Preserve, "preserve"},
        {WhiteSpace::Default, "normal"},
        {WhiteSpace::Default, "nowrap"},
        {WhiteSpace::Default, "pre-line"},
        {WhiteSpace::Preserve, "pre-wrap"},
        {WhiteSpace::Preserve, "pre"}
    };

    return parseEnumValue(input, entries, WhiteSpace::Default);
}

MixBlendMode parseMixBlendMode(std::string_view input)
{
    static const svg_enumeration_entry<MixBlendMode> entries[] = {
        {MixBlendMode::Normal, "normal"},
        {MixBlendMode::Multiply, "multiply"},
        {MixBlendMode::Screen, "screen"},
        {MixBlendMode::Darken, "darken"},
        {MixBlendMode::Lighten, "lighten"},
        {MixBlendMode::Difference, "difference"},
        {MixBlendMode::Exclusion, "exclusion"}
    };

    // CSS keywords are ASCII case-insensitive
    std::string keyword;
    for(char ch : input)
        keyword.push_back(char(std::tolower(static_cast<unsigned char>(ch))));
    return parseEnumValue(keyword, entries, MixBlendMode::Normal);
}

MaskType parseMaskType(std::string_view input)
{
    static const svg_enumeration_entry<MaskType> entries[] = {
        {MaskType::Luminance, "luminance"},
        {MaskType::Alpha, "alpha"}
    };

    return parseEnumValue(input, entries, MaskType::Luminance);
}

FillRule parseFillRule(std::string_view input)
{
    static const svg_enumeration_entry<FillRule> entries[] = {
        {FillRule::NonZero, "nonzero"},
        {FillRule::EvenOdd, "evenodd"}
    };

    return parseEnumValue(input, entries, FillRule::NonZero);
}

LineCap parseLineCap(std::string_view input)
{
    static const svg_enumeration_entry<LineCap> entries[] = {
        {LineCap::Butt, "butt"},
        {LineCap::Round, "round"},
        {LineCap::Square, "square"}
    };

    return parseEnumValue(input, entries, LineCap::Butt);
}

LineJoin parseLineJoin(std::string_view input)
{
    static const svg_enumeration_entry<LineJoin> entries[] = {
        {LineJoin::Miter, "miter"},
        {LineJoin::Round, "round"},
        {LineJoin::Bevel, "bevel"}
    };

    return parseEnumValue(input, entries, LineJoin::Miter);
}

ElementID elementid(std::string_view name)
{
    static const struct {
        std::string_view name;
        ElementID value;
    } table[] = {
        {"a", ElementID::G},
        {"circle", ElementID::Circle},
        {"clipPath", ElementID::ClipPath},
        {"defs", ElementID::Defs},
        {"ellipse", ElementID::Ellipse},
        {"feComposite", ElementID::FeComposite},
        {"feDropShadow", ElementID::FeDropShadow},
        {"feFlood", ElementID::FeFlood},
        {"feGaussianBlur", ElementID::FeGaussianBlur},
        {"feMerge", ElementID::FeMerge},
        {"feMergeNode", ElementID::FeMergeNode},
        {"feOffset", ElementID::FeOffset},
        {"filter", ElementID::Filter},
        {"foreignObject", ElementID::ForeignObject},
        {"g", ElementID::G},
        {"image", ElementID::Image},
        {"line", ElementID::Line},
        {"linearGradient", ElementID::LinearGradient},
        {"marker", ElementID::Marker},
        {"mask", ElementID::Mask},
        {"path", ElementID::Path},
        {"pattern", ElementID::Pattern},
        {"polygon", ElementID::Polygon},
        {"polyline", ElementID::Polyline},
        {"radialGradient", ElementID::RadialGradient},
        {"rect", ElementID::Rect},
        {"stop", ElementID::Stop},
        {"style", ElementID::Style},
        {"svg", ElementID::Svg},
        {"switch", ElementID::Switch},
        {"symbol", ElementID::Symbol},
        {"text", ElementID::Text},
        {"tspan", ElementID::Tspan},
        {"use", ElementID::Use}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return ElementID::Unknown;
    return it->value;
}

} // namespace nova::svg
