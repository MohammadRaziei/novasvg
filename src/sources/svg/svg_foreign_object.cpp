#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

#include <stb/image.hpp>

namespace nova::svg {
using namespace vg;

namespace {

/// A painter over one base face. Other faces (bold, italic, another size) are looked up with make_font() in
/// the same family, as svg <text> does, and fall back to the base face if the machine has none.
/// With `variants` false every request gets the base face (the caller wants no bold or italic of its own).
/// `state` may be null: such a painter only measures.
class element_painter final : public foreign_object_painter {
public:
    element_painter(const sized_font& base, std::string family, unsigned char base_style, svg_render_state* state, bool variants = true)
        : m_base(base), m_family(std::move(family)), m_base_style(base_style), m_state(state), m_variants(variants) {}

    void fill_rect(const box& box_, const stb::color& color_) override;
    void fill_text(std::u32string_view text, const foreign_object_font& font_, float x, float y, const stb::color& color_,
                   const text_squeeze& squeeze) override;
    void draw_image(const stb::image& image, const box& dest) override;
    foreign_object_font_metrics metrics(const foreign_object_font& font_) const override;
    float measure_text(std::u32string_view text, const foreign_object_font& font_) const override;

private:
    const sized_font& resolve(const foreign_object_font& spec) const;

    struct cached {
        foreign_object_font spec;
        sized_font font_;
    };

    sized_font m_base;
    std::string m_family;
    unsigned char m_base_style;
    svg_render_state* m_state;
    bool m_variants;
    mutable std::vector<cached> m_cache;
};

// ---- element_painter -------------------------------------------------------------------------------

const sized_font& element_painter::resolve(const foreign_object_font& spec) const
{
    if(!m_variants)
        return m_base;
    const unsigned char style = (unsigned char)((spec.bold ? svg_foreign_object_element::style_bold : 0)
                                              | (spec.italic ? svg_foreign_object_element::style_italic : 0));
    if(style == m_base_style && spec.size == m_base.size() && spec.family == m_family)
        return m_base;
    for(const auto& entry : m_cache) {
        if(entry.spec.family == spec.family && entry.spec.size == spec.size && entry.spec.bold == spec.bold && entry.spec.italic == spec.italic)
            return entry.font_;
    }

    auto variant = make_font(spec.family, spec.size, spec.bold, spec.italic);
    // A variant the machine can't provide at all must not blank the text: use the base face.
    m_cache.push_back({spec, variant.is_null() ? m_base : variant});
    return m_cache.back().font_;
}

foreign_object_font_metrics element_painter::metrics(const foreign_object_font& font_) const
{
    const auto& face = resolve(font_);
    if(face.is_null())
        return {};
    return {face.ascent(), face.height()};
}

float element_painter::measure_text(std::u32string_view text, const foreign_object_font& font_) const
{
    const auto& face = resolve(font_);
    return face.is_null() ? 0.f : face.measure_text(text);
}

void element_painter::fill_rect(const box& box_, const stb::color& color_)
{
    if(m_state == nullptr)
        return;
    (*m_state)->set_color(color_);
    stb::outline path_;
    path_.add_rect(box_.x, box_.y, box_.w, box_.h);
    (*m_state)->fill_path(path_, fill_rule::non_zero, m_state->current_transform());
}

void element_painter::fill_text(std::u32string_view text, const foreign_object_font& font_, float x, float y,
                                const stb::color& color_, const text_squeeze& squeeze)
{
    if(m_state == nullptr)
        return;
    const auto& face = resolve(font_);
    if(face.is_null())
        return;
    (*m_state)->set_color(color_);
    auto transform_ = m_state->current_transform();
    if(squeeze.factor < 1.f) {
        // Condense about squeeze.about_x so the text still lands where it was placed; only x is scaled.
        transform_ = transform_ * transform::translated(squeeze.about_x, 0.f) * transform::scaled(squeeze.factor, 1.f)
                    * transform::translated(-squeeze.about_x, 0.f);
    }

    (*m_state)->fill_text(text, face, point(x, y), transform_);
}

void element_painter::draw_image(const stb::image& image, const box& dest)
{
    if(m_state == nullptr || image.empty() || image.channels() != 4)
        return;
    pixmap pixels(image.width(), image.height());
    stb::rgba_to_premultiplied_argb(pixels.data(), size_t(pixels.stride()), image.data(), size_t(image.stride()),
                                    image.width(), image.height());
    const rect source(0.f, 0.f, float(image.width()), float(image.height()));
    (*m_state)->draw_image(pixels, rect(dest.x, dest.y, dest.w, dest.h), source, m_state->current_transform());
}

} // namespace

svg_foreign_object_element::svg_foreign_object_element(document* dom)
    : svg_graphics_element(dom, element_id::foreign_object)
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow, 0.f, length_units::none)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::none)
    , m_width(property_id::width, length_direction::horizontal, length_negative_mode::forbid, 0.f, length_units::none)
    , m_height(property_id::height, length_direction::vertical, length_negative_mode::forbid, 0.f, length_units::none)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

rect svg_foreign_object_element::fill_bounding_box() const
{
    length_context length_context_(this);
    return rect(
        length_context_.value_for_length(m_x),
        length_context_.value_for_length(m_y),
        length_context_.value_for_length(m_width),
        length_context_.value_for_length(m_height)
    );
}

rect svg_foreign_object_element::stroke_bounding_box() const
{
    return fill_bounding_box();
}

void svg_foreign_object_element::layout_element(const svg_layout_state& state)
{
    m_font = state.font();
    m_font_family = state.font_family();
    m_base_font_style = (unsigned char)((state.font_weight() == css_font_weight::bold ? style_bold : 0)
                                      | (state.font_style() == css_font_style::italic ? style_italic : 0));
    m_fill = get_paint_server(state.fill(), state.fill_opacity());
    svg_graphics_element::layout_element(state);
}

void svg_foreign_object_element::render(svg_render_state& state) const
{
    if(m_raw_content.empty() || is_display_none() || is_visibility_hidden())
        return;
    auto renderer = dom()->foreign_object_renderer_ptr();
    if(!renderer)
        return;

    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);

    const auto box_ = fill_bounding_box();
    foreign_object_context context;
    context.markup = m_raw_content;
    context.box = box(box_);
    context.font.family = m_font_family;
    context.font.size = m_font.size();
    context.font.bold = (m_base_font_style & style_bold) != 0;
    context.font.italic = (m_base_font_style & style_italic) != 0;
    const auto transform_ = new_state.current_transform();
    context.device_scale = STB_MAX(transform_.x_scale(), transform_.y_scale());
    context.prepared = m_prepared.get();

    element_painter painter(m_font, m_font_family, m_base_font_style, &new_state);
    renderer->render(context, painter);
    new_state.end_group(blend_info);
}

std::optional<foreign_object_size> measure_foreign_object(const foreign_object_renderer& renderer, std::string_view markup, const sized_font& base,
                                                          const std::string& family, bool bold, bool italic, float max_width)
{
    // `base` is the face of the base style (bold / italic say which). With a family the other variants are looked up in
    // it exactly as when painting; without one every run is measured with `base` alone.
    const unsigned char base_style = (unsigned char)((bold ? svg_foreign_object_element::style_bold : 0)
                                                     | (italic ? svg_foreign_object_element::style_italic : 0));
    element_painter painter(base, family, base_style, nullptr, !family.empty());
    foreign_object_context context;
    context.markup = markup;
    context.font.family = family;
    context.font.size = base.size();
    context.font.bold = bold;
    context.font.italic = italic;
    return renderer.measure(context, painter, max_width);
}

} // namespace nova::svg
