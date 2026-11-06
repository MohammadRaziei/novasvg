#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_image_element::svg_image_element(document* dom)
    : svg_graphics_element(dom, element_id::Image)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::None)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::None)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_preserve_aspect_ratio(property_id::PreserveAspectRatio)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
    add_property(m_preserve_aspect_ratio);
}

Rect svg_image_element::fill_bounding_box() const
{
    LengthContext lengthContext(this);
    const Rect viewport_rect = {
        lengthContext.value_for_length(m_x),
        lengthContext.value_for_length(m_y),
        lengthContext.value_for_length(m_width),
        lengthContext.value_for_length(m_height)
    };

    return viewport_rect;
}

Rect svg_image_element::stroke_bounding_box() const
{
    return fill_bounding_box();
}

void svg_image_element::render(svg_render_state& state) const
{
    if(m_image.is_null() || is_display_none() || is_visibility_hidden())
        return;
    Rect dstRect(fill_bounding_box());
    Rect srcRect(0, 0, m_image.width(), m_image.height());
    if(dstRect.empty() || srcRect.empty())
        return;
    m_preserve_aspect_ratio.transform_rect(dstRect, srcRect);

    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    newState->draw_image(m_image, dstRect, srcRect, newState.current_transform());
    newState.endGroup(blend_info);
}

pixmap load_image_resource(const std::string& href)
{
    if(href.compare(0, 5, "data:") == 0) {
        std::string_view input(href);
        auto index = input.find(',', 5);
        if(index == std::string_view::npos)
            return pixmap();
        input.remove_prefix(index + 1);
        return surface_load_from_image_base64(input.data(), input.length());
    }

    return surface_load_from_image_file(href.data());
}

void svg_image_element::parse_attribute(property_id id, const std::string& value)
{
    if(id == property_id::Href) {
        m_image = load_image_resource(value);
    } else {
        svg_graphics_element::parse_attribute(id, value);
    }
}

svg_foreign_object_element::svg_foreign_object_element(document* dom)
    : svg_graphics_element(dom, element_id::ForeignObject)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::None)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::None)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid, 0.f, length_units::None)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid, 0.f, length_units::None)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

Rect svg_foreign_object_element::fill_bounding_box() const
{
    LengthContext lengthContext(this);
    return Rect(
        lengthContext.value_for_length(m_x),
        lengthContext.value_for_length(m_y),
        lengthContext.value_for_length(m_width),
        lengthContext.value_for_length(m_height)
    );
}

Rect svg_foreign_object_element::stroke_bounding_box() const
{
    return fill_bounding_box();
}

foreign_object_fonts svg_foreign_object_element::fonts_for(unsigned used_styles) const
{
    foreign_object_fonts fonts;
    for(unsigned style = 0; style < 4; ++style) {
        if(!(used_styles & (1u << style)))
            continue;
        if(style == m_base_font_style) {
            fonts.face[style] = m_font;
            continue;
        }
        auto variant = make_font(m_fontFamily, m_font.size(), (style & foreign_object_fonts::Bold) != 0, (style & foreign_object_fonts::Italic) != 0);
        // A variant the machine can't provide at all must not blank the text: use the el's own face.
        fonts.face[style] = variant.is_null() ? m_font : variant;
    }
    return fonts;
}

void svg_foreign_object_element::layout_element(const svg_layout_state& state)
{
    m_font = state.font();
    m_fontFamily = state.font_family();
    m_base_font_style = (unsigned char)((state.font_weight() == FontWeight::Bold ? foreign_object_fonts::Bold : 0)
                                      | (state.font_style() == FontStyle::Italic ? foreign_object_fonts::Italic : 0));
    m_fill = get_paint_server(state.fill(), state.fill_opacity());
    svg_graphics_element::layout_element(state);
}

void svg_foreign_object_element::render(svg_render_state& state) const
{
    if(m_raw_content.empty() || is_display_none() || is_visibility_hidden())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    auto renderer = m_renderer ? m_renderer : foreign_object_simple::instance();
    renderer->render(this, newState);
    newState.endGroup(blend_info);
}

} // namespace nova::svg
