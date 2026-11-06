#pragma once

// Parsing of property values, CSS values and names (svg_parse.cpp).

#include "svg_element_helpers.h"

namespace nova::svg {
using namespace vg;

PropertyID propertyid(std::string_view name);
PropertyID csspropertyid(std::string_view name);
std::optional<stb::color> parseColorValue(std::string_view& input, const svg_layout_state* state);
stb::color parseColor(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);
stb::color parseColorOrNone(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);
bool parseUrlValue(std::string_view& input, std::string& value);
std::string parseUrl(std::string_view input);
Paint parsePaint(std::string_view input, const svg_layout_state* state, const stb::color& defaultValue);
float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue);
Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue);
BaselineShift parseBaselineShift(std::string_view input);
LengthList parseDashArray(std::string_view input);
Length parseLengthOrNormal(std::string_view input);
float parseFontSize(std::string_view input, const svg_layout_state* state);
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
ElementID elementid(std::string_view name);

} // namespace nova::svg
