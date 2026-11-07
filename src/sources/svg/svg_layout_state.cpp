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
    , m_color(parent.color_())
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
    , m_fill_rule(parent.fill_rule_())
    , m_clip_rule(parent.clip_rule())
    , m_font_weight(parent.font_weight_())
    , m_font_style(parent.font_style_())
    , m_dominant_baseline(parent.dominant_baseline_())
    , m_text_anchor(parent.text_anchor_())
    , m_white_space(parent.white_space_())
    , m_writing_mode(parent.writing_mode_())
    , m_text_orientation(parent.text_orientation_())
    , m_direction(parent.direction_())
    , m_visibility(parent.visibility_())
    , m_overflow(el->is_root_element() ? overflow::visible : overflow::hidden)
    , m_pointer_events(parent.pointer_events_())
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
        case property_id::fill:
            m_fill = parse_paint(input, this, stb::color::black());
            break;
        case property_id::stroke:
            m_stroke = parse_paint(input, this, stb::color::transparent());
            break;
        case property_id::color:
            m_color = parse_color(input, this, stb::color::black());
            break;
        case property_id::stop_color:
            m_stop_color = parse_color(input, this, stb::color::black());
            break;
        case property_id::opacity:
            m_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::mix_blend_mode_:
            m_mix_blend_mode = parse_mix_blend_mode(input);
            break;
        case property_id::fill_opacity:
            m_fill_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::stroke_opacity:
            m_stroke_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::stop_opacity:
            m_stop_opacity = parse_number_or_percentage(input, true, 1.f);
            break;
        case property_id::stroke_miterlimit:
            m_stroke_miterlimit = parse_number_or_percentage(input, false, 4.f);
            break;
        case property_id::font_size:
            m_font_size = parse_font_size(input, this);
            break;
        case property_id::letter_spacing:
            m_letter_spacing = parse_length_or_normal(input);
            break;
        case property_id::word_spacing:
            m_word_spacing = parse_length_or_normal(input);
            break;
        case property_id::baseline_shift_:
            m_baseline_shift = parse_baseline_shift(input);
            break;
        case property_id::stroke_width:
            m_stroke_width = parse_length(input, length_negative_mode::forbid, length(1.f, length_units::none));
            break;
        case property_id::stroke_dashoffset:
            m_stroke_dashoffset = parse_length(input, length_negative_mode::allow, length(0.f, length_units::none));
            break;
        case property_id::stroke_dasharray:
            m_stroke_dasharray = parse_dash_array(input);
            break;
        case property_id::stroke_linecap:
            m_stroke_linecap = parse_line_cap(input);
            break;
        case property_id::stroke_linejoin:
            m_stroke_linejoin = parse_line_join(input);
            break;
        case property_id::fill_rule_:
            m_fill_rule = parse_fill_rule(input);
            break;
        case property_id::clip_rule:
            m_clip_rule = parse_fill_rule(input);
            break;
        case property_id::font_weight_:
            m_font_weight = parse_font_weight(input);
            break;
        case property_id::font_style_:
            m_font_style = parse_font_style(input);
            break;
        case property_id::alignment_baseline_:
            m_alignment_baseline = parse_alignment_baseline(input);
            break;
        case property_id::dominant_baseline_:
            m_dominant_baseline = parse_dominant_baseline(input);
            break;
        case property_id::direction:
            m_direction = parse_direction(input);
            break;
        case property_id::text_anchor_:
            m_text_anchor = parse_text_anchor(input);
            break;
        case property_id::white_space_:
            m_white_space = parse_white_space(input);
            break;
        case property_id::writing_mode_:
            m_writing_mode = parse_writing_mode(input);
            break;
        case property_id::text_orientation_:
            m_text_orientation = parse_text_orientation(input);
            break;
        case property_id::display:
            m_display = parse_display(input);
            break;
        case property_id::visibility:
            m_visibility = parse_visibility(input);
            break;
        case property_id::overflow:
            m_overflow = parse_overflow(input);
            break;
        case property_id::pointer_events_:
            m_pointer_events = parse_pointer_events(input);
            break;
        case property_id::mask_type_:
            m_mask_type = parse_mask_type(input);
            break;
        case property_id::mask:
            m_mask = parse_url(input);
            break;
        case property_id::clip_path:
            m_clip_path = parse_url(input);
            break;
        case property_id::filter:
            m_filter = parse_url(input);
            break;
        case property_id::marker_start:
            m_marker_start = parse_url(input);
            break;
        case property_id::marker_mid:
            m_marker_mid = parse_url(input);
            break;
        case property_id::marker_end:
            m_marker_end = parse_url(input);
            break;
        case property_id::font_family:
            m_font_family.assign(input);
            break;
        default:
            break;
        }
    }
}

font make_font(const std::string& family, float size_, bool bold, bool italic)
{
    // Walk the font-family stack name by name (see
    // FontFaceCache::getFontFaceForFamilyStack()): an @font-face-embedded
    // font wins under its exact declared name, an uninstalled name falls
    // back to a metric-compatible substitute, unresolvable names are skipped.
    font_face face = font_face_cache_()->get_font_face_for_family_stack(family, bold, italic);

    // Absolute last resort: the hardcoded generic-family table.
    if(face.is_null())
        face = font_face_cache_()->get_font_face(empty_string, bold, italic);
    return font(face, size_);
}

font svg_layout_state::font_() const
{
    return make_font(m_font_family, m_font_size, m_font_weight == font_weight::bold, m_font_style == font_style::italic);
}

} // namespace nova::svg
