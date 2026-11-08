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
inline bool svg_enumeration<spread_method>::parse(std::string_view input)
{
    static const svg_enumeration_entry<spread_method> entries[] = {
        {spread_method::pad, "pad"},
        {spread_method::reflect, "reflect"},
        {spread_method::repeat, "repeat"}
    };

    return parse_enum(input, entries);
}

template<>
inline bool svg_enumeration<units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<units> entries[] = {
        {units::user_space_on_use, "userSpaceOnUse"},
        {units::object_bounding_box, "objectBoundingBox"}
    };

    return parse_enum(input, entries);
}

template<>
inline bool svg_enumeration<marker_units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<marker_units> entries[] = {
        {marker_units::stroke_width, "strokeWidth"},
        {marker_units::user_space_on_use, "userSpaceOnUse"}
    };

    return parse_enum(input, entries);
}

template<>
inline bool svg_enumeration<length_adjust>::parse(std::string_view input)
{
    static const svg_enumeration_entry<length_adjust> entries[] = {
        {length_adjust::spacing, "spacing"},
        {length_adjust::spacing_and_glyphs, "spacingAndGlyphs"}
    };

    return parse_enum(input, entries);
}

template<typename enum_>
template<unsigned int N>
inline bool svg_enumeration<enum_>::parse_enum(std::string_view input, const svg_enumeration_entry<enum_>(&entries)[N])
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

gradient_stops build_gradient_stops(const svg_gradient_element* el, float opacity);

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

alignment_baseline resolve_dominant_baseline(const svg_text_positioning_element* el);

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

paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value);

float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value);

length parse_length(std::string_view input, length_negative_mode mode, const length& default_value);

baseline_shift parse_baseline_shift(std::string_view input);

length_list parse_dash_array(std::string_view input);

length parse_length_or_normal(std::string_view input);

float parse_font_size(std::string_view input, const svg_layout_state* state);

template<typename enum_, unsigned int N>
inline enum_ parse_enum_value(std::string_view input, const svg_enumeration_entry<enum_>(&entries)[N], enum_ default_value)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return default_value;
}

display parse_display(std::string_view input);

visibility parse_visibility(std::string_view input);

overflow parse_overflow(std::string_view input);

pointer_events parse_pointer_events(std::string_view input);

font_weight parse_font_weight(std::string_view input);

font_style parse_font_style(std::string_view input);

alignment_baseline parse_alignment_baseline(std::string_view input);

dominant_baseline parse_dominant_baseline(std::string_view input);

direction parse_direction(std::string_view input);

writing_mode parse_writing_mode(std::string_view input);

text_orientation parse_text_orientation(std::string_view input);

text_anchor parse_text_anchor(std::string_view input);

white_space parse_white_space(std::string_view input);

mix_blend_mode parse_mix_blend_mode(std::string_view input);

mask_type parse_mask_type(std::string_view input);

fill_rule parse_fill_rule(std::string_view input);

line_cap parse_line_cap(std::string_view input);

line_join parse_line_join(std::string_view input);

// The one place a (font-family stack, weight, style) triple becomes a Font, shared by svg <text>
// (svg_layout_state::font()) and by the bold/italic runs inside a <foreignObject>, so both always
// pick the very same face for the same request.
font make_font(const std::string& family, float size_, bool bold, bool italic);

// ---- svgelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

element_id elementid(std::string_view name);

inline const std::string empty_string;

// Whether `value` has a `var(` function in it. Not for the attributes that are names or references, never CSS values.
inline bool uses_variables(property_id id, const std::string& value)
{
    if(id == property_id::id || id == property_id::class_ || id == property_id::href)
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
    case element_id::circle:
    case element_id::ellipse:
    case element_id::foreign_object:
    case element_id::G:
    case element_id::image:
    case element_id::line:
    case element_id::path:
    case element_id::polygon:
    case element_id::polyline:
    case element_id::rect:
    case element_id::svg:
    case element_id::switch_:
    case element_id::symbol:
    case element_id::text:
    case element_id::tspan:
    case element_id::use:
        return false;
    default:
        return true;
    }
}

pixmap load_image_resource(const std::string& href);

} // namespace nova::svg
