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

    const svg_rect& viewBox() const { return m_view_box; }
    const svg_preserve_aspect_ratio& preserveAspectRatio() const { return m_preserve_aspect_ratio; }
    Transform view_box_to_view_transform(const Size& viewport_size) const;
    Rect get_clip_rect(const Size& viewport_size) const;

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
    svg_paint_server(const svg_paint_element* el, const stb::color& color, float opacity)
        : m_element(el), m_color(color), m_opacity(opacity)
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
    Transform local_transform() const override;

    svg_paint_server get_paint_server(const Paint& paint, float opacity) const;
    StrokeData get_stroke_data(const svg_layout_state& state) const;

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

    Transform local_transform() const override;
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

    Transform local_transform() const final;
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
    const svg_preserve_aspect_ratio& preserveAspectRatio() const { return m_preserve_aspect_ratio; }
    const pixmap& image() const { return m_image; }

    Rect fill_bounding_box() const final;
    Rect stroke_bounding_box() const final;
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

class svg_foreign_object_element;

/**
 * @brief Renders the (HTML-ish) content captured inside a <foreignObject>.
 *
 * This is an extension point on purpose: ForeignObjectSimple below only
 * understands a small, deliberately shallow slice of HTML5 -- it strips
 * every tag (div/span/p/i/b/...) and just draws the remaining plain text
 * centered in the foreignObject's box, using the font/fill already
 * inherited through the normal svg/CSS cascade. That's enough to cover
 * what tools like Mermaid actually emit (a single short, non-wrapping,
 * centered text run per foreignObject), without pretending to be a real
 * HTML/CSS layout engine.
 *
 * A more complete renderer (e.g. one backed by litehtml) can be hooked in
 * later without touching svg_foreign_object_element itself: implement this
 * interface and call svg_foreign_object_element::setRenderer() (or replace
 * ForeignObjectSimple::instance() below) to swap it in.
 */
class foreign_object_renderer {
public:
    virtual ~foreign_object_renderer() = default;
    virtual void render(const svg_foreign_object_element* el, svg_render_state& state) const = 0;
};

/**
 * @brief Default ForeignObjectRenderer: tag-stripped plain text, centered.
 */
class foreign_object_simple final : public foreign_object_renderer {
public:
    static foreign_object_simple* instance();

    void render(const svg_foreign_object_element* el, svg_render_state& state) const final;
};

/**
 * @brief The faces a <foreignObject> can paint with: its own font plus the bold / italic variants
 * of it that <b>, <strong>, <i>, <em> and inline `font-weight` / `font-style` ask for.
 *
 * Indexed by style bits (bit 0 = bold, bit 1 = italic), so `get(Bold | Italic)` is the bold italic
 * face. Variants nothing asked for may stay null. Each one is looked up exactly like svg <text>
 * looks up its font for the same weight/style, so the result is whatever faces the machine (or an
 * @font-face) provides -- there is no synthetic emboldening or slanting: with no bold face
 * installed, bold text simply renders in the regular face, as it does for <text>.
 */
struct foreign_object_fonts {
    static constexpr unsigned char Bold = 1;
    static constexpr unsigned char Italic = 2;

    Font face[4];
    const Font& get(unsigned char style) const { return face[style & 3]; }
};

class svg_foreign_object_element final : public svg_graphics_element {
public:
    svg_foreign_object_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const Font& font() const { return m_font; }
    const svg_paint_server& fill() const { return m_fill; }

    /// Bold/italic bits (ForeignObjectFonts::Bold/Italic) the el itself inherits, i.e. the style of font().
    unsigned char base_font_style() const { return m_base_font_style; }
    /// font() plus the variants named by `usedStyles` (a bit mask: bit n set = style n is needed). Nothing
    /// is looked up for styles that are not in the mask, so unstyled text costs nothing extra.
    foreign_object_fonts fonts_for(unsigned used_styles) const;

    /**
     * @brief Raw markup captured between <foreignObject ...> and
     * </foreignObject> (whatever HTML/XML the dom embedded there),
     * exactly as it appeared in the source -- untouched, unparsed. What a
     * ForeignObjectRenderer does with it is entirely up to that renderer.
     */
    const std::string& raw_content() const { return m_raw_content; }
    void set_raw_content(std::string content) { m_raw_content = std::move(content); }

    foreign_object_renderer* renderer() const { return m_renderer; }
    void set_renderer(foreign_object_renderer* renderer) { m_renderer = renderer; }

    /**
     * The text colour and the box colour the dom's CSS gives this el's HTML, worked out when a
     * stylesheet is applied (see resolveForeignObjectStyle()). Unset = nothing in the CSS asked for one
     * (text is then black, and no box is painted).
     */
    const std::optional<stb::color>& html_text_color() const { return m_html_text_color; }
    const std::optional<stb::color>& html_background_color() const { return m_html_background_color; }
    void set_html_style(std::optional<stb::color> text, std::optional<stb::color> background)
    {
        m_html_text_color = text;
        m_html_background_color = background;
    }

    Rect fill_bounding_box() const final;
    Rect stroke_bounding_box() const final;
    void layout_element(const svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    Font m_font;
    std::string m_fontFamily;
    unsigned char m_base_font_style = 0;
    svg_paint_server m_fill;
    std::string m_raw_content;
    foreign_object_renderer* m_renderer = nullptr;
    std::optional<stb::color> m_html_text_color;
    std::optional<stb::color> m_html_background_color;
};

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
    const svg_length& markerWidth() const { return m_marker_width; }
    const svg_length& markerHeight() const { return m_marker_height; }
    const svg_enumeration<MarkerUnits>& markerUnits() const { return m_marker_units; }
    const svg_angle& orient() const { return m_orient; }

    Point ref_point() const;
    Size marker_size() const;

    Transform marker_transform(const Point& origin, float angle, float strokeWidth) const;
    Rect marker_bounding_box(const Point& origin, float angle, float strokeWidth) const;
    void render_marker(svg_render_state& state, const Point& origin, float angle, float strokeWidth) const;

    Transform local_transform() const final;

private:
    svg_length m_ref_x;
    svg_length m_ref_y;
    svg_length m_marker_width;
    svg_length m_marker_height;
    svg_enumeration<MarkerUnits> m_marker_units;
    svg_angle m_orient;
};

class svg_clip_path_element final : public svg_graphics_element {
public:
    svg_clip_path_element(document* dom);

    const svg_enumeration<Units>& clipPathUnits() const { return m_clip_path_units; }
    Rect clip_bounding_box(const svg_element* el) const;

    void apply_clip_mask(svg_render_state& state) const;
    void apply_clip_path(svg_render_state& state) const;

    bool requires_masking() const;

private:
    svg_enumeration<Units> m_clip_path_units;
};

class svg_mask_element final : public svg_element {
public:
    svg_mask_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const svg_enumeration<Units>& maskUnits() const { return m_mask_units; }
    const svg_enumeration<Units>& maskContentUnits() const { return m_mask_content_units; }

    Rect mask_rect(const svg_element* el) const;
    Rect mask_bounding_box(const svg_element* el) const;
    void apply_mask(svg_render_state& state) const;

    void layout_element(const svg_layout_state& state) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_enumeration<Units> m_mask_units;
    svg_enumeration<Units> m_mask_content_units;
    MaskType m_mask_type = MaskType::luminance;
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
        auto output = context.clone_canvas(*context.resolve_input(attribute(property_id::In)));
        output->box_blur(sigma, sigma);
        return output;
    }
};

class svg_fe_offset_element final : public svg_element {
public:
    svg_fe_offset_element(document* dom) : svg_element(dom, element_id::fe_offset) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto dx = parse_first_float(attribute(property_id::Dx), 0.f) * context.scale();
        auto dy = parse_first_float(attribute(property_id::Dy), 0.f) * context.scale();
        auto output = context.clone_canvas(*context.resolve_input(attribute(property_id::In)));
        output->shift(dx, dy);
        return output;
    }
};

class svg_fe_flood_element final : public svg_element {
public:
    svg_fe_flood_element(document* dom) : svg_element(dom, element_id::fe_flood) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto flood_color_attr = attribute(property_id::Flood_Color);
        stb::color floodColor = flood_color_attr.empty() ? stb::color(0, 0, 0) : stb::color(flood_color_attr);
        auto floodOpacity = parse_first_float(attribute(property_id::Flood_Opacity), 1.f);

        auto output = context.new_canvas();
        output->fill_opaque_white();
        output->tint_to_flood_color(floodColor, floodOpacity);
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
                output->composite_over(*context.resolve_input(el->attribute(property_id::In)));
        }

        return output;
    }
};

class svg_fe_composite_element final : public svg_element {
public:
    svg_fe_composite_element(document* dom) : svg_element(dom, element_id::fe_composite) {}

    std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const final
    {
        auto input = context.resolve_input(attribute(property_id::In));
        auto input2 = context.resolve_input(attribute(property_id::in2));
        auto output = context.clone_canvas(*input2);

        auto op = attribute(property_id::Operator);
        if(op == "arithmetic") {
            auto k1 = parse_first_float(attribute(property_id::K1), 0.f);
            auto k2 = parse_first_float(attribute(property_id::K2), 0.f);
            auto k3 = parse_first_float(attribute(property_id::K3), 0.f);
            auto k4 = parse_first_float(attribute(property_id::K4), 0.f);
            output->composite_arithmetic(*input, k1, k2, k3, k4);
            return output;
        }

        auto mode = raster_canvas::porter_duff::Over;
        if(op == "in") mode = raster_canvas::porter_duff::In;
        else if(op == "out") mode = raster_canvas::porter_duff::Out;
        else if(op == "atop") mode = raster_canvas::porter_duff::atop;
        else if(op == "xor") mode = raster_canvas::porter_duff::Xor;

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
        auto dx = parse_first_float(attribute(property_id::Dx), 2.f) * context.scale();
        auto dy = parse_first_float(attribute(property_id::Dy), 2.f) * context.scale();
        auto flood_color_attr = attribute(property_id::Flood_Color);
        stb::color floodColor = flood_color_attr.empty() ? stb::color(0, 0, 0) : stb::color(flood_color_attr);
        auto floodOpacity = parse_first_float(attribute(property_id::Flood_Opacity), 1.f);

        auto input = context.resolve_input(attribute(property_id::In));
        auto shadow = context.clone_canvas(*input);
        shadow->tint_to_flood_color(floodColor, floodOpacity);
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
