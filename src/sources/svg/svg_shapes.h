#pragma once

// Geometry elements: line, rect, ellipse, circle, polyline/polygon, path.

#include "svg_paint.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

class svg_geometry_element : public svg_graphics_element {
public:
    svg_geometry_element(document* dom, ElementID id);

    bool isGeometryElement() const final { return true; }

    Rect fillBoundingBox() const override { return m_fillBoundingBox; }
    Rect strokeBoundingBox() const override;
    void layoutElement(const svg_layout_state& state) override;

    bool isRenderable() const { return !m_path.empty() && !isDisplayNone() && !isVisibilityHidden(); }

    FillRule fill_rule() const { return m_fill_rule; }
    FillRule clip_rule() const { return m_clip_rule; }

    virtual Rect updateShape(stb::outline& path) = 0;

    void updateMarkerPositions(svg_marker_position_list& positions, const svg_layout_state& state);
    void render(svg_render_state& state) const override;

    const stb::outline& path() const { return m_path; }

private:
    stb::outline m_path;
    Rect m_fillBoundingBox;
    StrokeData m_strokeData;

    svg_paint_server m_fill;
    svg_paint_server m_stroke;
    svg_marker_position_list m_markerPositions;

    FillRule m_fill_rule = FillRule::NonZero;
    FillRule m_clip_rule = FillRule::NonZero;
};

class svg_line_element final : public svg_geometry_element {
public:
    svg_line_element(document* dom);

    Rect updateShape(stb::outline& path) final;

private:
    svg_length m_x1;
    svg_length m_y1;
    svg_length m_x2;
    svg_length m_y2;
};

class svg_rect_element final : public svg_geometry_element {
public:
    svg_rect_element(document* dom);

    Rect updateShape(stb::outline& path) final;

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

    Rect updateShape(stb::outline& path) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_rx;
    svg_length m_ry;
};

class svg_circle_element final : public svg_geometry_element {
public:
    svg_circle_element(document* dom);

    Rect updateShape(stb::outline& path) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_r;
};

class svg_poly_element final : public svg_geometry_element {
public:
    svg_poly_element(document* dom, ElementID id);

    Rect updateShape(stb::outline& path) final;

private:
    svg_point_list m_points;
};

class svg_path_element final : public svg_geometry_element {
public:
    svg_path_element(document* dom);

    Rect updateShape(stb::outline& path) final;

private:
    svg_path m_d;
};

} // namespace nova::svg
