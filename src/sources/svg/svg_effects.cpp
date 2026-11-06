#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_marker_element::svg_marker_element(document* dom)
    : svg_element(dom, ElementID::Marker)
    , svg_fit_to_view_box(this)
    , m_refX(PropertyID::RefX, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_refY(PropertyID::RefY, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_markerWidth(PropertyID::MarkerWidth, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerHeight(PropertyID::MarkerHeight, LengthDirection::Vertical, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerUnits(PropertyID::MarkerUnits, MarkerUnits::StrokeWidth)
    , m_orient(PropertyID::Orient)
{
    addProperty(m_refX);
    addProperty(m_refY);
    addProperty(m_markerWidth);
    addProperty(m_markerHeight);
    addProperty(m_markerUnits);
    addProperty(m_orient);
}

Point svg_marker_element::refPoint() const
{
    LengthContext lengthContext(this);
    const Point refPoint = {
        lengthContext.valueForLength(m_refX),
        lengthContext.valueForLength(m_refY)
    };

    return refPoint;
}

Size svg_marker_element::markerSize() const
{
    LengthContext lengthContext(this);
    const Size markerSize = {
        lengthContext.valueForLength(m_markerWidth),
        lengthContext.valueForLength(m_markerHeight)
    };

    return markerSize;
}

Transform svg_marker_element::markerTransform(const Point& origin, float angle, float strokeWidth) const
{
    auto transform = Transform::translated(origin.x, origin.y);
    if(m_orient.orientType() == svg_angle::OrientType::Angle) {
        transform.rotate(m_orient.value());
    } else {
        transform.rotate(angle);
    }

    auto viewTransform = viewBoxToViewTransform(markerSize());
    auto refOrigin = viewTransform.mapPoint(refPoint());
    if(m_markerUnits.value() == MarkerUnits::StrokeWidth)
        transform.scale(strokeWidth, strokeWidth);
    transform.translate(-refOrigin.x, -refOrigin.y);
    return transform * viewTransform;
}

Rect svg_marker_element::markerBoundingBox(const Point& origin, float angle, float strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).map_rect(paint_bounding_box());
}

void svg_marker_element::renderMarker(svg_render_state& state, const Point& origin, float angle, float strokeWidth) const
{
    if(state.hasCycleReference(this))
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, markerTransform(origin, angle, strokeWidth));
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(markerSize()), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

Transform svg_marker_element::local_transform() const
{
    return viewBoxToViewTransform(markerSize());
}

svg_clip_path_element::svg_clip_path_element(document* dom)
    : svg_graphics_element(dom, ElementID::ClipPath)
    , m_clipPathUnits(PropertyID::ClipPathUnits, Units::UserSpaceOnUse)
{
    addProperty(m_clipPathUnits);
}

Rect svg_clip_path_element::clipBoundingBox(const svg_element* el) const
{
    auto clipBoundingBox = paint_bounding_box();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        clipBoundingBox.x = clipBoundingBox.x * bbox.w + bbox.x;
        clipBoundingBox.y = clipBoundingBox.y * bbox.h + bbox.y;
        clipBoundingBox.w = clipBoundingBox.w * bbox.w;
        clipBoundingBox.h = clipBoundingBox.h * bbox.h;
    }

    return local_transform().map_rect(clipBoundingBox);
}

void svg_clip_path_element::applyClipMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Clipping, maskImage);
    renderChildren(newState);
    if(clipper()) {
        clipper()->applyClipMask(newState);
    }

    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

void svg_clip_path_element::applyClipPath(svg_render_state& state) const
{
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        Transform clipTransform(currentTransform);
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->id() != ElementID::Use)
                continue;
            clipTransform.multiply(el->local_transform());
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        state->clip_path(shapeElement->path(), shapeElement->clip_rule(), clipTransform * shapeElement->local_transform());
        return;
    }

    state->clip_rect(Rect::Empty, FillRule::NonZero, Transform::Identity);
}

bool svg_clip_path_element::requiresMasking() const
{
    if(clipper())
        return true;
    const svg_geometry_element* prevShapeElement = nullptr;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->isTextPositioningElement())
                return true;
            if(el->id() != ElementID::Use)
                continue;
            if(el->clipper())
                return true;
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        if(prevShapeElement || shapeElement->clipper())
            return true;
        prevShapeElement = shapeElement;
    }

    return false;
}

svg_mask_element::svg_mask_element(document* dom)
    : svg_element(dom, ElementID::Mask)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_maskUnits(PropertyID::MaskUnits, Units::ObjectBoundingBox)
    , m_maskContentUnits(PropertyID::MaskContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_maskUnits);
    addProperty(m_maskContentUnits);
}

Rect svg_mask_element::maskRect(const svg_element* el) const
{
    LengthContext lengthContext(this, m_maskUnits.value());
    Rect maskRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(m_maskUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    return maskRect;
}

Rect svg_mask_element::maskBoundingBox(const svg_element* el) const
{
    auto maskBoundingBox = paint_bounding_box();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskBoundingBox.x = maskBoundingBox.x * bbox.w + bbox.x;
        maskBoundingBox.y = maskBoundingBox.y * bbox.h + bbox.y;
        maskBoundingBox.w = maskBoundingBox.w * bbox.w;
        maskBoundingBox.h = maskBoundingBox.h * bbox.h;
    }

    return maskBoundingBox.intersected(maskRect(el));
}

void svg_mask_element::applyMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    maskImage->clip_rect(maskRect(state.el()), FillRule::NonZero, state.currentTransform());

    auto currentTransform = state.currentTransform();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Painting, maskImage);
    renderChildren(newState);
    if(clipper())
        clipper()->applyClipMask(newState);
    if(masker()) {
        masker()->applyMask(newState);
    }

    if(m_mask_type == MaskType::Luminance)
        maskImage->convert_to_luminance_mask();
    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

svg_filter_element::svg_filter_element(document* dom)
    : svg_element(dom, ElementID::Filter)
{
}

float svg_filter_element::pixelMargin() const
{
    float margin = 0.f;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        if(el->id() == ElementID::FeGaussianBlur) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 0.f);
            margin = STB_MAX(margin, sigma * 3.f);
        } else if(el->id() == ElementID::FeDropShadow) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 2.f);
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 2.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 2.f);
            margin = STB_MAX(margin, sigma * 3.f + STB_MAX(std::abs(dx), std::abs(dy)));
        } else if(el->id() == ElementID::FeOffset) {
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 0.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 0.f);
            margin = STB_MAX(margin, STB_MAX(std::abs(dx), std::abs(dy)));
        }
        // feFlood/feComposite/feMerge don't themselves need extra margin;
        // if they combine an already-blurred/offset named result, that
        // primitive already grew the margin.
    }

    return margin;
}

void svg_filter_element::applyFilter(svg_render_state& state, float scale) const
{
    svg_filter_context context(state.raster(), scale);
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        auto output = el->applyFilterPrimitive(context);
        if(!output)
            continue; // not a primitive (e.g. a stray feMergeNode outside feMerge)
        context.setResult(el->attribute(PropertyID::Result), std::move(output));
    }

    state.raster()->copy_pixels_from(*context.lastResult());
}

void svg_mask_element::layoutElement(const svg_layout_state& state)
{
    m_mask_type = state.mask_type();
    svg_element::layoutElement(state);
}

} // namespace nova::svg
