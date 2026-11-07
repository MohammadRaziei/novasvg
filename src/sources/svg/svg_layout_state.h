#pragma once

// svg_layout_state: the inherited presentation properties.

#include "svg_text.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

// ---- svglayoutstate ----

class svg_layout_state {
public:
    svg_layout_state() = default;
    svg_layout_state(const svg_layout_state& parent, const svg_element* el);

    const svg_layout_state* parent() const { return m_parent; }
    const svg_element* el() const { return m_element; }

    const paint& fill() const { return m_fill; }
    const paint& stroke() const { return m_stroke; }

    const stb::color& color_() const { return m_color; }
    const stb::color& stop_color() const { return m_stop_color; }

    float opacity() const { return m_opacity; }
    mix_blend_mode mix_blend_mode_() const { return m_mix_blend_mode; }
    float stop_opacity() const { return m_stop_opacity; }
    float fill_opacity() const { return m_fill_opacity; }
    float stroke_opacity() const { return m_stroke_opacity; }
    float stroke_miterlimit() const { return m_stroke_miterlimit; }
    float font_size() const { return m_font_size; }

    const length& letter_spacing() const { return m_letter_spacing; }
    const length& word_spacing() const { return m_word_spacing; }

    const baseline_shift& baseline_shift_() const { return m_baseline_shift; }
    const length& stroke_width() const { return m_stroke_width; }
    const length& stroke_dashoffset() const { return m_stroke_dashoffset; }
    const length_list& stroke_dasharray() const { return m_stroke_dasharray; }

    line_cap stroke_linecap() const { return m_stroke_linecap; }
    line_join stroke_linejoin() const { return m_stroke_linejoin; }

    fill_rule fill_rule_() const { return m_fill_rule; }
    fill_rule clip_rule() const { return m_clip_rule; }

    font_weight font_weight_() const { return m_font_weight; }
    font_style font_style_() const { return m_font_style; }

    alignment_baseline alignment_baseline_() const { return m_alignment_baseline; }
    dominant_baseline dominant_baseline_() const { return m_dominant_baseline; }

    text_anchor text_anchor_() const { return m_text_anchor; }
    white_space white_space_() const { return m_white_space; }
    writing_mode writing_mode_() const { return m_writing_mode; }
    text_orientation text_orientation_() const { return m_text_orientation; }
    direction direction_() const { return m_direction; }

    display display_() const { return m_display; }
    visibility visibility_() const { return m_visibility; }
    overflow overflow_() const { return m_overflow; }
    pointer_events pointer_events_() const { return m_pointer_events; }
    mask_type mask_type_() const { return m_mask_type; }

    const std::string& mask() const { return m_mask; }
    const std::string& clip_path() const { return m_clip_path; }
    const std::string& filter() const { return m_filter; }
    const std::string& marker_start() const { return m_marker_start; }
    const std::string& marker_mid() const { return m_marker_mid; }
    const std::string& marker_end() const { return m_marker_end; }
    const std::string& font_family() const { return m_font_family; }

    font font_() const;

private:
    const svg_layout_state* m_parent = nullptr;
    const svg_element* m_element = nullptr;

    paint m_fill{stb::color::black()};
    paint m_stroke{stb::color::transparent()};

    stb::color m_color = stb::color::black();
    stb::color m_stop_color = stb::color::black();

    float m_opacity = 1.f;
    mix_blend_mode m_mix_blend_mode = mix_blend_mode::normal; // like opacity: not inherited
    float m_fill_opacity = 1.f;
    float m_stroke_opacity = 1.f;
    float m_stop_opacity = 1.f;
    float m_stroke_miterlimit = 4.f;
    float m_font_size = 12.f;

    length m_letter_spacing{0.f, length_units::none};
    length m_word_spacing{0.f, length_units::none};

    baseline_shift m_baseline_shift;
    length m_stroke_width{1.f, length_units::none};
    length m_stroke_dashoffset{0.f, length_units::none};
    length_list m_stroke_dasharray;

    line_cap m_stroke_linecap = line_cap::butt;
    line_join m_stroke_linejoin = line_join::miter;

    fill_rule m_fill_rule = fill_rule::non_zero;
    fill_rule m_clip_rule = fill_rule::non_zero;

    font_weight m_font_weight = font_weight::normal;
    font_style m_font_style = font_style::normal;

    alignment_baseline m_alignment_baseline = alignment_baseline::auto_;
    dominant_baseline m_dominant_baseline = dominant_baseline::auto_;

    text_anchor m_text_anchor = text_anchor::start;
    white_space m_white_space = white_space::default_;
    writing_mode m_writing_mode = writing_mode::horizontal;
    text_orientation m_text_orientation = text_orientation::mixed;
    direction m_direction = direction::ltr;

    display m_display = display::inline_;
    visibility m_visibility = visibility::visible;
    overflow m_overflow = overflow::visible;
    pointer_events m_pointer_events = pointer_events::auto_;
    mask_type m_mask_type = mask_type::luminance;

    std::string m_mask;
    std::string m_clip_path;
    std::string m_filter;
    std::string m_marker_start;
    std::string m_marker_mid;
    std::string m_marker_end;
    std::string m_font_family;
};

} // namespace nova::svg
