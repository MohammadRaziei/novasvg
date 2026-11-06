#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

Rect svg_marker_position::marker_bounding_box(float strokeWidth) const
{
    return m_element->marker_bounding_box(m_origin, m_angle, strokeWidth);
}

void svg_marker_position::render_marker(svg_render_state& state, float strokeWidth) const
{
    m_element->render_marker(state, m_origin, m_angle, strokeWidth);
}

svg_geometry_element::svg_geometry_element(document* dom, element_id id)
    : svg_graphics_element(dom, id)
{
}

Rect svg_geometry_element::stroke_bounding_box() const
{
    auto stroke_bounding_box = fill_bounding_box();
    if(m_stroke.is_renderable()) {
        float cap_limit = m_stroke_data.lineWidth() / 2.f;
        if(m_stroke_data.lineCap() == LineCap::Square)
            cap_limit *= NOVA_SQRT2;
        float join_limit = m_stroke_data.lineWidth() / 2.f;
        if(m_stroke_data.lineJoin() == LineJoin::Miter) {
            join_limit *= m_stroke_data.miterLimit();
        }

        stroke_bounding_box.inflate(STB_MAX(cap_limit, join_limit));
    }

    for(const auto& marker_position : m_marker_positions)
        stroke_bounding_box.unite(marker_position.marker_bounding_box(m_stroke_data.lineWidth()));
    return stroke_bounding_box;
}

void svg_geometry_element::layout_element(const svg_layout_state& state)
{
    m_fill_rule = state.fill_rule();
    m_clip_rule = state.clip_rule();
    m_fill = get_paint_server(state.fill(), state.fill_opacity());
    m_stroke = get_paint_server(state.stroke(), state.stroke_opacity());
    m_stroke_data = get_stroke_data(state);
    svg_graphics_element::layout_element(state);

    m_path.clear();
    m_marker_positions.clear();
    m_fill_bounding_box = update_shape(m_path);
    update_marker_positions(m_marker_positions, state);
}

void svg_geometry_element::update_marker_positions(svg_marker_position_list& positions, const svg_layout_state& state)
{
    if(m_path.empty())
        return;
    auto markerStart = get_marker(state.marker_start());
    auto markerMid = get_marker(state.marker_mid());
    auto markerEnd = get_marker(state.marker_end());
    if(markerStart == nullptr && markerMid == nullptr && markerEnd == nullptr) {
        return;
    }

    Point origin;
    Point startPoint;
    Point inslope_points[2];
    Point outslope_points[2];

    int index = 0;
    std::array<Point, 3> points;
    outline_walker it(m_path);
    while(!it.done()) {
        switch(it.current_segment(points)) {
        case segment_kind::move_to:
            startPoint = points[0];
            inslope_points[0] = origin;
            inslope_points[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::line_to:
            inslope_points[0] = origin;
            inslope_points[1] = points[0];
            origin = points[0];
            break;
        case segment_kind::cubic_to:
            inslope_points[0] = points[1];
            inslope_points[1] = points[2];
            origin = points[2];
            break;
        case segment_kind::close:
            inslope_points[0] = origin;
            inslope_points[1] = points[0];
            origin = startPoint;
            startPoint = Point();
            break;
        }

        it.next();

        if(!it.done() && (markerStart || markerMid)) {
            it.current_segment(points);
            outslope_points[0] = origin;
            outslope_points[1] = points[0];
            if(index == 0 && markerStart) {
                auto slope = outslope_points[1] - outslope_points[0];
                auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
                const auto& orient = markerStart->orient();
                if(orient.orientType() == svg_angle::OrientType::auto_start_reverse)
                    angle -= 180.f;
                positions.emplace_back(markerStart, origin, angle);
            }

            if(index > 0 && markerMid) {
                auto inslope = inslope_points[1] - inslope_points[0];
                auto outslope = outslope_points[1] - outslope_points[0];
                auto inangle = 180.f * std::atan2(inslope.y, inslope.x) / NOVA_PI;
                auto outangle = 180.f * std::atan2(outslope.y, outslope.x) / NOVA_PI;
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                positions.emplace_back(markerMid, origin, angle);
            }
        }

        if(markerEnd && it.done()) {
            auto slope = inslope_points[1] - inslope_points[0];
            auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
            positions.emplace_back(markerEnd, origin, angle);
        }

        index += 1;
    }
}

void svg_geometry_element::render(svg_render_state& state) const
{
    if(!is_renderable())
        return;
    svg_blend_info blend_info(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blend_info);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(stb::color::white());
        newState->fill_path(m_path, m_clip_rule, newState.current_transform());
    } else {
        if(m_fill.apply_paint(newState))
            newState->fill_path(m_path, m_fill_rule, newState.current_transform());
        if(m_stroke.apply_paint(newState)) {
            newState->stroke_path(m_path, m_stroke_data, newState.current_transform());
        }

        for(const auto& marker_position : m_marker_positions) {
            marker_position.render_marker(newState, m_stroke_data.lineWidth());
        }
    }

    newState.endGroup(blend_info);
}

svg_line_element::svg_line_element(document* dom)
    : svg_geometry_element(dom, element_id::Line)
    , m_x1(property_id::X1, length_direction::Horizontal, length_negative_mode::allow)
    , m_y1(property_id::Y1, length_direction::vertical, length_negative_mode::allow)
    , m_x2(property_id::X2, length_direction::Horizontal, length_negative_mode::allow)
    , m_y2(property_id::Y2, length_direction::vertical, length_negative_mode::allow)
{
    add_property(m_x1);
    add_property(m_y1);
    add_property(m_x2);
    add_property(m_y2);
}

Rect svg_line_element::update_shape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto x1 = lengthContext.value_for_length(m_x1);
    auto y1 = lengthContext.value_for_length(m_y1);
    auto x2 = lengthContext.value_for_length(m_x2);
    auto y2 = lengthContext.value_for_length(m_y2);

    path.move_to(x1, y1);
    path.line_to(x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

svg_rect_element::svg_rect_element(document* dom)
    : svg_geometry_element(dom, element_id::Rect)
    , m_x(property_id::X, length_direction::Horizontal, length_negative_mode::allow)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow)
    , m_width(property_id::Width, length_direction::Horizontal, length_negative_mode::forbid)
    , m_height(property_id::Height, length_direction::vertical, length_negative_mode::forbid)
    , m_rx(property_id::Rx, length_direction::Horizontal, length_negative_mode::forbid)
    , m_ry(property_id::Ry, length_direction::vertical, length_negative_mode::forbid)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
    add_property(m_rx);
    add_property(m_ry);
}

Rect svg_rect_element::update_shape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto width = lengthContext.value_for_length(m_width);
    auto height = lengthContext.value_for_length(m_height);
    if(width <= 0.f || height <= 0.f) {
        return Rect::Empty;
    }

    auto x = lengthContext.value_for_length(m_x);
    auto y = lengthContext.value_for_length(m_y);

    auto rx = lengthContext.value_for_length(m_rx);
    auto ry = lengthContext.value_for_length(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = STB_MIN(rx, width / 2.f);
    ry = STB_MIN(ry, height / 2.f);

    path.add_rounded_rect(x, y, width, height, rx, ry);
    return Rect(x, y, width, height);
}

svg_ellipse_element::svg_ellipse_element(document* dom)
    : svg_geometry_element(dom, element_id::ellipse)
    , m_cx(property_id::Cx, length_direction::Horizontal, length_negative_mode::allow)
    , m_cy(property_id::Cy, length_direction::vertical, length_negative_mode::allow)
    , m_rx(property_id::Rx, length_direction::diagonal, length_negative_mode::forbid)
    , m_ry(property_id::Ry, length_direction::diagonal, length_negative_mode::forbid)
{
    add_property(m_cx);
    add_property(m_cy);
    add_property(m_rx);
    add_property(m_ry);
}

Rect svg_ellipse_element::update_shape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto rx = lengthContext.value_for_length(m_rx);
    auto ry = lengthContext.value_for_length(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.value_for_length(m_cx);
    auto cy = lengthContext.value_for_length(m_cy);
    path.add_ellipse(cx, cy, rx, ry);
    return Rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

svg_circle_element::svg_circle_element(document* dom)
    : svg_geometry_element(dom, element_id::Circle)
    , m_cx(property_id::Cx, length_direction::Horizontal, length_negative_mode::allow)
    , m_cy(property_id::Cy, length_direction::vertical, length_negative_mode::allow)
    , m_r(property_id::R, length_direction::diagonal, length_negative_mode::forbid)
{
    add_property(m_cx);
    add_property(m_cy);
    add_property(m_r);
}

Rect svg_circle_element::update_shape(stb::outline& path)
{
    LengthContext lengthContext(this);
    auto r = lengthContext.value_for_length(m_r);
    if(r <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.value_for_length(m_cx);
    auto cy = lengthContext.value_for_length(m_cy);
    path.add_ellipse(cx, cy, r, r);
    return Rect(cx - r, cy - r, r + r, r + r);
}

svg_poly_element::svg_poly_element(document* dom, element_id id)
    : svg_geometry_element(dom, id)
    , m_points(property_id::Points)
{
    add_property(m_points);
}

Rect svg_poly_element::update_shape(stb::outline& path)
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return Rect::Empty;
    }

    path.move_to(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path.line_to(points[i].x, points[i].y);
    }

    if(id() == element_id::polygon)
        path.close();
    return bounding_rect(path);
}

svg_path_element::svg_path_element(document* dom)
    : svg_geometry_element(dom, element_id::Path)
    , m_d(property_id::D)
{
    add_property(m_d);
}

Rect svg_path_element::update_shape(stb::outline& path)
{
    path = m_d.value();
    return bounding_rect(path);
}

} // namespace nova::svg
