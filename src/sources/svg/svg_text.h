#pragma once

// Text elements and the fragment builder.

#include "svg_shapes.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

// ---- svgtextelement ----

class svg_text_positioning_element;
class svg_text_element;

struct svg_character_position {
    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> dx;
    std::optional<float> dy;
    std::optional<float> rotate;
};

using svg_character_positions = std::map<size_t, svg_character_position>;

struct svg_text_position {
    svg_text_position(const svg_node* nd, size_t start_offset, size_t end_offset)
        : nd(nd), start_offset(start_offset), end_offset(end_offset)
    {}

    const svg_node* nd;
    size_t start_offset;
    size_t end_offset;
};

using svg_text_position_list = std::vector<svg_text_position>;

struct svg_text_fragment {
    explicit svg_text_fragment(const svg_text_positioning_element* el) : el(el) {}
    const svg_text_positioning_element* el;
    transform length_adjust_transform;
    size_t offset = 0;
    size_t length_ = 0;
    bool starts_new_text_chunk = false;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float angle = 0;
};

using svg_text_fragment_list = std::vector<svg_text_fragment>;

class svg_text_fragments_builder {
public:
    svg_text_fragments_builder(std::u32string& text, svg_text_fragment_list& fragments);

    void build(const svg_text_element* text_element);

private:
    void handle_text(const svg_text_node* nd);
    void handle_element(const svg_text_positioning_element* el);
    void fill_character_positions(const svg_text_position& position);
    std::u32string& m_text;
    svg_text_fragment_list& m_fragments;
    svg_character_positions m_character_positions;
    svg_text_position_list m_text_positions;
    size_t m_character_offset = 0;
    float m_x = 0;
    float m_y = 0;
};

class svg_text_positioning_element : public svg_graphics_element {
public:
    svg_text_positioning_element(document* dom, element_id id);

    bool is_text_positioning_element() const final { return true; }

    const length_list& x() const { return m_x.values(); }
    const length_list& y() const { return m_y.values(); }
    const length_list& dx() const { return m_dx.values(); }
    const length_list& dy() const { return m_dy.values(); }
    const number_list& rotate() const { return m_rotate.values(); }

    const svg_length& text_length() const { return m_text_length; }
    svg_length_adjust length_adjust() const { return m_length_adjust.value(); }

    const sized_font& font() const { return m_font; }
    const svg_paint_server& fill() const { return m_fill; }
    const svg_paint_server& stroke() const { return m_stroke; }

    bool is_vertical_writing_mode() const { return m_writing_mode == css_writing_mode::vertical; }
    bool is_upright_text_orientation() const { return m_text_orientation == css_text_orientation::upright; }

    float stroke_width() const { return m_stroke_width; }
    float letter_spacing() const { return m_letter_spacing; }
    float word_spacing() const { return m_word_spacing; }
    float baseline_offset() const { return m_baseline_offset; }
    css_alignment_baseline alignment_baseline() const { return m_alignment_baseline; }
    css_dominant_baseline dominant_baseline() const { return m_dominant_baseline; }
    svg_text_anchor text_anchor() const { return m_text_anchor; }
    css_white_space white_space() const { return m_white_space; }
    css_direction direction() const { return m_direction; }

    void layout_element(const svg_layout_state& state) override;

private:
    float convert_baseline_offset(const css_baseline_shift& baseline_shift_) const;
    svg_length_list m_x;
    svg_length_list m_y;
    svg_length_list m_dx;
    svg_length_list m_dy;
    svg_number_list m_rotate;

    svg_length m_text_length;
    svg_enumeration<svg_length_adjust> m_length_adjust;

    sized_font m_font;
    svg_paint_server m_fill;
    svg_paint_server m_stroke;

    float m_stroke_width = 1.f;
    float m_letter_spacing = 0.f;
    float m_word_spacing = 0.f;
    float m_baseline_offset = 0.f;
    css_alignment_baseline m_alignment_baseline = css_alignment_baseline::auto_;
    css_dominant_baseline m_dominant_baseline = css_dominant_baseline::auto_;
    svg_text_anchor m_text_anchor = svg_text_anchor::start;
    css_white_space m_white_space = css_white_space::default_;
    css_writing_mode m_writing_mode = css_writing_mode::horizontal;
    css_text_orientation m_text_orientation = css_text_orientation::mixed;
    css_direction m_direction = css_direction::ltr;
};

class svg_t_span_element final : public svg_text_positioning_element {
public:
    svg_t_span_element(document* dom);
};

class svg_text_element final : public svg_text_positioning_element {
public:
    svg_text_element(document* dom);

    rect fill_bounding_box() const final { return bounding_box(false); }
    rect stroke_bounding_box() const final { return bounding_box(true); }

    void layout(svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    rect bounding_box(bool include_stroke) const;
    svg_text_fragment_list m_fragments;
    std::u32string m_text;
};

} // namespace nova::svg
