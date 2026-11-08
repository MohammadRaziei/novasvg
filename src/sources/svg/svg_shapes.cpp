#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

rect svg_marker_position::marker_bounding_box(float stroke_width) const
{
    return m_element->marker_bounding_box(m_origin, m_angle, stroke_width);
}

void svg_marker_position::render_marker(svg_render_state& state, float stroke_width) const
{
    m_element->render_marker(state, m_origin, m_angle, stroke_width);
}

svg_geometry_element::svg_geometry_element(document* dom, element_id id)
    : svg_graphics_element(dom, id)
{
}

rect svg_geometry_element::stroke_bounding_box() const
{
    auto stroke_bounding_box = fill_bounding_box();
    if(m_stroke.is_renderable()) {
        float cap_limit = m_stroke_data.line_width() / 2.f;
        if(m_stroke_data.line_cap_() == line_cap::square)
            cap_limit *= NOVA_SQRT2;
        float join_limit = m_stroke_data.line_width() / 2.f;
        if(m_stroke_data.line_join_() == line_join::miter) {
            join_limit *= m_stroke_data.miter_limit();
        }

        stroke_bounding_box.inflate(STB_MAX(cap_limit, join_limit));
    }

    for(const auto& marker_position : m_marker_positions)
        stroke_bounding_box.unite(marker_position.marker_bounding_box(m_stroke_data.line_width()));
    return stroke_bounding_box;
}

void svg_geometry_element::layout_element(const svg_layout_state& state)
{
    m_fill_rule = state.fill_rule_();
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
    auto marker_start = get_marker(state.marker_start());
    auto marker_mid = get_marker(state.marker_mid());
    auto marker_end = get_marker(state.marker_end());
    if(marker_start == nullptr && marker_mid == nullptr && marker_end == nullptr) {
        return;
    }

    point origin;
    point start_point;
    point inslope_points[2];
    point outslope_points[2];

    int index = 0;
    std::array<point, 3> points;
    outline_walker it(m_path);
    while(!it.done()) {
        switch(it.current_segment(points)) {
        case segment_kind::move_to:
            start_point = points[0];
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
            origin = start_point;
            start_point = point();
            break;
        }

        it.next();

        if(!it.done() && (marker_start || marker_mid)) {
            it.current_segment(points);
            outslope_points[0] = origin;
            outslope_points[1] = points[0];
            if(index == 0 && marker_start) {
                auto slope = outslope_points[1] - outslope_points[0];
                auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
                const auto& orient = marker_start->orient();
                if(orient.orient_type_() == svg_angle::orient_type::auto_start_reverse)
                    angle -= 180.f;
                positions.emplace_back(marker_start, origin, angle);
            }

            if(index > 0 && marker_mid) {
                auto inslope = inslope_points[1] - inslope_points[0];
                auto outslope = outslope_points[1] - outslope_points[0];
                auto inangle = 180.f * std::atan2(inslope.y, inslope.x) / NOVA_PI;
                auto outangle = 180.f * std::atan2(outslope.y, outslope.x) / NOVA_PI;
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                positions.emplace_back(marker_mid, origin, angle);
            }
        }

        if(marker_end && it.done()) {
            auto slope = inslope_points[1] - inslope_points[0];
            auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVA_PI;
            positions.emplace_back(marker_end, origin, angle);
        }

        index += 1;
    }
}

void svg_geometry_element::render(svg_render_state& state) const
{
    if(!is_renderable())
        return;
    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    if(new_state.mode() == svg_render_mode::clipping) {
        new_state->set_color(stb::color::white());
        new_state->fill_path(m_path, m_clip_rule, new_state.current_transform());
    } else {
        if(m_fill.apply_paint(new_state))
            new_state->fill_path(m_path, m_fill_rule, new_state.current_transform());
        if(m_stroke.apply_paint(new_state)) {
            new_state->stroke_path(m_path, m_stroke_data, new_state.current_transform());
        }

        for(const auto& marker_position : m_marker_positions) {
            marker_position.render_marker(new_state, m_stroke_data.line_width());
        }
    }

    new_state.end_group(blend_info);
}

svg_line_element::svg_line_element(document* dom)
    : svg_geometry_element(dom, element_id::line)
    , m_x1(property_id::X1, length_direction::horizontal, length_negative_mode::allow)
    , m_y1(property_id::Y1, length_direction::vertical, length_negative_mode::allow)
    , m_x2(property_id::X2, length_direction::horizontal, length_negative_mode::allow)
    , m_y2(property_id::Y2, length_direction::vertical, length_negative_mode::allow)
{
    add_property(m_x1);
    add_property(m_y1);
    add_property(m_x2);
    add_property(m_y2);
}

rect svg_line_element::update_shape(stb::outline& path_)
{
    length_context length_context_(this);
    auto x1 = length_context_.value_for_length(m_x1);
    auto y1 = length_context_.value_for_length(m_y1);
    auto x2 = length_context_.value_for_length(m_x2);
    auto y2 = length_context_.value_for_length(m_y2);

    path_.move_to(x1, y1);
    path_.line_to(x2, y2);
    return rect(x1, y1, x2 - x1, y2 - y1);
}

svg_rect_element::svg_rect_element(document* dom)
    : svg_geometry_element(dom, element_id::rect)
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow)
    , m_width(property_id::width, length_direction::horizontal, length_negative_mode::forbid)
    , m_height(property_id::height, length_direction::vertical, length_negative_mode::forbid)
    , m_rx(property_id::rx, length_direction::horizontal, length_negative_mode::forbid)
    , m_ry(property_id::ry, length_direction::vertical, length_negative_mode::forbid)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_width);
    add_property(m_height);
    add_property(m_rx);
    add_property(m_ry);
}

rect svg_rect_element::update_shape(stb::outline& path_)
{
    length_context length_context_(this);
    auto width = length_context_.value_for_length(m_width);
    auto height = length_context_.value_for_length(m_height);
    if(width <= 0.f || height <= 0.f) {
        return rect::empty_rect;
    }

    auto x = length_context_.value_for_length(m_x);
    auto y = length_context_.value_for_length(m_y);

    auto rx = length_context_.value_for_length(m_rx);
    auto ry = length_context_.value_for_length(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = STB_MIN(rx, width / 2.f);
    ry = STB_MIN(ry, height / 2.f);

    path_.add_rounded_rect(x, y, width, height, rx, ry);
    return rect(x, y, width, height);
}

svg_ellipse_element::svg_ellipse_element(document* dom)
    : svg_geometry_element(dom, element_id::ellipse)
    , m_cx(property_id::cx, length_direction::horizontal, length_negative_mode::allow)
    , m_cy(property_id::cy, length_direction::vertical, length_negative_mode::allow)
    , m_rx(property_id::rx, length_direction::diagonal, length_negative_mode::forbid)
    , m_ry(property_id::ry, length_direction::diagonal, length_negative_mode::forbid)
{
    add_property(m_cx);
    add_property(m_cy);
    add_property(m_rx);
    add_property(m_ry);
}

rect svg_ellipse_element::update_shape(stb::outline& path_)
{
    length_context length_context_(this);
    auto rx = length_context_.value_for_length(m_rx);
    auto ry = length_context_.value_for_length(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return rect::empty_rect;
    }

    auto cx = length_context_.value_for_length(m_cx);
    auto cy = length_context_.value_for_length(m_cy);
    path_.add_ellipse(cx, cy, rx, ry);
    return rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

svg_circle_element::svg_circle_element(document* dom)
    : svg_geometry_element(dom, element_id::circle)
    , m_cx(property_id::cx, length_direction::horizontal, length_negative_mode::allow)
    , m_cy(property_id::cy, length_direction::vertical, length_negative_mode::allow)
    , m_r(property_id::R, length_direction::diagonal, length_negative_mode::forbid)
{
    add_property(m_cx);
    add_property(m_cy);
    add_property(m_r);
}

rect svg_circle_element::update_shape(stb::outline& path_)
{
    length_context length_context_(this);
    auto r = length_context_.value_for_length(m_r);
    if(r <= 0.f) {
        return rect::empty_rect;
    }

    auto cx = length_context_.value_for_length(m_cx);
    auto cy = length_context_.value_for_length(m_cy);
    path_.add_ellipse(cx, cy, r, r);
    return rect(cx - r, cy - r, r + r, r + r);
}

svg_poly_element::svg_poly_element(document* dom, element_id id)
    : svg_geometry_element(dom, id)
    , m_points(property_id::points)
{
    add_property(m_points);
}

rect svg_poly_element::update_shape(stb::outline& path_)
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return rect::empty_rect;
    }

    path_.move_to(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path_.line_to(points[i].x, points[i].y);
    }

    if(id() == element_id::polygon)
        path_.close();
    return bounding_rect(path_);
}

svg_path_element::svg_path_element(document* dom)
    : svg_geometry_element(dom, element_id::path)
    , m_d(property_id::D)
{
    add_property(m_d);
}

rect svg_path_element::update_shape(stb::outline& path_)
{
    path_ = m_d.value();
    return bounding_rect(path_);
}

} // namespace nova::svg
