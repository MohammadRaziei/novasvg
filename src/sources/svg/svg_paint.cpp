#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_blend_info::svg_blend_info(const svg_element* el)
    : m_clipper(el->clipper())
    , m_masker(el->masker())
    , m_filter(el->filterElement())
    , m_opacity(el->opacity())
    , m_mixBlendMode(el->mixBlendMode())
{
}

bool svg_blend_info::requiresCompositing(svg_render_mode mode) const
{
    return (m_clipper && m_clipper->requiresMasking()) || (mode == svg_render_mode::Painting && (m_masker || m_filter || m_opacity < 1.f || m_mixBlendMode != MixBlendMode::Normal));
}

bool svg_render_state::hasCycleReference(const svg_element* el) const
{
    auto current = this;
    do {
        if(el == current->el())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

void svg_render_state::beginGroup(const svg_blend_info& blendInfo)
{
    auto requiresCompositing = blendInfo.requiresCompositing(m_mode);
    pdf_canvas* pdf = dynamic_cast<pdf_canvas*>(m_canvas.get());
    m_layer = layer_kind::none;
    if(requiresCompositing && pdf) {
        // On a vector page: opacity and blend mode alone stay vector (a transparency group); what PDF cannot
        // express (masks, clip masks, filters) is drawn into pixels at raster_scale() and placed as an image.
        const bool masking = (blendInfo.clipper() && blendInfo.clipper()->requiresMasking()) || blendInfo.masker() || blendInfo.filterElement();
        if(m_mode == svg_render_mode::Painting && !masking) {
            pdf->begin_group();
            m_layer = layer_kind::pdf_group;
            if(blendInfo.clipper())
                blendInfo.clipper()->applyClipPath(*this);
        } else {
            auto bounding_box = m_currentTransform.map_rect(m_element->paint_bounding_box());
            if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
                bounding_box.inflate(blendInfo.filterElement()->pixelMargin() * m_currentTransform.xScale());
            }
            bounding_box.intersect(pdf->extents());
            m_layerScale = pdf->raster_scale();
            m_canvas = raster_canvas::create(Rect(bounding_box.x * m_layerScale, bounding_box.y * m_layerScale, bounding_box.w * m_layerScale, bounding_box.h * m_layerScale));
            m_currentTransform = Transform::scaled(m_layerScale, m_layerScale) * m_currentTransform;
            m_layer = layer_kind::raster_in_pdf;
        }
        return;
    }

    if(requiresCompositing) {
        auto bounding_box = m_currentTransform.map_rect(m_element->paint_bounding_box());
        if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
            bounding_box.inflate(blendInfo.filterElement()->pixelMargin() * m_currentTransform.xScale());
        }
        bounding_box.intersect(static_cast<raster_canvas&>(*m_canvas).extents());
        m_canvas = raster_canvas::create(bounding_box);
        m_layer = layer_kind::raster;
    } else {
        m_canvas->save();
    }

    if(!requiresCompositing && blendInfo.clipper()) {
        blendInfo.clipper()->applyClipPath(*this);
    }
}

void svg_render_state::endGroup(const svg_blend_info& blendInfo)
{
    if(m_layer == layer_kind::none) {
        m_canvas->restore();
        return;
    }

    // the finished group is blended into what is already painted under it, with its mix-blend-mode
    BlendMode blendMode = BlendMode::Src_Over;
    if(m_mode == svg_render_mode::Painting) {
        switch(blendInfo.mixBlendMode()) {
        case MixBlendMode::Multiply: blendMode = BlendMode::Multiply; break;
        case MixBlendMode::Screen: blendMode = BlendMode::Screen; break;
        case MixBlendMode::Darken: blendMode = BlendMode::Darken; break;
        case MixBlendMode::Lighten: blendMode = BlendMode::Lighten; break;
        case MixBlendMode::Difference: blendMode = BlendMode::Difference; break;
        case MixBlendMode::Exclusion: blendMode = BlendMode::Exclusion; break;
        case MixBlendMode::Normal: break;
        }
    }

    auto opacity = m_mode == svg_render_mode::Clipping ? 1.f : blendInfo.opacity();
    if(m_layer == layer_kind::pdf_group) {
        static_cast<pdf_canvas&>(*m_canvas).end_group(blendMode, opacity);
        m_layer = layer_kind::none;
        return;
    }

    if(blendInfo.clipper())
        blendInfo.clipper()->applyClipMask(*this);
    if(m_mode == svg_render_mode::Painting && blendInfo.masker()) {
        blendInfo.masker()->applyMask(*this);
    }
    if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
        blendInfo.filterElement()->applyFilter(*this, m_currentTransform.xScale());
    }

    if(m_layer == layer_kind::raster_in_pdf) {
        auto& layer = static_cast<raster_canvas&>(*m_canvas);
        auto& page = static_cast<pdf_canvas&>(*m_parent->m_canvas);
        const Rect e = layer.extents();
        page.draw_raster_group(pixmap(surface_reference(layer.surface())), Rect(e.x / m_layerScale, e.y / m_layerScale, e.w / m_layerScale, e.h / m_layerScale), blendMode, opacity);
        return;
    }

    static_cast<raster_canvas&>(*m_parent->m_canvas).blend_canvas(static_cast<raster_canvas&>(*m_canvas), blendMode, opacity);
}

svg_paint_element::svg_paint_element(document* dom, ElementID id)
    : svg_element(dom, id)
{
}

svg_stop_element::svg_stop_element(document* dom)
    : svg_element(dom, ElementID::Stop)
    , m_offset(PropertyID::Offset, 0.f)
{
    addProperty(m_offset);
}

void svg_stop_element::layoutElement(const svg_layout_state& state)
{
    m_stop_color = state.stop_color();
    m_stop_opacity = state.stop_opacity();
    svg_element::layoutElement(state);
}

GradientStop svg_stop_element::gradientStop(float opacity) const
{
    stb::color stopColor = with_alpha(m_stop_color, m_stop_opacity * opacity);
    GradientStop gradientStop = {
        m_offset.value(), { red_f(stopColor), green_f(stopColor), blue_f(stopColor), alpha_f(stopColor) }
    };

    return gradientStop;
}

svg_gradient_element::svg_gradient_element(document* dom, ElementID id)
    : svg_paint_element(dom, id)
    , svg_uri_reference(this)
    , m_gradientTransform(PropertyID::GradientTransform)
    , m_gradientUnits(PropertyID::GradientUnits, Units::ObjectBoundingBox)
    , m_spreadMethod(PropertyID::SpreadMethod, SpreadMethod::Pad)
{
    addProperty(m_gradientTransform);
    addProperty(m_gradientUnits);
    addProperty(m_spreadMethod);
}

void svg_gradient_element::collectGradientAttributes(svg_gradient_attributes& attributes) const
{
    if(!attributes.hasGradientTransform() && has_attribute(PropertyID::GradientTransform))
        attributes.setGradientTransform(this);
    if(!attributes.hasSpreadMethod() && has_attribute(PropertyID::SpreadMethod))
        attributes.setSpreadMethod(this);
    if(!attributes.hasGradientUnits() && has_attribute(PropertyID::GradientUnits))
        attributes.setGradientUnits(this);
    if(!attributes.hasGradientContentElement()) {
        for(const auto& child : children()) {
            if(auto el = toSVGElement(child); el && el->id() == ElementID::Stop) {
                attributes.setGradientContentElement(this);
                break;
            }
        }
    }
}

svg_linear_gradient_element::svg_linear_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::LinearGradient)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow, 100.f, LengthUnits::Percent)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

svg_linear_gradient_attributes svg_linear_gradient_element::collectGradientAttributes() const
{
    svg_linear_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::LinearGradient) {
            auto el = static_cast<const svg_linear_gradient_element*>(current);
            if(!attributes.hasX1() && el->has_attribute(PropertyID::X1))
                attributes.setX1(el);
            if(!attributes.hasY1() && el->has_attribute(PropertyID::Y1))
                attributes.setY1(el);
            if(!attributes.hasX2() && el->has_attribute(PropertyID::X2))
                attributes.setX2(el);
            if(!attributes.hasY2() && el->has_attribute(PropertyID::Y2)) {
                attributes.setY2(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

GradientStops buildGradientStops(const svg_gradient_element* el, float opacity)
{
    GradientStops gradientStops;

    const auto& children = el->children();
    gradientStops.reserve(children.size());
    for(const auto& child : children) {
        auto childElement = toSVGElement(child);
        if(childElement && childElement->id() == ElementID::Stop) {
            auto stopElement = static_cast<svg_stop_element*>(childElement);
            gradientStops.push_back(stopElement->gradientStop(opacity));
        }
    }

    return gradientStops;
}

bool svg_linear_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto x1 = lengthContext.valueForLength(attributes.x1());
    auto y1 = lengthContext.valueForLength(attributes.y1());
    auto x2 = lengthContext.valueForLength(attributes.x2());
    auto y2 = lengthContext.valueForLength(attributes.y2());
    if(gradientStops.size() == 1 || (x1 == x2 && y1 == y2)) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_linear_gradient(x1, y1, x2, y2, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_element::svg_radial_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::RadialGradient)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid, 50.f, LengthUnits::Percent)
    , m_fx(PropertyID::Fx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_fy(PropertyID::Fy, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
    addProperty(m_fx);
    addProperty(m_fy);
}

bool svg_radial_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto r = lengthContext.valueForLength(attributes.r());
    if(r == 0.f || gradientStops.size() == 1) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto fx = lengthContext.valueForLength(attributes.fx());
    auto fy = lengthContext.valueForLength(attributes.fy());
    auto cx = lengthContext.valueForLength(attributes.cx());
    auto cy = lengthContext.valueForLength(attributes.cy());

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_radial_gradient(cx, cy, r, fx, fy, spreadMethod, gradientStops, gradientTransform);
    return true;
}

svg_radial_gradient_attributes svg_radial_gradient_element::collectGradientAttributes() const
{
    svg_radial_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::RadialGradient) {
            auto el = static_cast<const svg_radial_gradient_element*>(current);
            if(!attributes.hasCx() && el->has_attribute(PropertyID::Cx))
                attributes.setCx(el);
            if(!attributes.hasCy() && el->has_attribute(PropertyID::Cy))
                attributes.setCy(el);
            if(!attributes.hasR() && el->has_attribute(PropertyID::R))
                attributes.setR(el);
            if(!attributes.hasFx() && el->has_attribute(PropertyID::Fx))
                attributes.setFx(el);
            if(!attributes.hasFy() && el->has_attribute(PropertyID::Fy)) {
                attributes.setFy(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

svg_pattern_element::svg_pattern_element(document* dom)
    : svg_paint_element(dom, ElementID::Pattern)
    , svg_uri_reference(this)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_patternTransform(PropertyID::PatternTransform)
    , m_patternUnits(PropertyID::PatternUnits, Units::ObjectBoundingBox)
    , m_patternContentUnits(PropertyID::PatternContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_patternTransform);
    addProperty(m_patternUnits);
    addProperty(m_patternContentUnits);
}

bool svg_pattern_element::applyPaint(svg_render_state& state, float opacity) const
{
    if(state.hasCycleReference(this))
        return false;
    auto attributes = collectPatternAttributes();
    auto patternContentElement = attributes.patternContentElement();
    if(patternContentElement == nullptr)
        return false;
    LengthContext lengthContext(this, attributes.patternUnits());
    Rect patternRect = {
        lengthContext.valueForLength(attributes.x()),
        lengthContext.valueForLength(attributes.y()),
        lengthContext.valueForLength(attributes.width()),
        lengthContext.valueForLength(attributes.height())
    };

    if(attributes.patternUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternRect.x = patternRect.x * bbox.w + bbox.x;
        patternRect.y = patternRect.y * bbox.h + bbox.y;
        patternRect.w = patternRect.w * bbox.w;
        patternRect.h = patternRect.h * bbox.h;
    }

    auto currentTransform = attributes.patternTransform() * state.currentTransform();
    // On a vector page the tile is a bitmap too; make it sharp at the page's raster scale.
    auto* pdf = dynamic_cast<pdf_canvas*>(&*state);
    const float tileScale = pdf ? pdf->raster_scale() : 1.f;
    auto xScale = currentTransform.xScale() * tileScale;
    auto yScale = currentTransform.yScale() * tileScale;

    auto patternImage = raster_canvas::create(0, 0, patternRect.w * xScale, patternRect.h * yScale);
    auto patternImageTransform = Transform::scaled(xScale, yScale);

    const auto& viewBoxRect = attributes.viewBox();
    if(viewBoxRect.isValid()) {
        const auto& preserveAspectRatio = attributes.preserveAspectRatio();
        patternImageTransform.multiply(preserveAspectRatio.getTransform(viewBoxRect, patternRect.size()));
    } else if(attributes.patternContentUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternImageTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, patternImageTransform, svg_render_mode::Painting, patternImage);
    patternContentElement->renderChildren(newState);

    auto patternTransform = attributes.patternTransform();
    patternTransform.translate(patternRect.x, patternRect.y);
    patternTransform.scale(patternRect.w / patternImage->width(), patternRect.h / patternImage->height());
    if(pdf)
        pdf->set_tiled_image(pixmap(surface_reference(patternImage->surface())), patternTransform, opacity);
    else
        state.raster()->set_texture(*patternImage, TextureType::Tiled, opacity, patternTransform);
    return true;
}

svg_pattern_attributes svg_pattern_element::collectPatternAttributes() const
{
    svg_pattern_attributes attributes;
    std::set<const svg_pattern_element*> processedPatterns;
    const svg_pattern_element* current = this;
    while(true) {
        if(!attributes.hasX() && current->has_attribute(PropertyID::X))
            attributes.setX(current);
        if(!attributes.hasY() && current->has_attribute(PropertyID::Y))
            attributes.setY(current);
        if(!attributes.hasWidth() && current->has_attribute(PropertyID::Width))
            attributes.setWidth(current);
        if(!attributes.hasHeight() && current->has_attribute(PropertyID::Height))
            attributes.setHeight(current);
        if(!attributes.hasPatternTransform() && current->has_attribute(PropertyID::PatternTransform))
            attributes.setPatternTransform(current);
        if(!attributes.hasPatternUnits() && current->has_attribute(PropertyID::PatternUnits))
            attributes.setPatternUnits(current);
        if(!attributes.hasPatternContentUnits() && current->has_attribute(PropertyID::PatternContentUnits))
            attributes.setPatternContentUnits(current);
        if(!attributes.hasViewBox() && current->has_attribute(PropertyID::ViewBox))
            attributes.setViewBox(current);
        if(!attributes.hasPreserveAspectRatio() && current->has_attribute(PropertyID::PreserveAspectRatio))
            attributes.setPreserveAspectRatio(current);
        if(!attributes.hasPatternContentElement()) {
            for(const auto& child : current->children()) {
                if(child->is_element()) {
                    attributes.setPatternContentElement(current);
                    break;
                }
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || targetElement->id() != ElementID::Pattern)
            break;
        processedPatterns.insert(current);
        current = static_cast<const svg_pattern_element*>(targetElement);
        if(processedPatterns.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

} // namespace nova::svg
