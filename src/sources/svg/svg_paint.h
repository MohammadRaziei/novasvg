#pragma once

// Render state, blend info, paint elements: stop, gradients, pattern, marker position.

#include "svg_elements.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

enum class svg_render_mode {
    Painting,
    Clipping
};

class svg_blend_info {
public:
    explicit svg_blend_info(const svg_element* el);
    svg_blend_info(const svg_clip_path_element* clipper, const svg_mask_element* masker, const svg_filter_element* filter, float opacity,
                 MixBlendMode mixBlendMode = MixBlendMode::Normal)
        : m_clipper(clipper), m_masker(masker), m_filter(filter), m_opacity(opacity), m_mixBlendMode(mixBlendMode)
    {}

    bool requiresCompositing(svg_render_mode mode) const;
    const svg_clip_path_element* clipper() const { return m_clipper; }
    const svg_mask_element* masker() const { return m_masker; }
    const svg_filter_element* filterElement() const { return m_filter; }
    float opacity() const { return m_opacity; }
    MixBlendMode mixBlendMode() const { return m_mixBlendMode; }

private:
    const svg_clip_path_element* m_clipper;
    const svg_mask_element* m_masker;
    const svg_filter_element* m_filter;
    const float m_opacity;
    const MixBlendMode m_mixBlendMode;
};

class svg_render_state {
public:
    svg_render_state(const svg_element* el, const svg_render_state& parent, const Transform& local_transform)
        : m_element(el), m_parent(&parent), m_currentTransform(parent.currentTransform() * local_transform)
        , m_mode(parent.mode()), m_canvas(parent.m_canvas)
    {}

    svg_render_state(const svg_element* el, const svg_render_state* parent, const Transform& currentTransform, svg_render_mode mode, std::shared_ptr<canvas> canvas)
        : m_element(el), m_parent(parent), m_currentTransform(currentTransform), m_mode(mode), m_canvas(std::move(canvas))
    {}

    canvas& operator*() const { return *m_canvas; }
    canvas* operator->() const { return &*m_canvas; }

    const svg_element* el() const { return m_element; }
    const svg_render_state* parent() const { return m_parent; }
    const Transform& currentTransform() const { return m_currentTransform; }
    const svg_render_mode mode() const { return m_mode; }
    // The pixel canvas this state draws on, or null when the target is a vector page (pdf_canvas).
    std::shared_ptr<raster_canvas> raster() const { return std::dynamic_pointer_cast<raster_canvas>(m_canvas); }

    Rect fillBoundingBox() const { return m_element->fillBoundingBox(); }
    Rect paint_bounding_box() const { return m_element->paint_bounding_box(); }

    bool hasCycleReference(const svg_element* el) const;

    void beginGroup(const svg_blend_info& blendInfo);
    void endGroup(const svg_blend_info& blendInfo);

private:
    // What beginGroup() made for this element: nothing (a save()), a pixel layer, a transparency group on a pdf_canvas,
    // or a pixel layer that goes onto a pdf_canvas as an image when the group ends.
    enum class layer_kind { none, raster, pdf_group, raster_in_pdf };

    const svg_element* m_element;
    const svg_render_state* m_parent;
    Transform m_currentTransform;
    const svg_render_mode m_mode;
    std::shared_ptr<canvas> m_canvas;
    layer_kind m_layer = layer_kind::none;
    float m_layerScale = 1.f; // raster_in_pdf: pixels per user unit of the layer
};

// ---- svgpaintelement ----

class svg_paint_element : public svg_element {
public:
    svg_paint_element(document* dom, ElementID id);

    bool isPaintElement() const final { return true; }

    virtual bool applyPaint(svg_render_state& state, float opacity) const = 0;
};

class svg_stop_element final : public svg_element {
public:
    svg_stop_element(document* dom);

    void layoutElement(const svg_layout_state& state) final;
    const svg_number_percentage& offset() const { return m_offset; }
    GradientStop gradientStop(float opacity) const;

private:
    svg_number_percentage m_offset;
    stb::color m_stop_color = stb::color::black();
    float m_stop_opacity = 1.f;
};

class svg_gradient_attributes;

class svg_gradient_element : public svg_paint_element, public svg_uri_reference {
public:
    svg_gradient_element(document* dom, ElementID id);

    const svg_transform& gradientTransform() const { return m_gradientTransform; }
    const svg_enumeration<Units>& gradientUnits() const { return m_gradientUnits; }
    const svg_enumeration<SpreadMethod>& spreadMethod() const { return m_spreadMethod; }
    void collectGradientAttributes(svg_gradient_attributes& attributes) const;

private:
    svg_transform m_gradientTransform;
    svg_enumeration<Units> m_gradientUnits;
    svg_enumeration<SpreadMethod> m_spreadMethod;
};

class svg_gradient_attributes {
public:
    svg_gradient_attributes() = default;

    const Transform& gradientTransform() const { return m_gradientTransform->gradientTransform().value(); }
    SpreadMethod spreadMethod() const { return m_spreadMethod->spreadMethod().value(); }
    Units gradientUnits() const { return m_gradientUnits->gradientUnits().value(); }
    const svg_gradient_element* gradientContentElement() const { return m_gradientContentElement; }

    bool hasGradientTransform() const { return m_gradientTransform; }
    bool hasSpreadMethod() const { return m_spreadMethod; }
    bool hasGradientUnits() const { return m_gradientUnits; }
    bool hasGradientContentElement() const { return m_gradientContentElement; }

    void setGradientTransform(const svg_gradient_element* value) { m_gradientTransform = value; }
    void setSpreadMethod(const svg_gradient_element* value) { m_spreadMethod = value; }
    void setGradientUnits(const svg_gradient_element* value) { m_gradientUnits = value; }
    void setGradientContentElement(const svg_gradient_element* value) { m_gradientContentElement = value; }

    void setDefaultValues(const svg_gradient_element* el) {
        if(!m_gradientTransform) { m_gradientTransform = el; }
        if(!m_spreadMethod) { m_spreadMethod = el; }
        if(!m_gradientUnits) { m_gradientUnits = el; }
        if(!m_gradientContentElement) { m_gradientContentElement = el; }
    }

private:
    const svg_gradient_element* m_gradientTransform{nullptr};
    const svg_gradient_element* m_spreadMethod{nullptr};
    const svg_gradient_element* m_gradientUnits{nullptr};
    const svg_gradient_element* m_gradientContentElement{nullptr};
};

class svg_linear_gradient_attributes;

class svg_linear_gradient_element final : public svg_gradient_element {
public:
    svg_linear_gradient_element(document* dom);

    const svg_length& x1() const { return m_x1; }
    const svg_length& y1() const { return m_y1; }
    const svg_length& x2() const { return m_x2; }
    const svg_length& y2() const { return m_y2; }

    bool applyPaint(svg_render_state& state, float opacity) const final;

private:
    svg_linear_gradient_attributes collectGradientAttributes() const;
    svg_length m_x1;
    svg_length m_y1;
    svg_length m_x2;
    svg_length m_y2;
};

class svg_linear_gradient_attributes : public svg_gradient_attributes {
public:
    svg_linear_gradient_attributes() = default;

    const svg_length& x1() const { return m_x1->x1(); }
    const svg_length& y1() const { return m_y1->y1(); }
    const svg_length& x2() const { return m_x2->x2(); }
    const svg_length& y2() const { return m_y2->y2(); }

    bool hasX1() const { return m_x1; }
    bool hasY1() const { return m_y1; }
    bool hasX2() const { return m_x2; }
    bool hasY2() const { return m_y2; }

    void setX1(const svg_linear_gradient_element* value) { m_x1 = value; }
    void setY1(const svg_linear_gradient_element* value) { m_y1 = value; }
    void setX2(const svg_linear_gradient_element* value) { m_x2 = value; }
    void setY2(const svg_linear_gradient_element* value) { m_y2 = value; }

    void setDefaultValues(const svg_linear_gradient_element* el) {
        svg_gradient_attributes::setDefaultValues(el);
        if(!m_x1) { m_x1 = el; }
        if(!m_y1) { m_y1 = el; }
        if(!m_x2) { m_x2 = el; }
        if(!m_y2) { m_y2 = el; }
    }

private:
    const svg_linear_gradient_element* m_x1{nullptr};
    const svg_linear_gradient_element* m_y1{nullptr};
    const svg_linear_gradient_element* m_x2{nullptr};
    const svg_linear_gradient_element* m_y2{nullptr};
};

class svg_radial_gradient_attributes;

class svg_radial_gradient_element final : public svg_gradient_element {
public:
    svg_radial_gradient_element(document* dom);

    const svg_length& cx() const { return m_cx; }
    const svg_length& cy() const { return m_cy; }
    const svg_length& r() const { return m_r; }
    const svg_length& fx() const { return m_fx; }
    const svg_length& fy() const { return m_fy; }

    bool applyPaint(svg_render_state& state, float opacity) const final;

private:
    svg_radial_gradient_attributes collectGradientAttributes() const;
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_r;
    svg_length m_fx;
    svg_length m_fy;
};

class svg_radial_gradient_attributes : public svg_gradient_attributes {
public:
    svg_radial_gradient_attributes() = default;

    const svg_length& cx() const { return m_cx->cx(); }
    const svg_length& cy() const { return m_cy->cy(); }
    const svg_length& r() const { return m_r->r(); }
    const svg_length& fx() const { return m_fx ? m_fx->fx() : m_cx->cx(); }
    const svg_length& fy() const { return m_fy ? m_fy->fy() : m_cy->cy(); }

    bool hasCx() const { return m_cx; }
    bool hasCy() const { return m_cy; }
    bool hasR() const { return m_r; }
    bool hasFx() const { return m_fx; }
    bool hasFy() const { return m_fy; }

    void setCx(const svg_radial_gradient_element* value) { m_cx = value; }
    void setCy(const svg_radial_gradient_element* value) { m_cy = value; }
    void setR(const svg_radial_gradient_element* value) { m_r = value; }
    void setFx(const svg_radial_gradient_element* value) { m_fx = value; }
    void setFy(const svg_radial_gradient_element* value) { m_fy = value; }

    void setDefaultValues(const svg_radial_gradient_element* el) {
        svg_gradient_attributes::setDefaultValues(el);
        if(!m_cx) { m_cx = el; }
        if(!m_cy) { m_cy = el; }
        if(!m_r) { m_r = el; }
    }

private:
    const svg_radial_gradient_element* m_cx{nullptr};
    const svg_radial_gradient_element* m_cy{nullptr};
    const svg_radial_gradient_element* m_r{nullptr};
    const svg_radial_gradient_element* m_fx{nullptr};
    const svg_radial_gradient_element* m_fy{nullptr};
};

class svg_pattern_attributes;

class svg_pattern_element final : public svg_paint_element, public svg_uri_reference, public svg_fit_to_view_box {
public:
    svg_pattern_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const svg_transform& patternTransform() const { return m_patternTransform; }
    const svg_enumeration<Units>& patternUnits() const { return m_patternUnits; }
    const svg_enumeration<Units>& patternContentUnits() const { return m_patternContentUnits; }

    bool applyPaint(svg_render_state& state, float opacity) const final;

private:
    svg_pattern_attributes collectPatternAttributes() const;
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_transform m_patternTransform;
    svg_enumeration<Units> m_patternUnits;
    svg_enumeration<Units> m_patternContentUnits;
};

class svg_pattern_attributes {
public:
    svg_pattern_attributes() = default;

    const svg_length& x() const { return m_x->x(); }
    const svg_length& y() const { return m_y->y(); }
    const svg_length& width() const { return m_width->width(); }
    const svg_length& height() const { return m_height->height(); }
    const Transform& patternTransform() const { return m_patternTransform->patternTransform().value(); }
    Units patternUnits() const { return m_patternUnits->patternUnits().value(); }
    Units patternContentUnits() const { return m_patternContentUnits->patternContentUnits().value(); }
    const Rect& viewBox() const { return m_viewBox->viewBox().value(); }
    const svg_preserve_aspect_ratio& preserveAspectRatio() const { return m_preserveAspectRatio->preserveAspectRatio(); }
    const svg_pattern_element* patternContentElement() const { return m_patternContentElement; }

    bool hasX() const { return m_x; }
    bool hasY() const { return m_y; }
    bool hasWidth() const { return m_width; }
    bool hasHeight() const { return m_height; }
    bool hasPatternTransform() const { return m_patternTransform; }
    bool hasPatternUnits() const { return m_patternUnits; }
    bool hasPatternContentUnits() const { return m_patternContentUnits; }
    bool hasViewBox() const { return m_viewBox; }
    bool hasPreserveAspectRatio() const { return m_preserveAspectRatio; }
    bool hasPatternContentElement() const { return m_patternContentElement; }

    void setX(const svg_pattern_element* value) { m_x = value; }
    void setY(const svg_pattern_element* value) { m_y = value; }
    void setWidth(const svg_pattern_element* value) { m_width = value; }
    void setHeight(const svg_pattern_element* value) { m_height = value; }
    void setPatternTransform(const svg_pattern_element* value) { m_patternTransform = value; }
    void setPatternUnits(const svg_pattern_element* value) { m_patternUnits = value; }
    void setPatternContentUnits(const svg_pattern_element* value) { m_patternContentUnits = value; }
    void setViewBox(const svg_pattern_element* value) { m_viewBox = value; }
    void setPreserveAspectRatio(const svg_pattern_element* value) { m_preserveAspectRatio = value; }
    void setPatternContentElement(const svg_pattern_element* value) { m_patternContentElement = value; }

    void setDefaultValues(const svg_pattern_element* el) {
        if(!m_x) { m_x = el; }
        if(!m_y) { m_y = el; }
        if(!m_width) { m_width = el; }
        if(!m_height) { m_height = el; }
        if(!m_patternTransform) { m_patternTransform = el; }
        if(!m_patternUnits) { m_patternUnits = el; }
        if(!m_patternContentUnits) { m_patternContentUnits = el; }
        if(!m_viewBox) { m_viewBox = el; }
        if(!m_preserveAspectRatio) { m_preserveAspectRatio = el; }
        if(!m_patternContentElement) { m_patternContentElement = el; }
    }

private:
    const svg_pattern_element* m_x{nullptr};
    const svg_pattern_element* m_y{nullptr};
    const svg_pattern_element* m_width{nullptr};
    const svg_pattern_element* m_height{nullptr};
    const svg_pattern_element* m_patternTransform{nullptr};
    const svg_pattern_element* m_patternUnits{nullptr};
    const svg_pattern_element* m_patternContentUnits{nullptr};
    const svg_pattern_element* m_viewBox{nullptr};
    const svg_pattern_element* m_preserveAspectRatio{nullptr};
    const svg_pattern_element* m_patternContentElement{nullptr};
};

// ---- svggeometryelement ----

class svg_marker_position {
public:
    svg_marker_position(const svg_marker_element* el, const Point& origin, float angle)
        : m_element(el), m_origin(origin), m_angle(angle)
    {}

    const svg_marker_element* el() const { return m_element; }
    const Point& origin() const { return m_origin; }
    float angle() const { return m_angle; }

    Rect markerBoundingBox(float strokeWidth) const;
    void renderMarker(svg_render_state& state, float strokeWidth) const;

private:
    const svg_marker_element* m_element;
    Point m_origin;
    float m_angle;
};

using svg_marker_position_list = std::vector<svg_marker_position>;

} // namespace nova::svg
