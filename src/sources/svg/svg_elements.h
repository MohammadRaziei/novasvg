#pragma once

// The structural elements: svg/g/use/image/foreignObject/symbol/switch/defs, marker, clipPath, mask, filter + fe*.

#include "svg_node.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

class svg_style_element final : public svg_element {
public:
    svg_style_element(document* dom);
};

class svg_fit_to_view_box {
public:
    svg_fit_to_view_box(svg_element* el);

    const svg_rect& view_box() const { return m_view_box; }
    const svg_preserve_aspect_ratio& preserve_aspect_ratio() const { return m_preserve_aspect_ratio; }
    transform view_box_to_view_transform(const size& viewport_size) const;
    rect get_clip_rect(const size& viewport_size) const;

private:
    svg_rect m_view_box;
    svg_preserve_aspect_ratio m_preserve_aspect_ratio;
};

class svg_uri_reference {
public:
    svg_uri_reference(svg_element* el);

    const svg_string& href() const { return m_href; }
    const std::string& href_string() const { return m_href.value(); }
    svg_element* get_target_element(const document* dom) const;

private:
    svg_string m_href;
};

class svg_paint_server {
public:
    svg_paint_server() = default;
    svg_paint_server(const svg_paint_element* el, const stb::color& color_, float opacity)
        : m_element(el), m_color(color_), m_opacity(opacity)
    {}

    bool is_renderable() const { return m_opacity > 0.f && (m_element || m_color.a() > 0); }

    const svg_paint_element* el() const { return m_element; }
    const stb::color& color() const { return  m_color; }
    float opacity() const { return m_opacity; }

    bool apply_paint(svg_render_state& state) const;

private:
    const svg_paint_element* m_element = nullptr;
    stb::color m_color = stb::color::transparent();
    float m_opacity = 0.f;
};

class svg_graphics_element : public svg_element {
public:
    svg_graphics_element(document* dom, element_id id);

    bool is_graphics_element() const final { return true; }

    const svg_transform& transform() const { return m_transform; }
    vg::transform local_transform() const override;

    svg_paint_server get_paint_server(const paint& paint_, float opacity) const;
    stroke_data get_stroke_data(const svg_layout_state& state) const;

private:
    svg_transform m_transform;
    svg_transform_origin m_transform_origin;
    svg_transform_box m_transform_box;
};

class svg_svg_element : public svg_graphics_element, public svg_fit_to_view_box {
public:
    svg_svg_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }

    vg::transform local_transform() const override;
    void render(svg_render_state& state) const override;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
};

class svg_root_element final : public svg_svg_element {
public:
    svg_root_element(document* dom);

    float intrinsic_width() const { return m_intrinsic_width; }
    float intrinsic_height() const { return m_intrinsic_height; }

    void set_needs_layout() { m_intrinsic_width = -1.f; }
    bool needs_layout() const { return m_intrinsic_width == -1.f; }

    svg_root_element* layout_if_needed();

    svg_element* element_by_id(std::string_view id) const;
    void add_element_by_id(const std::string& id, svg_element* el);
    void layout(svg_layout_state& state) final;

    void force_layout();

    // Every stylesheet applied to the dom -- the <style> text found while parsing, then anything
    // passed to document::apply_style_sheet() -- kept verbatim and in order. The HTML inside a
    // <foreignObject> sits outside the svg el tree and so never goes through the resolver that sets
    // svg presentation properties; it is styled from these (see resolveForeignObjectStyle()).
    const std::vector<std::string>& style_sheets() const { return m_style_sheets; }
    void add_style_sheet(std::string text) { m_style_sheets.push_back(std::move(text)); }

private:
    std::map<std::string, svg_element*, std::less<>> m_id_cache;
    float m_intrinsic_width{-1.f};
    float m_intrinsic_height{-1.f};
    std::vector<std::string> m_style_sheets;
};

class svg_use_element final : public svg_graphics_element, public svg_uri_reference {
public:
    svg_use_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }

    vg::transform local_transform() const final;
    void render(svg_render_state& state) const final;
    void build() final;

private:
    std::unique_ptr<svg_element> clone_target_element(svg_element* target_element);
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
};

class svg_image_element final : public svg_graphics_element {
public:
    svg_image_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const svg_preserve_aspect_ratio& preserve_aspect_ratio() const { return m_preserve_aspect_ratio; }
    const pixmap& image() const { return m_image; }

    rect fill_bounding_box() const final;
    rect stroke_bounding_box() const final;
    void render(svg_render_state& state) const final;
    void parse_attribute(property_id id, const std::string& value) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_preserve_aspect_ratio m_preserve_aspect_ratio;
    pixmap m_image;
};

// A <foreignObject>: a box and a piece of markup. What the markup means is the business of the document's
// foreign_object_renderer (nova/svg/foreign_object.h, ADR-0010); this element knows only the box, the font it
// inherits and what the renderer prepared from the CSS.
class svg_foreign_object_element final : public svg_graphics_element {
public:
    // Bold/italic bits of font_style(); also the index into the faces a renderer asks for.
    static constexpr unsigned char style_bold = 1;
    static constexpr unsigned char style_italic = 2;

    svg_foreign_object_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const sized_font& font() const { return m_font; }
    const std::string& font_family() const { return m_font_family; }
    const svg_paint_server& fill() const { return m_fill; }

    /// style_bold / style_italic bits the element itself inherits, i.e. the style of font().
    unsigned char base_font_style() const { return m_base_font_style; }

    /// The markup between <foreignObject ...> and </foreignObject>, exactly as it appeared in the source.
    const std::string& raw_content() const { return m_raw_content; }
    void set_raw_content(std::string content) { m_raw_content = std::move(content); }

    /// What the document's renderer worked out from the CSS (foreign_object_renderer::prepare()).
    const std::shared_ptr<void>& prepared() const { return m_prepared; }
    void set_prepared(std::shared_ptr<void> prepared) { m_prepared = std::move(prepared); }

    rect fill_bounding_box() const final;
    rect stroke_bounding_box() const final;
    void layout_element(const svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    sized_font m_font;
    std::string m_font_family;
    unsigned char m_base_font_style = 0;
    svg_paint_server m_fill;
    std::string m_raw_content;
    std::shared_ptr<void> m_prepared;
};

/// What `renderer` says the markup measures when drawn with `base` (and, with a `family`, the bold / italic variants of
/// it), without rendering anything. For callers that size their own layout from it (the Python binding).
std::optional<foreign_object_size> measure_foreign_object(const foreign_object_renderer& renderer, std::string_view markup, const sized_font& base,
                                                          const std::string& family, bool bold, bool italic, float max_width);

class svg_symbol_element final : public svg_graphics_element, public svg_fit_to_view_box {
public:
    svg_symbol_element(document* dom);
};

class svg_g_element final : public svg_graphics_element {
public:
    svg_g_element(document* dom);

    void render(svg_render_state& state) const final;
};

// <switch>: renders ONLY the first direct child whose conditional-processing attributes all
// evaluate true (a child with none always does), exactly as a browser does. mermaid's journey
// diagram writes every label as `<switch><foreignObject>...</foreignObject><text>...</text></switch>`:
// the foreignObject (which this library paints) wins and the native <text> is only the fallback
// for renderers without foreignObject support. Before this el existed, `switch` was an
// unknown el and its whole subtree -- label included -- was skipped.
class svg_switch_element final : public svg_graphics_element {
public:
    svg_switch_element(document* dom);

    void render(svg_render_state& state) const final;

private:
    const svg_element* selected_child() const;
};

class svg_defs_element final : public svg_graphics_element {
public:
    svg_defs_element(document* dom);
};

class svg_marker_element final : public svg_element, public svg_fit_to_view_box {
public:
    svg_marker_element(document* dom);

    const svg_length& refX() const { return m_ref_x; }
    const svg_length& refY() const { return m_ref_y; }
    const svg_length& marker_width() const { return m_marker_width; }
    const svg_length& marker_height() const { return m_marker_height; }
    const svg_enumeration<svg_marker_units>& marker_units() const { return m_marker_units; }
    const svg_angle& orient() const { return m_orient; }

    point ref_point() const;
    size marker_size() const;

    transform marker_transform(const point& origin, float angle, float stroke_width) const;
    rect marker_bounding_box(const point& origin, float angle, float stroke_width) const;
    void render_marker(svg_render_state& state, const point& origin, float angle, float stroke_width) const;

    transform local_transform() const final;

private:
    svg_length m_ref_x;
    svg_length m_ref_y;
    svg_length m_marker_width;
    svg_length m_marker_height;
    svg_enumeration<svg_marker_units> m_marker_units;
    svg_angle m_orient;
};

class svg_clip_path_element final : public svg_graphics_element {
public:
    svg_clip_path_element(document* dom);

    const svg_enumeration<units>& clip_path_units() const { return m_clip_path_units; }
    rect clip_bounding_box(const svg_element* el) const;

    void apply_clip_mask(svg_render_state& state) const;
    void apply_clip_path(svg_render_state& state) const;

    bool requires_masking() const;

private:
    svg_enumeration<units> m_clip_path_units;
};

class svg_mask_element final : public svg_element {
public:
    svg_mask_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const svg_enumeration<units>& mask_units() const { return m_mask_units; }
    const svg_enumeration<units>& mask_content_units() const { return m_mask_content_units; }

    rect mask_rect(const svg_element* el) const;
    rect mask_bounding_box(const svg_element* el) const;
    void apply_mask(svg_render_state& state) const;

    void layout_element(const svg_layout_state& state) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_enumeration<units> m_mask_units;
    svg_enumeration<units> m_mask_content_units;
    css_mask_type m_mask_type = css_mask_type::luminance;
};

// ---- svgfilterelement ----
// Real svg filter-primitive pipeline: svg_filter_context carries the
// running state (SourceGraphic, named `result=` buffers, implicit
// chaining, device scale); each primitive is its own svg_element subclass
// overriding the applyFilterPrimitive() Template Method; svg_filter_element
// itself is just a pipeline executor with no per-type switch statement --
// it asks each child to compute its own output and threads the result
// through. Adding a new primitive later means adding one small class, not
// touching the executor.
static float parse_first_float(std::string_view input, float fallback)
{
    if(input.empty())
        return fallback;
    return std::strtof(std::string(input).c_str(), nullptr);
}

class svg_filter_context {
public:
    svg_filter_context(const std::shared_ptr<raster_canvas>& source_graphic, float scale)
        : m_source_graphic(source_graphic), m_last_result(source_graphic), m_scale(scale)
    {}

    float scale() const { return m_scale; }

    // A blank transparent canvas matching SourceGraphic's position/size --
    // every primitive's output shares this geometry (ponytail: real UAs
    // allow per-primitive filter regions; one shared region for the whole
    // chain is a deliberate simplification, see checklist.md).
    std::shared_ptr<raster_canvas> new_canvas() const
    {
        return raster_canvas::create(float(m_source_graphic->x()), float(m_source_graphic->y()), float(m_source_graphic->width()), float(m_source_graphic->height()));
    }

    std::shared_ptr<raster_canvas> clone_canvas(const raster_canvas& source) const
    {
        auto canvas = new_canvas();
        canvas->composite_over(source);
        return canvas;
    }

    // Resolves an `in=`/`in2=` reference: empty -> implicit chain (result
    // of the previous primitive, or SourceGraphic for the first one);
    // "SourceGraphic"/"SourceAlpha" -> the original rendered el (full
    // or alpha-only); otherwise a named `result=` from an earlier
    // primitive, falling back to the implicit chain if the name is
    // unresolvable (spec says this is undefined behavior; failing soft
    // beats crashing).
    std::shared_ptr<raster_canvas> resolve_input(std::string_view in) const
    {
        if(in.empty())
            return m_last_result;
        if(in == "SourceGraphic")
            return m_source_graphic;
        if(in == "SourceAlpha") {
            auto alpha = clone_canvas(*m_source_graphic);
            alpha->tint_to_flood_color(stb::color(0, 0, 0), 1.f);
            return alpha;
        }
        auto it = m_named_results.find(std::string(in));
        if(it != m_named_results.end())
            return it->second;
        return m_last_result;
    }

    // Records a primitive's output as the new implicit chain head, and
    // additionally under `name` if it declared `result=`.
    void set_result(std::string_view name, std::shared_ptr<raster_canvas> canvas)
    {
        if(!name.empty())
            m_named_results.emplace(std::string(name), canvas);
        m_last_result = std::move(canvas);
    }

    const std::shared_ptr<raster_canvas>& last_result() const { return m_last_result; }

private:
    std::shared_ptr<raster_canvas> m_source_graphic;
    std::shared_ptr<raster_canvas> m_last_result;
    std::unordered_map<std::string, std::shared_ptr<raster_canvas>> m_named_results;
    float m_scale;
};

class svg_fe_gaussian_blur_element final : public svg_element {
public:
    svg_fe_gaussian_blur_element(document* dom) : svg_element(dom, element_id::fe_gaussian_blur) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto sigma = parse_first_float(attribute(property_id::std_deviation), 0.f) * context.scale();
        auto output = context.clone_canvas(*context.resolve_input(attribute(property_id::in)));
        output->box_blur(sigma, sigma);
        return output;
    }
};

class svg_fe_offset_element final : public svg_element {
public:
    svg_fe_offset_element(document* dom) : svg_element(dom, element_id::fe_offset) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto dx = parse_first_float(attribute(property_id::dx), 0.f) * context.scale();
        auto dy = parse_first_float(attribute(property_id::dy), 0.f) * context.scale();
        auto output = context.clone_canvas(*context.resolve_input(attribute(property_id::in)));
        output->shift(dx, dy);
        return output;
    }
};

class svg_fe_flood_element final : public svg_element {
public:
    svg_fe_flood_element(document* dom) : svg_element(dom, element_id::fe_flood) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto flood_color_attr = attribute(property_id::flood_color);
        stb::color flood_color = flood_color_attr.empty() ? stb::color(0, 0, 0) : stb::color(flood_color_attr);
        auto flood_opacity = parse_first_float(attribute(property_id::flood_opacity), 1.f);

        auto output = context.new_canvas();
        output->fill_opaque_white();
        output->tint_to_flood_color(flood_color, flood_opacity);
        return output;
    }
};

class svg_fe_merge_element final : public svg_element {
public:
    svg_fe_merge_element(document* dom) : svg_element(dom, element_id::fe_merge) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto output = context.new_canvas();
        for(const auto& child : children()) {
            auto el = to_svg_element(child);
            if(el && el->id() == element_id::fe_merge_node)
                output->composite_over(*context.resolve_input(el->attribute(property_id::in)));
        }

        return output;
    }
};

class svg_fe_composite_element final : public svg_element {
public:
    svg_fe_composite_element(document* dom) : svg_element(dom, element_id::fe_composite) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto input = context.resolve_input(attribute(property_id::in));
        auto input2 = context.resolve_input(attribute(property_id::in2));
        auto output = context.clone_canvas(*input2);

        auto op = attribute(property_id::operator_);
        if(op == "arithmetic") {
            auto k1 = parse_first_float(attribute(property_id::K1), 0.f);
            auto k2 = parse_first_float(attribute(property_id::K2), 0.f);
            auto k3 = parse_first_float(attribute(property_id::K3), 0.f);
            auto k4 = parse_first_float(attribute(property_id::K4), 0.f);
            output->composite_arithmetic(*input, k1, k2, k3, k4);
            return output;
        }

        auto mode = raster_canvas::porter_duff::over;
        if(op == "in") mode = raster_canvas::porter_duff::in;
        else if(op == "out") mode = raster_canvas::porter_duff::out;
        else if(op == "atop") mode = raster_canvas::porter_duff::atop;
        else if(op == "xor") mode = raster_canvas::porter_duff::exclusive_or;

        output->composite_with(*input, mode);
        return output;
    }
};

class svg_fe_drop_shadow_element final : public svg_element {
public:
    svg_fe_drop_shadow_element(document* dom) : svg_element(dom, element_id::fe_drop_shadow) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto sigma = parse_first_float(attribute(property_id::std_deviation), 2.f) * context.scale();
        auto dx = parse_first_float(attribute(property_id::dx), 2.f) * context.scale();
        auto dy = parse_first_float(attribute(property_id::dy), 2.f) * context.scale();
        auto flood_color_attr = attribute(property_id::flood_color);
        stb::color flood_color = flood_color_attr.empty() ? stb::color(0, 0, 0) : stb::color(flood_color_attr);
        auto flood_opacity = parse_first_float(attribute(property_id::flood_opacity), 1.f);

        auto input = context.resolve_input(attribute(property_id::in));
        auto shadow = context.clone_canvas(*input);
        shadow->tint_to_flood_color(flood_color, flood_opacity);
        shadow->box_blur(sigma, sigma);
        shadow->shift(dx, dy);
        shadow->composite_over(*input);
        return shadow;
    }
};

class svg_filter_element final : public svg_element {
public:
    svg_filter_element(document* dom);

    // Extra space (in the *filtered el's own* user-space units,
    // unscaled) the offscreen buffer needs so blur/shadow spread doesn't
    // get clipped at the el's plain bounding box.
    float pixel_margin() const;

    // Runs every child primitive in dom order, threading each
    // result through svg_filter_context, then writes the final result back
    // into `state`'s current (already-rendered) canvas. `scale` converts
    // user-space stdDeviation/dx/dy to device pixels.
    void apply_filter(svg_render_state& state, float scale) const;
};

// ---- svgrenderstate ----

} // namespace nova::svg
