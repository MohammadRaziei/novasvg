// Definitions of the SVG element classes declared in svgelement.h.
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

svg_property::svg_property(PropertyID id)
    : m_id(id)
{
}

bool svg_string::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    m_value.assign(input);
    return true;
}

bool svg_angle::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    if(input == "auto") {
        m_value = 0.f;
        m_orientType = OrientType::Auto;
        return true;
    }

    if(input == "auto-start-reverse") {
        m_value = 0.f;
        m_orientType = OrientType::AutoStartReverse;
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
    m_orientType = OrientType::Angle;
    return true;
}

bool Length::parse(std::string_view input, LengthNegativeMode mode)
{
    float value = 0.f;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value))
        return false;
    if(value < 0.f && mode == LengthNegativeMode::Forbid)
        return false;
    if(input.empty()) {
        m_value = value;
        m_units = LengthUnits::None;
        return true;
    }

    constexpr auto dpi = 96.f;
    switch(input.front()) {
    case '%':
        m_value = value;
        m_units = LengthUnits::Percent;
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
        m_units = LengthUnits::Px;
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
        m_units = LengthUnits::Px;
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
        m_units = LengthUnits::Px;
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
        m_units = LengthUnits::Px;
        input.remove_prefix(1);
        break;
    case 'e':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_units = LengthUnits::Em;
        else if(input.front() == 'x')
            m_units = LengthUnits::Ex;
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

float LengthContext::valueForLength(const Length& length, LengthDirection direction) const
{
    if(length.units() == LengthUnits::Percent) {
        if(m_units == Units::UserSpaceOnUse)
            return length.value() * viewportDimension(direction) / 100.f;
        return length.value() / 100.f;
    }

    if(length.units() == LengthUnits::Ex)
        return length.value() * m_element->font_size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_element->font_size();
    return length.value();
}

float LengthContext::viewportDimension(LengthDirection direction) const
{
    auto viewportSize = m_element->currentViewportSize();
    switch(direction) {
    case LengthDirection::Horizontal:
        return viewportSize.w;
    case LengthDirection::Vertical:
        return viewportSize.h;
    default:
        return std::sqrt(viewportSize.w * viewportSize.w + viewportSize.h * viewportSize.h) / NOVA_SQRT2;
    }
}

bool svg_length::parse(std::string_view input)
{
    return m_value.parse(input, m_negativeMode);
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
        Length value(0, LengthUnits::None);
        if(!value.parse(input.substr(0, count), m_negativeMode))
            return false;
        input.remove_prefix(count);
        skipOptionalSpacesOrComma(input);
        m_values.push_back(value);
    }

    return true;
}

bool svg_number::parse(std::string_view input)
{
    float value = 0.f;
    stripLeadingAndTrailingSpaces(input);
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
    stripLeadingAndTrailingSpaces(input);
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
    stripLeadingSpaces(input);
    while(!input.empty()) {
        float value = 0.f;
        if(!parseNumber(input, value))
            return false;
        skipOptionalSpacesOrComma(input);
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
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value.x)
        || !skipOptionalSpaces(input)
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
    stripLeadingSpaces(input);
    while(!input.empty()) {
        Point value;
        if(!parseNumber(input, value.x)
            || !skipOptionalSpacesOrComma(input)
            || !parseNumber(input, value.y)) {
            return false;
        }

        m_values.push_back(value);
        skipOptionalSpacesOrComma(input);
    }

    return true;
}

bool svg_rect::parse(std::string_view input)
{
    Rect value;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value.x)
        || !skipOptionalSpacesOrComma(input)
        || !parseNumber(input, value.y)
        || !skipOptionalSpacesOrComma(input)
        || !parseNumber(input, value.w)
        || !skipOptionalSpacesOrComma(input)
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
    stripLeadingAndTrailingSpaces(input);
    if(input == "fill-box" || input == "content-box") {
        m_value = TransformBoxType::FillBox;
    } else if(input == "view-box" || input == "border-box" || input == "stroke-box") {
        // border-box/stroke-box aren't distinguished from view-box here (see
        // the class comment) -- treated as "not fill-box" for origin purposes.
        m_value = TransformBoxType::ViewBox;
    } else {
        return false;
    }

    m_specified = true;
    return true;
}

bool svg_transform_origin::parse(std::string_view input)
{
    auto parseComponent = [](std::string_view token, float& value, bool& isPercent) {
        if(token == "left" || token == "top") { value = 0.f; isPercent = true; return true; }
        if(token == "center") { value = 0.5f; isPercent = true; return true; }
        if(token == "right" || token == "bottom") { value = 1.f; isPercent = true; return true; }
        float number = 0.f;
        if(!parseNumber(token, number))
            return false;
        if(!token.empty() && token.front() == '%') {
            value = number / 100.f;
            isPercent = true;
            token.remove_prefix(1);
        } else {
            value = number;
            isPercent = false;
        }
        return token.empty();
    };

    std::vector<std::string_view> tokens;
    stripLeadingAndTrailingSpaces(input);
    while(!input.empty()) {
        size_t count = 0;
        while(count < input.length() && !IS_WS(input[count]))
            ++count;
        tokens.push_back(input.substr(0, count));
        input.remove_prefix(count);
        skipOptionalSpaces(input);
    }

    if(tokens.empty() || tokens.size() > 2)
        return false;

    float x = 0.5f, y = 0.5f;
    bool xIsPercent = true, yIsPercent = true;
    if(tokens.size() == 1) {
        if(tokens[0] == "top" || tokens[0] == "bottom") {
            if(!parseComponent(tokens[0], y, yIsPercent))
                return false;
        } else if(!parseComponent(tokens[0], x, xIsPercent)) {
            return false;
        }
    } else {
        if(!parseComponent(tokens[0], x, xIsPercent) || !parseComponent(tokens[1], y, yIsPercent))
            return false;
    }

    m_x = x;
    m_y = y;
    m_xIsPercent = xIsPercent;
    m_yIsPercent = yIsPercent;
    m_specified = true;
    return true;
}

bool svg_preserve_aspect_ratio::parse(std::string_view input)
{
    auto alignType = AlignType::xMidYMid;
    stripLeadingSpaces(input);
    if(skipString(input, "none"))
        alignType = AlignType::None;
    else if(skipString(input, "xMinYMin"))
        alignType = AlignType::xMinYMin;
    else if(skipString(input, "xMidYMin"))
        alignType = AlignType::xMidYMin;
    else if(skipString(input, "xMaxYMin"))
        alignType = AlignType::xMaxYMin;
    else if(skipString(input, "xMinYMid"))
        alignType = AlignType::xMinYMid;
    else if(skipString(input, "xMidYMid"))
        alignType = AlignType::xMidYMid;
    else if(skipString(input, "xMaxYMid"))
        alignType = AlignType::xMaxYMid;
    else if(skipString(input, "xMinYMax"))
        alignType = AlignType::xMinYMax;
    else if(skipString(input, "xMidYMax"))
        alignType = AlignType::xMidYMax;
    else if(skipString(input, "xMaxYMax"))
        alignType = AlignType::xMaxYMax;
    else {
        return false;
    }

    auto meetOrSlice = MeetOrSlice::Meet;
    skipOptionalSpaces(input);
    if(skipString(input, "meet")) {
        meetOrSlice = MeetOrSlice::Meet;
    } else if(skipString(input, "slice")) {
        meetOrSlice = MeetOrSlice::Slice;
    }

    if(alignType == AlignType::None)
        meetOrSlice = MeetOrSlice::Meet;
    skipOptionalSpaces(input);
    if(!input.empty())
        return false;
    m_alignType = alignType;
    m_meetOrSlice = meetOrSlice;
    return true;
}

Rect svg_preserve_aspect_ratio::getClipRect(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.empty() && !viewportSize.empty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Rect(viewBoxRect.x, viewBoxRect.y, viewportSize.w / xScale, viewportSize.h / yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? STB_MIN(xScale, yScale) : STB_MAX(xScale, yScale);
    auto xOffset = -viewBoxRect.x * scale;
    auto yOffset = -viewBoxRect.y * scale;
    auto viewWidth = viewBoxRect.w * scale;
    auto viewHeight = viewBoxRect.h * scale;
    switch(m_alignType) {
    case AlignType::xMidYMin:
    case AlignType::xMidYMid:
    case AlignType::xMidYMax:
        xOffset += (viewportSize.w - viewWidth) * 0.5f;
        break;
    case AlignType::xMaxYMin:
    case AlignType::xMaxYMid:
    case AlignType::xMaxYMax:
        xOffset += (viewportSize.w - viewWidth);
        break;
    default:
        break;
    }

    switch(m_alignType) {
    case AlignType::xMinYMid:
    case AlignType::xMidYMid:
    case AlignType::xMaxYMid:
        yOffset += (viewportSize.h - viewHeight) * 0.5f;
        break;
    case AlignType::xMinYMax:
    case AlignType::xMidYMax:
    case AlignType::xMaxYMax:
        yOffset += (viewportSize.h - viewHeight);
        break;
    default:
        break;
    }

    return Rect(-xOffset / scale, -yOffset / scale, viewportSize.w / scale, viewportSize.h / scale);
}

Transform svg_preserve_aspect_ratio::getTransform(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.empty() && !viewportSize.empty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Transform(xScale, 0, 0, yScale, -viewBoxRect.x * xScale, -viewBoxRect.y * yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? STB_MIN(xScale, yScale) : STB_MAX(xScale, yScale);
    auto xOffset = -viewBoxRect.x * scale;
    auto yOffset = -viewBoxRect.y * scale;
    auto viewWidth = viewBoxRect.w * scale;
    auto viewHeight = viewBoxRect.h * scale;
    switch(m_alignType) {
    case AlignType::xMidYMin:
    case AlignType::xMidYMid:
    case AlignType::xMidYMax:
        xOffset += (viewportSize.w - viewWidth) * 0.5f;
        break;
    case AlignType::xMaxYMin:
    case AlignType::xMaxYMid:
    case AlignType::xMaxYMax:
        xOffset += (viewportSize.w - viewWidth);
        break;
    default:
        break;
    }

    switch(m_alignType) {
    case AlignType::xMinYMid:
    case AlignType::xMidYMid:
    case AlignType::xMaxYMid:
        yOffset += (viewportSize.h - viewHeight) * 0.5f;
        break;
    case AlignType::xMinYMax:
    case AlignType::xMidYMax:
    case AlignType::xMaxYMax:
        yOffset += (viewportSize.h - viewHeight);
        break;
    default:
        break;
    }

    return Transform(scale, 0, 0, scale, xOffset, yOffset);
}

void svg_preserve_aspect_ratio::transformRect(Rect& dstRect, Rect& srcRect) const
{
    if(m_alignType == AlignType::None)
        return;
    auto viewSize = dstRect.size();
    auto imageSize = srcRect.size();
    if(m_meetOrSlice == MeetOrSlice::Meet) {
        auto scale = imageSize.h / imageSize.w;
        if(viewSize.h > viewSize.w * scale) {
            dstRect.h = viewSize.w * scale;
            switch(m_alignType) {
            case AlignType::xMinYMid:
            case AlignType::xMidYMid:
            case AlignType::xMaxYMid:
                dstRect.y += (viewSize.h - dstRect.h) * 0.5f;
                break;
            case AlignType::xMinYMax:
            case AlignType::xMidYMax:
            case AlignType::xMaxYMax:
                dstRect.y += viewSize.h - dstRect.h;
                break;
            default:
                break;
            }
        }

        if(viewSize.w > viewSize.h / scale) {
            dstRect.w = viewSize.h / scale;
            switch(m_alignType) {
            case AlignType::xMidYMin:
            case AlignType::xMidYMid:
            case AlignType::xMidYMax:
                dstRect.x += (viewSize.w - dstRect.w) * 0.5f;
                break;
            case AlignType::xMaxYMin:
            case AlignType::xMaxYMid:
            case AlignType::xMaxYMax:
                dstRect.x += viewSize.w - dstRect.w;
                break;
            default:
                break;
            }
        }
    } else if(m_meetOrSlice == MeetOrSlice::Slice) {
        auto scale = imageSize.h / imageSize.w;
        if(viewSize.h < viewSize.w * scale) {
            srcRect.h = viewSize.h * (imageSize.w / viewSize.w);
            switch(m_alignType) {
            case AlignType::xMinYMid:
            case AlignType::xMidYMid:
            case AlignType::xMaxYMid:
                srcRect.y += (imageSize.h - srcRect.h) * 0.5f;
                break;
            case AlignType::xMinYMax:
            case AlignType::xMidYMax:
            case AlignType::xMaxYMax:
                srcRect.y += imageSize.h - srcRect.h;
                break;
            default:
                break;
            }
        }

        if(viewSize.w < viewSize.h / scale) {
            srcRect.w = viewSize.w * (imageSize.h / viewSize.h);
            switch(m_alignType) {
            case AlignType::xMidYMin:
            case AlignType::xMidYMid:
            case AlignType::xMidYMax:
                srcRect.x += (imageSize.w - srcRect.w) * 0.5f;
                break;
            case AlignType::xMaxYMin:
            case AlignType::xMaxYMid:
            case AlignType::xMaxYMax:
                srcRect.x += imageSize.w - srcRect.w;
                break;
            default:
                break;
            }
        }
    }
}

svg_blend_info::svg_blend_info(const svg_element* el)
    : m_clipper(el->clipper())
    , m_masker(el->masker())
    , m_filter(el->filterElement())
    , m_opacity(el->opacity())
    , m_mixBlendMode(el->mixBlendMode())
{
}

bool svg_blend_info::requiresCompositing(svg_render_mode mode) const
{
    return (m_clipper && m_clipper->requiresMasking()) || (mode == svg_render_mode::Painting && (m_masker || m_filter || m_opacity < 1.f || m_mixBlendMode != MixBlendMode::Normal));
}

bool svg_render_state::hasCycleReference(const svg_element* el) const
{
    auto current = this;
    do {
        if(el == current->el())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

void svg_render_state::beginGroup(const svg_blend_info& blendInfo)
{
    auto requiresCompositing = blendInfo.requiresCompositing(m_mode);
    pdf_canvas* pdf = dynamic_cast<pdf_canvas*>(m_canvas.get());
    m_layer = layer_kind::none;
    if(requiresCompositing && pdf) {
        // On a vector page: opacity and blend mode alone stay vector (a transparency group); what PDF cannot
        // express (masks, clip masks, filters) is drawn into pixels at raster_scale() and placed as an image.
        const bool masking = (blendInfo.clipper() && blendInfo.clipper()->requiresMasking()) || blendInfo.masker() || blendInfo.filterElement();
        if(m_mode == svg_render_mode::Painting && !masking) {
            pdf->begin_group();
            m_layer = layer_kind::pdf_group;
            if(blendInfo.clipper())
                blendInfo.clipper()->applyClipPath(*this);
        } else {
            auto bounding_box = m_currentTransform.map_rect(m_element->paint_bounding_box());
            if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
                bounding_box.inflate(blendInfo.filterElement()->pixelMargin() * m_currentTransform.xScale());
            }
            bounding_box.intersect(pdf->extents());
            m_layerScale = pdf->raster_scale();
            m_canvas = raster_canvas::create(Rect(bounding_box.x * m_layerScale, bounding_box.y * m_layerScale, bounding_box.w * m_layerScale, bounding_box.h * m_layerScale));
            m_currentTransform = Transform::scaled(m_layerScale, m_layerScale) * m_currentTransform;
            m_layer = layer_kind::raster_in_pdf;
        }
        return;
    }

    if(requiresCompositing) {
        auto bounding_box = m_currentTransform.map_rect(m_element->paint_bounding_box());
        if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
            bounding_box.inflate(blendInfo.filterElement()->pixelMargin() * m_currentTransform.xScale());
        }
        bounding_box.intersect(static_cast<raster_canvas&>(*m_canvas).extents());
        m_canvas = raster_canvas::create(bounding_box);
        m_layer = layer_kind::raster;
    } else {
        m_canvas->save();
    }

    if(!requiresCompositing && blendInfo.clipper()) {
        blendInfo.clipper()->applyClipPath(*this);
    }
}

void svg_render_state::endGroup(const svg_blend_info& blendInfo)
{
    if(m_layer == layer_kind::none) {
        m_canvas->restore();
        return;
    }

    // the finished group is blended into what is already painted under it, with its mix-blend-mode
    BlendMode blendMode = BlendMode::Src_Over;
    if(m_mode == svg_render_mode::Painting) {
        switch(blendInfo.mixBlendMode()) {
        case MixBlendMode::Multiply: blendMode = BlendMode::Multiply; break;
        case MixBlendMode::Screen: blendMode = BlendMode::Screen; break;
        case MixBlendMode::Darken: blendMode = BlendMode::Darken; break;
        case MixBlendMode::Lighten: blendMode = BlendMode::Lighten; break;
        case MixBlendMode::Difference: blendMode = BlendMode::Difference; break;
        case MixBlendMode::Exclusion: blendMode = BlendMode::Exclusion; break;
        case MixBlendMode::Normal: break;
        }
    }

    auto opacity = m_mode == svg_render_mode::Clipping ? 1.f : blendInfo.opacity();
    if(m_layer == layer_kind::pdf_group) {
        static_cast<pdf_canvas&>(*m_canvas).end_group(blendMode, opacity);
        m_layer = layer_kind::none;
        return;
    }

    if(blendInfo.clipper())
        blendInfo.clipper()->applyClipMask(*this);
    if(m_mode == svg_render_mode::Painting && blendInfo.masker()) {
        blendInfo.masker()->applyMask(*this);
    }
    if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
        blendInfo.filterElement()->applyFilter(*this, m_currentTransform.xScale());
    }

    if(m_layer == layer_kind::raster_in_pdf) {
        auto& layer = static_cast<raster_canvas&>(*m_canvas);
        auto& page = static_cast<pdf_canvas&>(*m_parent->m_canvas);
        const Rect e = layer.extents();
        page.draw_raster_group(pixmap(surface_reference(layer.surface())), Rect(e.x / m_layerScale, e.y / m_layerScale, e.w / m_layerScale, e.h / m_layerScale), blendMode, opacity);
        return;
    }

    static_cast<raster_canvas&>(*m_parent->m_canvas).blend_canvas(static_cast<raster_canvas&>(*m_canvas), blendMode, opacity);
}

svg_paint_element::svg_paint_element(document* dom, ElementID id)
    : svg_element(dom, id)
{
}

svg_stop_element::svg_stop_element(document* dom)
    : svg_element(dom, ElementID::Stop)
    , m_offset(PropertyID::Offset, 0.f)
{
    addProperty(m_offset);
}

void svg_stop_element::layoutElement(const svg_layout_state& state)
{
    m_stop_color = state.stop_color();
    m_stop_opacity = state.stop_opacity();
    svg_element::layoutElement(state);
}

GradientStop svg_stop_element::gradientStop(float opacity) const
{
    stb::color stopColor = with_alpha(m_stop_color, m_stop_opacity * opacity);
    GradientStop gradientStop = {
        m_offset.value(), { red_f(stopColor), green_f(stopColor), blue_f(stopColor), alpha_f(stopColor) }
    };

    return gradientStop;
}

svg_gradient_element::svg_gradient_element(document* dom, ElementID id)
    : svg_paint_element(dom, id)
    , svg_uri_reference(this)
    , m_gradientTransform(PropertyID::GradientTransform)
    , m_gradientUnits(PropertyID::GradientUnits, Units::ObjectBoundingBox)
    , m_spreadMethod(PropertyID::SpreadMethod, SpreadMethod::Pad)
{
    addProperty(m_gradientTransform);
    addProperty(m_gradientUnits);
    addProperty(m_spreadMethod);
}

void svg_gradient_element::collectGradientAttributes(svg_gradient_attributes& attributes) const
{
    if(!attributes.hasGradientTransform() && has_attribute(PropertyID::GradientTransform))
        attributes.setGradientTransform(this);
    if(!attributes.hasSpreadMethod() && has_attribute(PropertyID::SpreadMethod))
        attributes.setSpreadMethod(this);
    if(!attributes.hasGradientUnits() && has_attribute(PropertyID::GradientUnits))
        attributes.setGradientUnits(this);
    if(!attributes.hasGradientContentElement()) {
        for(const auto& child : children()) {
            if(auto el = toSVGElement(child); el && el->id() == ElementID::Stop) {
                attributes.setGradientContentElement(this);
                break;
            }
        }
    }
}

svg_linear_gradient_element::svg_linear_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::LinearGradient)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow, 100.f, LengthUnits::Percent)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

svg_linear_gradient_attributes svg_linear_gradient_element::collectGradientAttributes() const
{
    svg_linear_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::LinearGradient) {
            auto el = static_cast<const svg_linear_gradient_element*>(current);
            if(!attributes.hasX1() && el->has_attribute(PropertyID::X1))
                attributes.setX1(el);
            if(!attributes.hasY1() && el->has_attribute(PropertyID::Y1))
                attributes.setY1(el);
            if(!attributes.hasX2() && el->has_attribute(PropertyID::X2))
                attributes.setX2(el);
            if(!attributes.hasY2() && el->has_attribute(PropertyID::Y2)) {
                attributes.setY2(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

GradientStops buildGradientStops(const svg_gradient_element* el, float opacity)
{
    GradientStops gradientStops;

    const auto& children = el->children();
    gradientStops.reserve(children.size());
    for(const auto& child : children) {
        auto childElement = toSVGElement(child);
        if(childElement && childElement->id() == ElementID::Stop) {
            auto stopElement = static_cast<svg_stop_element*>(childElement);
            gradientStops.push_back(stopElement->gradientStop(opacity));
        }
    }

    return gradientStops;
}

bool svg_linear_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto x1 = lengthContext.valueForLength(attributes.x1());
    auto y1 = lengthContext.valueForLength(attributes.y1());
    auto x2 = lengthContext.valueForLength(attributes.x2());
    auto y2 = lengthContext.valueForLength(attributes.y2());
    if(gradientStops.size() == 1 || (x1 == x2 && y1 == y2)) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_linear_gradient(x1, y1, x2, y2, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_element::svg_radial_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::RadialGradient)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid, 50.f, LengthUnits::Percent)
    , m_fx(PropertyID::Fx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_fy(PropertyID::Fy, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
    addProperty(m_fx);
    addProperty(m_fy);
}

bool svg_radial_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto r = lengthContext.valueForLength(attributes.r());
    if(r == 0.f || gradientStops.size() == 1) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto fx = lengthContext.valueForLength(attributes.fx());
    auto fy = lengthContext.valueForLength(attributes.fy());
    auto cx = lengthContext.valueForLength(attributes.cx());
    auto cy = lengthContext.valueForLength(attributes.cy());

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_radial_gradient(cx, cy, r, fx, fy, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_attributes svg_radial_gradient_element::collectGradientAttributes() const
{
    svg_radial_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::RadialGradient) {
            auto el = static_cast<const svg_radial_gradient_element*>(current);
            if(!attributes.hasCx() && el->has_attribute(PropertyID::Cx))
                attributes.setCx(el);
            if(!attributes.hasCy() && el->has_attribute(PropertyID::Cy))
                attributes.setCy(el);
            if(!attributes.hasR() && el->has_attribute(PropertyID::R))
                attributes.setR(el);
            if(!attributes.hasFx() && el->has_attribute(PropertyID::Fx))
                attributes.setFx(el);
            if(!attributes.hasFy() && el->has_attribute(PropertyID::Fy)) {
                attributes.setFy(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

svg_pattern_element::svg_pattern_element(document* dom)
    : svg_paint_element(dom, ElementID::Pattern)
    , svg_uri_reference(this)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_patternTransform(PropertyID::PatternTransform)
    , m_patternUnits(PropertyID::PatternUnits, Units::ObjectBoundingBox)
    , m_patternContentUnits(PropertyID::PatternContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_patternTransform);
    addProperty(m_patternUnits);
    addProperty(m_patternContentUnits);
}

bool svg_pattern_element::applyPaint(svg_render_state& state, float opacity) const
{
    if(state.hasCycleReference(this))
        return false;
    auto attributes = collectPatternAttributes();
    auto patternContentElement = attributes.patternContentElement();
    if(patternContentElement == nullptr)
        return false;
    LengthContext lengthContext(this, attributes.patternUnits());
    Rect patternRect = {
        lengthContext.valueForLength(attributes.x()),
        lengthContext.valueForLength(attributes.y()),
        lengthContext.valueForLength(attributes.width()),
        lengthContext.valueForLength(attributes.height())
    };

    if(attributes.patternUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternRect.x = patternRect.x * bbox.w + bbox.x;
        patternRect.y = patternRect.y * bbox.h + bbox.y;
        patternRect.w = patternRect.w * bbox.w;
        patternRect.h = patternRect.h * bbox.h;
    }

    auto currentTransform = attributes.patternTransform() * state.currentTransform();
    // On a vector page the tile is a bitmap too; make it sharp at the page's raster scale.
    auto* pdf = dynamic_cast<pdf_canvas*>(&*state);
    const float tileScale = pdf ? pdf->raster_scale() : 1.f;
    auto xScale = currentTransform.xScale() * tileScale;
    auto yScale = currentTransform.yScale() * tileScale;

    auto patternImage = raster_canvas::create(0, 0, patternRect.w * xScale, patternRect.h * yScale);
    auto patternImageTransform = Transform::scaled(xScale, yScale);

    const auto& viewBoxRect = attributes.viewBox();
    if(viewBoxRect.isValid()) {
        const auto& preserveAspectRatio = attributes.preserveAspectRatio();
        patternImageTransform.multiply(preserveAspectRatio.getTransform(viewBoxRect, patternRect.size()));
    } else if(attributes.patternContentUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternImageTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, patternImageTransform, svg_render_mode::Painting, patternImage);
    patternContentElement->renderChildren(newState);

    auto patternTransform = attributes.patternTransform();
    patternTransform.translate(patternRect.x, patternRect.y);
    patternTransform.scale(patternRect.w / patternImage->width(), patternRect.h / patternImage->height());
    if(pdf)
        pdf->set_tiled_image(pixmap(surface_reference(patternImage->surface())), patternTransform, opacity);
    else
        state.raster()->set_texture(*patternImage, TextureType::Tiled, opacity, patternTransform);
    return true;
}

svg_pattern_attributes svg_pattern_element::collectPatternAttributes() const
{
    svg_pattern_attributes attributes;
    std::set<const svg_pattern_element*> processedPatterns;
    const svg_pattern_element* current = this;
    while(true) {
        if(!attributes.hasX() && current->has_attribute(PropertyID::X))
            attributes.setX(current);
        if(!attributes.hasY() && current->has_attribute(PropertyID::Y))
            attributes.setY(current);
        if(!attributes.hasWidth() && current->has_attribute(PropertyID::Width))
            attributes.setWidth(current);
        if(!attributes.hasHeight() && current->has_attribute(PropertyID::Height))
            attributes.setHeight(current);
        if(!attributes.hasPatternTransform() && current->has_attribute(PropertyID::PatternTransform))
            attributes.setPatternTransform(current);
        if(!attributes.hasPatternUnits() && current->has_attribute(PropertyID::PatternUnits))
            attributes.setPatternUnits(current);
        if(!attributes.hasPatternContentUnits() && current->has_attribute(PropertyID::PatternContentUnits))
            attributes.setPatternContentUnits(current);
        if(!attributes.hasViewBox() && current->has_attribute(PropertyID::ViewBox))
            attributes.setViewBox(current);
        if(!attributes.hasPreserveAspectRatio() && current->has_attribute(PropertyID::PreserveAspectRatio))
            attributes.setPreserveAspectRatio(current);
        if(!attributes.hasPatternContentElement()) {
            for(const auto& child : current->children()) {
                if(child->is_element()) {
                    attributes.setPatternContentElement(current);
                    break;
                }
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || targetElement->id() != ElementID::Pattern)
            break;
        processedPatterns.insert(current);
        current = static_cast<const svg_pattern_element*>(targetElement);
        if(processedPatterns.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

Rect svg_marker_position::markerBoundingBox(float strokeWidth) const
{
    return m_element->markerBoundingBox(m_origin, m_angle, strokeWidth);
}

void svg_marker_position::renderMarker(svg_render_state& state, float strokeWidth) const
{
    m_element->renderMarker(state, m_origin, m_angle, strokeWidth);
}

svg_geometry_element::svg_geometry_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
{
}

Rect svg_geometry_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = fillBoundingBox();
    if(m_stroke.isRenderable()) {
        float capLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineCap() == LineCap::Square)
            capLimit *= NOVA_SQRT2;
        float joinLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineJoin() == LineJoin::Miter) {
            joinLimit *= m_strokeData.miterLimit();
        }

        strokeBoundingBox.inflate(STB_MAX(capLimit, joinLimit));
    }

    for(const auto& markerPosition : m_markerPositions)
        strokeBoundingBox.unite(markerPosition.markerBoundingBox(m_strokeData.lineWidth()));
    return strokeBoundingBox;
}

void svg_geometry_element::layoutElement(const svg_layout_state& state)
{
    m_fill_rule = state.fill_rule();
    m_clip_rule = state.clip_rule();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    m_strokeData = getStrokeData(state);
    svg_graphics_element::layoutElement(state);

    m_path.clear();
    m_markerPositions.clear();
    m_fillBoundingBox = updateShape(m_path);
    updateMarkerPositions(m_markerPositions, state);
}

void svg_geometry_element::updateMarkerPositions(svg_marker_position_list& positions, const svg_layout_state& state)
{
    if(m_path.empty())
        return;
    auto markerStart = getMarker(state.marker_start());
    auto markerMid = getMarker(state.marker_mid());
    auto markerEnd = getMarker(state.marker_end());
    if(markerStart == nullptr && markerMid == nullptr && markerEnd == nullptr) {
        return;
    }

    Point origin;
    Point startPoint;
    Point inslopePoints[2];
    Point outslopePoints[2];

    int index = 0;
    std::array<Point, 3> points;
    outline_walker it(m_path);
    while(!it.done()) {
        switch(it.current_segment(points)) {
        case segment_kind::move_to:
            startPoint = points[0];
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::line_to:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::cubic_to:
            inslopePoints[0] = points[1];
            inslopePoints[1] = points[2];
            origin = points[2];
            break;
        case segment_kind::close:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = startPoint;
            startPoint = Point();
            break;
        }

        it.next();

        if(!it.done() && (markerStart || markerMid)) {
            it.current_segment(points);
            outslopePoints[0] = origin;
            outslopePoints[1] = points[0];
            if(index == 0 && markerStart) {
                auto slope = outslopePoints[1] - outslopePoints[0];
                auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
                const auto& orient = markerStart->orient();
                if(orient.orientType() == svg_angle::OrientType::AutoStartReverse)
                    angle -= 180.f;
                positions.emplace_back(markerStart, origin, angle);
            }

            if(index > 0 && markerMid) {
                auto inslope = inslopePoints[1] - inslopePoints[0];
                auto outslope = outslopePoints[1] - outslopePoints[0];
                auto inangle = 180.f * std::atan2(inslope.y, inslope.x) / NOVA_PI;
                auto outangle = 180.f * std::atan2(outslope.y, outslope.x) / NOVA_PI;
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                positions.emplace_back(markerMid, origin, angle);
            }
        }

        if(markerEnd && it.done()) {
            auto slope = inslopePoints[1] - inslopePoints[0];
            auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
            positions.emplace_back(markerEnd, origin, angle);
        }

        index += 1;
    }
}

void svg_geometry_element::render(svg_render_state& state) const
{
    if(!isRenderable())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(stb::color::white());
        newState->fill_path(m_path, m_clip_rule, newState.currentTransform());
    } else {
        if(m_fill.applyPaint(newState))
            newState->fill_path(m_path, m_fill_rule, newState.currentTransform());
        if(m_stroke.applyPaint(newState)) {
            newState->stroke_path(m_path, m_strokeData, newState.currentTransform());
        }

        for(const auto& markerPosition : m_markerPositions) {
            markerPosition.renderMarker(newState, m_strokeData.lineWidth());
        }
    }

    newState.endGroup(blendInfo);
}

svg_line_element::svg_line_element(document* dom)
    : svg_geometry_element(dom, ElementID::Line)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

Rect svg_line_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto x1 = lengthContext.valueForLength(m_x1);
    auto y1 = lengthContext.valueForLength(m_y1);
    auto x2 = lengthContext.valueForLength(m_x2);
    auto y2 = lengthContext.valueForLength(m_y2);

    path.move_to(x1, y1);
    path.line_to(x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

svg_rect_element::svg_rect_element(document* dom)
    : svg_geometry_element(dom, ElementID::Rect)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_rx(PropertyID::Rx, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Vertical, LengthNegativeMode::Forbid)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_rx);
    addProperty(m_ry);
}

Rect svg_rect_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto width = lengthContext.valueForLength(m_width);
    auto height = lengthContext.valueForLength(m_height);
    if(width <= 0.f || height <= 0.f) {
        return Rect::Empty;
    }

    auto x = lengthContext.valueForLength(m_x);
    auto y = lengthContext.valueForLength(m_y);

    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = STB_MIN(rx, width / 2.f);
    ry = STB_MIN(ry, height / 2.f);

    path.add_rounded_rect(x, y, width, height, rx, ry);
    return Rect(x, y, width, height);
}

svg_ellipse_element::svg_ellipse_element(document* dom)
    : svg_geometry_element(dom, ElementID::Ellipse)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rx(PropertyID::Rx, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_rx);
    addProperty(m_ry);
}

Rect svg_ellipse_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.add_ellipse(cx, cy, rx, ry);
    return Rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

svg_circle_element::svg_circle_element(document* dom)
    : svg_geometry_element(dom, ElementID::Circle)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
}

Rect svg_circle_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto r = lengthContext.valueForLength(m_r);
    if(r <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.add_ellipse(cx, cy, r, r);
    return Rect(cx - r, cy - r, r + r, r + r);
}

svg_poly_element::svg_poly_element(document* dom, ElementID id)
    : svg_geometry_element(dom, id)
    , m_points(PropertyID::Points)
{
    addProperty(m_points);
}

Rect svg_poly_element::updateShape(stb::outline& path)
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return Rect::Empty;
    }

    path.move_to(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path.line_to(points[i].x, points[i].y);
    }

    if(id() == ElementID::Polygon)
        path.close();
    return bounding_rect(path);
}

svg_path_element::svg_path_element(document* dom)
    : svg_geometry_element(dom, ElementID::Path)
    , m_d(PropertyID::D)
{
    addProperty(m_d);
}

Rect svg_path_element::updateShape(stb::outline& path)
{
    path = m_d.value();
    return bounding_rect(path);
}

AlignmentBaseline resolveDominantBaseline(const svg_text_positioning_element* el)
{
    switch(el->dominant_baseline()) {
    case DominantBaseline::Auto:
        if(el->isVerticalWritingMode())
            return AlignmentBaseline::Central;
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::UseScript:
    case DominantBaseline::NoChange:
    case DominantBaseline::ResetSize:
        return AlignmentBaseline::Auto;
    case DominantBaseline::Ideographic:
        return AlignmentBaseline::Ideographic;
    case DominantBaseline::Alphabetic:
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::Hanging:
        return AlignmentBaseline::Hanging;
    case DominantBaseline::Mathematical:
        return AlignmentBaseline::Mathematical;
    case DominantBaseline::Central:
        return AlignmentBaseline::Central;
    case DominantBaseline::Middle:
        return AlignmentBaseline::Middle;
    case DominantBaseline::TextAfterEdge:
        return AlignmentBaseline::TextAfterEdge;
    case DominantBaseline::TextBeforeEdge:
        return AlignmentBaseline::TextBeforeEdge;
    default:
        assert(false);
    }

    return AlignmentBaseline::Auto;
}

float calculateBaselineOffset(const svg_text_positioning_element* el)
{
    auto offset = el->baseline_offset();
    auto parent = el->parent_element();
    while(parent->isTextPositioningElement()) {
        offset += toSVGTextPositioningElement(parent)->baseline_offset();
        parent = parent->parent_element();
    }

    auto baseline = el->alignment_baseline();
    if(baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
        baseline = resolveDominantBaseline(el);
    }

    const auto& font = el->font();
    switch(baseline) {
    case AlignmentBaseline::BeforeEdge:
    case AlignmentBaseline::TextBeforeEdge:
        offset -= font.ascent();
        break;
    case AlignmentBaseline::Middle:
        offset -= font.xHeight() / 2.f;
        break;
    case AlignmentBaseline::Central:
        offset -= (font.ascent() + font.descent()) / 2.f;
        break;
    case AlignmentBaseline::AfterEdge:
    case AlignmentBaseline::TextAfterEdge:
    case AlignmentBaseline::Ideographic:
        offset -= font.descent();
        break;
    case AlignmentBaseline::Hanging:
        offset -= font.ascent() * 8.f / 10.f;
        break;
    case AlignmentBaseline::Mathematical:
        offset -= font.ascent() / 2.f;
        break;
    default:
        break;
    }

    return offset;
}

bool needsTextAnchorAdjustment(const svg_text_positioning_element* el)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        return direction == Direction::Rtl;
    case TextAnchor::Middle:
        return true;
    case TextAnchor::End:
        return direction == Direction::Ltr;
    default:
        assert(false);
    }

    return false;
}

float calculateTextAnchorOffset(const svg_text_positioning_element* el, float width)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        if(direction == Direction::Ltr)
            return 0.f;
        return -width;
    case TextAnchor::Middle:
        return -width / 2.f;
    case TextAnchor::End:
        if(direction == Direction::Ltr)
            return -width;
        return 0.f;
    default:
        assert(false);
    }

    return 0.f;
}

float calculateTextChunkLength(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool isVerticalText)
{
    float chunkLength = 0;
    const svg_text_fragment* lastFragment = nullptr;
    for(auto it = begin; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        chunkLength += isVerticalText ? fragment.height : fragment.width;
        if(!lastFragment) {
            lastFragment = &fragment;
            continue;
        }

        if(isVerticalText) {
            chunkLength += fragment.y - (lastFragment->y + lastFragment->height);
        } else {
            chunkLength += fragment.x - (lastFragment->x + lastFragment->width);
        }

        lastFragment = &fragment;
    }

    return chunkLength;
}

void handleTextChunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end)
{
    const svg_text_fragment& firstFragment = *begin;
    const auto isVerticalText = firstFragment.el->isVerticalWritingMode();
    if(firstFragment.el->has_attribute(PropertyID::TextLength)) {
        LengthContext lengthContext(firstFragment.el);
        auto textLength = lengthContext.valueForLength(firstFragment.el->textLength());
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        if(textLength > 0.f && chunkLength > 0.f) {
            size_t numCharacters = 0;
            for(auto it = begin; it != end; ++it) {
                const svg_text_fragment& fragment = *it;
                numCharacters += fragment.length;
            }

            if(firstFragment.el->lengthAdjust() == LengthAdjust::SpacingAndGlyphs) {
                auto textLengthScale = textLength / chunkLength;
                auto lengthAdjustTransform = Transform::translated(firstFragment.x, firstFragment.y);
                if(isVerticalText) {
                    lengthAdjustTransform.scale(1.f, textLengthScale);
                } else {
                    lengthAdjustTransform.scale(textLengthScale, 1.f);
                }

                lengthAdjustTransform.translate(-firstFragment.x, -firstFragment.y);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    fragment.lengthAdjustTransform = lengthAdjustTransform;
                }
            } else if(numCharacters > 1) {
                assert(firstFragment.el->lengthAdjust() == LengthAdjust::Spacing);
                size_t characterOffset = 0;
                auto textLengthShift = (textLength - chunkLength) / (numCharacters - 1);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    if(isVerticalText) {
                        fragment.y += textLengthShift * characterOffset;
                    } else {
                        fragment.x += textLengthShift * characterOffset;
                    }

                    characterOffset += fragment.length;
                }
            }
        }
    }

    if(needsTextAnchorAdjustment(firstFragment.el)) {
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        auto chunkOffset = calculateTextAnchorOffset(firstFragment.el, chunkLength);
        for(auto it = begin; it != end; ++it) {
            svg_text_fragment& fragment = *it;
            if(isVerticalText) {
                fragment.y += chunkOffset;
            } else {
                fragment.x += chunkOffset;
            }
        }
    }
}

svg_text_fragments_builder::svg_text_fragments_builder(std::u32string& text, svg_text_fragment_list& fragments)
    : m_text(text), m_fragments(fragments)
{
    m_text.clear();
    m_fragments.clear();
}

void svg_text_fragments_builder::build(const svg_text_element* textElement)
{
    handleElement(textElement);
    for(const auto& position : m_textPositions) {
        fillCharacterPositions(position);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& textPosition : m_textPositions) {
        if(!textPosition.nd->is_text_node())
            continue;
        auto el = toSVGTextPositioningElement(textPosition.nd->parent_element());
        const auto isVerticalText = el->isVerticalWritingMode();
        const auto isUprightText = el->isUprightTextOrientation();
        const auto& font = el->font();

        svg_text_fragment fragment(el);
        auto recordTextFragment = [&](auto startOffset, auto endOffset) {
            auto text = wholeText.substr(startOffset, endOffset - startOffset);
            fragment.offset = startOffset;
            fragment.length = text.length();
            fragment.width = font.measureText(text);
            fragment.height = font.height() + font.lineGap();
            if(isVerticalText) {
                m_y += isUprightText ? fragment.height : fragment.width;
            } else {
                m_x += fragment.width;
            }

            m_fragments.push_back(fragment);
        };

        auto needsTextLengthSpacing = el->lengthAdjust() == LengthAdjust::Spacing && el->has_attribute(PropertyID::TextLength);
        auto baselineOffset = calculateBaselineOffset(el);
        auto startOffset = textPosition.startOffset;
        auto textOffset = textPosition.startOffset;
        auto didStartTextFragment = false;
        auto applySpacingToNextCharacter = false;
        auto lastCharacter = 0u;
        auto lastAngle = 0.f;
        while(textOffset < textPosition.endOffset) {
            svg_character_position characterPosition;
            if(auto it = m_characterPositions.find(m_characterOffset); it != m_characterPositions.end()) {
                characterPosition = it->second;
            }

            auto currentCharacter = wholeText.at(textOffset);
            auto angle = characterPosition.rotate.value_or(0);
            auto dx = characterPosition.dx.value_or(0);
            auto dy = characterPosition.dy.value_or(0);

            auto shouldStartNewFragment = needsTextLengthSpacing || isVerticalText || applySpacingToNextCharacter
                || characterPosition.x || characterPosition.y || dx || dy || angle || angle != lastAngle;
            if(shouldStartNewFragment && didStartTextFragment) {
                recordTextFragment(startOffset, textOffset);
                applySpacingToNextCharacter = false;
                startOffset = textOffset;
            }

            auto startsNewTextChunk = (characterPosition.x || characterPosition.y) && textOffset == textPosition.startOffset;
            if(startsNewTextChunk || shouldStartNewFragment || !didStartTextFragment) {
                m_x = dx + characterPosition.x.value_or(m_x);
                m_y = dy + characterPosition.y.value_or(m_y);
                fragment.x = isVerticalText ? m_x + baselineOffset : m_x;
                fragment.y = isVerticalText ? m_y : m_y - baselineOffset;
                fragment.angle = angle;
                if(isVerticalText) {
                    if(isUprightText) {
                        fragment.y += font.height();
                    } else {
                        fragment.angle += 90.f;
                    }
                }

                fragment.startsNewTextChunk = startsNewTextChunk;
                didStartTextFragment = true;
            }

            auto spacing = el->letter_spacing();
            if(currentCharacter && lastCharacter && el->word_spacing()) {
                if(currentCharacter == ' ' && lastCharacter != ' ') {
                    spacing += el->word_spacing();
                }
            }

            if(spacing) {
                applySpacingToNextCharacter = true;
                if(isVerticalText) {
                    m_y += spacing;
                } else {
                    m_x += spacing;
                }
            }

            lastAngle = angle;
            lastCharacter = currentCharacter;
            ++textOffset;
            ++m_characterOffset;
        }

        recordTextFragment(startOffset, textOffset);
    }

    if(m_fragments.empty())
        return;
    auto it = m_fragments.begin();
    auto begin = m_fragments.begin();
    auto end = m_fragments.end();
    for(++it; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        if(!fragment.startsNewTextChunk)
            continue;
        handleTextChunk(begin, it);
        begin = it;
    }

    handleTextChunk(begin, it);
}

void svg_text_fragments_builder::handleText(const svg_text_node* nd)
{
    const auto& text = nd->data();
    if(text.empty())
        return;
    auto el = toSVGTextPositioningElement(nd->parent_element());
    const auto startOffset = m_text.length();
    uint32_t lastCharacter = ' ';
    if(!m_text.empty()) {
        lastCharacter = m_text.back();
    }

    text_iterator_t it;
    text_iterator_init(&it, text.data(), text.length(), NOVA_TEXT_ENCODING_UTF8);
    while(text_iterator_has_next(&it)) {
        auto currentCharacter = text_iterator_next(&it);
        if(currentCharacter == '\t' || currentCharacter == '\n' || currentCharacter == '\r')
            currentCharacter = ' ';
        if(currentCharacter == ' ' && lastCharacter == ' ' && el->white_space() == WhiteSpace::Default)
            continue;
        m_text.push_back(currentCharacter);
        lastCharacter = currentCharacter;
    }

    if(startOffset < m_text.length()) {
        m_textPositions.emplace_back(nd, startOffset, m_text.length());
    }
}

void svg_text_fragments_builder::handleElement(const svg_text_positioning_element* el)
{
    if(el->isDisplayNone())
        return;
    const auto itemIndex = m_textPositions.size();
    m_textPositions.emplace_back(el, m_text.length(), m_text.length());
    for(const auto& child : el->children()) {
        if(child->is_text_node()) {
            handleText(toSVGTextNode(child.get()));
        } else if(child->isTextPositioningElement()) {
            handleElement(toSVGTextPositioningElement(child.get()));
        }
    }

    auto& position = m_textPositions[itemIndex];
    assert(position.nd == el);
    position.endOffset = m_text.length();
}

void svg_text_fragments_builder::fillCharacterPositions(const svg_text_position& position)
{
    if(!position.nd->isTextPositioningElement())
        return;
    auto el = toSVGTextPositioningElement(position.nd);
    const auto& xList = el->x();
    const auto& yList = el->y();
    const auto& dxList = el->dx();
    const auto& dyList = el->dy();
    const auto& rotateList = el->rotate();

    auto xListSize = xList.size();
    auto yListSize = yList.size();
    auto dxListSize = dxList.size();
    auto dyListSize = dyList.size();
    auto rotateListSize = rotateList.size();
    if(!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
        return;
    }

    LengthContext lengthContext(el);
    std::optional<float> lastRotation;
    for(auto offset = position.startOffset; offset < position.endOffset; ++offset) {
        auto index = offset - position.startOffset;
        if(index >= xListSize && index >= yListSize && index >= dxListSize && index >= dyListSize && index >= rotateListSize)
            break;
        auto& characterPosition = m_characterPositions[offset];
        if(index < xListSize)
            characterPosition.x = lengthContext.valueForLength(xList[index], LengthDirection::Horizontal);
        if(index < yListSize)
            characterPosition.y = lengthContext.valueForLength(yList[index], LengthDirection::Vertical);
        if(index < dxListSize)
            characterPosition.dx = lengthContext.valueForLength(dxList[index], LengthDirection::Horizontal);
        if(index < dyListSize)
            characterPosition.dy = lengthContext.valueForLength(dyList[index], LengthDirection::Vertical);
        if(index < rotateListSize) {
            characterPosition.rotate = rotateList[index];
            lastRotation = characterPosition.rotate;
        }
    }

    if(lastRotation == std::nullopt)
        return;
    auto offset = position.startOffset + rotateList.size();
    while(offset < position.endOffset) {
        m_characterPositions[offset++].rotate = lastRotation;
    }
}

svg_text_positioning_element::svg_text_positioning_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_dx(PropertyID::Dx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_dy(PropertyID::Dy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rotate(PropertyID::Rotate)
    , m_textLength(PropertyID::TextLength, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_lengthAdjust(PropertyID::LengthAdjust, LengthAdjust::Spacing)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_dx);
    addProperty(m_dy);
    addProperty(m_rotate);
    addProperty(m_textLength);
    addProperty(m_lengthAdjust);
}

void svg_text_positioning_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    svg_graphics_element::layoutElement(state);

    LengthContext lengthContext(this);
    m_stroke_width = lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal);
    m_letter_spacing = lengthContext.valueForLength(state.letter_spacing(), LengthDirection::Diagonal);
    m_word_spacing = lengthContext.valueForLength(state.word_spacing(), LengthDirection::Diagonal);

    m_baseline_offset = convertBaselineOffset(state.baseline_shift());
    m_alignment_baseline = state.alignment_baseline();
    m_dominant_baseline = state.dominant_baseline();
    m_text_anchor = state.text_anchor();
    m_white_space = state.white_space();
    m_writing_mode = state.writing_mode();
    m_text_orientation = state.text_orientation();
    m_direction = state.direction();
}

float svg_text_positioning_element::convertBaselineOffset(const BaselineShift& baselineShift) const
{
    if(baselineShift.type() == BaselineShift::Type::Baseline)
        return 0.f;
    if(baselineShift.type() == BaselineShift::Type::Sub)
        return -m_font.height() / 2.f;
    if(baselineShift.type() == BaselineShift::Type::Super) {
        return m_font.height() / 2.f;
    }

    const auto& length = baselineShift.length();
    if(length.units() == LengthUnits::Percent)
        return length.value() * m_font.size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * m_font.size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_font.size();
    return length.value();
}

svg_t_span_element::svg_t_span_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Tspan)
{
}

svg_text_element::svg_text_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Text)
{
}

void svg_text_element::layout(svg_layout_state& state)
{
    svg_text_positioning_element::layout(state);
    svg_text_fragments_builder(m_text, m_fragments).build(this);
}

void svg_text_element::render(svg_render_state& state) const
{
    if(m_fragments.empty() || isVisibilityHidden() || isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(stb::color::white());
    }

    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        if(fragment.el->isVisibilityHidden())
            continue;
        auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto text = wholeText.substr(fragment.offset, fragment.length);
        auto origin = Point(fragment.x, fragment.y);

        const auto& font = fragment.el->font();
        if(newState.mode() == svg_render_mode::Clipping) {
            newState->fill_text(text, font, origin, transform);
        } else {
            const auto& fill = fragment.el->fill();
            const auto& stroke = fragment.el->stroke();
            auto stroke_width = fragment.el->stroke_width();
            if(fill.applyPaint(newState))
                newState->fill_text(text, font, origin, transform);
            if(stroke.applyPaint(newState)) {
                newState->stroke_text(text, stroke_width, font, origin, transform);
            }
        }
    }

    newState.endGroup(blendInfo);
}

Rect svg_text_element::bounding_box(bool includeStroke) const
{
    auto bounding_box = Rect::Invalid;
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.el->font();
        const auto& stroke = fragment.el->stroke();
        auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto fragmentRect = Rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.height);
        if(includeStroke && stroke.isRenderable())
            fragmentRect.inflate(fragment.el->stroke_width() / 2.f);
        bounding_box.unite(fragmentTranform.map_rect(fragmentRect));
    }

    if(!bounding_box.isValid())
        bounding_box = Rect::Empty;
    return bounding_box;
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

svg_layout_state::svg_layout_state(const svg_layout_state& parent, const svg_element* el)
    : m_parent(&parent)
    , m_element(el)
    , m_fill(parent.fill())
    , m_stroke(parent.stroke())
    , m_color(parent.color())
    , m_fill_opacity(parent.fill_opacity())
    , m_stroke_opacity(parent.stroke_opacity())
    , m_stroke_miterlimit(parent.stroke_miterlimit())
    , m_font_size(parent.font_size())
    , m_letter_spacing(parent.letter_spacing())
    , m_word_spacing(parent.word_spacing())
    , m_stroke_width(parent.stroke_width())
    , m_stroke_dashoffset(parent.stroke_dashoffset())
    , m_stroke_dasharray(parent.stroke_dasharray())
    , m_stroke_linecap(parent.stroke_linecap())
    , m_stroke_linejoin(parent.stroke_linejoin())
    , m_fill_rule(parent.fill_rule())
    , m_clip_rule(parent.clip_rule())
    , m_font_weight(parent.font_weight())
    , m_font_style(parent.font_style())
    , m_dominant_baseline(parent.dominant_baseline())
    , m_text_anchor(parent.text_anchor())
    , m_white_space(parent.white_space())
    , m_writing_mode(parent.writing_mode())
    , m_text_orientation(parent.text_orientation())
    , m_direction(parent.direction())
    , m_visibility(parent.visibility())
    , m_overflow(el->isRootElement() ? Overflow::Visible : Overflow::Hidden)
    , m_pointer_events(parent.pointer_events())
    , m_marker_start(parent.marker_start())
    , m_marker_mid(parent.marker_mid())
    , m_marker_end(parent.marker_end())
    , m_font_family(parent.font_family())
{
    for(const auto& attribute : el->attributes()) {
        std::string_view input(attribute.value());
        stripLeadingAndTrailingSpaces(input);
        if(input.empty() || input.compare("inherit") == 0)
            continue;
        switch(attribute.id()) {
        case PropertyID::Fill:
            m_fill = parsePaint(input, this, stb::color::black());
            break;
        case PropertyID::Stroke:
            m_stroke = parsePaint(input, this, stb::color::transparent());
            break;
        case PropertyID::Color:
            m_color = parseColor(input, this, stb::color::black());
            break;
        case PropertyID::Stop_Color:
            m_stop_color = parseColor(input, this, stb::color::black());
            break;
        case PropertyID::Opacity:
            m_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Mix_Blend_Mode:
            m_mix_blend_mode = parseMixBlendMode(input);
            break;
        case PropertyID::Fill_Opacity:
            m_fill_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stroke_Opacity:
            m_stroke_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stop_Opacity:
            m_stop_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stroke_Miterlimit:
            m_stroke_miterlimit = parseNumberOrPercentage(input, false, 4.f);
            break;
        case PropertyID::Font_Size:
            m_font_size = parseFontSize(input, this);
            break;
        case PropertyID::Letter_Spacing:
            m_letter_spacing = parseLengthOrNormal(input);
            break;
        case PropertyID::Word_Spacing:
            m_word_spacing = parseLengthOrNormal(input);
            break;
        case PropertyID::Baseline_Shift:
            m_baseline_shift = parseBaselineShift(input);
            break;
        case PropertyID::Stroke_Width:
            m_stroke_width = parseLength(input, LengthNegativeMode::Forbid, Length(1.f, LengthUnits::None));
            break;
        case PropertyID::Stroke_Dashoffset:
            m_stroke_dashoffset = parseLength(input, LengthNegativeMode::Allow, Length(0.f, LengthUnits::None));
            break;
        case PropertyID::Stroke_Dasharray:
            m_stroke_dasharray = parseDashArray(input);
            break;
        case PropertyID::Stroke_Linecap:
            m_stroke_linecap = parseLineCap(input);
            break;
        case PropertyID::Stroke_Linejoin:
            m_stroke_linejoin = parseLineJoin(input);
            break;
        case PropertyID::Fill_Rule:
            m_fill_rule = parseFillRule(input);
            break;
        case PropertyID::Clip_Rule:
            m_clip_rule = parseFillRule(input);
            break;
        case PropertyID::Font_Weight:
            m_font_weight = parseFontWeight(input);
            break;
        case PropertyID::Font_Style:
            m_font_style = parseFontStyle(input);
            break;
        case PropertyID::Alignment_Baseline:
            m_alignment_baseline = parseAlignmentBaseline(input);
            break;
        case PropertyID::Dominant_Baseline:
            m_dominant_baseline = parseDominantBaseline(input);
            break;
        case PropertyID::Direction:
            m_direction = parseDirection(input);
            break;
        case PropertyID::Text_Anchor:
            m_text_anchor = parseTextAnchor(input);
            break;
        case PropertyID::White_Space:
            m_white_space = parseWhiteSpace(input);
            break;
        case PropertyID::Writing_Mode:
            m_writing_mode = parseWritingMode(input);
            break;
        case PropertyID::Text_Orientation:
            m_text_orientation = parseTextOrientation(input);
            break;
        case PropertyID::Display:
            m_display = parseDisplay(input);
            break;
        case PropertyID::Visibility:
            m_visibility = parseVisibility(input);
            break;
        case PropertyID::Overflow:
            m_overflow = parseOverflow(input);
            break;
        case PropertyID::Pointer_Events:
            m_pointer_events = parsePointerEvents(input);
            break;
        case PropertyID::Mask_Type:
            m_mask_type = parseMaskType(input);
            break;
        case PropertyID::Mask:
            m_mask = parseUrl(input);
            break;
        case PropertyID::Clip_Path:
            m_clip_path = parseUrl(input);
            break;
        case PropertyID::Filter:
            m_filter = parseUrl(input);
            break;
        case PropertyID::Marker_Start:
            m_marker_start = parseUrl(input);
            break;
        case PropertyID::Marker_Mid:
            m_marker_mid = parseUrl(input);
            break;
        case PropertyID::Marker_End:
            m_marker_end = parseUrl(input);
            break;
        case PropertyID::Font_Family:
            m_font_family.assign(input);
            break;
        default:
            break;
        }
    }
}

Font makeFont(const std::string& family, float size, bool bold, bool italic)
{
    // Walk the font-family stack name by name (see
    // FontFaceCache::getFontFaceForFamilyStack()): an @font-face-embedded
    // font wins under its exact declared name, an uninstalled name falls
    // back to a metric-compatible substitute, unresolvable names are skipped.
    FontFace face = fontFaceCache()->getFontFaceForFamilyStack(family, bold, italic);

    // Absolute last resort: the hardcoded generic-family table.
    if(face.is_null())
        face = fontFaceCache()->getFontFace(emptyString, bold, italic);
    return Font(face, size);
}

Font svg_layout_state::font() const
{
    return makeFont(m_font_family, m_font_size, m_font_weight == FontWeight::Bold, m_font_style == FontStyle::Italic);
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

svg_text_node::svg_text_node(document* dom)
    : svg_node(dom)
{
}

void svg_text_node::set_data(const std::string& data)
{
    root_element()->setNeedsLayout();
    m_data.assign(data);
}

std::unique_ptr<svg_node> svg_text_node::clone(bool deep) const
{
    auto nd = std::make_unique<svg_text_node>(dom());
    nd->set_data(m_data);
    return nd;
}

std::unique_ptr<svg_element> svg_element::create(document* dom, ElementID id)
{
    switch(id) {
    case ElementID::Svg:
        return std::make_unique<svg_svg_element>(dom);
    case ElementID::Path:
        return std::make_unique<svg_path_element>(dom);
    case ElementID::G:
        return std::make_unique<svg_g_element>(dom);
    case ElementID::Switch:
        return std::make_unique<svg_switch_element>(dom);
    case ElementID::Rect:
        return std::make_unique<svg_rect_element>(dom);
    case ElementID::Circle:
        return std::make_unique<svg_circle_element>(dom);
    case ElementID::Ellipse:
        return std::make_unique<svg_ellipse_element>(dom);
    case ElementID::Line:
        return std::make_unique<svg_line_element>(dom);
    case ElementID::Defs:
        return std::make_unique<svg_defs_element>(dom);
    case ElementID::Polygon:
    case ElementID::Polyline:
        return std::make_unique<svg_poly_element>(dom, id);
    case ElementID::Stop:
        return std::make_unique<svg_stop_element>(dom);
    case ElementID::LinearGradient:
        return std::make_unique<svg_linear_gradient_element>(dom);
    case ElementID::RadialGradient:
        return std::make_unique<svg_radial_gradient_element>(dom);
    case ElementID::Symbol:
        return std::make_unique<svg_symbol_element>(dom);
    case ElementID::Use:
        return std::make_unique<svg_use_element>(dom);
    case ElementID::Pattern:
        return std::make_unique<svg_pattern_element>(dom);
    case ElementID::Mask:
        return std::make_unique<svg_mask_element>(dom);
    case ElementID::ClipPath:
        return std::make_unique<svg_clip_path_element>(dom);
    case ElementID::Filter:
        return std::make_unique<svg_filter_element>(dom);
    case ElementID::FeGaussianBlur:
        return std::make_unique<svg_fe_gaussian_blur_element>(dom);
    case ElementID::FeDropShadow:
        return std::make_unique<svg_fe_drop_shadow_element>(dom);
    case ElementID::FeOffset:
        return std::make_unique<svg_fe_offset_element>(dom);
    case ElementID::FeFlood:
        return std::make_unique<svg_fe_flood_element>(dom);
    case ElementID::FeComposite:
        return std::make_unique<svg_fe_composite_element>(dom);
    case ElementID::FeMerge:
        return std::make_unique<svg_fe_merge_element>(dom);
    case ElementID::FeMergeNode:
        return std::make_unique<svg_element>(dom, id);
    case ElementID::Marker:
        return std::make_unique<svg_marker_element>(dom);
    case ElementID::Image:
        return std::make_unique<svg_image_element>(dom);
    case ElementID::ForeignObject:
        return std::make_unique<svg_foreign_object_element>(dom);
    case ElementID::Style:
        return std::make_unique<svg_style_element>(dom);
    case ElementID::Text:
        return std::make_unique<svg_text_element>(dom);
    case ElementID::Tspan:
        return std::make_unique<svg_t_span_element>(dom);
    default:
        assert(false);
    }

    return nullptr;
}

svg_element::svg_element(document* dom, ElementID id)
    : svg_node(dom)
    , m_id(id)
{
}

bool svg_element::has_attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return has_attribute(id);
}

const std::string& svg_element::attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return emptyString;
    return attribute(id);
}

bool svg_element::set_attribute(std::string_view name, const std::string& value)
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return set_attribute(Specificity::Api, id, value);
}

const Attribute* svg_element::findAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return &attribute;
        }
    }

    return nullptr;
}

bool svg_element::has_attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return true;
        }
    }

    return false;
}

const std::string& svg_element::attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return attribute.value();
        }
    }

    return emptyString;
}

bool svg_element::set_attribute(int specificity, PropertyID id, const std::string& value)
{
    // A value with var() cannot be parsed before the custom properties it names are all known (a stylesheet
    // can come after the el): it is kept, and parsed by resolveVariables() when the dom is complete.
    const bool deferred = usesVariables(id, value);
    for(auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            if(specificity < attribute.specificity())
                return false;
            if(deferred)
                m_variableSources[id] = value;
            else {
                m_variableSources.erase(id);
                parseAttribute(id, value);
            }
            attribute = Attribute(specificity, id, value);
            return true;
        }
    }

    if(deferred)
        m_variableSources[id] = value;
    else
        parseAttribute(id, value);
    m_attributes.emplace_front(specificity, id, value);
    return true;
}

void svg_element::setCustomProperty(int specificity, const std::string& name, const std::string& value)
{
    auto it = m_customProperties.find(name);
    if(it != m_customProperties.end() && specificity < it->second.first)
        return;
    std::string_view trimmed(value);
    stripLeadingAndTrailingSpaces(trimmed);
    m_customProperties[name] = {specificity, std::string(trimmed)};
}

const svg_element* svg_element::findCustomPropertyOwner(const std::string& name) const
{
    for(auto el = this; el != nullptr; el = el->parent_element()) {
        if(el->m_customProperties.count(name))
            return el;
    }
    return nullptr;
}

const std::string& svg_element::customProperty(const std::string& name) const
{
    if(auto owner = findCustomPropertyOwner(name))
        return owner->m_customProperties.at(name).second;
    return emptyString;
}

void svg_element::resolveVariables()
{
    for(const auto& [id, source] : m_variableSources) {
        std::string resolved;
        if(!substituteVariables(source, this, resolved, 0))
            continue;
        for(auto& attribute : m_attributes) {
            if(attribute.id() == id) {
                attribute = Attribute(attribute.specificity(), id, resolved);
                break;
            }
        }
        parseAttribute(id, resolved);
    }
}

void svg_element::setAttributes(const AttributeList& attributes)
{
    for(const auto& attribute : attributes) {
        set_attribute(attribute);
    }
}

bool svg_element::set_attribute(const Attribute& attribute)
{
    return set_attribute(attribute.specificity(), attribute.id(), attribute.value());
}

void svg_element::parseAttribute(PropertyID id, const std::string& value)
{
    root_element()->setNeedsLayout();
    if(auto property = getProperty(id)) {
        property->parse(value);
    }
}

svg_element* svg_element::previousElement() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.begin();
    auto end = children.end();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

svg_element* svg_element::nextElement() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.rbegin();
    auto end = children.rend();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

svg_node* svg_element::addChild(std::unique_ptr<svg_node> child)
{
    child->setParentElement(this);
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

svg_node* svg_element::firstChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.front();
}

svg_node* svg_element::lastChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.back();
}

Rect svg_element::fillBoundingBox() const
{
    auto fillBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            fillBoundingBox.unite(el->local_transform().map_rect(el->fillBoundingBox()));
        }
    }

    if(!fillBoundingBox.isValid())
        fillBoundingBox = Rect::Empty;
    return fillBoundingBox;
}

Rect svg_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            strokeBoundingBox.unite(el->local_transform().map_rect(el->strokeBoundingBox()));
        }
    }

    if(!strokeBoundingBox.isValid())
        strokeBoundingBox = Rect::Empty;
    return strokeBoundingBox;
}

Rect svg_element::paint_bounding_box() const
{
    if(m_paintBoundingBox.isValid())
        return m_paintBoundingBox;
    m_paintBoundingBox = Rect::Empty;
    m_paintBoundingBox = strokeBoundingBox();
    assert(m_paintBoundingBox.isValid());
    if(m_clipper) m_paintBoundingBox.intersect(m_clipper->clipBoundingBox(this));
    if(m_masker) m_paintBoundingBox.intersect(m_masker->maskBoundingBox(this));
    return m_paintBoundingBox;
}

svg_marker_element* svg_element::getMarker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Marker)
        return static_cast<svg_marker_element*>(el);
    return nullptr;
}

svg_clip_path_element* svg_element::getClipper(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::ClipPath)
        return static_cast<svg_clip_path_element*>(el);
    return nullptr;
}

svg_mask_element* svg_element::getMasker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Mask)
        return static_cast<svg_mask_element*>(el);
    return nullptr;
}

svg_filter_element* svg_element::getFilterElement(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Filter)
        return static_cast<svg_filter_element*>(el);
    return nullptr;
}

svg_paint_element* svg_element::getPainter(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->isPaintElement())
        return static_cast<svg_paint_element*>(el);
    return nullptr;
}

svg_element* svg_element::element_from_point(float x, float y)
{
    auto it = m_children.rbegin();
    auto end = m_children.rend();
    for(; it != end; ++it) {
        auto child = toSVGElement(*it);
        if(child && !child->isHiddenElement()) {
            if(auto el = child->element_from_point(x, y)) {
                return el;
            }
        }
    }

    if(isPointableElement()) {
        auto transform = local_transform();
        for(auto parent = parent_element(); parent; parent = parent->parent_element())
            transform.post_multiply(parent->local_transform());
        auto bbox = transform.map_rect(paint_bounding_box());
        if(bbox.contains(x, y)) {
            return this;
        }
    }

    return nullptr;
}

void svg_element::addProperty(svg_property& value)
{
    m_properties.push_front(&value);
}

svg_property* svg_element::getProperty(PropertyID id) const
{
    for(auto property : m_properties) {
        if(id == property->id()) {
            return property;
        }
    }

    return nullptr;
}

Size svg_element::currentViewportSize() const
{
    auto parent = parent_element();
    if(parent == nullptr) {
        auto el = static_cast<const svg_svg_element*>(this);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        return Size(300, 150);
    }

    if(parent->id() == ElementID::Svg) {
        auto el = static_cast<const svg_svg_element*>(parent);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        LengthContext lengthContext(el);
        auto width = lengthContext.valueForLength(el->width());
        auto height = lengthContext.valueForLength(el->height());
        return Size(width, height);
    }

    return parent->currentViewportSize();
}

void svg_element::cloneChildren(svg_element* parent_element) const
{
    for(const auto& child : m_children) {
        parent_element->addChild(child->clone(true));
    }
}

std::unique_ptr<svg_node> svg_element::clone(bool deep) const
{
    auto el = svg_element::create(dom(), m_id);
    el->setAttributes(m_attributes);
    if(deep) { cloneChildren(el.get()); }
    return el;
}

void svg_element::build()
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->build();
        }
    }
}

void svg_element::layoutElement(const svg_layout_state& state)
{
    m_paintBoundingBox = Rect::Invalid;
    m_clipper = getClipper(state.clip_path());
    m_masker = getMasker(state.mask());
    m_filter = getFilterElement(state.filter());
    m_opacity = state.opacity();
    m_mix_blend_mode = state.mix_blend_mode();

    m_font_size = state.font_size();
    m_display = state.display();
    m_overflow = state.overflow();
    m_visibility = state.visibility();
    m_pointer_events = state.pointer_events();
}

void svg_element::layoutChildren(svg_layout_state& state)
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->layout(state);
        }
    }
}

void svg_element::layout(svg_layout_state& state)
{
    svg_layout_state newState(state, this);
    layoutElement(newState);
    layoutChildren(newState);
}

void svg_element::renderChildren(svg_render_state& state) const
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->render(state);
        }
    }
}

void svg_element::render(svg_render_state& state) const
{
}

bool svg_element::isHiddenElement() const
{
    if(isDisplayNone())
        return true;
    switch(m_id) {
    case ElementID::Defs:
    case ElementID::Symbol:
    case ElementID::Marker:
    case ElementID::ClipPath:
    case ElementID::Mask:
    case ElementID::Filter:
    case ElementID::FeGaussianBlur:
    case ElementID::FeDropShadow:
    case ElementID::FeOffset:
    case ElementID::FeFlood:
    case ElementID::FeComposite:
    case ElementID::FeMerge:
    case ElementID::FeMergeNode:
    case ElementID::LinearGradient:
    case ElementID::RadialGradient:
    case ElementID::Pattern:
    case ElementID::Stop:
        return true;
    default:
        return false;
    }
}

bool svg_element::isPointableElement() const
{
    if(m_pointer_events != PointerEvents::None
        && m_visibility != Visibility::Hidden
        && m_display != Display::None
        && m_opacity != 0.f) {
        switch(m_id) {
        case ElementID::Line:
        case ElementID::Rect:
        case ElementID::Ellipse:
        case ElementID::Circle:
        case ElementID::Polyline:
        case ElementID::Polygon:
        case ElementID::Path:
        case ElementID::Text:
        case ElementID::Image:
        case ElementID::ForeignObject:
            return true;
        default:
            break;
        }
    }

    return false;
}

svg_style_element::svg_style_element(document* dom)
    : svg_element(dom, ElementID::Style)
{
}

svg_fit_to_view_box::svg_fit_to_view_box(svg_element* el)
    : m_viewBox(PropertyID::ViewBox)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    el->addProperty(m_viewBox);
    el->addProperty(m_preserveAspectRatio);
}

Transform svg_fit_to_view_box::viewBoxToViewTransform(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Transform::Identity;
    return m_preserveAspectRatio.getTransform(viewBoxRect, viewportSize);
}

Rect svg_fit_to_view_box::getClipRect(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Rect(0, 0, viewportSize.w, viewportSize.h);
    return m_preserveAspectRatio.getClipRect(viewBoxRect, viewportSize);
}

svg_uri_reference::svg_uri_reference(svg_element* el)
    : m_href(PropertyID::Href)
{
    el->addProperty(m_href);
}

svg_element* svg_uri_reference::getTargetElement(const document* dom) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return dom->root_element()->element_by_id(value.substr(1));
}

bool svg_paint_server::applyPaint(svg_render_state& state) const
{
    if(!isRenderable())
        return false;
    if(m_element) return m_element->applyPaint(state, m_opacity);
    state->set_color(with_alpha(m_color, m_opacity));
    return true;
}

svg_graphics_element::svg_graphics_element(document* dom, ElementID id)
    : svg_element(dom, id)
    , m_transform(PropertyID::Transform)
    , m_transformOrigin(PropertyID::Transform_Origin)
    , m_transformBox(PropertyID::Transform_Box)
{
    addProperty(m_transform);
    addProperty(m_transformOrigin);
    addProperty(m_transformBox);
}

Transform svg_graphics_element::local_transform() const
{
    // Only shift the pivot when transform-origin was actually specified --
    // matches legacy behavior (origin at the current user-space (0,0)) for
    // every el that doesn't use it, i.e. everything except elements
    // that explicitly opt in the way feature-css-use-symbol.svg's `.rot`
    // class does.
    if(!m_transformOrigin.specified())
        return m_transform.value();

    Rect box;
    if(m_transformBox.specified() && m_transformBox.value() == TransformBoxType::FillBox) {
        box = fillBoundingBox();
    } else {
        auto size = currentViewportSize();
        box = Rect(0.f, 0.f, size.w, size.h);
    }

    if(box.empty())
        return m_transform.value();

    auto origin = m_transformOrigin.resolve(box);
    return Transform::translated(origin.x, origin.y) * m_transform.value() * Transform::translated(-origin.x, -origin.y);
}

svg_paint_server svg_graphics_element::getPaintServer(const Paint& paint, float opacity) const
{
    if(paint.isNone())
        return svg_paint_server();
    if(auto el = getPainter(paint.id()))
        return svg_paint_server(el, paint.color(), opacity);
    return svg_paint_server(nullptr, paint.color(), opacity);
}

StrokeData svg_graphics_element::getStrokeData(const svg_layout_state& state) const
{
    LengthContext lengthContext(this);
    StrokeData strokeData(lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal));
    strokeData.setMiterLimit(state.stroke_miterlimit());
    strokeData.setLineCap(state.stroke_linecap());
    strokeData.setLineJoin(state.stroke_linejoin());
    strokeData.setDashOffset(lengthContext.valueForLength(state.stroke_dashoffset(), LengthDirection::Diagonal));

    DashArray dashArray;
    for(const auto& dash : state.stroke_dasharray())
        dashArray.push_back(lengthContext.valueForLength(dash, LengthDirection::Diagonal));
    strokeData.setDashArray(std::move(dashArray));
    return strokeData;
}

svg_svg_element::svg_svg_element(document* dom)
    : svg_graphics_element(dom, ElementID::Svg)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Transform svg_svg_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(isRootElement())
        return viewBoxToViewTransform(viewportRect.size());
    return svg_graphics_element::local_transform() * Transform::translated(viewportRect.x, viewportRect.y) * viewBoxToViewTransform(viewportRect.size());
}

void svg_svg_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    LengthContext lengthContext(this);
    const Size viewportSize = {
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(viewportSize.empty())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(viewportSize), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

svg_root_element::svg_root_element(document* dom)
    : svg_svg_element(dom)
{
}

svg_root_element* svg_root_element::layout_if_needed()
{
    if(needsLayout())
        force_layout();
    return this;
}

svg_element* svg_root_element::element_by_id(std::string_view id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

void svg_root_element::addElementById(const std::string& id, svg_element* el)
{
    m_idCache.emplace(id, el);
}

void svg_root_element::layout(svg_layout_state& state)
{
    svg_svg_element::layout(state);

    LengthContext lengthContext(this);
    if(!width().isPercent()) {
        m_intrinsicWidth = lengthContext.valueForLength(width());
    } else {
        m_intrinsicWidth = 0.f;
    }

    if(!height().isPercent()) {
        m_intrinsicHeight = lengthContext.valueForLength(height());
    } else {
        m_intrinsicHeight = 0.f;
    }

    const auto& viewBoxRect = viewBox().value();
    if(!viewBoxRect.empty() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        auto intrinsicRatio = viewBoxRect.w / viewBoxRect.h;
        if(!m_intrinsicWidth && m_intrinsicHeight)
            m_intrinsicWidth = m_intrinsicHeight * intrinsicRatio;
        else if(m_intrinsicWidth && !m_intrinsicHeight) {
            m_intrinsicHeight = m_intrinsicWidth / intrinsicRatio;
        }
    }

    if(viewBoxRect.isValid() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        m_intrinsicWidth = viewBoxRect.w;
        m_intrinsicHeight = viewBoxRect.h;
    }

    if(!m_intrinsicWidth || !m_intrinsicHeight) {
        auto bounding_box = paint_bounding_box();
        if(!m_intrinsicWidth)
            m_intrinsicWidth = bounding_box.right();
        if(!m_intrinsicHeight) {
            m_intrinsicHeight = bounding_box.bottom();
        }
    }
}

void svg_root_element::force_layout()
{
    svg_layout_state state;
    layout(state);
}

svg_use_element::svg_use_element(document* dom)
    : svg_graphics_element(dom, ElementID::Use)
    , svg_uri_reference(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Transform svg_use_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Point translation = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y)
    };

    return svg_graphics_element::local_transform() * Transform::translated(translation.x, translation.y);
}

void svg_use_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

void svg_use_element::build()
{
    if(auto targetElement = getTargetElement(dom())) {
        if(auto newElement = cloneTargetElement(targetElement)) {
            addChild(std::move(newElement));
        }
    }

    svg_graphics_element::build();
}

std::unique_ptr<svg_element> svg_use_element::cloneTargetElement(svg_element* targetElement)
{
    if(targetElement == this || isDisallowedElement(targetElement))
        return nullptr;
    const auto& idAttr = targetElement->attribute(PropertyID::Id);
    auto parent = parent_element();
    while(parent) {
        auto attribute = parent->findAttribute(PropertyID::Id);
        if(attribute && idAttr == attribute->value())
            return nullptr;
        parent = parent->parent_element();
    }

    auto tagId = targetElement->id();
    if(tagId == ElementID::Symbol) {
        tagId = ElementID::Svg;
    }

    auto newElement = svg_element::create(dom(), tagId);
    newElement->setAttributes(targetElement->attributes());
    if(newElement->id() == ElementID::Svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == PropertyID::Width || attribute.id() == PropertyID::Height) {
                newElement->set_attribute(attribute);
            }
        }
    }

    if(newElement->id() != ElementID::Use)
        targetElement->cloneChildren(newElement.get());
    return newElement;
}

svg_image_element::svg_image_element(document* dom)
    : svg_graphics_element(dom, ElementID::Image)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_preserveAspectRatio);
}

Rect svg_image_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    return viewportRect;
}

Rect svg_image_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

void svg_image_element::render(svg_render_state& state) const
{
    if(m_image.is_null() || isDisplayNone() || isVisibilityHidden())
        return;
    Rect dstRect(fillBoundingBox());
    Rect srcRect(0, 0, m_image.width(), m_image.height());
    if(dstRect.empty() || srcRect.empty())
        return;
    m_preserveAspectRatio.transformRect(dstRect, srcRect);

    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    newState->draw_image(m_image, dstRect, srcRect, newState.currentTransform());
    newState.endGroup(blendInfo);
}

pixmap loadImageResource(const std::string& href)
{
    if(href.compare(0, 5, "data:") == 0) {
        std::string_view input(href);
        auto index = input.find(',', 5);
        if(index == std::string_view::npos)
            return pixmap();
        input.remove_prefix(index + 1);
        return surface_load_from_image_base64(input.data(), input.length());
    }

    return surface_load_from_image_file(href.data());
}

void svg_image_element::parseAttribute(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Href) {
        m_image = loadImageResource(value);
    } else {
        svg_graphics_element::parseAttribute(id, value);
    }
}

svg_foreign_object_element::svg_foreign_object_element(document* dom)
    : svg_graphics_element(dom, ElementID::ForeignObject)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Rect svg_foreign_object_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    return Rect(
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    );
}

Rect svg_foreign_object_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

ForeignObjectFonts svg_foreign_object_element::fontsFor(unsigned usedStyles) const
{
    ForeignObjectFonts fonts;
    for(unsigned style = 0; style < 4; ++style) {
        if(!(usedStyles & (1u << style)))
            continue;
        if(style == m_baseFontStyle) {
            fonts.face[style] = m_font;
            continue;
        }
        auto variant = makeFont(m_fontFamily, m_font.size(), (style & ForeignObjectFonts::Bold) != 0, (style & ForeignObjectFonts::Italic) != 0);
        // A variant the machine can't provide at all must not blank the text: use the el's own face.
        fonts.face[style] = variant.is_null() ? m_font : variant;
    }
    return fonts;
}

void svg_foreign_object_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fontFamily = state.font_family();
    m_baseFontStyle = (unsigned char)((state.font_weight() == FontWeight::Bold ? ForeignObjectFonts::Bold : 0)
                                      | (state.font_style() == FontStyle::Italic ? ForeignObjectFonts::Italic : 0));
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    svg_graphics_element::layoutElement(state);
}

void svg_foreign_object_element::render(svg_render_state& state) const
{
    if(m_rawContent.empty() || isDisplayNone() || isVisibilityHidden())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    auto renderer = m_renderer ? m_renderer : ForeignObjectSimple::instance();
    renderer->render(this, newState);
    newState.endGroup(blendInfo);
}

ForeignObjectSimple* ForeignObjectSimple::instance()
{
    static ForeignObjectSimple s_instance;
    return &s_instance;
}

void ForeignObjectSimple::render(const svg_foreign_object_element* el, svg_render_state& state) const
{
    auto segments = foreignObjectStyledText(el->rawContent(), el->baseFontStyle());
    if(segments.empty())
        return;

    const auto& font = el->font();
    if(font.is_null())
        return;

    // One face per style that actually occurs (just font() for unstyled text, nothing else is looked up).
    const auto fonts = el->fontsFor(foreignObjectStylesUsed(segments));

    auto box = el->fillBoundingBox();
    // A browser wraps HTML text at its box's edge unless CSS says nowrap/pre.
    auto lines = layoutForeignObjectText(segments, fonts, foreignObjectAllowsWrap(el->rawContent()) ? box.w : 0.f);

    // Real line-height from the HTML's own `line-height:` (mermaid's
    // content usually says 1.5) when present and parseable; falls back
    // to a 1.2x default (roughly a browser's own "normal") otherwise --
    // e.g. for %/em values or "normal" itself, which aren't handled.
    auto lineHeight = foreignObjectLineHeight(el->rawContent(), font.size()).value_or(font.height() * 1.2f);
    auto totalHeight = lineHeight * float(lines.size());
    auto topY = box.y + (box.h - totalHeight) / 2.f;

    if(auto backgroundColor = el->htmlBackgroundColor()) {
        state->set_color(*backgroundColor);
        stb::outline backgroundPath;
        backgroundPath.add_rect(box.x, box.y, box.w, box.h);
        state->fill_path(backgroundPath, FillRule::NonZero, state.currentTransform());
    }

    state->set_color(el->htmlTextColor().value_or(stb::color(0, 0, 0)));

    for(size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        auto lineWidth = measureForeignObjectLine(line, fonts);

        // A line is as tall as its tallest face (bold or italic can differ a little from the regular one);
        // a line of one style, the usual case, is simply that style's face.
        float ascent = font.ascent();
        float fontHeight = font.height();
        if(!line.empty()) {
            ascent = 0.f;
            fontHeight = 0.f;
            for(const auto& run : line) {
                ascent = STB_MAX(ascent, fonts.get(run.style).ascent());
                fontHeight = STB_MAX(fontHeight, fonts.get(run.style).height());
            }
        }

        // A real Trebuchet-MS-metrics render always fits each wrapped
        // line with room to spare -- so overflow here only ever means
        // OUR substitute font is wider than the one mermaid measured
        // with (this environment lacks Trebuchet MS). There's no
        // reliable way to know how much wider without the real font, so
        // rather than truncating (tried before, still didn't match a
        // real Chromium render) we condense that one line horizontally.
        float scale = 1.f;
        if(lineWidth > box.w && lineWidth > 0.f)
            scale = box.w / lineWidth;

        Point origin(
            box.x + (box.w - lineWidth * scale) / 2.f,
            // Baseline for this line = top of the text block + however
            // many line-heights precede it + half the leading (the extra
            // space line-height adds beyond the font's own ascent+descent,
            // split evenly above and below -- standard CSS line-box
            // behavior) + this font's ascent. Without the half-leading
            // term, text sits noticeably above center (font.height() is
            // usually well under a 1.5x line-height, so the omitted term
            // isn't a rounding-sized error).
            topY + float(i) * lineHeight + (lineHeight - fontHeight) / 2.f + ascent
        );

        auto textTransform = state.currentTransform();
        if(scale < 1.f) {
            // Condense horizontally about the line's own left edge so it
            // still lands at `origin`; only x is scaled, so glyph height
            // (and therefore this line's vertical position) is untouched.
            textTransform = textTransform * Transform::translated(origin.x, 0.f) * Transform::scaled(scale, 1.f) * Transform::translated(-origin.x, 0.f);
        }

        // The runs of the line side by side on the one baseline, each in its own face.
        float x = origin.x;
        for(const auto& run : line) {
            const std::u32string_view runView(run.text);
            const auto& runFont = fonts.get(run.style);
            state->fill_text(runView, runFont, Point(x, origin.y), textTransform);
            x += runFont.measureText(runView);
        }
    }
}

svg_symbol_element::svg_symbol_element(document* dom)
    : svg_graphics_element(dom, ElementID::Symbol)
    , svg_fit_to_view_box(this)
{
}

svg_g_element::svg_g_element(document* dom)
    : svg_graphics_element(dom, ElementID::G)
{
}

void svg_g_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

svg_switch_element::svg_switch_element(document* dom)
    : svg_graphics_element(dom, ElementID::Switch)
{
}

const svg_element* svg_switch_element::selectedChild() const
{
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        // only graphics elements take part; <title>, <desc>, <defs>... are skipped
        if(!el || isDisallowedElement(el))
            continue;
        if(switchChildPasses(el))
            return el;
    }
    return nullptr;
}

void svg_switch_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(auto child = selectedChild())
        child->render(newState);
    newState.endGroup(blendInfo);
}

svg_defs_element::svg_defs_element(document* dom)
    : svg_graphics_element(dom, ElementID::Defs)
{
}

svg_marker_element::svg_marker_element(document* dom)
    : svg_element(dom, ElementID::Marker)
    , svg_fit_to_view_box(this)
    , m_refX(PropertyID::RefX, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_refY(PropertyID::RefY, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_markerWidth(PropertyID::MarkerWidth, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerHeight(PropertyID::MarkerHeight, LengthDirection::Vertical, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerUnits(PropertyID::MarkerUnits, MarkerUnits::StrokeWidth)
    , m_orient(PropertyID::Orient)
{
    addProperty(m_refX);
    addProperty(m_refY);
    addProperty(m_markerWidth);
    addProperty(m_markerHeight);
    addProperty(m_markerUnits);
    addProperty(m_orient);
}

Point svg_marker_element::refPoint() const
{
    LengthContext lengthContext(this);
    const Point refPoint = {
        lengthContext.valueForLength(m_refX),
        lengthContext.valueForLength(m_refY)
    };

    return refPoint;
}

Size svg_marker_element::markerSize() const
{
    LengthContext lengthContext(this);
    const Size markerSize = {
        lengthContext.valueForLength(m_markerWidth),
        lengthContext.valueForLength(m_markerHeight)
    };

    return markerSize;
}

Transform svg_marker_element::markerTransform(const Point& origin, float angle, float strokeWidth) const
{
    auto transform = Transform::translated(origin.x, origin.y);
    if(m_orient.orientType() == svg_angle::OrientType::Angle) {
        transform.rotate(m_orient.value());
    } else {
        transform.rotate(angle);
    }

    auto viewTransform = viewBoxToViewTransform(markerSize());
    auto refOrigin = viewTransform.mapPoint(refPoint());
    if(m_markerUnits.value() == MarkerUnits::StrokeWidth)
        transform.scale(strokeWidth, strokeWidth);
    transform.translate(-refOrigin.x, -refOrigin.y);
    return transform * viewTransform;
}

Rect svg_marker_element::markerBoundingBox(const Point& origin, float angle, float strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).map_rect(paint_bounding_box());
}

void svg_marker_element::renderMarker(svg_render_state& state, const Point& origin, float angle, float strokeWidth) const
{
    if(state.hasCycleReference(this))
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, markerTransform(origin, angle, strokeWidth));
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(markerSize()), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

Transform svg_marker_element::local_transform() const
{
    return viewBoxToViewTransform(markerSize());
}

svg_clip_path_element::svg_clip_path_element(document* dom)
    : svg_graphics_element(dom, ElementID::ClipPath)
    , m_clipPathUnits(PropertyID::ClipPathUnits, Units::UserSpaceOnUse)
{
    addProperty(m_clipPathUnits);
}

Rect svg_clip_path_element::clipBoundingBox(const svg_element* el) const
{
    auto clipBoundingBox = paint_bounding_box();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        clipBoundingBox.x = clipBoundingBox.x * bbox.w + bbox.x;
        clipBoundingBox.y = clipBoundingBox.y * bbox.h + bbox.y;
        clipBoundingBox.w = clipBoundingBox.w * bbox.w;
        clipBoundingBox.h = clipBoundingBox.h * bbox.h;
    }

    return local_transform().map_rect(clipBoundingBox);
}

void svg_clip_path_element::applyClipMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Clipping, maskImage);
    renderChildren(newState);
    if(clipper()) {
        clipper()->applyClipMask(newState);
    }

    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

void svg_clip_path_element::applyClipPath(svg_render_state& state) const
{
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        Transform clipTransform(currentTransform);
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->id() != ElementID::Use)
                continue;
            clipTransform.multiply(el->local_transform());
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        state->clip_path(shapeElement->path(), shapeElement->clip_rule(), clipTransform * shapeElement->local_transform());
        return;
    }

    state->clip_rect(Rect::Empty, FillRule::NonZero, Transform::Identity);
}

bool svg_clip_path_element::requiresMasking() const
{
    if(clipper())
        return true;
    const svg_geometry_element* prevShapeElement = nullptr;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->isTextPositioningElement())
                return true;
            if(el->id() != ElementID::Use)
                continue;
            if(el->clipper())
                return true;
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        if(prevShapeElement || shapeElement->clipper())
            return true;
        prevShapeElement = shapeElement;
    }

    return false;
}

svg_mask_element::svg_mask_element(document* dom)
    : svg_element(dom, ElementID::Mask)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_maskUnits(PropertyID::MaskUnits, Units::ObjectBoundingBox)
    , m_maskContentUnits(PropertyID::MaskContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_maskUnits);
    addProperty(m_maskContentUnits);
}

Rect svg_mask_element::maskRect(const svg_element* el) const
{
    LengthContext lengthContext(this, m_maskUnits.value());
    Rect maskRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(m_maskUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    return maskRect;
}

Rect svg_mask_element::maskBoundingBox(const svg_element* el) const
{
    auto maskBoundingBox = paint_bounding_box();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskBoundingBox.x = maskBoundingBox.x * bbox.w + bbox.x;
        maskBoundingBox.y = maskBoundingBox.y * bbox.h + bbox.y;
        maskBoundingBox.w = maskBoundingBox.w * bbox.w;
        maskBoundingBox.h = maskBoundingBox.h * bbox.h;
    }

    return maskBoundingBox.intersected(maskRect(el));
}

void svg_mask_element::applyMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    maskImage->clip_rect(maskRect(state.el()), FillRule::NonZero, state.currentTransform());

    auto currentTransform = state.currentTransform();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Painting, maskImage);
    renderChildren(newState);
    if(clipper())
        clipper()->applyClipMask(newState);
    if(masker()) {
        masker()->applyMask(newState);
    }

    if(m_mask_type == MaskType::Luminance)
        maskImage->convert_to_luminance_mask();
    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

svg_filter_element::svg_filter_element(document* dom)
    : svg_element(dom, ElementID::Filter)
{
}

float svg_filter_element::pixelMargin() const
{
    float margin = 0.f;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        if(el->id() == ElementID::FeGaussianBlur) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 0.f);
            margin = STB_MAX(margin, sigma * 3.f);
        } else if(el->id() == ElementID::FeDropShadow) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 2.f);
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 2.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 2.f);
            margin = STB_MAX(margin, sigma * 3.f + STB_MAX(std::abs(dx), std::abs(dy)));
        } else if(el->id() == ElementID::FeOffset) {
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 0.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 0.f);
            margin = STB_MAX(margin, STB_MAX(std::abs(dx), std::abs(dy)));
        }
        // feFlood/feComposite/feMerge don't themselves need extra margin;
        // if they combine an already-blurred/offset named result, that
        // primitive already grew the margin.
    }

    return margin;
}

void svg_filter_element::applyFilter(svg_render_state& state, float scale) const
{
    svg_filter_context context(state.raster(), scale);
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        auto output = el->applyFilterPrimitive(context);
        if(!output)
            continue; // not a primitive (e.g. a stray feMergeNode outside feMerge)
        context.setResult(el->attribute(PropertyID::Result), std::move(output));
    }

    state.raster()->copy_pixels_from(*context.lastResult());
}

void svg_mask_element::layoutElement(const svg_layout_state& state)
{
    m_mask_type = state.mask_type();
    svg_element::layoutElement(state);
}

} // namespace nova::svg
