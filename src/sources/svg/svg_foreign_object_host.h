#pragma once

// The core's side of ADR-0010: the foreign_object_painter that draws on a canvas. Used by
// svg_foreign_object_element::render() and by the Python binding (to measure with a given face).

#include "nova/svg/foreign_object.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

/// A painter over one base face. Other faces (bold, italic, another size) are looked up with make_font() in
/// the same family, as svg <text> does, and fall back to the base face if the machine has none.
/// With `variants` false every request gets the base face (the caller wants no bold or italic of its own).
/// `state` may be null: such a painter only measures.
class element_painter final : public foreign_object_painter {
public:
    element_painter(const Font& base, std::string family, unsigned char base_style, svg_render_state* state, bool variants = true)
        : m_base(base), m_family(std::move(family)), m_base_style(base_style), m_state(state), m_variants(variants) {}

    void fill_rect(const Box& box, const stb::color& color) override;
    void fill_text(std::u32string_view text, const foreign_object_font& font, float x, float y, const stb::color& color,
                   const text_squeeze& squeeze) override;
    void draw_image(const stb::image& image, const Box& dest) override;
    foreign_object_font_metrics metrics(const foreign_object_font& font) const override;
    float measure_text(std::u32string_view text, const foreign_object_font& font) const override;

private:
    const Font& resolve(const foreign_object_font& spec) const;

    struct cached {
        foreign_object_font spec;
        Font font;
    };

    Font m_base;
    std::string m_family;
    unsigned char m_base_style;
    svg_render_state* m_state;
    bool m_variants;
    mutable std::vector<cached> m_cache;
};

} // namespace nova::svg
