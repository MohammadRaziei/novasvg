#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_blend_info::svg_blend_info(const svg_element* el)
    : m_clipper(el->clipper())
    , m_masker(el->masker())
    , m_filter(el->filter_element())
    , m_opacity(el->opacity())
    , m_mixBlendMode(el->mixBlendMode())
{
}

bool svg_blend_info::requires_compositing(svg_render_mode mode) const
{
    return (m_clipper && m_clipper->requires_masking()) || (mode == svg_render_mode::painting && (m_masker || m_filter || m_opacity < 1.f || m_mixBlendMode != MixBlendMode::normal));
}

bool svg_render_state::has_cycle_reference(const svg_element* el) const
{
    auto current = this;
    do {
        if(el == current->el())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

void svg_render_state::beginGroup(const svg_blend_info& blend_info)
{
    auto requires_compositing = blend_info.requires_compositing(m_mode);
    pdf_canvas* pdf = dynamic_cast<pdf_canvas*>(m_canvas.get());
    m_layer = layer_kind::none;
    if(requires_compositing && pdf) {
        // On a vector page: opacity and blend mode alone stay vector (a transparency group); what PDF cannot
        // express (masks, clip masks, filters) is drawn into pixels at raster_scale() and placed as an image.
        const bool masking = (blend_info.clipper() && blend_info.clipper()->requires_masking()) || blend_info.masker() || blend_info.filter_element();
        if(m_mode == svg_render_mode::painting && !masking) {
            pdf->begin_group();
            m_layer = layer_kind::pdf_group;
            if(blend_info.clipper())
                blend_info.clipper()->apply_clip_path(*this);
        } else {
            auto bounding_box = m_current_transform.map_rect(m_element->paint_bounding_box());
            if(m_mode == svg_render_mode::painting && blend_info.filter_element()) {
                bounding_box.inflate(blend_info.filter_element()->pixel_margin() * m_current_transform.x_scale());
            }
            bounding_box.intersect(pdf->extents());
            m_layer_scale = pdf->raster_scale();
            m_canvas = raster_canvas::create(Rect(bounding_box.x * m_layer_scale, bounding_box.y * m_layer_scale, bounding_box.w * m_layer_scale, bounding_box.h * m_layer_scale));
            m_current_transform = Transform::scaled(m_layer_scale, m_layer_scale) * m_current_transform;
            m_layer = layer_kind::raster_in_pdf;
        }
        return;
    }

    if(requires_compositing) {
        auto bounding_box = m_current_transform.map_rect(m_element->paint_bounding_box());
        if(m_mode == svg_render_mode::painting && blend_info.filter_element()) {
            bounding_box.inflate(blend_info.filter_element()->pixel_margin() * m_current_transform.x_scale());
        }
        bounding_box.intersect(static_cast<raster_canvas&>(*m_canvas).extents());
        m_canvas = raster_canvas::create(bounding_box);
        m_layer = layer_kind::raster;
    } else {
        m_canvas->save();
    }

    if(!requires_compositing && blend_info.clipper()) {
        blend_info.clipper()->apply_clip_path(*this);
    }
}

void svg_render_state::endGroup(const svg_blend_info& blend_info)
{
    if(m_layer == layer_kind::none) {
        m_canvas->restore();
        return;
    }

    // the finished group is blended into what is already painted under it, with its mix-blend-mode
    BlendMode blendMode = BlendMode::src_over;
    if(m_mode == svg_render_mode::painting) {
        switch(blend_info.mixBlendMode()) {
        case MixBlendMode::Multiply: blendMode = BlendMode::Multiply; break;
        case MixBlendMode::screen: blendMode = BlendMode::screen; break;
        case MixBlendMode::darken: blendMode = BlendMode::darken; break;
        case MixBlendMode::lighten: blendMode = BlendMode::lighten; break;
        case MixBlendMode::difference: blendMode = BlendMode::difference; break;
        case MixBlendMode::exclusion: blendMode = BlendMode::exclusion; break;
        case MixBlendMode::normal: break;
        }
    }

    auto opacity = m_mode == svg_render_mode::Clipping ? 1.f : blend_info.opacity();
    if(m_layer == layer_kind::pdf_group) {
        static_cast<pdf_canvas&>(*m_canvas).end_group(blendMode, opacity);
        m_layer = layer_kind::none;
        return;
    }

    if(blend_info.clipper())
        blend_info.clipper()->apply_clip_mask(*this);
    if(m_mode == svg_render_mode::painting && blend_info.masker()) {
        blend_info.masker()->apply_mask(*this);
    }
    if(m_mode == svg_render_mode::painting && blend_info.filter_element()) {
        blend_info.filter_element()->apply_filter(*this, m_current_transform.x_scale());
    }

    if(m_layer == layer_kind::raster_in_pdf) {
        auto& layer = static_cast<raster_canvas&>(*m_canvas);
        auto& page = static_cast<pdf_canvas&>(*m_parent->m_canvas);
        const Rect e = layer.extents();
        page.draw_raster_group(pixmap(surface_reference(layer.surface())), Rect(e.x / m_layer_scale, e.y / m_layer_scale, e.w / m_layer_scale, e.h / m_layer_scale), blendMode, opacity);
        return;
    }

    static_cast<raster_canvas&>(*m_parent->m_canvas).blend_canvas(static_cast<raster_canvas&>(*m_canvas), blendMode, opacity);
}

svg_paint_element::svg_paint_element(document* dom, element_id id)
    : svg_element(dom, id)
{
}

svg_stop_element::svg_stop_element(document* dom)
    : svg_element(dom, element_id::Stop)
    , m_offset(property_id::Offset, 0.f)
{
    add_property(m_offset);
}

void svg_stop_element::layout_element(const svg_layout_state& state)
{
    m_stop_color = state.stop_color();
    m_stop_opacity = state.stop_opacity();
    svg_element::layout_element(state);
}

GradientStop svg_stop_element::gradientStop(float opacity) const
{
    stb::color stopColor = with_alpha(m_stop_color, m_stop_opacity * opacity);
    GradientStop gradientStop = {
        m_offset.value(), { red_f(stopColor), green_f(stopColor), blue_f(stopColor), alpha_f(stopColor) }
    };

    return gradientStop;
}

svg_gradient_element::svg_gradient_element(document* dom, element_id id)
    : svg_paint_element(dom, id)
    , svg_uri_reference(this)
    , m_gradient_transform(property_id::GradientTransform)
    , m_gradient_units(property_id::GradientUnits, Units::object_bounding_box)
    , m_spread_method(property_id::SpreadMethod, SpreadMethod::pad)
{
    add_property(m_gradient_transform);
    add_property(m_gradient_units);
    add_property(m_spread_method);
}

void svg_gradient_element::collect_gradient_attributes(svg_gradient_attributes& attributes) const
{
    if(!attributes.has_gradient_transform() && has_attribute(property_id::GradientTransform))
        attributes.set_gradient_transform(this);
    if(!attributes.has_spread_method() && has_attribute(property_id::SpreadMethod))
        attributes.set_spread_method(this);
    if(!attributes.has_gradient_units() && has_attribute(property_id::GradientUnits))
        attributes.set_gradient_units(this);
    if(!attributes.has_gradient_content_element()) {
        for(const auto& child : children()) {
            if(auto el = to_svg_element(child); el && el->id() == element_id::Stop) {
                attributes.set_gradient_content_element(this);
                break;
            }
        }
    }
}

svg_linear_gradient_element::svg_linear_gradient_element(document* dom)
    : svg_gradient_element(dom, element_id::linear_gradient)
    , m_x1(property_id::X1, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::percent)
    , m_y1(property_id::Y1, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::percent)
    , m_x2(property_id::X2, length_direction::Horizontal, length_negative_mode::allow, 100.f, length_units::percent)
    , m_y2(property_id::Y2, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::percent)
{
    add_property(m_x1);
    add_property(m_y1);
    add_property(m_x2);
    add_property(m_y2);
}

svg_linear_gradient_attributes svg_linear_gradient_element::collect_gradient_attributes() const
{
    svg_linear_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processed_gradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collect_gradient_attributes(attributes);
        if(current->id() == element_id::linear_gradient) {
            auto el = static_cast<const svg_linear_gradient_element*>(current);
            if(!attributes.has_x1() && el->has_attribute(property_id::X1))
                attributes.set_x1(el);
            if(!attributes.has_y1() && el->has_attribute(property_id::Y1))
                attributes.set_y1(el);
            if(!attributes.has_x2() && el->has_attribute(property_id::X2))
                attributes.set_x2(el);
            if(!attributes.has_y2() && el->has_attribute(property_id::Y2)) {
                attributes.set_y2(el);
            }
        }

        auto target_element = current->get_target_element(dom());
        if(!target_element || !(target_element->id() == element_id::linear_gradient || target_element->id() == element_id::radial_gradient))
            break;
        processed_gradients.insert(current);
        current = static_cast<const svg_gradient_element*>(target_element);
        if(processed_gradients.count(current) > 0) {
            break;
        }
    }

    attributes.set_default_values(this);
    return attributes;
}

GradientStops build_gradient_stops(const svg_gradient_element* el, float opacity)
{
    GradientStops gradientStops;

    const auto& children = el->children();
    gradientStops.reserve(children.size());
    for(const auto& child : children) {
        auto child_element = to_svg_element(child);
        if(child_element && child_element->id() == element_id::Stop) {
            auto stop_element = static_cast<svg_stop_element*>(child_element);
            gradientStops.push_back(stop_element->gradientStop(opacity));
        }
    }

    return gradientStops;
}

bool svg_linear_gradient_element::apply_paint(svg_render_state& state, float opacity) const
{
    auto attributes = collect_gradient_attributes();
    auto gradient_content_element = attributes.gradient_content_element();
    auto gradientStops = build_gradient_stops(gradient_content_element, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto x1 = lengthContext.value_for_length(attributes.x1());
    auto y1 = lengthContext.value_for_length(attributes.y1());
    auto x2 = lengthContext.value_for_length(attributes.x2());
    auto y2 = lengthContext.value_for_length(attributes.y2());
    if(gradientStops.size() == 1 || (x1 == x2 && y1 == y2)) {
        const auto& last_stop = gradientStops.back();
        state->set_color(last_stop.color.r, last_stop.color.g, last_stop.color.b, last_stop.color.a);
        return true;
    }

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_linear_gradient(x1, y1, x2, y2, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_element::svg_radial_gradient_element(document* dom)
    : svg_gradient_element(dom, element_id::radial_gradient)
    , m_cx(property_id::Cx, length_direction::Horizontal, length_negative_mode::allow, 50.f, length_units::percent)
    , m_cy(property_id::Cy, length_direction::vertical, length_negative_mode::allow, 50.f, length_units::percent)
    , m_r(property_id::R, length_direction::diagonal, length_negative_mode::forbid, 50.f, length_units::percent)
    , m_fx(property_id::Fx, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::None)
    , m_fy(property_id::Fy, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::None)
{
    add_property(m_cx);
    add_property(m_cy);
    add_property(m_r);
    add_property(m_fx);
    add_property(m_fy);
}

bool svg_radial_gradient_element::apply_paint(svg_render_state& state, float opacity) const
{
    auto attributes = collect_gradient_attributes();
    auto gradient_content_element = attributes.gradient_content_element();
    auto gradientStops = build_gradient_stops(gradient_content_element, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto r = lengthContext.value_for_length(attributes.r());
    if(r == 0.f || gradientStops.size() == 1) {
        const auto& last_stop = gradientStops.back();
        state->set_color(last_stop.color.r, last_stop.color.g, last_stop.color.b, last_stop.color.a);
        return true;
    }

    auto fx = lengthContext.value_for_length(attributes.fx());
    auto fy = lengthContext.value_for_length(attributes.fy());
    auto cx = lengthContext.value_for_length(attributes.cx());
    auto cy = lengthContext.value_for_length(attributes.cy());

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_radial_gradient(cx, cy, r, fx, fy, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_attributes svg_radial_gradient_element::collect_gradient_attributes() const
{
    svg_radial_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processed_gradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collect_gradient_attributes(attributes);
        if(current->id() == element_id::radial_gradient) {
            auto el = static_cast<const svg_radial_gradient_element*>(current);
            if(!attributes.has_cx() && el->has_attribute(property_id::Cx))
                attributes.set_cx(el);
            if(!attributes.has_cy() && el->has_attribute(property_id::Cy))
                attributes.set_cy(el);
            if(!attributes.has_r() && el->has_attribute(property_id::R))
                attributes.setR(el);
            if(!attributes.has_fx() && el->has_attribute(property_id::Fx))
                attributes.set_fx(el);
            if(!attributes.has_fy() && el->has_attribute(property_id::Fy)) {
                attributes.set_fy(el);
            }
        }

        auto target_element = current->get_target_element(dom());
        if(!target_element || !(target_element->id() == element_id::linear_gradient || target_element->id() == element_id::radial_gradient))
            break;
        processed_gradients.insert(current);
        current = static_cast<const svg_gradient_element*>(target_element);
        if(processed_gradients.count(current) > 0) {
            break;
        }
    }

    attributes.set_default_values(this);
    return attributes;
}

svg_pattern_element::svg_pattern_element(document* dom)
    : svg_paint_element(dom, element_id::Pattern)
    , svg_uri_reference(this)
    , svg_fit_to_view_box(this)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid)
    , m_pattern_transform(property_id::PatternTransform)
    , m_pattern_units(property_id::PatternUnits, Units::object_bounding_box)
    , m_pattern_content_units(property_id::PatternContentUnits, Units::user_space_on_use)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
    add_property(m_pattern_transform);
    add_property(m_pattern_units);
    add_property(m_pattern_content_units);
}

bool svg_pattern_element::apply_paint(svg_render_state& state, float opacity) const
{
    if(state.has_cycle_reference(this))
        return false;
    auto attributes = collect_pattern_attributes();
    auto pattern_content_element = attributes.pattern_content_element();
    if(pattern_content_element == nullptr)
        return false;
    LengthContext lengthContext(this, attributes.patternUnits());
    Rect pattern_rect = {
        lengthContext.value_for_length(attributes.x()),
        lengthContext.value_for_length(attributes.y()),
        lengthContext.value_for_length(attributes.width()),
        lengthContext.value_for_length(attributes.height())
    };

    if(attributes.patternUnits() == Units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        pattern_rect.x = pattern_rect.x * bbox.w + bbox.x;
        pattern_rect.y = pattern_rect.y * bbox.h + bbox.y;
        pattern_rect.w = pattern_rect.w * bbox.w;
        pattern_rect.h = pattern_rect.h * bbox.h;
    }

    auto current_transform = attributes.patternTransform() * state.current_transform();
    // On a vector page the tile is a bitmap too; make it sharp at the page's raster scale.
    auto* pdf = dynamic_cast<pdf_canvas*>(&*state);
    const float tile_scale = pdf ? pdf->raster_scale() : 1.f;
    auto x_scale = current_transform.x_scale() * tile_scale;
    auto y_scale = current_transform.y_scale() * tile_scale;

    auto pattern_image = raster_canvas::create(0, 0, pattern_rect.w * x_scale, pattern_rect.h * y_scale);
    auto pattern_image_transform = Transform::scaled(x_scale, y_scale);

    const auto& view_box_rect = attributes.viewBox();
    if(view_box_rect.is_valid()) {
        const auto& preserveAspectRatio = attributes.preserveAspectRatio();
        pattern_image_transform.multiply(preserveAspectRatio.get_transform(view_box_rect, pattern_rect.size()));
    } else if(attributes.patternContentUnits() == Units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        pattern_image_transform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, pattern_image_transform, svg_render_mode::painting, pattern_image);
    pattern_content_element->render_children(newState);

    auto patternTransform = attributes.patternTransform();
    patternTransform.translate(pattern_rect.x, pattern_rect.y);
    patternTransform.scale(pattern_rect.w / pattern_image->width(), pattern_rect.h / pattern_image->height());
    if(pdf)
        pdf->set_tiled_image(pixmap(surface_reference(pattern_image->surface())), patternTransform, opacity);
    else
        state.raster()->set_texture(*pattern_image, texture_type::Tiled, opacity, patternTransform);
    return true;
}

svg_pattern_attributes svg_pattern_element::collect_pattern_attributes() const
{
    svg_pattern_attributes attributes;
    std::set<const svg_pattern_element*> processed_patterns;
    const svg_pattern_element* current = this;
    while(true) {
        if(!attributes.has_x() && current->has_attribute(property_id::X))
            attributes.set_x(current);
        if(!attributes.has_y() && current->has_attribute(property_id::Y))
            attributes.set_y(current);
        if(!attributes.has_width() && current->has_attribute(property_id::Width))
            attributes.set_width(current);
        if(!attributes.has_height() && current->has_attribute(property_id::Height))
            attributes.set_height(current);
        if(!attributes.has_pattern_transform() && current->has_attribute(property_id::PatternTransform))
            attributes.set_pattern_transform(current);
        if(!attributes.has_pattern_units() && current->has_attribute(property_id::PatternUnits))
            attributes.set_pattern_units(current);
        if(!attributes.has_pattern_content_units() && current->has_attribute(property_id::PatternContentUnits))
            attributes.set_pattern_content_units(current);
        if(!attributes.has_view_box() && current->has_attribute(property_id::ViewBox))
            attributes.set_view_box(current);
        if(!attributes.has_preserve_aspect_ratio() && current->has_attribute(property_id::PreserveAspectRatio))
            attributes.set_preserve_aspect_ratio(current);
        if(!attributes.has_pattern_content_element()) {
            for(const auto& child : current->children()) {
                if(child->is_element()) {
                    attributes.set_pattern_content_element(current);
                    break;
                }
            }
        }

        auto target_element = current->get_target_element(dom());
        if(!target_element || target_element->id() != element_id::Pattern)
            break;
        processed_patterns.insert(current);
        current = static_cast<const svg_pattern_element*>(target_element);
        if(processed_patterns.count(current) > 0) {
            break;
        }
    }

    attributes.set_default_values(this);
    return attributes;
}

} // namespace nova::svg
