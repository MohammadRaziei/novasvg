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

property_id propertyid(std::string_view name);

property_id csspropertyid(std::string_view name);

template<>
NOVA_INLINE bool svg_enumeration<SpreadMethod>::parse(std::string_view input)
{
    static const svg_enumeration_entry<SpreadMethod> entries[] = {
        {SpreadMethod::pad, "pad"},
        {SpreadMethod::reflect, "reflect"},
        {SpreadMethod::repeat, "repeat"}
    };

    return parse_enum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<Units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<Units> entries[] = {
        {Units::user_space_on_use, "userSpaceOnUse"},
        {Units::object_bounding_box, "objectBoundingBox"}
    };

    return parse_enum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<MarkerUnits>::parse(std::string_view input)
{
    static const svg_enumeration_entry<MarkerUnits> entries[] = {
        {MarkerUnits::StrokeWidth, "strokeWidth"},
        {MarkerUnits::user_space_on_use, "userSpaceOnUse"}
    };

    return parse_enum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<LengthAdjust>::parse(std::string_view input)
{
    static const svg_enumeration_entry<LengthAdjust> entries[] = {
        {LengthAdjust::Spacing, "spacing"},
        {LengthAdjust::spacing_and_glyphs, "spacingAndGlyphs"}
    };

    return parse_enum(input, entries);
}

template<typename Enum>
template<unsigned int N>
NOVA_INLINE bool svg_enumeration<Enum>::parse_enum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N])
{
    strip_leading_and_trailing_spaces(input);
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

GradientStops build_gradient_stops(const svg_gradient_element* el, float opacity);

// ---- svggeometryelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

// ---- svgtextelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

inline const svg_text_node* to_svg_text_node(const svg_node* nd)
{
    assert(nd && nd->is_text_node());
    return static_cast<const svg_text_node*>(nd);
}

inline const svg_text_positioning_element* to_svg_text_positioning_element(const svg_node* nd)
{
    assert(nd && nd->is_text_positioning_element());
    return static_cast<const svg_text_positioning_element*>(nd);
}

AlignmentBaseline resolve_dominant_baseline(const svg_text_positioning_element* el);

float calculate_baseline_offset(const svg_text_positioning_element* el);

bool needs_text_anchor_adjustment(const svg_text_positioning_element* el);

float calculate_text_anchor_offset(const svg_text_positioning_element* el, float width);

using svg_text_fragment_iterator = svg_text_fragment_list::iterator;

float calculate_text_chunk_length(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool is_vertical_text);

void handle_text_chunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end);

// ---- svglayoutstate (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

stb::color parse_color(std::string_view input, const svg_layout_state* state, const stb::color& default_value);

stb::color parse_color_or_none(std::string_view input, const svg_layout_state* state, const stb::color& default_value);

bool parse_url_value(std::string_view& input, std::string& value);

Paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value);

float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value);

Length parse_length(std::string_view input, length_negative_mode mode, const Length& default_value);

BaselineShift parse_baseline_shift(std::string_view input);

length_list parse_dash_array(std::string_view input);

Length parse_length_or_normal(std::string_view input);

float parse_font_size(std::string_view input, const svg_layout_state* state);

template<typename Enum, unsigned int N>
NOVA_INLINE Enum parse_enum_value(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N], Enum default_value)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return default_value;
}

Display parse_display(std::string_view input);

Visibility parse_visibility(std::string_view input);

Overflow parse_overflow(std::string_view input);

PointerEvents parse_pointer_events(std::string_view input);

FontWeight parse_font_weight(std::string_view input);

FontStyle parse_font_style(std::string_view input);

AlignmentBaseline parse_alignment_baseline(std::string_view input);

DominantBaseline parse_dominant_baseline(std::string_view input);

Direction parse_direction(std::string_view input);

WritingMode parse_writing_mode(std::string_view input);

TextOrientation parse_text_orientation(std::string_view input);

TextAnchor parse_text_anchor(std::string_view input);

WhiteSpace parse_white_space(std::string_view input);

MixBlendMode parse_mix_blend_mode(std::string_view input);

MaskType parse_mask_type(std::string_view input);

FillRule parse_fill_rule(std::string_view input);

LineCap parse_line_cap(std::string_view input);

LineJoin parse_line_join(std::string_view input);

// The one place a (font-family stack, weight, style) triple becomes a Font, shared by svg <text>
// (svg_layout_state::font()) and by the bold/italic runs inside a <foreignObject>, so both always
// pick the very same face for the same request.
Font make_font(const std::string& family, float size, bool bold, bool italic);

// ---- svgelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

element_id elementid(std::string_view name);

NOVA_INLINE const std::string empty_string;

// Whether `value` has a `var(` function in it. Not for the attributes that are names or references, never CSS values.
inline bool uses_variables(property_id id, const std::string& value)
{
    if(id == property_id::Id || id == property_id::Class || id == property_id::Href)
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
inline bool substitute_variables(std::string_view text, const svg_element* el, std::string& out, int depth)
{
    constexpr int max_depth = 32;
    if(depth > max_depth)
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
        strip_leading_and_trailing_spaces(name);
        if(name.size() < 3 || name[0] != '-' || name[1] != '-')
            return false;

        std::string value;
        bool found = false;
        if(auto owner = el->find_custom_property_owner(std::string(name))) {
            // Resolved where it is declared, then inherited as that.
            found = substitute_variables(owner->custom_property(std::string(name)), owner, value, depth + 1);
        }
        if(!found) {
            if(comma == std::string_view::npos)
                return false;
            auto fallback = text.substr(comma + 1, end - comma - 1);
            strip_leading_and_trailing_spaces(fallback);
            if(!substitute_variables(fallback, el, value, depth + 1))
                return false;
        }
        out.append(value);
        text.remove_prefix(end + 1);
    }
    return true;
}

inline bool is_disallowed_element(const svg_element* el)
{
    switch(el->id()) {
    case element_id::Circle:
    case element_id::ellipse:
    case element_id::ForeignObject:
    case element_id::G:
    case element_id::Image:
    case element_id::Line:
    case element_id::Path:
    case element_id::polygon:
    case element_id::polyline:
    case element_id::Rect:
    case element_id::Svg:
    case element_id::Switch:
    case element_id::symbol:
    case element_id::Text:
    case element_id::tspan:
    case element_id::use:
        return false;
    default:
        return true;
    }
}

pixmap load_image_resource(const std::string& href);

} // namespace nova::svg
