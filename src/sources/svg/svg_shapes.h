#pragma once

// Geometry elements: line, rect, ellipse, circle, polyline/polygon, path.

#include "svg_paint.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

class svg_geometry_element : public svg_graphics_element {
public:
    svg_geometry_element(document* dom, element_id id);

    bool is_geometry_element() const final { return true; }

    rect fill_bounding_box() const override { return m_fill_bounding_box; }
    rect stroke_bounding_box() const override;
    void layout_element(const svg_layout_state& state) override;

    bool is_renderable() const { return !m_path.empty() && !is_display_none() && !is_visibility_hidden(); }

    fill_rule fill_rule_() const { return m_fill_rule; }
    fill_rule clip_rule() const { return m_clip_rule; }

    virtual rect update_shape(stb::outline& path_) = 0;

    void update_marker_positions(svg_marker_position_list& positions, const svg_layout_state& state);
    void render(svg_render_state& state) const override;

    const stb::outline& path_() const { return m_path; }

private:
    stb::outline m_path;
    rect m_fill_bounding_box;
    stroke_data m_stroke_data;

    svg_paint_server m_fill;
    svg_paint_server m_stroke;
    svg_marker_position_list m_marker_positions;

    fill_rule m_fill_rule = fill_rule::non_zero;
    fill_rule m_clip_rule = fill_rule::non_zero;
};

class svg_line_element final : public svg_geometry_element {
public:
    svg_line_element(document* dom);

    rect update_shape(stb::outline& path_) final;

private:
    svg_length m_x1;
    svg_length m_y1;
    svg_length m_x2;
    svg_length m_y2;
};

class svg_rect_element final : public svg_geometry_element {
public:
    svg_rect_element(document* dom);

    rect update_shape(stb::outline& path_) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_length m_rx;
    svg_length m_ry;
};

class svg_ellipse_element final : public svg_geometry_element {
public:
    svg_ellipse_element(document* dom);

    rect update_shape(stb::outline& path_) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_rx;
    svg_length m_ry;
};

class svg_circle_element final : public svg_geometry_element {
public:
    svg_circle_element(document* dom);

    rect update_shape(stb::outline& path_) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_r;
};

class svg_poly_element final : public svg_geometry_element {
public:
    svg_poly_element(document* dom, element_id id);

    rect update_shape(stb::outline& path_) final;

private:
    svg_point_list m_points;
};

class svg_path_element final : public svg_geometry_element {
public:
    svg_path_element(document* dom);

    rect update_shape(stb::outline& path_) final;

private:
    svg_path m_d;
};

} // namespace nova::svg
