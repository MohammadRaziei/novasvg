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
    root_element()->setNeedsLayout();
    m_data.assign(data);
}

std::unique_ptr<svg_node> svg_text_node::clone(bool deep) const
{
    auto nd = std::make_unique<svg_text_node>(dom());
    nd->set_data(m_data);
    return nd;
}

std::unique_ptr<svg_element> svg_element::create(document* dom, ElementID id)
{
    switch(id) {
    case ElementID::Svg:
        return std::make_unique<svg_svg_element>(dom);
    case ElementID::Path:
        return std::make_unique<svg_path_element>(dom);
    case ElementID::G:
        return std::make_unique<svg_g_element>(dom);
    case ElementID::Switch:
        return std::make_unique<svg_switch_element>(dom);
    case ElementID::Rect:
        return std::make_unique<svg_rect_element>(dom);
    case ElementID::Circle:
        return std::make_unique<svg_circle_element>(dom);
    case ElementID::Ellipse:
        return std::make_unique<svg_ellipse_element>(dom);
    case ElementID::Line:
        return std::make_unique<svg_line_element>(dom);
    case ElementID::Defs:
        return std::make_unique<svg_defs_element>(dom);
    case ElementID::Polygon:
    case ElementID::Polyline:
        return std::make_unique<svg_poly_element>(dom, id);
    case ElementID::Stop:
        return std::make_unique<svg_stop_element>(dom);
    case ElementID::LinearGradient:
        return std::make_unique<svg_linear_gradient_element>(dom);
    case ElementID::RadialGradient:
        return std::make_unique<svg_radial_gradient_element>(dom);
    case ElementID::Symbol:
        return std::make_unique<svg_symbol_element>(dom);
    case ElementID::Use:
        return std::make_unique<svg_use_element>(dom);
    case ElementID::Pattern:
        return std::make_unique<svg_pattern_element>(dom);
    case ElementID::Mask:
        return std::make_unique<svg_mask_element>(dom);
    case ElementID::ClipPath:
        return std::make_unique<svg_clip_path_element>(dom);
    case ElementID::Filter:
        return std::make_unique<svg_filter_element>(dom);
    case ElementID::FeGaussianBlur:
        return std::make_unique<svg_fe_gaussian_blur_element>(dom);
    case ElementID::FeDropShadow:
        return std::make_unique<svg_fe_drop_shadow_element>(dom);
    case ElementID::FeOffset:
        return std::make_unique<svg_fe_offset_element>(dom);
    case ElementID::FeFlood:
        return std::make_unique<svg_fe_flood_element>(dom);
    case ElementID::FeComposite:
        return std::make_unique<svg_fe_composite_element>(dom);
    case ElementID::FeMerge:
        return std::make_unique<svg_fe_merge_element>(dom);
    case ElementID::FeMergeNode:
        return std::make_unique<svg_element>(dom, id);
    case ElementID::Marker:
        return std::make_unique<svg_marker_element>(dom);
    case ElementID::Image:
        return std::make_unique<svg_image_element>(dom);
    case ElementID::ForeignObject:
        return std::make_unique<svg_foreign_object_element>(dom);
    case ElementID::Style:
        return std::make_unique<svg_style_element>(dom);
    case ElementID::Text:
        return std::make_unique<svg_text_element>(dom);
    case ElementID::Tspan:
        return std::make_unique<svg_t_span_element>(dom);
    default:
        assert(false);
    }

    return nullptr;
}

svg_element::svg_element(document* dom, ElementID id)
    : svg_node(dom)
    , m_id(id)
{
}

bool svg_element::has_attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return has_attribute(id);
}

const std::string& svg_element::attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return emptyString;
    return attribute(id);
}

bool svg_element::set_attribute(std::string_view name, const std::string& value)
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return set_attribute(Specificity::Api, id, value);
}

const Attribute* svg_element::findAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return &attribute;
        }
    }

    return nullptr;
}

bool svg_element::has_attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return true;
        }
    }

    return false;
}

const std::string& svg_element::attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return attribute.value();
        }
    }

    return emptyString;
}

bool svg_element::set_attribute(int specificity, PropertyID id, const std::string& value)
{
    // A value with var() cannot be parsed before the custom properties it names are all known (a stylesheet
    // can come after the el): it is kept, and parsed by resolveVariables() when the dom is complete.
    const bool deferred = usesVariables(id, value);
    for(auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            if(specificity < attribute.specificity())
                return false;
            if(deferred)
                m_variableSources[id] = value;
            else {
                m_variableSources.erase(id);
                parseAttribute(id, value);
            }
            attribute = Attribute(specificity, id, value);
            return true;
        }
    }

    if(deferred)
        m_variableSources[id] = value;
    else
        parseAttribute(id, value);
    m_attributes.emplace_front(specificity, id, value);
    return true;
}

void svg_element::setCustomProperty(int specificity, const std::string& name, const std::string& value)
{
    auto it = m_customProperties.find(name);
    if(it != m_customProperties.end() && specificity < it->second.first)
        return;
    std::string_view trimmed(value);
    stripLeadingAndTrailingSpaces(trimmed);
    m_customProperties[name] = {specificity, std::string(trimmed)};
}

const svg_element* svg_element::findCustomPropertyOwner(const std::string& name) const
{
    for(auto el = this; el != nullptr; el = el->parent_element()) {
        if(el->m_customProperties.count(name))
            return el;
    }
    return nullptr;
}

const std::string& svg_element::customProperty(const std::string& name) const
{
    if(auto owner = findCustomPropertyOwner(name))
        return owner->m_customProperties.at(name).second;
    return emptyString;
}

void svg_element::resolveVariables()
{
    for(const auto& [id, source] : m_variableSources) {
        std::string resolved;
        if(!substituteVariables(source, this, resolved, 0))
            continue;
        for(auto& attribute : m_attributes) {
            if(attribute.id() == id) {
                attribute = Attribute(attribute.specificity(), id, resolved);
                break;
            }
        }
        parseAttribute(id, resolved);
    }
}

void svg_element::setAttributes(const AttributeList& attributes)
{
    for(const auto& attribute : attributes) {
        set_attribute(attribute);
    }
}

bool svg_element::set_attribute(const Attribute& attribute)
{
    return set_attribute(attribute.specificity(), attribute.id(), attribute.value());
}

void svg_element::parseAttribute(PropertyID id, const std::string& value)
{
    root_element()->setNeedsLayout();
    if(auto property = getProperty(id)) {
        property->parse(value);
    }
}

svg_element* svg_element::previousElement() const
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

svg_element* svg_element::nextElement() const
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

svg_node* svg_element::addChild(std::unique_ptr<svg_node> child)
{
    child->setParentElement(this);
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

Rect svg_element::fillBoundingBox() const
{
    auto fillBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            fillBoundingBox.unite(el->local_transform().map_rect(el->fillBoundingBox()));
        }
    }

    if(!fillBoundingBox.isValid())
        fillBoundingBox = Rect::Empty;
    return fillBoundingBox;
}

Rect svg_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            strokeBoundingBox.unite(el->local_transform().map_rect(el->strokeBoundingBox()));
        }
    }

    if(!strokeBoundingBox.isValid())
        strokeBoundingBox = Rect::Empty;
    return strokeBoundingBox;
}

Rect svg_element::paint_bounding_box() const
{
    if(m_paintBoundingBox.isValid())
        return m_paintBoundingBox;
    m_paintBoundingBox = Rect::Empty;
    m_paintBoundingBox = strokeBoundingBox();
    assert(m_paintBoundingBox.isValid());
    if(m_clipper) m_paintBoundingBox.intersect(m_clipper->clipBoundingBox(this));
    if(m_masker) m_paintBoundingBox.intersect(m_masker->maskBoundingBox(this));
    return m_paintBoundingBox;
}

svg_marker_element* svg_element::getMarker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Marker)
        return static_cast<svg_marker_element*>(el);
    return nullptr;
}

svg_clip_path_element* svg_element::getClipper(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::ClipPath)
        return static_cast<svg_clip_path_element*>(el);
    return nullptr;
}

svg_mask_element* svg_element::getMasker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Mask)
        return static_cast<svg_mask_element*>(el);
    return nullptr;
}

svg_filter_element* svg_element::getFilterElement(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Filter)
        return static_cast<svg_filter_element*>(el);
    return nullptr;
}

svg_paint_element* svg_element::getPainter(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->isPaintElement())
        return static_cast<svg_paint_element*>(el);
    return nullptr;
}

svg_element* svg_element::element_from_point(float x, float y)
{
    auto it = m_children.rbegin();
    auto end = m_children.rend();
    for(; it != end; ++it) {
        auto child = toSVGElement(*it);
        if(child && !child->isHiddenElement()) {
            if(auto el = child->element_from_point(x, y)) {
                return el;
            }
        }
    }

    if(isPointableElement()) {
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

void svg_element::addProperty(svg_property& value)
{
    m_properties.push_front(&value);
}

svg_property* svg_element::getProperty(PropertyID id) const
{
    for(auto property : m_properties) {
        if(id == property->id()) {
            return property;
        }
    }

    return nullptr;
}

Size svg_element::currentViewportSize() const
{
    auto parent = parent_element();
    if(parent == nullptr) {
        auto el = static_cast<const svg_svg_element*>(this);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        return Size(300, 150);
    }

    if(parent->id() == ElementID::Svg) {
        auto el = static_cast<const svg_svg_element*>(parent);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        LengthContext lengthContext(el);
        auto width = lengthContext.valueForLength(el->width());
        auto height = lengthContext.valueForLength(el->height());
        return Size(width, height);
    }

    return parent->currentViewportSize();
}

void svg_element::cloneChildren(svg_element* parent_element) const
{
    for(const auto& child : m_children) {
        parent_element->addChild(child->clone(true));
    }
}

std::unique_ptr<svg_node> svg_element::clone(bool deep) const
{
    auto el = svg_element::create(dom(), m_id);
    el->setAttributes(m_attributes);
    if(deep) { cloneChildren(el.get()); }
    return el;
}

void svg_element::build()
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->build();
        }
    }
}

void svg_element::layoutElement(const svg_layout_state& state)
{
    m_paintBoundingBox = Rect::Invalid;
    m_clipper = getClipper(state.clip_path());
    m_masker = getMasker(state.mask());
    m_filter = getFilterElement(state.filter());
    m_opacity = state.opacity();
    m_mix_blend_mode = state.mix_blend_mode();

    m_font_size = state.font_size();
    m_display = state.display();
    m_overflow = state.overflow();
    m_visibility = state.visibility();
    m_pointer_events = state.pointer_events();
}

void svg_element::layoutChildren(svg_layout_state& state)
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->layout(state);
        }
    }
}

void svg_element::layout(svg_layout_state& state)
{
    svg_layout_state newState(state, this);
    layoutElement(newState);
    layoutChildren(newState);
}

void svg_element::renderChildren(svg_render_state& state) const
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->render(state);
        }
    }
}

void svg_element::render(svg_render_state& state) const
{
}

bool svg_element::isHiddenElement() const
{
    if(isDisplayNone())
        return true;
    switch(m_id) {
    case ElementID::Defs:
    case ElementID::Symbol:
    case ElementID::Marker:
    case ElementID::ClipPath:
    case ElementID::Mask:
    case ElementID::Filter:
    case ElementID::FeGaussianBlur:
    case ElementID::FeDropShadow:
    case ElementID::FeOffset:
    case ElementID::FeFlood:
    case ElementID::FeComposite:
    case ElementID::FeMerge:
    case ElementID::FeMergeNode:
    case ElementID::LinearGradient:
    case ElementID::RadialGradient:
    case ElementID::Pattern:
    case ElementID::Stop:
        return true;
    default:
        return false;
    }
}

bool svg_element::isPointableElement() const
{
    if(m_pointer_events != PointerEvents::None
        && m_visibility != Visibility::Hidden
        && m_display != Display::None
        && m_opacity != 0.f) {
        switch(m_id) {
        case ElementID::Line:
        case ElementID::Rect:
        case ElementID::Ellipse:
        case ElementID::Circle:
        case ElementID::Polyline:
        case ElementID::Polygon:
        case ElementID::Path:
        case ElementID::Text:
        case ElementID::Image:
        case ElementID::ForeignObject:
            return true;
        default:
            break;
        }
    }

    return false;
}

svg_style_element::svg_style_element(document* dom)
    : svg_element(dom, ElementID::Style)
{
}

svg_fit_to_view_box::svg_fit_to_view_box(svg_element* el)
    : m_viewBox(PropertyID::ViewBox)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    el->addProperty(m_viewBox);
    el->addProperty(m_preserveAspectRatio);
}

Transform svg_fit_to_view_box::viewBoxToViewTransform(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Transform::Identity;
    return m_preserveAspectRatio.getTransform(viewBoxRect, viewportSize);
}

Rect svg_fit_to_view_box::getClipRect(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Rect(0, 0, viewportSize.w, viewportSize.h);
    return m_preserveAspectRatio.getClipRect(viewBoxRect, viewportSize);
}

svg_uri_reference::svg_uri_reference(svg_element* el)
    : m_href(PropertyID::Href)
{
    el->addProperty(m_href);
}

svg_element* svg_uri_reference::getTargetElement(const document* dom) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return dom->root_element()->element_by_id(value.substr(1));
}

bool svg_paint_server::applyPaint(svg_render_state& state) const
{
    if(!isRenderable())
        return false;
    if(m_element) return m_element->applyPaint(state, m_opacity);
    state->set_color(with_alpha(m_color, m_opacity));
    return true;
}

svg_graphics_element::svg_graphics_element(document* dom, ElementID id)
    : svg_element(dom, id)
    , m_transform(PropertyID::Transform)
    , m_transformOrigin(PropertyID::Transform_Origin)
    , m_transformBox(PropertyID::Transform_Box)
{
    addProperty(m_transform);
    addProperty(m_transformOrigin);
    addProperty(m_transformBox);
}

Transform svg_graphics_element::local_transform() const
{
    // Only shift the pivot when transform-origin was actually specified --
    // matches legacy behavior (origin at the current user-space (0,0)) for
    // every el that doesn't use it, i.e. everything except elements
    // that explicitly opt in the way feature-css-use-symbol.svg's `.rot`
    // class does.
    if(!m_transformOrigin.specified())
        return m_transform.value();

    Rect box;
    if(m_transformBox.specified() && m_transformBox.value() == TransformBoxType::FillBox) {
        box = fillBoundingBox();
    } else {
        auto size = currentViewportSize();
        box = Rect(0.f, 0.f, size.w, size.h);
    }

    if(box.empty())
        return m_transform.value();

    auto origin = m_transformOrigin.resolve(box);
    return Transform::translated(origin.x, origin.y) * m_transform.value() * Transform::translated(-origin.x, -origin.y);
}

svg_paint_server svg_graphics_element::getPaintServer(const Paint& paint, float opacity) const
{
    if(paint.isNone())
        return svg_paint_server();
    if(auto el = getPainter(paint.id()))
        return svg_paint_server(el, paint.color(), opacity);
    return svg_paint_server(nullptr, paint.color(), opacity);
}

StrokeData svg_graphics_element::getStrokeData(const svg_layout_state& state) const
{
    LengthContext lengthContext(this);
    StrokeData strokeData(lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal));
    strokeData.setMiterLimit(state.stroke_miterlimit());
    strokeData.setLineCap(state.stroke_linecap());
    strokeData.setLineJoin(state.stroke_linejoin());
    strokeData.setDashOffset(lengthContext.valueForLength(state.stroke_dashoffset(), LengthDirection::Diagonal));

    DashArray dashArray;
    for(const auto& dash : state.stroke_dasharray())
        dashArray.push_back(lengthContext.valueForLength(dash, LengthDirection::Diagonal));
    strokeData.setDashArray(std::move(dashArray));
    return strokeData;
}

svg_svg_element::svg_svg_element(document* dom)
    : svg_graphics_element(dom, ElementID::Svg)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Transform svg_svg_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(isRootElement())
        return viewBoxToViewTransform(viewportRect.size());
    return svg_graphics_element::local_transform() * Transform::translated(viewportRect.x, viewportRect.y) * viewBoxToViewTransform(viewportRect.size());
}

void svg_svg_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    LengthContext lengthContext(this);
    const Size viewportSize = {
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(viewportSize.empty())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(viewportSize), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

svg_root_element::svg_root_element(document* dom)
    : svg_svg_element(dom)
{
}

svg_root_element* svg_root_element::layout_if_needed()
{
    if(needsLayout())
        force_layout();
    return this;
}

svg_element* svg_root_element::element_by_id(std::string_view id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

void svg_root_element::addElementById(const std::string& id, svg_element* el)
{
    m_idCache.emplace(id, el);
}

void svg_root_element::layout(svg_layout_state& state)
{
    svg_svg_element::layout(state);

    LengthContext lengthContext(this);
    if(!width().isPercent()) {
        m_intrinsicWidth = lengthContext.valueForLength(width());
    } else {
        m_intrinsicWidth = 0.f;
    }

    if(!height().isPercent()) {
        m_intrinsicHeight = lengthContext.valueForLength(height());
    } else {
        m_intrinsicHeight = 0.f;
    }

    const auto& viewBoxRect = viewBox().value();
    if(!viewBoxRect.empty() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        auto intrinsicRatio = viewBoxRect.w / viewBoxRect.h;
        if(!m_intrinsicWidth && m_intrinsicHeight)
            m_intrinsicWidth = m_intrinsicHeight * intrinsicRatio;
        else if(m_intrinsicWidth && !m_intrinsicHeight) {
            m_intrinsicHeight = m_intrinsicWidth / intrinsicRatio;
        }
    }

    if(viewBoxRect.isValid() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        m_intrinsicWidth = viewBoxRect.w;
        m_intrinsicHeight = viewBoxRect.h;
    }

    if(!m_intrinsicWidth || !m_intrinsicHeight) {
        auto bounding_box = paint_bounding_box();
        if(!m_intrinsicWidth)
            m_intrinsicWidth = bounding_box.right();
        if(!m_intrinsicHeight) {
            m_intrinsicHeight = bounding_box.bottom();
        }
    }
}

void svg_root_element::force_layout()
{
    svg_layout_state state;
    layout(state);
}

svg_use_element::svg_use_element(document* dom)
    : svg_graphics_element(dom, ElementID::Use)
    , svg_uri_reference(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Transform svg_use_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Point translation = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y)
    };

    return svg_graphics_element::local_transform() * Transform::translated(translation.x, translation.y);
}

void svg_use_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

void svg_use_element::build()
{
    if(auto targetElement = getTargetElement(dom())) {
        if(auto newElement = cloneTargetElement(targetElement)) {
            addChild(std::move(newElement));
        }
    }

    svg_graphics_element::build();
}

std::unique_ptr<svg_element> svg_use_element::cloneTargetElement(svg_element* targetElement)
{
    if(targetElement == this || isDisallowedElement(targetElement))
        return nullptr;
    const auto& idAttr = targetElement->attribute(PropertyID::Id);
    auto parent = parent_element();
    while(parent) {
        auto attribute = parent->findAttribute(PropertyID::Id);
        if(attribute && idAttr == attribute->value())
            return nullptr;
        parent = parent->parent_element();
    }

    auto tagId = targetElement->id();
    if(tagId == ElementID::Symbol) {
        tagId = ElementID::Svg;
    }

    auto newElement = svg_element::create(dom(), tagId);
    newElement->setAttributes(targetElement->attributes());
    if(newElement->id() == ElementID::Svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == PropertyID::Width || attribute.id() == PropertyID::Height) {
                newElement->set_attribute(attribute);
            }
        }
    }

    if(newElement->id() != ElementID::Use)
        targetElement->cloneChildren(newElement.get());
    return newElement;
}

svg_symbol_element::svg_symbol_element(document* dom)
    : svg_graphics_element(dom, ElementID::Symbol)
    , svg_fit_to_view_box(this)
{
}

svg_g_element::svg_g_element(document* dom)
    : svg_graphics_element(dom, ElementID::G)
{
}

void svg_g_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

svg_switch_element::svg_switch_element(document* dom)
    : svg_graphics_element(dom, ElementID::Switch)
{
}

const svg_element* svg_switch_element::selectedChild() const
{
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        // only graphics elements take part; <title>, <desc>, <defs>... are skipped
        if(!el || isDisallowedElement(el))
            continue;
        if(switchChildPasses(el))
            return el;
    }
    return nullptr;
}

void svg_switch_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(auto child = selectedChild())
        child->render(newState);
    newState.endGroup(blendInfo);
}

svg_defs_element::svg_defs_element(document* dom)
    : svg_graphics_element(dom, ElementID::Defs)
{
}

} // namespace nova::svg
