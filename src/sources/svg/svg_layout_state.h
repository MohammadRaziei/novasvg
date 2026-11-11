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

    const stb::color& color() const { return m_color; }
    const stb::color& stop_color() const { return m_stop_color; }

    float opacity() const { return m_opacity; }
    stb::blend_mode mix_blend_mode() const { return m_mix_blend_mode; }
    float stop_opacity() const { return m_stop_opacity; }
    float fill_opacity() const { return m_fill_opacity; }
    float stroke_opacity() const { return m_stroke_opacity; }
    float stroke_miterlimit() const { return m_stroke_miterlimit; }
    float font_size() const { return m_font_size; }

    const css_length& letter_spacing() const { return m_letter_spacing; }
    const css_length& word_spacing() const { return m_word_spacing; }

    const css_baseline_shift& baseline_shift() const { return m_baseline_shift; }
    const css_length& stroke_width() const { return m_stroke_width; }
    const css_length& stroke_dashoffset() const { return m_stroke_dashoffset; }
    const length_list& stroke_dasharray() const { return m_stroke_dasharray; }

    line_cap stroke_linecap() const { return m_stroke_linecap; }
    line_join stroke_linejoin() const { return m_stroke_linejoin; }

    stb::fill_rule fill_rule() const { return m_fill_rule; }
    stb::fill_rule clip_rule() const { return m_clip_rule; }

    css_font_weight font_weight() const { return m_font_weight; }
    css_font_style font_style() const { return m_font_style; }

    css_alignment_baseline alignment_baseline() const { return m_alignment_baseline; }
    css_dominant_baseline dominant_baseline() const { return m_dominant_baseline; }

    svg_text_anchor text_anchor() const { return m_text_anchor; }
    css_white_space white_space() const { return m_white_space; }
    css_writing_mode writing_mode() const { return m_writing_mode; }
    css_text_orientation text_orientation() const { return m_text_orientation; }
    css_direction direction() const { return m_direction; }

    css_display display() const { return m_display; }
    css_visibility visibility() const { return m_visibility; }
    css_overflow overflow() const { return m_overflow; }
    css_pointer_events pointer_events() const { return m_pointer_events; }
    css_mask_type mask_type() const { return m_mask_type; }

    const std::string& mask() const { return m_mask; }
    const std::string& clip_path() const { return m_clip_path; }
    const std::string& filter() const { return m_filter; }
    const std::string& marker_start() const { return m_marker_start; }
    const std::string& marker_mid() const { return m_marker_mid; }
    const std::string& marker_end() const { return m_marker_end; }
    const std::string& font_family() const { return m_font_family; }

    sized_font font() const;

private:
    const svg_layout_state* m_parent = nullptr;
    const svg_element* m_element = nullptr;

    paint m_fill{stb::color::black()};
    paint m_stroke{stb::color::transparent()};

    stb::color m_color = stb::color::black();
    stb::color m_stop_color = stb::color::black();

    float m_opacity = 1.f;
    stb::blend_mode m_mix_blend_mode = stb::blend_mode::normal; // like opacity: not inherited
    float m_fill_opacity = 1.f;
    float m_stroke_opacity = 1.f;
    float m_stop_opacity = 1.f;
    float m_stroke_miterlimit = 4.f;
    float m_font_size = 12.f;

    css_length m_letter_spacing{0.f, length_units::none};
    css_length m_word_spacing{0.f, length_units::none};

    css_baseline_shift m_baseline_shift;
    css_length m_stroke_width{1.f, length_units::none};
    css_length m_stroke_dashoffset{0.f, length_units::none};
    length_list m_stroke_dasharray;

    line_cap m_stroke_linecap = line_cap::butt;
    line_join m_stroke_linejoin = line_join::miter;

    stb::fill_rule m_fill_rule = stb::fill_rule::non_zero;
    stb::fill_rule m_clip_rule = stb::fill_rule::non_zero;

    css_font_weight m_font_weight = css_font_weight::normal;
    css_font_style m_font_style = css_font_style::normal;

    css_alignment_baseline m_alignment_baseline = css_alignment_baseline::auto_;
    css_dominant_baseline m_dominant_baseline = css_dominant_baseline::auto_;

    svg_text_anchor m_text_anchor = svg_text_anchor::start;
    css_white_space m_white_space = css_white_space::default_;
    css_writing_mode m_writing_mode = css_writing_mode::horizontal;
    css_text_orientation m_text_orientation = css_text_orientation::mixed;
    css_direction m_direction = css_direction::ltr;

    css_display m_display = css_display::inline_;
    css_visibility m_visibility = css_visibility::visible;
    css_overflow m_overflow = css_overflow::visible;
    css_pointer_events m_pointer_events = css_pointer_events::auto_;
    css_mask_type m_mask_type = css_mask_type::luminance;

    std::string m_mask;
    std::string m_clip_path;
    std::string m_filter;
    std::string m_marker_start;
    std::string m_marker_mid;
    std::string m_marker_end;
    std::string m_font_family;
};

} // namespace nova::svg
