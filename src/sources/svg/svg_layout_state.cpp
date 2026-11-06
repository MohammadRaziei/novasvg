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
    , m_overflow(el->is_root_element() ? Overflow::visible : Overflow::hidden)
    , m_pointer_events(parent.pointer_events())
    , m_marker_start(parent.marker_start())
    , m_marker_mid(parent.marker_mid())
    , m_marker_end(parent.marker_end())
    , m_font_family(parent.font_family())
{
    for(const auto& attribute : el->attributes()) {
        std::string_view input(attribute.value());
        strip_leading_and_trailing_spaces(input);
        if(input.empty() || input.compare("inherit") == 0)
            continue;
        switch(attribute.id()) {
        case property_id::Fill:
            m_fill = parse_paint(input, this, stb::color::black());
            break;
        case property_id::Stroke:
            m_stroke = parse_paint(input, this, stb::color::transparent());
            break;
        case property_id::Color:
            m_color = parse_color(input, this, stb::color::black());
            break;
        case property_id::Stop_Color:
            m_stop_color = parse_color(input, this, stb::color::black());
            break;
        case property_id::Opacity:
            m_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::Mix_Blend_Mode:
            m_mix_blend_mode = parse_mix_blend_mode(input);
            break;
        case property_id::Fill_Opacity:
            m_fill_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::Stroke_Opacity:
            m_stroke_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::Stop_Opacity:
            m_stop_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::Stroke_Miterlimit:
            m_stroke_miterlimit = parse_number_or_percentage(input, false, 4.f);
            break;
        case property_id::Font_Size:
            m_font_size = parse_font_size(input, this);
            break;
        case property_id::Letter_Spacing:
            m_letter_spacing = parse_length_or_normal(input);
            break;
        case property_id::Word_Spacing:
            m_word_spacing = parse_length_or_normal(input);
            break;
        case property_id::Baseline_Shift:
            m_baseline_shift = parse_baseline_shift(input);
            break;
        case property_id::Stroke_Width:
            m_stroke_width = parse_length(input, length_negative_mode::forbid, Length(1.f, length_units::None));
            break;
        case property_id::Stroke_Dashoffset:
            m_stroke_dashoffset = parse_length(input, length_negative_mode::allow, Length(0.f, length_units::None));
            break;
        case property_id::Stroke_Dasharray:
            m_stroke_dasharray = parse_dash_array(input);
            break;
        case property_id::Stroke_Linecap:
            m_stroke_linecap = parse_line_cap(input);
            break;
        case property_id::Stroke_Linejoin:
            m_stroke_linejoin = parse_line_join(input);
            break;
        case property_id::Fill_Rule:
            m_fill_rule = parse_fill_rule(input);
            break;
        case property_id::Clip_Rule:
            m_clip_rule = parse_fill_rule(input);
            break;
        case property_id::Font_Weight:
            m_font_weight = parse_font_weight(input);
            break;
        case property_id::Font_Style:
            m_font_style = parse_font_style(input);
            break;
        case property_id::Alignment_Baseline:
            m_alignment_baseline = parse_alignment_baseline(input);
            break;
        case property_id::Dominant_Baseline:
            m_dominant_baseline = parse_dominant_baseline(input);
            break;
        case property_id::Direction:
            m_direction = parse_direction(input);
            break;
        case property_id::Text_Anchor:
            m_text_anchor = parse_text_anchor(input);
            break;
        case property_id::White_Space:
            m_white_space = parse_white_space(input);
            break;
        case property_id::Writing_Mode:
            m_writing_mode = parse_writing_mode(input);
            break;
        case property_id::Text_Orientation:
            m_text_orientation = parse_text_orientation(input);
            break;
        case property_id::Display:
            m_display = parse_display(input);
            break;
        case property_id::Visibility:
            m_visibility = parse_visibility(input);
            break;
        case property_id::Overflow:
            m_overflow = parse_overflow(input);
            break;
        case property_id::Pointer_Events:
            m_pointer_events = parse_pointer_events(input);
            break;
        case property_id::Mask_Type:
            m_mask_type = parse_mask_type(input);
            break;
        case property_id::Mask:
            m_mask = parse_url(input);
            break;
        case property_id::Clip_Path:
            m_clip_path = parse_url(input);
            break;
        case property_id::Filter:
            m_filter = parse_url(input);
            break;
        case property_id::Marker_Start:
            m_marker_start = parse_url(input);
            break;
        case property_id::Marker_Mid:
            m_marker_mid = parse_url(input);
            break;
        case property_id::Marker_End:
            m_marker_end = parse_url(input);
            break;
        case property_id::Font_Family:
            m_font_family.assign(input);
            break;
        default:
            break;
        }
    }
}

Font make_font(const std::string& family, float size, bool bold, bool italic)
{
    // Walk the font-family stack name by name (see
    // FontFaceCache::getFontFaceForFamilyStack()): an @font-face-embedded
    // font wins under its exact declared name, an uninstalled name falls
    // back to a metric-compatible substitute, unresolvable names are skipped.
    FontFace face = fontFaceCache()->get_font_face_for_family_stack(family, bold, italic);

    // Absolute last resort: the hardcoded generic-family table.
    if(face.is_null())
        face = fontFaceCache()->get_font_face(empty_string, bold, italic);
    return Font(face, size);
}

Font svg_layout_state::font() const
{
    return make_font(m_font_family, m_font_size, m_font_weight == FontWeight::Bold, m_font_style == FontStyle::Italic);
}

} // namespace nova::svg
