#pragma once

// Parsing of property values, CSS values and names (svg_parse.cpp).

#include "svg_element_helpers.h"

namespace nova::svg {
using namespace vg;

property_id propertyid(std::string_view name);
property_id csspropertyid(std::string_view name);
std::optional<stb::color> parse_color_value(std::string_view& input, const svg_layout_state* state);
stb::color parse_color(std::string_view input, const svg_layout_state* state, const stb::color& default_value);
stb::color parse_color_or_none(std::string_view input, const svg_layout_state* state, const stb::color& default_value);
bool parse_url_value(std::string_view& input, std::string& value);
std::string parse_url(std::string_view input);
Paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value);
float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value);
Length parse_length(std::string_view input, length_negative_mode mode, const Length& default_value);
BaselineShift parse_baseline_shift(std::string_view input);
length_list parse_dash_array(std::string_view input);
Length parse_length_or_normal(std::string_view input);
float parse_font_size(std::string_view input, const svg_layout_state* state);
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
element_id elementid(std::string_view name);

} // namespace nova::svg
