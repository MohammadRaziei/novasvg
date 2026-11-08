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
    case element_id::svg:
        return std::make_unique<svg_svg_element>(dom);
    case element_id::path:
        return std::make_unique<svg_path_element>(dom);
    case element_id::G:
        return std::make_unique<svg_g_element>(dom);
    case element_id::switch_:
        return std::make_unique<svg_switch_element>(dom);
    case element_id::rect:
        return std::make_unique<svg_rect_element>(dom);
    case element_id::circle:
        return std::make_unique<svg_circle_element>(dom);
    case element_id::ellipse:
        return std::make_unique<svg_ellipse_element>(dom);
    case element_id::line:
        return std::make_unique<svg_line_element>(dom);
    case element_id::defs:
        return std::make_unique<svg_defs_element>(dom);
    case element_id::polygon:
    case element_id::polyline:
        return std::make_unique<svg_poly_element>(dom, id);
    case element_id::stop:
        return std::make_unique<svg_stop_element>(dom);
    case element_id::linear_gradient:
        return std::make_unique<svg_linear_gradient_element>(dom);
    case element_id::radial_gradient:
        return std::make_unique<svg_radial_gradient_element>(dom);
    case element_id::symbol:
        return std::make_unique<svg_symbol_element>(dom);
    case element_id::use:
        return std::make_unique<svg_use_element>(dom);
    case element_id::pattern:
        return std::make_unique<svg_pattern_element>(dom);
    case element_id::mask:
        return std::make_unique<svg_mask_element>(dom);
    case element_id::clip_path:
        return std::make_unique<svg_clip_path_element>(dom);
    case element_id::filter:
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
    case element_id::image:
        return std::make_unique<svg_image_element>(dom);
    case element_id::foreign_object:
        return std::make_unique<svg_foreign_object_element>(dom);
    case element_id::style:
        return std::make_unique<svg_style_element>(dom);
    case element_id::text:
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

const std::string& svg_element::attribute_(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == property_id::unknown)
        return empty_string;
    return attribute_(id);
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
    return set_attribute(specificity::api, id, value);
}

const attribute* svg_element::find_attribute(property_id id) const
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

const std::string& svg_element::attribute_(property_id id) const
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
    for(auto& attribute_ : m_attributes) {
        if(id == attribute_.id()) {
            if(specificity < attribute_.specificity())
                return false;
            if(deferred)
                m_variable_sources[id] = value;
            else {
                m_variable_sources.erase(id);
                parse_attribute(id, value);
            }
            attribute_ = attribute(specificity, id, value);
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
        for(auto& attribute_ : m_attributes) {
            if(attribute_.id() == id) {
                attribute_ = attribute(attribute_.specificity(), id, resolved);
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

bool svg_element::set_attribute(const attribute& attribute_)
{
    return set_attribute(attribute_.specificity(), attribute_.id(), attribute_.value());
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

svg_node* svg_element::first_child() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.front();
}

svg_node* svg_element::last_child() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.back();
}

rect svg_element::fill_bounding_box() const
{
    auto fill_bounding_box = rect::invalid;
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child); el && !el->is_hidden_element()) {
            fill_bounding_box.unite(el->local_transform().map_rect(el->fill_bounding_box()));
        }
    }

    if(!fill_bounding_box.is_valid())
        fill_bounding_box = rect::empty_rect;
    return fill_bounding_box;
}

rect svg_element::stroke_bounding_box() const
{
    auto stroke_bounding_box = rect::invalid;
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child); el && !el->is_hidden_element()) {
            stroke_bounding_box.unite(el->local_transform().map_rect(el->stroke_bounding_box()));
        }
    }

    if(!stroke_bounding_box.is_valid())
        stroke_bounding_box = rect::empty_rect;
    return stroke_bounding_box;
}

rect svg_element::paint_bounding_box() const
{
    if(m_paint_bounding_box.is_valid())
        return m_paint_bounding_box;
    m_paint_bounding_box = rect::empty_rect;
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
    if(el && el->id() == element_id::clip_path)
        return static_cast<svg_clip_path_element*>(el);
    return nullptr;
}

svg_mask_element* svg_element::get_masker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::mask)
        return static_cast<svg_mask_element*>(el);
    return nullptr;
}

svg_filter_element* svg_element::get_filter_element(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == element_id::filter)
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
        auto transform_ = local_transform();
        for(auto parent = parent_element(); parent; parent = parent->parent_element())
            transform_.post_multiply(parent->local_transform());
        auto bbox = transform_.map_rect(paint_bounding_box());
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

size svg_element::current_viewport_size() const
{
    auto parent = parent_element();
    if(parent == nullptr) {
        auto el = static_cast<const svg_svg_element*>(this);
        const auto& view_box = el->view_box();
        if(view_box.value().is_valid())
            return view_box.value().size_();
        return size(300, 150);
    }

    if(parent->id() == element_id::svg) {
        auto el = static_cast<const svg_svg_element*>(parent);
        const auto& view_box = el->view_box();
        if(view_box.value().is_valid())
            return view_box.value().size_();
        length_context length_context_(el);
        auto width = length_context_.value_for_length(el->width());
        auto height = length_context_.value_for_length(el->height());
        return size(width, height);
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
    m_paint_bounding_box = rect::invalid;
    m_clipper = get_clipper(state.clip_path());
    m_masker = get_masker(state.mask());
    m_filter = get_filter_element(state.filter());
    m_opacity = state.opacity();
    m_mix_blend_mode = state.mix_blend_mode_();

    m_font_size = state.font_size();
    m_display = state.display_();
    m_overflow = state.overflow_();
    m_visibility = state.visibility_();
    m_pointer_events = state.pointer_events_();
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
    svg_layout_state new_state(state, this);
    layout_element(new_state);
    layout_children(new_state);
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
    case element_id::defs:
    case element_id::symbol:
    case element_id::marker:
    case element_id::clip_path:
    case element_id::mask:
    case element_id::filter:
    case element_id::fe_gaussian_blur:
    case element_id::fe_drop_shadow:
    case element_id::fe_offset:
    case element_id::fe_flood:
    case element_id::fe_composite:
    case element_id::fe_merge:
    case element_id::fe_merge_node:
    case element_id::linear_gradient:
    case element_id::radial_gradient:
    case element_id::pattern:
    case element_id::stop:
        return true;
    default:
        return false;
    }
}

bool svg_element::is_pointable_element() const
{
    if(m_pointer_events != pointer_events::none
        && m_visibility != visibility::hidden
        && m_display != display::none
        && m_opacity != 0.f) {
        switch(m_id) {
        case element_id::line:
        case element_id::rect:
        case element_id::ellipse:
        case element_id::circle:
        case element_id::polyline:
        case element_id::polygon:
        case element_id::path:
        case element_id::text:
        case element_id::image:
        case element_id::foreign_object:
            return true;
        default:
            break;
        }
    }

    return false;
}

svg_style_element::svg_style_element(document* dom)
    : svg_element(dom, element_id::style)
{
}

svg_fit_to_view_box::svg_fit_to_view_box(svg_element* el)
    : m_view_box(property_id::view_box)
    , m_preserve_aspect_ratio(property_id::preserve_aspect_ratio)
{
    el->add_property(m_view_box);
    el->add_property(m_preserve_aspect_ratio);
}

transform svg_fit_to_view_box::view_box_to_view_transform(const size& viewport_size) const
{
    const auto& view_box_rect = m_view_box.value();
    if(view_box_rect.empty() || viewport_size.empty())
        return transform::identity;
    return m_preserve_aspect_ratio.get_transform(view_box_rect, viewport_size);
}

rect svg_fit_to_view_box::get_clip_rect(const size& viewport_size) const
{
    const auto& view_box_rect = m_view_box.value();
    if(view_box_rect.empty() || viewport_size.empty())
        return rect(0, 0, viewport_size.w, viewport_size.h);
    return m_preserve_aspect_ratio.get_clip_rect(view_box_rect, viewport_size);
}

svg_uri_reference::svg_uri_reference(svg_element* el)
    : m_href(property_id::href)
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
    , m_transform(property_id::transform)
    , m_transform_origin(property_id::transform_origin)
    , m_transform_box(property_id::transform_box)
{
    add_property(m_transform);
    add_property(m_transform_origin);
    add_property(m_transform_box);
}

transform svg_graphics_element::local_transform() const
{
    // Only shift the pivot when transform-origin was actually specified --
    // matches legacy behavior (origin at the current user-space (0,0)) for
    // every el that doesn't use it, i.e. everything except elements
    // that explicitly opt in the way feature-css-use-symbol.svg's `.rot`
    // class does.
    if(!m_transform_origin.specified())
        return m_transform.value();

    rect box_;
    if(m_transform_box.specified() && m_transform_box.value() == transform_box_type::fill_box) {
        box_ = fill_bounding_box();
    } else {
        auto size_ = current_viewport_size();
        box_ = rect(0.f, 0.f, size_.w, size_.h);
    }

    if(box_.empty())
        return m_transform.value();

    auto origin = m_transform_origin.resolve(box_);
    return transform::translated(origin.x, origin.y) * m_transform.value() * transform::translated(-origin.x, -origin.y);
}

svg_paint_server svg_graphics_element::get_paint_server(const paint& paint_, float opacity) const
{
    if(paint_.is_none())
        return svg_paint_server();
    if(auto el = get_painter(paint_.id()))
        return svg_paint_server(el, paint_.color_(), opacity);
    return svg_paint_server(nullptr, paint_.color_(), opacity);
}

stroke_data svg_graphics_element::get_stroke_data(const svg_layout_state& state) const
{
    length_context length_context_(this);
    stroke_data stroke_data_(length_context_.value_for_length(state.stroke_width(), length_direction::diagonal));
    stroke_data_.set_miter_limit(state.stroke_miterlimit());
    stroke_data_.set_line_cap(state.stroke_linecap());
    stroke_data_.set_line_join(state.stroke_linejoin());
    stroke_data_.set_dash_offset(length_context_.value_for_length(state.stroke_dashoffset(), length_direction::diagonal));

    dash_array dash_array_;
    for(const auto& dash : state.stroke_dasharray())
        dash_array_.push_back(length_context_.value_for_length(dash, length_direction::diagonal));
    stroke_data_.set_dash_array(std::move(dash_array_));
    return stroke_data_;
}

svg_svg_element::svg_svg_element(document* dom)
    : svg_graphics_element(dom, element_id::svg)
    , svg_fit_to_view_box(this)
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow, 0.f, length_units::none)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::none)
    , m_width(property_id::width, length_direction::horizontal, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_height(property_id::height, length_direction::vertical, length_negative_mode::forbid, 100.f, length_units::percent)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

transform svg_svg_element::local_transform() const
{
    length_context length_context_(this);
    const rect viewport_rect = {
        length_context_.value_for_length(m_x),
        length_context_.value_for_length(m_y),
        length_context_.value_for_length(m_width),
        length_context_.value_for_length(m_height)
    };

    if(is_root_element())
        return view_box_to_view_transform(viewport_rect.size_());
    return svg_graphics_element::local_transform() * transform::translated(viewport_rect.x, viewport_rect.y) * view_box_to_view_transform(viewport_rect.size_());
}

void svg_svg_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    length_context length_context_(this);
    const size viewport_size = {
        length_context_.value_for_length(m_width),
        length_context_.value_for_length(m_height)
    };

    if(viewport_size.empty())
        return;
    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    if(is_overflow_hidden())
        new_state->clip_rect(get_clip_rect(viewport_size), fill_rule::non_zero, new_state.current_transform());
    render_children(new_state);
    new_state.end_group(blend_info);
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

    length_context length_context_(this);
    if(!width().is_percent()) {
        m_intrinsic_width = length_context_.value_for_length(width());
    } else {
        m_intrinsic_width = 0.f;
    }

    if(!height().is_percent()) {
        m_intrinsic_height = length_context_.value_for_length(height());
    } else {
        m_intrinsic_height = 0.f;
    }

    const auto& view_box_rect = view_box().value();
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
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow, 0.f, length_units::none)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow, 0.f, length_units::none)
    , m_width(property_id::width, length_direction::horizontal, length_negative_mode::forbid, 100.f, length_units::percent)
    , m_height(property_id::height, length_direction::vertical, length_negative_mode::forbid, 100.f, length_units::percent)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
}

transform svg_use_element::local_transform() const
{
    length_context length_context_(this);
    const point translation = {
        length_context_.value_for_length(m_x),
        length_context_.value_for_length(m_y)
    };

    return svg_graphics_element::local_transform() * transform::translated(translation.x, translation.y);
}

void svg_use_element::render(svg_render_state& state) const
{
    if(is_display_none())
        return;
    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    render_children(new_state);
    new_state.end_group(blend_info);
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
    const auto& id_attr = target_element->attribute_(property_id::id);
    auto parent = parent_element();
    while(parent) {
        auto attribute_ = parent->find_attribute(property_id::id);
        if(attribute_ && id_attr == attribute_->value())
            return nullptr;
        parent = parent->parent_element();
    }

    auto tag_id = target_element->id();
    if(tag_id == element_id::symbol) {
        tag_id = element_id::svg;
    }

    auto new_element = svg_element::create(dom(), tag_id);
    new_element->set_attributes(target_element->attributes());
    if(new_element->id() == element_id::svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == property_id::width || attribute.id() == property_id::height) {
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
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    render_children(new_state);
    new_state.end_group(blend_info);
}

svg_switch_element::svg_switch_element(document* dom)
    : svg_graphics_element(dom, element_id::switch_)
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
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    if(auto child = selected_child())
        child->render(new_state);
    new_state.end_group(blend_info);
}

svg_defs_element::svg_defs_element(document* dom)
    : svg_graphics_element(dom, element_id::defs)
{
}

} // namespace nova::svg
