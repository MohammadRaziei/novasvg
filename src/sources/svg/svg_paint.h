#pragma once

// Render state, blend info, paint elements: stop, gradients, pattern, marker position.

#include "svg_elements.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

enum class svg_render_mode {
    painting,
    clipping
};

class svg_blend_info {
public:
    explicit svg_blend_info(const svg_element* el);
    svg_blend_info(const svg_clip_path_element* clipper, const svg_mask_element* masker, const svg_filter_element* filter, float opacity,
                 css_mix_blend_mode mix_blend_mode_ = css_mix_blend_mode::normal)
        : m_clipper(clipper), m_masker(masker), m_filter(filter), m_opacity(opacity), m_mix_blend_mode(mix_blend_mode_)
    {}

    bool requires_compositing(svg_render_mode mode) const;
    const svg_clip_path_element* clipper() const { return m_clipper; }
    const svg_mask_element* masker() const { return m_masker; }
    const svg_filter_element* filter_element() const { return m_filter; }
    float opacity() const { return m_opacity; }
    css_mix_blend_mode mix_blend_mode() const { return m_mix_blend_mode; }

private:
    const svg_clip_path_element* m_clipper;
    const svg_mask_element* m_masker;
    const svg_filter_element* m_filter;
    const float m_opacity;
    const css_mix_blend_mode m_mix_blend_mode;
};

class svg_render_state {
public:
    svg_render_state(const svg_element* el, const svg_render_state& parent, const transform& local_transform)
        : m_element(el), m_parent(&parent), m_current_transform(parent.current_transform() * local_transform)
        , m_mode(parent.mode()), m_canvas(parent.m_canvas)
    {}

    svg_render_state(const svg_element* el, const svg_render_state* parent, const transform& current_transform, svg_render_mode mode, std::shared_ptr<canvas> canvas)
        : m_element(el), m_parent(parent), m_current_transform(current_transform), m_mode(mode), m_canvas(std::move(canvas))
    {}

    canvas& operator*() const { return *m_canvas; }
    canvas* operator->() const { return &*m_canvas; }

    const svg_element* el() const { return m_element; }
    const svg_render_state* parent() const { return m_parent; }
    const transform& current_transform() const { return m_current_transform; }
    const svg_render_mode mode() const { return m_mode; }
    // The pixel canvas this state draws on, or null when the target is a vector page (pdf_canvas).
    std::shared_ptr<raster_canvas> raster() const { return std::dynamic_pointer_cast<raster_canvas>(m_canvas); }

    rect fill_bounding_box() const { return m_element->fill_bounding_box(); }
    rect paint_bounding_box() const { return m_element->paint_bounding_box(); }

    bool has_cycle_reference(const svg_element* el) const;

    void begin_group(const svg_blend_info& blend_info);
    void end_group(const svg_blend_info& blend_info);

private:
    // What beginGroup() made for this element: nothing (a save()), a pixel layer, a transparency group on a pdf_canvas,
    // or a pixel layer that goes onto a pdf_canvas as an image when the group ends.
    enum class layer_kind { none, raster, pdf_group, raster_in_pdf };

    const svg_element* m_element;
    const svg_render_state* m_parent;
    transform m_current_transform;
    const svg_render_mode m_mode;
    std::shared_ptr<canvas> m_canvas;
    layer_kind m_layer = layer_kind::none;
    float m_layer_scale = 1.f; // raster_in_pdf: pixels per user unit of the layer
};

// ---- svgpaintelement ----

class svg_paint_element : public svg_element {
public:
    svg_paint_element(document* dom, element_id id);

    bool is_paint_element() const final { return true; }

    virtual bool apply_paint(svg_render_state& state, float opacity) const = 0;
};

class svg_stop_element final : public svg_element {
public:
    svg_stop_element(document* dom);

    void layout_element(const svg_layout_state& state) final;
    const svg_number_percentage& offset() const { return m_offset; }
    gradient_stop make_gradient_stop(float opacity) const;

private:
    svg_number_percentage m_offset;
    stb::color m_stop_color = stb::color::black();
    float m_stop_opacity = 1.f;
};

class svg_gradient_attributes;

class svg_gradient_element : public svg_paint_element, public svg_uri_reference {
public:
    svg_gradient_element(document* dom, element_id id);

    const svg_transform& gradient_transform() const { return m_gradient_transform; }
    const svg_enumeration<units>& gradient_units() const { return m_gradient_units; }
    const svg_enumeration<spread_method_value>& spread_method() const { return m_spread_method; }
    void collect_gradient_attributes(svg_gradient_attributes& attributes) const;

private:
    svg_transform m_gradient_transform;
    svg_enumeration<units> m_gradient_units;
    svg_enumeration<spread_method_value> m_spread_method;
};

class svg_gradient_attributes {
public:
    svg_gradient_attributes() = default;

    const transform& gradient_transform() const { return m_gradient_transform->gradient_transform().value(); }
    spread_method_value spread_method() const { return m_spread_method->spread_method().value(); }
    units gradient_units() const { return m_gradient_units->gradient_units().value(); }
    const svg_gradient_element* gradient_content_element() const { return m_gradient_content_element; }

    bool has_gradient_transform() const { return m_gradient_transform; }
    bool has_spread_method() const { return m_spread_method; }
    bool has_gradient_units() const { return m_gradient_units; }
    bool has_gradient_content_element() const { return m_gradient_content_element; }

    void set_gradient_transform(const svg_gradient_element* value) { m_gradient_transform = value; }
    void set_spread_method(const svg_gradient_element* value) { m_spread_method = value; }
    void set_gradient_units(const svg_gradient_element* value) { m_gradient_units = value; }
    void set_gradient_content_element(const svg_gradient_element* value) { m_gradient_content_element = value; }

    void set_default_values(const svg_gradient_element* el) {
        if(!m_gradient_transform) { m_gradient_transform = el; }
        if(!m_spread_method) { m_spread_method = el; }
        if(!m_gradient_units) { m_gradient_units = el; }
        if(!m_gradient_content_element) { m_gradient_content_element = el; }
    }

private:
    const svg_gradient_element* m_gradient_transform{nullptr};
    const svg_gradient_element* m_spread_method{nullptr};
    const svg_gradient_element* m_gradient_units{nullptr};
    const svg_gradient_element* m_gradient_content_element{nullptr};
};

class svg_linear_gradient_attributes;

class svg_linear_gradient_element final : public svg_gradient_element {
public:
    svg_linear_gradient_element(document* dom);

    const svg_length& x1() const { return m_x1; }
    const svg_length& y1() const { return m_y1; }
    const svg_length& x2() const { return m_x2; }
    const svg_length& y2() const { return m_y2; }

    bool apply_paint(svg_render_state& state, float opacity) const final;

private:
    svg_linear_gradient_attributes collect_gradient_attributes() const;
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

    bool has_x1() const { return m_x1; }
    bool has_y1() const { return m_y1; }
    bool has_x2() const { return m_x2; }
    bool has_y2() const { return m_y2; }

    void set_x1(const svg_linear_gradient_element* value) { m_x1 = value; }
    void set_y1(const svg_linear_gradient_element* value) { m_y1 = value; }
    void set_x2(const svg_linear_gradient_element* value) { m_x2 = value; }
    void set_y2(const svg_linear_gradient_element* value) { m_y2 = value; }

    void set_default_values(const svg_linear_gradient_element* el) {
        svg_gradient_attributes::set_default_values(el);
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

    bool apply_paint(svg_render_state& state, float opacity) const final;

private:
    svg_radial_gradient_attributes collect_gradient_attributes() const;
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

    bool has_cx() const { return m_cx; }
    bool has_cy() const { return m_cy; }
    bool has_r() const { return m_r; }
    bool has_fx() const { return m_fx; }
    bool has_fy() const { return m_fy; }

    void set_cx(const svg_radial_gradient_element* value) { m_cx = value; }
    void set_cy(const svg_radial_gradient_element* value) { m_cy = value; }
    void set_r(const svg_radial_gradient_element* value) { m_r = value; }
    void set_fx(const svg_radial_gradient_element* value) { m_fx = value; }
    void set_fy(const svg_radial_gradient_element* value) { m_fy = value; }

    void set_default_values(const svg_radial_gradient_element* el) {
        svg_gradient_attributes::set_default_values(el);
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
    const svg_transform& pattern_transform() const { return m_pattern_transform; }
    const svg_enumeration<units>& pattern_units() const { return m_pattern_units; }
    const svg_enumeration<units>& pattern_content_units() const { return m_pattern_content_units; }

    bool apply_paint(svg_render_state& state, float opacity) const final;

private:
    svg_pattern_attributes collect_pattern_attributes() const;
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_transform m_pattern_transform;
    svg_enumeration<units> m_pattern_units;
    svg_enumeration<units> m_pattern_content_units;
};

class svg_pattern_attributes {
public:
    svg_pattern_attributes() = default;

    const svg_length& x() const { return m_x->x(); }
    const svg_length& y() const { return m_y->y(); }
    const svg_length& width() const { return m_width->width(); }
    const svg_length& height() const { return m_height->height(); }
    const transform& pattern_transform() const { return m_pattern_transform->pattern_transform().value(); }
    units pattern_units() const { return m_pattern_units->pattern_units().value(); }
    units pattern_content_units() const { return m_pattern_content_units->pattern_content_units().value(); }
    const rect& view_box() const { return m_view_box->view_box().value(); }
    const svg_preserve_aspect_ratio& preserve_aspect_ratio() const { return m_preserve_aspect_ratio->preserve_aspect_ratio(); }
    const svg_pattern_element* pattern_content_element() const { return m_pattern_content_element; }

    bool has_x() const { return m_x; }
    bool has_y() const { return m_y; }
    bool has_width() const { return m_width; }
    bool has_height() const { return m_height; }
    bool has_pattern_transform() const { return m_pattern_transform; }
    bool has_pattern_units() const { return m_pattern_units; }
    bool has_pattern_content_units() const { return m_pattern_content_units; }
    bool has_view_box() const { return m_view_box; }
    bool has_preserve_aspect_ratio() const { return m_preserve_aspect_ratio; }
    bool has_pattern_content_element() const { return m_pattern_content_element; }

    void set_x(const svg_pattern_element* value) { m_x = value; }
    void set_y(const svg_pattern_element* value) { m_y = value; }
    void set_width(const svg_pattern_element* value) { m_width = value; }
    void set_height(const svg_pattern_element* value) { m_height = value; }
    void set_pattern_transform(const svg_pattern_element* value) { m_pattern_transform = value; }
    void set_pattern_units(const svg_pattern_element* value) { m_pattern_units = value; }
    void set_pattern_content_units(const svg_pattern_element* value) { m_pattern_content_units = value; }
    void set_view_box(const svg_pattern_element* value) { m_view_box = value; }
    void set_preserve_aspect_ratio(const svg_pattern_element* value) { m_preserve_aspect_ratio = value; }
    void set_pattern_content_element(const svg_pattern_element* value) { m_pattern_content_element = value; }

    void set_default_values(const svg_pattern_element* el) {
        if(!m_x) { m_x = el; }
        if(!m_y) { m_y = el; }
        if(!m_width) { m_width = el; }
        if(!m_height) { m_height = el; }
        if(!m_pattern_transform) { m_pattern_transform = el; }
        if(!m_pattern_units) { m_pattern_units = el; }
        if(!m_pattern_content_units) { m_pattern_content_units = el; }
        if(!m_view_box) { m_view_box = el; }
        if(!m_preserve_aspect_ratio) { m_preserve_aspect_ratio = el; }
        if(!m_pattern_content_element) { m_pattern_content_element = el; }
    }

private:
    const svg_pattern_element* m_x{nullptr};
    const svg_pattern_element* m_y{nullptr};
    const svg_pattern_element* m_width{nullptr};
    const svg_pattern_element* m_height{nullptr};
    const svg_pattern_element* m_pattern_transform{nullptr};
    const svg_pattern_element* m_pattern_units{nullptr};
    const svg_pattern_element* m_pattern_content_units{nullptr};
    const svg_pattern_element* m_view_box{nullptr};
    const svg_pattern_element* m_preserve_aspect_ratio{nullptr};
    const svg_pattern_element* m_pattern_content_element{nullptr};
};

// ---- svggeometryelement ----

class svg_marker_position {
public:
    svg_marker_position(const svg_marker_element* el, const point& origin, float angle)
        : m_element(el), m_origin(origin), m_angle(angle)
    {}

    const svg_marker_element* el() const { return m_element; }
    const point& origin() const { return m_origin; }
    float angle() const { return m_angle; }

    rect marker_bounding_box(float stroke_width) const;
    void render_marker(svg_render_state& state, float stroke_width) const;

private:
    const svg_marker_element* m_element;
    point m_origin;
    float m_angle;
};

using svg_marker_position_list = std::vector<svg_marker_position>;

} // namespace nova::svg
