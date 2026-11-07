#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_marker_element::svg_marker_element(document* dom)
    : svg_element(dom, element_id::marker)
    , svg_fit_to_view_box(this)
    , m_ref_x(property_id::RefX, length_direction::horizontal, length_negative_mode::allow, 0.f, length_units::none)
    , m_ref_y(property_id::RefY, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::none)
    , m_marker_width(property_id::MarkerWidth, length_direction::horizontal, length_negative_mode::forbid, 3.f, length_units::none)
    , m_marker_height(property_id::MarkerHeight, length_direction::vertical, length_negative_mode::forbid, 3.f, length_units::none)
    , m_marker_units(property_id::MarkerUnits, MarkerUnits::stroke_width)
    , m_orient(property_id::orient)
{
    add_property(m_ref_x);
    add_property(m_ref_y);
    add_property(m_marker_width);
    add_property(m_marker_height);
    add_property(m_marker_units);
    add_property(m_orient);
}

point svg_marker_element::ref_point() const
{
    LengthContext lengthContext(this);
    const point ref_point = {
        lengthContext.value_for_length(m_ref_x),
        lengthContext.value_for_length(m_ref_y)
    };

    return ref_point;
}

size svg_marker_element::marker_size() const
{
    LengthContext lengthContext(this);
    const size marker_size = {
        lengthContext.value_for_length(m_marker_width),
        lengthContext.value_for_length(m_marker_height)
    };

    return marker_size;
}

transform svg_marker_element::marker_transform(const point& origin, float angle, float stroke_width) const
{
    auto transform_ = transform::translated(origin.x, origin.y);
    if(m_orient.orientType() == svg_angle::OrientType::angle) {
        transform_.rotate(m_orient.value());
    } else {
        transform_.rotate(angle);
    }

    auto view_transform = view_box_to_view_transform(marker_size());
    auto ref_origin = view_transform.map_point(ref_point());
    if(m_marker_units.value() == MarkerUnits::stroke_width)
        transform_.scale(stroke_width, stroke_width);
    transform_.translate(-ref_origin.x, -ref_origin.y);
    return transform_ * view_transform;
}

rect svg_marker_element::marker_bounding_box(const point& origin, float angle, float stroke_width) const
{
    return marker_transform(origin, angle, stroke_width).map_rect(paint_bounding_box());
}

void svg_marker_element::render_marker(svg_render_state& state, const point& origin, float angle, float stroke_width) const
{
    if(state.has_cycle_reference(this))
        return;
    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, marker_transform(origin, angle, stroke_width));
    new_state.begin_group(blend_info);
    if(is_overflow_hidden())
        new_state->clip_rect(get_clip_rect(marker_size()), fill_rule::non_zero, new_state.current_transform());
    render_children(new_state);
    new_state.end_group(blend_info);
}

transform svg_marker_element::local_transform() const
{
    return view_box_to_view_transform(marker_size());
}

svg_clip_path_element::svg_clip_path_element(document* dom)
    : svg_graphics_element(dom, element_id::clip_path)
    , m_clip_path_units(property_id::ClipPathUnits, units::user_space_on_use)
{
    add_property(m_clip_path_units);
}

rect svg_clip_path_element::clip_bounding_box(const svg_element* el) const
{
    auto clip_bounding_box = paint_bounding_box();
    if(m_clip_path_units.value() == units::object_bounding_box) {
        auto bbox = el->fill_bounding_box();
        clip_bounding_box.x = clip_bounding_box.x * bbox.w + bbox.x;
        clip_bounding_box.y = clip_bounding_box.y * bbox.h + bbox.y;
        clip_bounding_box.w = clip_bounding_box.w * bbox.w;
        clip_bounding_box.h = clip_bounding_box.h * bbox.h;
    }

    return local_transform().map_rect(clip_bounding_box);
}

void svg_clip_path_element::apply_clip_mask(svg_render_state& state) const
{
    if(state.has_cycle_reference(this))
        return;
    auto mask_image = raster_canvas::create(state.current_transform().map_rect(state.paint_bounding_box()));
    auto current_transform = state.current_transform() * local_transform();
    if(m_clip_path_units.value() == units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        current_transform.translate(bbox.x, bbox.y);
        current_transform.scale(bbox.w, bbox.h);
    }

    svg_render_state new_state(this, &state, current_transform, svg_render_mode::clipping, mask_image);
    render_children(new_state);
    if(clipper()) {
        clipper()->apply_clip_mask(new_state);
    }

    state.raster()->blend_canvas(*mask_image, blend_mode::dst_in, 1.f);
}

void svg_clip_path_element::apply_clip_path(svg_render_state& state) const
{
    auto current_transform = state.current_transform() * local_transform();
    if(m_clip_path_units.value() == units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        current_transform.translate(bbox.x, bbox.y);
        current_transform.scale(bbox.w, bbox.h);
    }

    for(const auto& child : children()) {
        auto el = to_svg_element(child);
        if(el == nullptr || el->is_display_none())
            continue;
        transform clip_transform(current_transform);
        auto shape_element = to_svg_geometry_element(el);
        if(shape_element == nullptr) {
            if(el->id() != element_id::use)
                continue;
            clip_transform.multiply(el->local_transform());
            shape_element = to_svg_geometry_element(el->firstChild());
        }

        if(shape_element == nullptr || !shape_element->is_renderable())
            continue;
        state->clip_path(shape_element->path_(), shape_element->clip_rule(), clip_transform * shape_element->local_transform());
        return;
    }

    state->clip_rect(rect::empty_rect, fill_rule::non_zero, transform::identity);
}

bool svg_clip_path_element::requires_masking() const
{
    if(clipper())
        return true;
    const svg_geometry_element* prev_shape_element = nullptr;
    for(const auto& child : children()) {
        auto el = to_svg_element(child);
        if(el == nullptr || el->is_display_none())
            continue;
        auto shape_element = to_svg_geometry_element(el);
        if(shape_element == nullptr) {
            if(el->is_text_positioning_element())
                return true;
            if(el->id() != element_id::use)
                continue;
            if(el->clipper())
                return true;
            shape_element = to_svg_geometry_element(el->firstChild());
        }

        if(shape_element == nullptr || !shape_element->is_renderable())
            continue;
        if(prev_shape_element || shape_element->clipper())
            return true;
        prev_shape_element = shape_element;
    }

    return false;
}

svg_mask_element::svg_mask_element(document* dom)
    : svg_element(dom, element_id::mask)
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow, -10.f, length_units::percent)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, -10.f, length_units::percent)
    , m_width(property_id::width, length_direction::horizontal, length_negative_mode::forbid, 120.f, length_units::percent)
    , m_height(property_id::height, length_direction::vertical, length_negative_mode::forbid, 120.f, length_units::percent)
    , m_mask_units(property_id::MaskUnits, units::object_bounding_box)
    , m_mask_content_units(property_id::MaskContentUnits, units::user_space_on_use)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
    add_property(m_mask_units);
    add_property(m_mask_content_units);
}

rect svg_mask_element::mask_rect(const svg_element* el) const
{
    LengthContext lengthContext(this, m_mask_units.value());
    rect mask_rect = {
        lengthContext.value_for_length(m_x),
        lengthContext.value_for_length(m_y),
        lengthContext.value_for_length(m_width),
        lengthContext.value_for_length(m_height)
    };

    if(m_mask_units.value() == units::object_bounding_box) {
        auto bbox = el->fill_bounding_box();
        mask_rect.x = mask_rect.x * bbox.w + bbox.x;
        mask_rect.y = mask_rect.y * bbox.h + bbox.y;
        mask_rect.w = mask_rect.w * bbox.w;
        mask_rect.h = mask_rect.h * bbox.h;
    }

    return mask_rect;
}

rect svg_mask_element::mask_bounding_box(const svg_element* el) const
{
    auto mask_bounding_box = paint_bounding_box();
    if(m_mask_content_units.value() == units::object_bounding_box) {
        auto bbox = el->fill_bounding_box();
        mask_bounding_box.x = mask_bounding_box.x * bbox.w + bbox.x;
        mask_bounding_box.y = mask_bounding_box.y * bbox.h + bbox.y;
        mask_bounding_box.w = mask_bounding_box.w * bbox.w;
        mask_bounding_box.h = mask_bounding_box.h * bbox.h;
    }

    return mask_bounding_box.intersected(mask_rect(el));
}

void svg_mask_element::apply_mask(svg_render_state& state) const
{
    if(state.has_cycle_reference(this))
        return;
    auto mask_image = raster_canvas::create(state.current_transform().map_rect(state.paint_bounding_box()));
    mask_image->clip_rect(mask_rect(state.el()), fill_rule::non_zero, state.current_transform());

    auto current_transform = state.current_transform();
    if(m_mask_content_units.value() == units::object_bounding_box) {
        auto bbox = state.fill_bounding_box();
        current_transform.translate(bbox.x, bbox.y);
        current_transform.scale(bbox.w, bbox.h);
    }

    svg_render_state new_state(this, &state, current_transform, svg_render_mode::painting, mask_image);
    render_children(new_state);
    if(clipper())
        clipper()->apply_clip_mask(new_state);
    if(masker()) {
        masker()->apply_mask(new_state);
    }

    if(m_mask_type == mask_type::luminance)
        mask_image->convert_to_luminance_mask();
    state.raster()->blend_canvas(*mask_image, blend_mode::dst_in, 1.f);
}

svg_filter_element::svg_filter_element(document* dom)
    : svg_element(dom, element_id::filter)
{
}

float svg_filter_element::pixel_margin() const
{
    float margin = 0.f;
    for(const auto& child : children()) {
        auto el = to_svg_element(child);
        if(!el)
            continue;
        if(el->id() == element_id::fe_gaussian_blur) {
            auto sigma = parse_first_float(el->attribute_(property_id::std_deviation), 0.f);
            margin = STB_MAX(margin, sigma * 3.f);
        } else if(el->id() == element_id::fe_drop_shadow) {
            auto sigma = parse_first_float(el->attribute_(property_id::std_deviation), 2.f);
            auto dx = parse_first_float(el->attribute_(property_id::dx), 2.f);
            auto dy = parse_first_float(el->attribute_(property_id::dy), 2.f);
            margin = STB_MAX(margin, sigma * 3.f + STB_MAX(std::abs(dx), std::abs(dy)));
        } else if(el->id() == element_id::fe_offset) {
            auto dx = parse_first_float(el->attribute_(property_id::dx), 0.f);
            auto dy = parse_first_float(el->attribute_(property_id::dy), 0.f);
            margin = STB_MAX(margin, STB_MAX(std::abs(dx), std::abs(dy)));
        }
        // feFlood/feComposite/feMerge don't themselves need extra margin;
        // if they combine an already-blurred/offset named result, that
        // primitive already grew the margin.
    }

    return margin;
}

void svg_filter_element::apply_filter(svg_render_state& state, float scale) const
{
    svg_filter_context context(state.raster(), scale);
    for(const auto& child : children()) {
        auto el = to_svg_element(child);
        if(!el)
            continue;
        auto output = el->apply_filter_primitive(context);
        if(!output)
            continue; // not a primitive (e.g. a stray feMergeNode outside feMerge)
        context.set_result(el->attribute_(property_id::result), std::move(output));
    }

    state.raster()->copy_pixels_from(*context.last_result());
}

void svg_mask_element::layout_element(const svg_layout_state& state)
{
    m_mask_type = state.mask_type_();
    svg_element::layout_element(state);
}

} // namespace nova::svg
