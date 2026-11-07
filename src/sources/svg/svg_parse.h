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
paint parse_paint(std::string_view input, const svg_layout_state* state, const stb::color& default_value);
float parse_number_or_percentage(std::string_view input, bool allow_percentage, float default_value);
length parse_length(std::string_view input, length_negative_mode mode, const length& default_value);
baseline_shift parse_baseline_shift(std::string_view input);
length_list parse_dash_array(std::string_view input);
length parse_length_or_normal(std::string_view input);
float parse_font_size(std::string_view input, const svg_layout_state* state);
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
element_id elementid(std::string_view name);

} // namespace nova::svg
