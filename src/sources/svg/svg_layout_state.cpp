#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

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

} // namespace nova::svg
