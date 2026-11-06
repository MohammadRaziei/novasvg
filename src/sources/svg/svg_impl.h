#pragma once

// Inline and template definitions of the classes declared above, and the CSS variable substitution.

#include "svg_layout_state.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

// ================================================================
// Implementation
// ================================================================

// ---- svgproperty (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

PropertyID propertyid(std::string_view name);

PropertyID csspropertyid(std::string_view name);

template<>
NOVA_INLINE bool svg_enumeration<SpreadMethod>::parse(std::string_view input)
{
    static const svg_enumeration_entry<SpreadMethod> entries[] = {
        {SpreadMethod::Pad, "pad"},
        {SpreadMethod::Reflect, "reflect"},
        {SpreadMethod::Repeat, "repeat"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<Units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<Units> entries[] = {
        {Units::UserSpaceOnUse, "userSpaceOnUse"},
        {Units::ObjectBoundingBox, "objectBoundingBox"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<MarkerUnits>::parse(std::string_view input)
{
    static const svg_enumeration_entry<MarkerUnits> entries[] = {
        {MarkerUnits::StrokeWidth, "strokeWidth"},
        {MarkerUnits::UserSpaceOnUse, "userSpaceOnUse"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<LengthAdjust>::parse(std::string_view input)
{
    static const svg_enumeration_entry<LengthAdjust> entries[] = {
        {LengthAdjust::Spacing, "spacing"},
        {LengthAdjust::SpacingAndGlyphs, "spacingAndGlyphs"}
    };

    return parseEnum(input, entries);
}

template<typename Enum>
template<unsigned int N>
NOVA_INLINE bool svg_enumeration<Enum>::parseEnum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N])
{
    stripLeadingAndTrailingSpaces(input);
    for(const auto& entry : entries) {
        if(input == entry.second) {
            m_value = entry.first;
            return true;
        }
    }

    return false;
}

// ---- svgrenderstate (impl) ----

// ---- svgpaintelement (impl) ----

GradientStops buildGradientStops(const svg_gradient_element* el, float opacity);

// ---- svggeometryelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

// ---- svgtextelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

inline const svg_text_node* toSVGTextNode(const svg_node* nd)
{
    assert(nd && nd->is_text_node());
    return static_cast<const svg_text_node*>(nd);
}

inline const svg_text_positioning_element* toSVGTextPositioningElement(const svg_node* nd)
{
    assert(nd && nd->isTextPositioningElement());
    return static_cast<const svg_text_positioning_element*>(nd);
}

AlignmentBaseline resolveDominantBaseline(const svg_text_positioning_element* el);

float calculateBaselineOffset(const svg_text_positioning_element* el);

bool needsTextAnchorAdjustment(const svg_text_positioning_element* el);

float calculateTextAnchorOffset(const svg_text_positioning_element* el, float width);

using svg_text_fragment_iterator = svg_text_fragment_list::iterator;

float calculateTextChunkLength(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool isVerticalText);

void handleTextChunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end);

// ---- svglayoutstate (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

stb::color parseColor(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);

stb::color parseColorOrNone(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);

bool parseUrlValue(std::string_view& input, std::string& value);

Paint parsePaint(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);

float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue);

Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue);

BaselineShift parseBaselineShift(std::string_view input);

LengthList parseDashArray(std::string_view input);

Length parseLengthOrNormal(std::string_view input);

float parseFontSize(std::string_view input, const svg_layout_state* state);

template<typename Enum, unsigned int N>
NOVA_INLINE Enum parseEnumValue(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N], Enum defaultValue)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return defaultValue;
}

Display parseDisplay(std::string_view input);

Visibility parseVisibility(std::string_view input);

Overflow parseOverflow(std::string_view input);

PointerEvents parsePointerEvents(std::string_view input);

FontWeight parseFontWeight(std::string_view input);

FontStyle parseFontStyle(std::string_view input);

AlignmentBaseline parseAlignmentBaseline(std::string_view input);

DominantBaseline parseDominantBaseline(std::string_view input);

Direction parseDirection(std::string_view input);

WritingMode parseWritingMode(std::string_view input);

TextOrientation parseTextOrientation(std::string_view input);

TextAnchor parseTextAnchor(std::string_view input);

WhiteSpace parseWhiteSpace(std::string_view input);

MixBlendMode parseMixBlendMode(std::string_view input);

MaskType parseMaskType(std::string_view input);

FillRule parseFillRule(std::string_view input);

LineCap parseLineCap(std::string_view input);

LineJoin parseLineJoin(std::string_view input);

// The one place a (font-family stack, weight, style) triple becomes a Font, shared by svg <text>
// (svg_layout_state::font()) and by the bold/italic runs inside a <foreignObject>, so both always
// pick the very same face for the same request.
Font makeFont(const std::string& family, float size, bool bold, bool italic);

// ---- svgelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

ElementID elementid(std::string_view name);

NOVA_INLINE const std::string emptyString;

// Whether `value` has a `var(` function in it. Not for the attributes that are names or references, never CSS values.
inline bool usesVariables(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Id || id == PropertyID::Class || id == PropertyID::Href)
        return false;
    for(size_t pos = value.find("var("); pos != std::string::npos; pos = value.find("var(", pos + 1)) {
        if(pos == 0 || !IS_CSS_NAMECHAR(static_cast<unsigned char>(value[pos - 1])))
            return true;
    }
    return false;
}

// Replaces each `var(--name, fallback)` in `text` with the value `--name` has on `el` -- or the fallback, if
// there is no such property or its own var() cannot be resolved (a cycle, say). False if one cannot be replaced
// at all: the value is then invalid and the caller leaves the attribute alone. `depth` stops a cycle.
inline bool substituteVariables(std::string_view text, const svg_element* el, std::string& out, int depth)
{
    constexpr int MaxDepth = 32;
    if(depth > MaxDepth)
        return false;
    out.clear();
    while(!text.empty()) {
        const auto pos = text.find("var(");
        if(pos == std::string_view::npos) {
            out.append(text);
            break;
        }
        if(pos > 0 && IS_CSS_NAMECHAR(static_cast<unsigned char>(text[pos - 1]))) { // `somevar(`: not ours
            out.append(text.substr(0, pos + 4));
            text.remove_prefix(pos + 4);
            continue;
        }
        out.append(text.substr(0, pos));
        text.remove_prefix(pos + 4);

        // The arguments run to the matching `)`; `name` ends at the first comma outside any nesting.
        size_t nesting = 1, end = 0, comma = std::string_view::npos;
        char quote = 0;
        for(; end < text.size(); ++end) {
            const char c = text[end];
            if(quote) {
                if(c == quote)
                    quote = 0;
            } else if(c == '"' || c == '\'') {
                quote = c;
            } else if(c == '(') {
                ++nesting;
            } else if(c == ')') {
                if(--nesting == 0)
                    break;
            } else if(c == ',' && nesting == 1 && comma == std::string_view::npos) {
                comma = end;
            }
        }
        if(end == text.size())
            return false;

        auto name = text.substr(0, comma == std::string_view::npos ? end : comma);
        stripLeadingAndTrailingSpaces(name);
        if(name.size() < 3 || name[0] != '-' || name[1] != '-')
            return false;

        std::string value;
        bool found = false;
        if(auto owner = el->findCustomPropertyOwner(std::string(name))) {
            // Resolved where it is declared, then inherited as that.
            found = substituteVariables(owner->customProperty(std::string(name)), owner, value, depth + 1);
        }
        if(!found) {
            if(comma == std::string_view::npos)
                return false;
            auto fallback = text.substr(comma + 1, end - comma - 1);
            stripLeadingAndTrailingSpaces(fallback);
            if(!substituteVariables(fallback, el, value, depth + 1))
                return false;
        }
        out.append(value);
        text.remove_prefix(end + 1);
    }
    return true;
}

inline bool isDisallowedElement(const svg_element* el)
{
    switch(el->id()) {
    case ElementID::Circle:
    case ElementID::Ellipse:
    case ElementID::ForeignObject:
    case ElementID::G:
    case ElementID::Image:
    case ElementID::Line:
    case ElementID::Path:
    case ElementID::Polygon:
    case ElementID::Polyline:
    case ElementID::Rect:
    case ElementID::Svg:
    case ElementID::Switch:
    case ElementID::Symbol:
    case ElementID::Text:
    case ElementID::Tspan:
    case ElementID::Use:
        return false;
    default:
        return true;
    }
}

pixmap loadImageResource(const std::string& href);

} // namespace nova::svg
