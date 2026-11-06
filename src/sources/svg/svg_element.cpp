#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_text_node::svg_text_node(document* dom)
    : svg_node(dom)
{
}

void svg_text_node::set_data(const std::string& data)
{
    root_element()->set_needs_layout();
    m_data.assign(data);
}

std::unique_ptr<svg_node> svg_text_node::clone(bool deep) const
{
    auto nd = std::make_unique<svg_text_node>(dom());
    nd->set_data(m_data);
    return nd;
}

std::unique_ptr<svg_element> svg_element::create(document* dom, element_id id)
{
    switch(id) {
    case element_id::Svg:
        return std::make_unique<svg_svg_element>(dom);
    case element_id::Path:
        return std::make_unique<svg_path_element>(dom);
    case element_id::G:
        return std::make_unique<svg_g_element>(dom);
    case element_id::Switch:
        return std::make_unique<svg_switch_element>(dom);
    case element_id::Rect:
        return std::make_unique<svg_rect_element>(dom);
    case element_id::Circle:
        return std::make_unique<svg_circle_element>(dom);
    case element_id::ellipse:
        return std::make_unique<svg_ellipse_element>(dom);
    case element_id::Line:
        return std::make_unique<svg_line_element>(dom);
    case element_id::Defs:
        return std::make_unique<svg_defs_element>(dom);
    case element_id::polygon:
    case element_id::polyline:
        return std::make_unique<svg_poly_element>(dom, id);
    case element_id::Stop:
        return std::make_unique<svg_stop_element>(dom);
    case element_id::linear_gradient:
        return std::make_unique<svg_linear_gradient_element>(dom);
    case element_id::radial_gradient:
        return std::make_unique<svg_radial_gradient_element>(dom);
    case element_id::symbol:
        return std::make_unique<svg_symbol_element>(dom);
    case element_id::use:
        return std::make_unique<svg_use_element>(dom);
    case element_id::Pattern:
        return std::make_unique<svg_pattern_element>(dom);
    case element_id::Mask:
        return std::make_unique<svg_mask_element>(dom);
    case element_id::ClipPath:
        return std::make_unique<svg_clip_path_element>(dom);
    case element_id::Filter:
        return std::make_unique<svg_filter_element>(dom);
    case element_id::fe_gaussian_blur:
        return std::make_unique<svg_fe_gaussian_blur_element>(dom);
    case element_id::fe_drop_shadow:
        return std::make_unique<svg_fe_drop_shadow_element>(dom);
    case element_id::fe_offset:
        return std::make_unique<svg_fe_offset_element>(dom);
    case element_id::fe_flood:
        return std::make_unique<svg_fe_flood_element>(dom);
    case element_id::fe_composite:
        return std::make_unique<svg_fe_composite_element>(dom);
    case element_id::fe_merge:
        return std::make_unique<svg_fe_merge_element>(dom);
    case element_id::fe_merge_node:
        return std::make_unique<svg_element>(dom, id);
    case element_id::marker:
        return std::make_unique<svg_marker_element>(dom);
    case element_id::Image:
        return std::make_unique<svg_image_element>(dom);
    case element_id::ForeignObject:
        return std::make_unique<svg_foreign_object_element>(dom);
    case element_id::Style:
        return std::make_unique<svg_style_element>(dom);
    case element_id::Text:
        return std::make_unique<svg_text_element>(dom);
    case element_id::tspan:
        return std::make_unique<svg_t_span_element>(dom);
    default:
        assert(false);
    }

    return nullptr;
}

svg_element::svg_element(document* dom, element_id id)
    : svg_node(dom)
    , m_id(id)
{
}

bool svg_element::has_attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == property_id::unknown)
        return false;
    return has_attribute(id);
}

const std::string& svg_element::attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == property_id::unknown)
        return empty_string;
    return attribute(id);
}

const std::string* svg_element::find_custom_attribute(const std::string& name) const
{
    auto it = m_custom_attributes.find(name);
    return it == m_custom_attributes.end() ? nullptr : &it->second;
}

void svg_element::set_custom_attribute(const std::string& name, const std::string& value)
{
    m_custom_attributes[name] = value;
}

bool svg_element::set_attribute(std::string_view name, const std::string& value)
{
    auto id = propertyid(name);
    if(id == property_id::unknown)
        return false;
    return set_attribute(Specificity::api, id, value);
}

const Attribute* svg_element::find_attribute(property_id id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return &attribute;
        }
    }

    return nullptr;
}

bool svg_element::has_attribute(property_id id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return true;
        }
    }

    return false;
}

const std::string& svg_element::attribute(property_id id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return attribute.value();
        }
    }

    return empty_string;
}

bool svg_element::set_attribute(int specificity, property_id id, const std::string& value)
{
    // A value with var() cannot be parsed before the custom properties it names are all known (a stylesheet
    // can come after the el): it is kept, and parsed by resolveVariables() when the dom is complete.
    const bool deferred = uses_variables(id, value);
    for(auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            if(specificity < attribute.specificity())
                return false;
            if(deferred)
                m_variable_sources[id] = value;
            else {
                m_variable_sources.erase(id);
                parse_attribute(id, value);
            }
            attribute = Attribute(specificity, id, value);
            return true;
        }
    }

    if(deferred)
        m_variable_sources[id] = value;
    else
        parse_attribute(id, value);
    m_attributes.emplace_front(specificity, id, value);
    return true;
}

void svg_element::set_custom_property(int specificity, const std::string& name, const std::string& value)
{
    auto it = m_custom_properties.find(name);
    if(it != m_custom_properties.end() && specificity < it->second.first)
        return;
    std::string_view trimmed(value);
    strip_leading_and_trailing_spaces(trimmed);
    m_custom_properties[name] = {specificity, std::string(trimmed)};
}

const svg_element* svg_element::find_custom_property_owner(const std::string& name) const
{
    for(auto el = this; el != nullptr; el = el->parent_element()) {
        if(el->m_custom_properties.count(name))
            return el;
    }
    return nullptr;
}

const std::string& svg_element::custom_property(const std::string& name) const
{
    if(auto owner = find_custom_property_owner(name))
        return owner->m_custom_properties.at(name).second;
    return empty_string;
}

void svg_element::resolve_variables()
{
    for(const auto& [id, source] : m_variable_sources) {
        std::string resolved;
        if(!substitute_variables(source, this, resolved, 0))
            continue;
        for(auto& attribute : m_attributes) {
            if(attribute.id() == id) {
                attribute = Attribute(attribute.specificity(), id, resolved);
                break;
            }
        }
        parse_attribute(id, resolved);
    }
}

void svg_element::set_attributes(const attribute_list& attributes)
{
    for(const auto& attribute : attributes) {
        set_attribute(attribute);
    }
}

bool svg_element::set_attribute(const Attribute& attribute)
{
    return set_attribute(attribute.specificity(), attribute.id(), attribute.value());
}

void svg_element::parse_attribute(property_id id, const std::string& value)
{
    root_element()->set_needs_layout();
    if(auto property = get_property(id)) {
        property->parse(value);
    }
}

svg_element* svg_element::previous_element() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.begin();
    auto end = children.end();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

svg_element* svg_element::next_element() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.rbegin();
    auto end = children.rend();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

svg_node* svg_element::add_child(std::unique_ptr<svg_node> child)
{
    child->set_parent_element(this);
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

svg_node* svg_element::firstChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.front();
}

svg_node* svg_element::lastChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.back();
}

Rect svg_element::fill_bounding_box() const
{
    auto fill_bounding_box = Rect::invalid;
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child); el && !el->is_hidden_element()) {
            fill_bounding_box.unite(el->local_transform().map_rect(el->fill_bounding_box()));
        }
    }

    if(!fill_bounding_box.is_valid())
        fill_bounding_box = Rect::Empty;
    return fill_bounding_box;
}

Rect svg_element::stroke_bounding_box() const
{
    auto stroke_bounding_box = Rect::invalid;
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child); el && !el->is_hidden_element()) {
            stroke_bounding_box.unite(el->local_transform().map_rect(el->stroke_bounding_box()));
        }
    }

    if(!stroke_bounding_box.is_valid())
        stroke_bounding_box = Rect::Empty;
    return stroke_bounding_box;
}

Rect svg_element::paint_bounding_box() const
{
    if(m_paint_bounding_box.is_valid())
        return m_paint_bounding_box;
    m_paint_bounding_box = Rect::Empty;
    m_paint_bounding_box = stroke_bounding_box();
    assert(m_paint_bounding_box.is_valid());
    if(m_clipper) m_paint_bounding_box.intersect(m_clipper->clip_bounding_box(this));
    if(m_masker) m_paint_bounding_box.intersect(m_masker->mask_bounding_box(this));
    return m_paint_bounding_box;
}

svg_marker_element* svg_element::get_marker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::marker)
        return static_cast<svg_marker_element*>(el);
    return nullptr;
}

svg_clip_path_element* svg_element::get_clipper(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::ClipPath)
        return static_cast<svg_clip_path_element*>(el);
    return nullptr;
}

svg_mask_element* svg_element::get_masker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::Mask)
        return static_cast<svg_mask_element*>(el);
    return nullptr;
}

svg_filter_element* svg_element::get_filter_element(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::Filter)
        return static_cast<svg_filter_element*>(el);
    return nullptr;
}

svg_paint_element* svg_element::get_painter(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->is_paint_element())
        return static_cast<svg_paint_element*>(el);
    return nullptr;
}

svg_element* svg_element::element_from_point(float x, float y)
{
    auto it = m_children.rbegin();
    auto end = m_children.rend();
    for(; it != end; ++it) {
        auto child = to_svg_element(*it);
        if(child && !child->is_hidden_element()) {
            if(auto el = child->element_from_point(x, y)) {
                return el;
            }
        }
    }

    if(is_pointable_element()) {
        auto transform = local_transform();
        for(auto parent = parent_element(); parent; parent = parent->parent_element())
            transform.post_multiply(parent->local_transform());
        auto bbox = transform.map_rect(paint_bounding_box());
        if(bbox.contains(x, y)) {
            return this;
        }
    }

    return nullptr;
}

void svg_element::add_property(svg_property& value)
{
    m_properties.push_front(&value);
}

svg_property* svg_element::get_property(property_id id) const
{
    for(auto property : m_properties) {
        if(id == property->id()) {
            return property;
        }
    }

    return nullptr;
}

Size svg_element::current_viewport_size() const
{
    auto parent = parent_element();
    if(parent == nullptr) {
        auto el = static_cast<const svg_svg_element*>(this);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().is_valid())
            return viewBox.value().size();
        return Size(300, 150);
    }

    if(parent->id() == element_id::Svg) {
        auto el = static_cast<const svg_svg_element*>(parent);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().is_valid())
            return viewBox.value().size();
        LengthContext lengthContext(el);
        auto width = lengthContext.value_for_length(el->width());
        auto height = lengthContext.value_for_length(el->height());
        return Size(width, height);
    }

    return parent->current_viewport_size();
}

void svg_element::clone_children(svg_element* parent_element) const
{
    for(const auto& child : m_children) {
        parent_element->add_child(child->clone(true));
    }
}

std::unique_ptr<svg_node> svg_element::clone(bool deep) const
{
    auto el = svg_element::create(dom(), m_id);
    el->set_attributes(m_attributes);
    el->m_custom_attributes = m_custom_attributes;
    if(deep) { clone_children(el.get()); }
    return el;
}

void svg_element::build()
{
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child)) {
            el->build();
        }
    }
}

void svg_element::layout_element(const svg_layout_state& state)
{
    m_paint_bounding_box = Rect::invalid;
    m_clipper = get_clipper(state.clip_path());
    m_masker = get_masker(state.mask());
    m_filter = get_filter_element(state.filter());
    m_opacity = state.opacity();
    m_mix_blend_mode = state.mix_blend_mode();

    m_font_size = state.font_size();
    m_display = state.display();
    m_overflow = state.overflow();
    m_visibility = state.visibility();
    m_pointer_events = state.pointer_events();
}

void svg_element::layout_children(svg_layout_state& state)
{
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child)) {
            el->layout(state);
        }
    }
}

void svg_element::layout(svg_layout_state& state)
{
    svg_layout_state newState(state, this);
    layout_element(newState);
    layout_children(newState);
}

void svg_element::render_children(svg_render_state& state) const
{
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child)) {
            el->render(state);
        }
    }
}

void svg_element::render(svg_render_state& state) const
{
}

bool svg_element::is_hidden_element() const
{
    if(is_display_none())
        return true;
    switch(m_id) {
    case element_id::Defs:
    case element_id::symbol:
    case element_id::marker:
    case element_id::ClipPath:
    case element_id::Mask:
    case element_id::Filter:
    case element_id::fe_gaussian_blur:
    case element_id::fe_drop_shadow:
    case element_id::fe_offset:
    case element_id::fe_flood:
    case element_id::fe_composite:
    case element_id::fe_merge:
    case element_id::fe_merge_node:
    case element_id::linear_gradient:
    case element_id::radial_gradient:
    case element_id::Pattern:
    case element_id::Stop:
        return true;
    default:
        return false;
    }
}

bool svg_element::is_pointable_element() const
{
    if(m_pointer_events != PointerEvents::None
        && m_visibility != Visibility::hidden
        && m_display != Display::None
        && m_opacity != 0.f) {
        switch(m_id) {
        case element_id::Line:
        case element_id::Rect:
        case element_id::ellipse:
        case element_id::Circle:
        case element_id::polyline:
        case element_id::polygon:
        case element_id::Path:
        case element_id::Text:
        case element_id::Image:
        case element_id::ForeignObject:
            return true;
        default:
            break;
        }
    }

    return false;
}

svg_style_element::svg_style_element(document* dom)
    : svg_element(dom, element_id::Style)
{
}

svg_fit_to_view_box::svg_fit_to_view_box(svg_element* el)
    : m_view_box(property_id::ViewBox)
    , m_preserve_aspect_ratio(property_id::PreserveAspectRatio)
{
    el->add_property(m_view_box);
    el->add_property(m_preserve_aspect_ratio);
}

Transform svg_fit_to_view_box::view_box_to_view_transform(const Size& viewport_size) const
{
    const auto& view_box_rect = m_view_box.value();
    if(view_box_rect.empty() || viewport_size.empty())
        return Transform::identity;
    return m_preserve_aspect_ratio.get_transform(view_box_rect, viewport_size);
}

Rect svg_fit_to_view_box::get_clip_rect(const Size& viewport_size) const
{
    const auto& view_box_rect = m_view_box.value();
    if(view_box_rect.empty() || viewport_size.empty())
        return Rect(0, 0, viewport_size.w, viewport_size.h);
    return m_preserve_aspect_ratio.get_clip_rect(view_box_rect, viewport_size);
}

svg_uri_reference::svg_uri_reference(svg_element* el)
    : m_href(property_id::Href)
{
    el->add_property(m_href);
}

svg_element* svg_uri_reference::get_target_element(const document* dom) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return dom->root_element()->element_by_id(value.substr(1));
}

bool svg_paint_server::apply_paint(svg_render_state& state) const
{
    if(!is_renderable())
        return false;
    if(m_element) return m_element->apply_paint(state, m_opacity);
    state->set_color(with_alpha(m_color, m_opacity));
    return true;
}

svg_graphics_element::svg_graphics_element(document* dom, element_id id)
    : svg_element(dom, id)
    , m_transform(property_id::Transform)
    , m_transform_origin(property_id::transform_origin)
    , m_transform_box(property_id::transform_box)
{
    add_property(m_transform);
    add_property(m_transform_origin);
    add_property(m_transform_box);
}

Transform svg_graphics_element::local_transform() const
{
    // Only shift the pivot when transform-origin was actually specified --
    // matches legacy behavior (origin at the current user-space (0,0)) for
    // every el that doesn't use it, i.e. everything except elements
    // that explicitly opt in the way feature-css-use-symbol.svg's `.rot`
    // class does.
    if(!m_transform_origin.specified())
        return m_transform.value();

    Rect box;
    if(m_transform_box.specified() && m_transform_box.value() == transform_box_type::fill_box) {
        box = fill_bounding_box();
    } else {
        auto size = current_viewport_size();
        box = Rect(0.f, 0.f, size.w, size.h);
    }

    if(box.empty())
        return m_transform.value();

    auto origin = m_transform_origin.resolve(box);
    return Transform::translated(origin.x, origin.y) * m_transform.value() * Transform::translated(-origin.x, -origin.y);
}

svg_paint_server svg_graphics_element::get_paint_server(const Paint& paint, float opacity) const
{
    if(paint.isNone())
        return svg_paint_server();
    if(auto el = get_painter(paint.id()))
        return svg_paint_server(el, paint.color(), opacity);
    return svg_paint_server(nullptr, paint.color(), opacity);
}

StrokeData svg_graphics_element::get_stroke_data(const svg_layout_state& state) const
{
    LengthContext lengthContext(this);
    StrokeData strokeData(lengthContext.value_for_length(state.stroke_width(), length_direction::diagonal));
    strokeData.set_miter_limit(state.stroke_miterlimit());
    strokeData.set_line_cap(state.stroke_linecap());
    strokeData.set_line_join(state.stroke_linejoin());
    strokeData.set_dash_offset(lengthContext.value_for_length(state.stroke_dashoffset(), length_direction::diagonal));

    DashArray dashArray;
    for(const auto& dash : state.stroke_dasharray())
        dashArray.push_back(lengthContext.value_for_length(dash, length_direction::diagonal));
    strokeData.set_dash_array(std::move(dashArray));
    return strokeData;
}

svg_svg_element::svg_svg_element(document* dom)
    : svg_graphics_element(dom, element_id::Svg)
    , svg_fit_to_view_box(this)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::None)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::None)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid, 100.f, length_units::percent)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

Transform svg_svg_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Rect viewport_rect = {
        lengthContext.value_for_length(m_x),
        lengthContext.value_for_length(m_y),
        lengthContext.value_for_length(m_width),
        lengthContext.value_for_length(m_height)
    };

    if(is_root_element())
        return view_box_to_view_transform(viewport_rect.size());
    return svg_graphics_element::local_transform() * Transform::translated(viewport_rect.x, viewport_rect.y) * view_box_to_view_transform(viewport_rect.size());
}

void svg_svg_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    LengthContext lengthContext(this);
    const Size viewport_size = {
        lengthContext.value_for_length(m_width),
        lengthContext.value_for_length(m_height)
    };

    if(viewport_size.empty())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    if(is_overflow_hidden())
        newState->clip_rect(get_clip_rect(viewport_size), FillRule::NonZero, newState.current_transform());
    render_children(newState);
    newState.endGroup(blend_info);
}

svg_root_element::svg_root_element(document* dom)
    : svg_svg_element(dom)
{
}

svg_root_element* svg_root_element::layout_if_needed()
{
    if(needs_layout())
        force_layout();
    return this;
}

svg_element* svg_root_element::element_by_id(std::string_view id) const
{
    auto it = m_id_cache.find(id);
    if(it == m_id_cache.end())
        return nullptr;
    return it->second;
}

void svg_root_element::add_element_by_id(const std::string& id, svg_element* el)
{
    m_id_cache.emplace(id, el);
}

void svg_root_element::layout(svg_layout_state& state)
{
    svg_svg_element::layout(state);

    LengthContext lengthContext(this);
    if(!width().is_percent()) {
        m_intrinsic_width = lengthContext.value_for_length(width());
    } else {
        m_intrinsic_width = 0.f;
    }

    if(!height().is_percent()) {
        m_intrinsic_height = lengthContext.value_for_length(height());
    } else {
        m_intrinsic_height = 0.f;
    }

    const auto& view_box_rect = viewBox().value();
    if(!view_box_rect.empty() && (!m_intrinsic_width || !m_intrinsic_height)) {
        auto intrinsic_ratio = view_box_rect.w / view_box_rect.h;
        if(!m_intrinsic_width && m_intrinsic_height)
            m_intrinsic_width = m_intrinsic_height * intrinsic_ratio;
        else if(m_intrinsic_width && !m_intrinsic_height) {
            m_intrinsic_height = m_intrinsic_width / intrinsic_ratio;
        }
    }

    if(view_box_rect.is_valid() && (!m_intrinsic_width || !m_intrinsic_height)) {
        m_intrinsic_width = view_box_rect.w;
        m_intrinsic_height = view_box_rect.h;
    }

    if(!m_intrinsic_width || !m_intrinsic_height) {
        auto bounding_box = paint_bounding_box();
        if(!m_intrinsic_width)
            m_intrinsic_width = bounding_box.right();
        if(!m_intrinsic_height) {
            m_intrinsic_height = bounding_box.bottom();
        }
    }
}

void svg_root_element::force_layout()
{
    svg_layout_state state;
    layout(state);
}

svg_use_element::svg_use_element(document* dom)
    : svg_graphics_element(dom, element_id::use)
    , svg_uri_reference(this)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow, 0.f, length_units::None)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::None)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid, 100.f, length_units::percent)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

Transform svg_use_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Point translation = {
        lengthContext.value_for_length(m_x),
        lengthContext.value_for_length(m_y)
    };

    return svg_graphics_element::local_transform() * Transform::translated(translation.x, translation.y);
}

void svg_use_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    render_children(newState);
    newState.endGroup(blend_info);
}

void svg_use_element::build()
{
    if(auto target_element = get_target_element(dom())) {
        if(auto new_element = clone_target_element(target_element)) {
            add_child(std::move(new_element));
        }
    }

    svg_graphics_element::build();
}

std::unique_ptr<svg_element> svg_use_element::clone_target_element(svg_element* target_element)
{
    if(target_element == this || is_disallowed_element(target_element))
        return nullptr;
    const auto& id_attr = target_element->attribute(property_id::Id);
    auto parent = parent_element();
    while(parent) {
        auto attribute = parent->find_attribute(property_id::Id);
        if(attribute && id_attr == attribute->value())
            return nullptr;
        parent = parent->parent_element();
    }

    auto tag_id = target_element->id();
    if(tag_id == element_id::symbol) {
        tag_id = element_id::Svg;
    }

    auto new_element = svg_element::create(dom(), tag_id);
    new_element->set_attributes(target_element->attributes());
    if(new_element->id() == element_id::Svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == property_id::Width || attribute.id() == property_id::Height) {
                new_element->set_attribute(attribute);
            }
        }
    }

    if(new_element->id() != element_id::use)
        target_element->clone_children(new_element.get());
    return new_element;
}

svg_symbol_element::svg_symbol_element(document* dom)
    : svg_graphics_element(dom, element_id::symbol)
    , svg_fit_to_view_box(this)
{
}

svg_g_element::svg_g_element(document* dom)
    : svg_graphics_element(dom, element_id::G)
{
}

void svg_g_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    render_children(newState);
    newState.endGroup(blend_info);
}

svg_switch_element::svg_switch_element(document* dom)
    : svg_graphics_element(dom, element_id::Switch)
{
}

const svg_element* svg_switch_element::selected_child() const
{
    for(const auto& child : children()) {
        auto el = to_svg_element(child);
        // only graphics elements take part; <title>, <desc>, <defs>... are skipped
        if(!el || is_disallowed_element(el))
            continue;
        if(switch_child_passes(el))
            return el;
    }
    return nullptr;
}

void svg_switch_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    if(auto child = selected_child())
        child->render(newState);
    newState.endGroup(blend_info);
}

svg_defs_element::svg_defs_element(document* dom)
    : svg_graphics_element(dom, element_id::Defs)
{
}

} // namespace nova::svg
