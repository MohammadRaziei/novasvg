#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

Rect svg_marker_position::markerBoundingBox(float strokeWidth) const
{
    return m_element->markerBoundingBox(m_origin, m_angle, strokeWidth);
}

void svg_marker_position::renderMarker(svg_render_state& state, float strokeWidth) const
{
    m_element->renderMarker(state, m_origin, m_angle, strokeWidth);
}

svg_geometry_element::svg_geometry_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
{
}

Rect svg_geometry_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = fillBoundingBox();
    if(m_stroke.isRenderable()) {
        float capLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineCap() == LineCap::Square)
            capLimit *= NOVA_SQRT2;
        float joinLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineJoin() == LineJoin::Miter) {
            joinLimit *= m_strokeData.miterLimit();
        }

        strokeBoundingBox.inflate(STB_MAX(capLimit, joinLimit));
    }

    for(const auto& markerPosition : m_markerPositions)
        strokeBoundingBox.unite(markerPosition.markerBoundingBox(m_strokeData.lineWidth()));
    return strokeBoundingBox;
}

void svg_geometry_element::layoutElement(const svg_layout_state& state)
{
    m_fill_rule = state.fill_rule();
    m_clip_rule = state.clip_rule();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    m_strokeData = getStrokeData(state);
    svg_graphics_element::layoutElement(state);

    m_path.clear();
    m_markerPositions.clear();
    m_fillBoundingBox = updateShape(m_path);
    updateMarkerPositions(m_markerPositions, state);
}

void svg_geometry_element::updateMarkerPositions(svg_marker_position_list& positions, const svg_layout_state& state)
{
    if(m_path.empty())
        return;
    auto markerStart = getMarker(state.marker_start());
    auto markerMid = getMarker(state.marker_mid());
    auto markerEnd = getMarker(state.marker_end());
    if(markerStart == nullptr && markerMid == nullptr && markerEnd == nullptr) {
        return;
    }

    Point origin;
    Point startPoint;
    Point inslopePoints[2];
    Point outslopePoints[2];

    int index = 0;
    std::array<Point, 3> points;
    outline_walker it(m_path);
    while(!it.done()) {
        switch(it.current_segment(points)) {
        case segment_kind::move_to:
            startPoint = points[0];
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::line_to:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::cubic_to:
            inslopePoints[0] = points[1];
            inslopePoints[1] = points[2];
            origin = points[2];
            break;
        case segment_kind::close:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = startPoint;
            startPoint = Point();
            break;
        }

        it.next();

        if(!it.done() && (markerStart || markerMid)) {
            it.current_segment(points);
            outslopePoints[0] = origin;
            outslopePoints[1] = points[0];
            if(index == 0 && markerStart) {
                auto slope = outslopePoints[1] - outslopePoints[0];
                auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
                const auto& orient = markerStart->orient();
                if(orient.orientType() == svg_angle::OrientType::AutoStartReverse)
                    angle -= 180.f;
                positions.emplace_back(markerStart, origin, angle);
            }

            if(index > 0 && markerMid) {
                auto inslope = inslopePoints[1] - inslopePoints[0];
                auto outslope = outslopePoints[1] - outslopePoints[0];
                auto inangle = 180.f * std::atan2(inslope.y, inslope.x) / NOVA_PI;
                auto outangle = 180.f * std::atan2(outslope.y, outslope.x) / NOVA_PI;
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                positions.emplace_back(markerMid, origin, angle);
            }
        }

        if(markerEnd && it.done()) {
            auto slope = inslopePoints[1] - inslopePoints[0];
            auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
            positions.emplace_back(markerEnd, origin, angle);
        }

        index += 1;
    }
}

void svg_geometry_element::render(svg_render_state& state) const
{
    if(!isRenderable())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(stb::color::white());
        newState->fill_path(m_path, m_clip_rule, newState.currentTransform());
    } else {
        if(m_fill.applyPaint(newState))
            newState->fill_path(m_path, m_fill_rule, newState.currentTransform());
        if(m_stroke.applyPaint(newState)) {
            newState->stroke_path(m_path, m_strokeData, newState.currentTransform());
        }

        for(const auto& markerPosition : m_markerPositions) {
            markerPosition.renderMarker(newState, m_strokeData.lineWidth());
        }
    }

    newState.endGroup(blendInfo);
}

svg_line_element::svg_line_element(document* dom)
    : svg_geometry_element(dom, ElementID::Line)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

Rect svg_line_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto x1 = lengthContext.valueForLength(m_x1);
    auto y1 = lengthContext.valueForLength(m_y1);
    auto x2 = lengthContext.valueForLength(m_x2);
    auto y2 = lengthContext.valueForLength(m_y2);

    path.move_to(x1, y1);
    path.line_to(x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

svg_rect_element::svg_rect_element(document* dom)
    : svg_geometry_element(dom, ElementID::Rect)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_rx(PropertyID::Rx, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Vertical, LengthNegativeMode::Forbid)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_rx);
    addProperty(m_ry);
}

Rect svg_rect_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto width = lengthContext.valueForLength(m_width);
    auto height = lengthContext.valueForLength(m_height);
    if(width <= 0.f || height <= 0.f) {
        return Rect::Empty;
    }

    auto x = lengthContext.valueForLength(m_x);
    auto y = lengthContext.valueForLength(m_y);

    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = STB_MIN(rx, width / 2.f);
    ry = STB_MIN(ry, height / 2.f);

    path.add_rounded_rect(x, y, width, height, rx, ry);
    return Rect(x, y, width, height);
}

svg_ellipse_element::svg_ellipse_element(document* dom)
    : svg_geometry_element(dom, ElementID::Ellipse)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rx(PropertyID::Rx, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_rx);
    addProperty(m_ry);
}

Rect svg_ellipse_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.add_ellipse(cx, cy, rx, ry);
    return Rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

svg_circle_element::svg_circle_element(document* dom)
    : svg_geometry_element(dom, ElementID::Circle)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
}

Rect svg_circle_element::updateShape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto r = lengthContext.valueForLength(m_r);
    if(r <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.add_ellipse(cx, cy, r, r);
    return Rect(cx - r, cy - r, r + r, r + r);
}

svg_poly_element::svg_poly_element(document* dom, ElementID id)
    : svg_geometry_element(dom, id)
    , m_points(PropertyID::Points)
{
    addProperty(m_points);
}

Rect svg_poly_element::updateShape(stb::outline& path)
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return Rect::Empty;
    }

    path.move_to(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path.line_to(points[i].x, points[i].y);
    }

    if(id() == ElementID::Polygon)
        path.close();
    return bounding_rect(path);
}

svg_path_element::svg_path_element(document* dom)
    : svg_geometry_element(dom, ElementID::Path)
    , m_d(PropertyID::D)
{
    addProperty(m_d);
}

Rect svg_path_element::updateShape(stb::outline& path)
{
    path = m_d.value();
    return bounding_rect(path);
}

} // namespace nova::svg
