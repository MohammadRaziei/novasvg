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
    svg_text_position(const svg_node* nd, size_t startOffset, size_t endOffset)
        : nd(nd), startOffset(startOffset), endOffset(endOffset)
    {}

    const svg_node* nd;
    size_t startOffset;
    size_t endOffset;
};

using svg_text_position_list = std::vector<svg_text_position>;

struct svg_text_fragment {
    explicit svg_text_fragment(const svg_text_positioning_element* el) : el(el) {}
    const svg_text_positioning_element* el;
    Transform lengthAdjustTransform;
    size_t offset = 0;
    size_t length = 0;
    bool startsNewTextChunk = false;
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

    void build(const svg_text_element* textElement);

private:
    void handleText(const svg_text_node* nd);
    void handleElement(const svg_text_positioning_element* el);
    void fillCharacterPositions(const svg_text_position& position);
    std::u32string& m_text;
    svg_text_fragment_list& m_fragments;
    svg_character_positions m_characterPositions;
    svg_text_position_list m_textPositions;
    size_t m_characterOffset = 0;
    float m_x = 0;
    float m_y = 0;
};

class svg_text_positioning_element : public svg_graphics_element {
public:
    svg_text_positioning_element(document* dom, ElementID id);

    bool isTextPositioningElement() const final { return true; }

    const LengthList& x() const { return m_x.values(); }
    const LengthList& y() const { return m_y.values(); }
    const LengthList& dx() const { return m_dx.values(); }
    const LengthList& dy() const { return m_dy.values(); }
    const NumberList& rotate() const { return m_rotate.values(); }

    const svg_length& textLength() const { return m_textLength; }
    LengthAdjust lengthAdjust() const { return m_lengthAdjust.value(); }

    const Font& font() const { return m_font; }
    const svg_paint_server& fill() const { return m_fill; }
    const svg_paint_server& stroke() const { return m_stroke; }

    bool isVerticalWritingMode() const { return m_writing_mode == WritingMode::Vertical; }
    bool isUprightTextOrientation() const { return m_text_orientation == TextOrientation::Upright; }

    float stroke_width() const { return m_stroke_width; }
    float letter_spacing() const { return m_letter_spacing; }
    float word_spacing() const { return m_word_spacing; }
    float baseline_offset() const { return m_baseline_offset; }
    AlignmentBaseline alignment_baseline() const { return m_alignment_baseline; }
    DominantBaseline dominant_baseline() const { return m_dominant_baseline; }
    TextAnchor text_anchor() const { return m_text_anchor; }
    WhiteSpace white_space() const { return m_white_space; }
    Direction direction() const { return m_direction; }

    void layoutElement(const svg_layout_state& state) override;

private:
    float convertBaselineOffset(const BaselineShift& baselineShift) const;
    svg_length_list m_x;
    svg_length_list m_y;
    svg_length_list m_dx;
    svg_length_list m_dy;
    svg_number_list m_rotate;

    svg_length m_textLength;
    svg_enumeration<LengthAdjust> m_lengthAdjust;

    Font m_font;
    svg_paint_server m_fill;
    svg_paint_server m_stroke;

    float m_stroke_width = 1.f;
    float m_letter_spacing = 0.f;
    float m_word_spacing = 0.f;
    float m_baseline_offset = 0.f;
    AlignmentBaseline m_alignment_baseline = AlignmentBaseline::Auto;
    DominantBaseline m_dominant_baseline = DominantBaseline::Auto;
    TextAnchor m_text_anchor = TextAnchor::Start;
    WhiteSpace m_white_space = WhiteSpace::Default;
    WritingMode m_writing_mode = WritingMode::Horizontal;
    TextOrientation m_text_orientation = TextOrientation::Mixed;
    Direction m_direction = Direction::Ltr;
};

class svg_t_span_element final : public svg_text_positioning_element {
public:
    svg_t_span_element(document* dom);
};

class svg_text_element final : public svg_text_positioning_element {
public:
    svg_text_element(document* dom);

    Rect fillBoundingBox() const final { return bounding_box(false); }
    Rect strokeBoundingBox() const final { return bounding_box(true); }

    void layout(svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    Rect bounding_box(bool includeStroke) const;
    svg_text_fragment_list m_fragments;
    std::u32string m_text;
};

} // namespace nova::svg
