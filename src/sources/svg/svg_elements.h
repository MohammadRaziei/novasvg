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

    const svg_rect& viewBox() const { return m_viewBox; }
    const svg_preserve_aspect_ratio& preserveAspectRatio() const { return m_preserveAspectRatio; }
    Transform viewBoxToViewTransform(const Size& viewportSize) const;
    Rect getClipRect(const Size& viewportSize) const;

private:
    svg_rect m_viewBox;
    svg_preserve_aspect_ratio m_preserveAspectRatio;
};

class svg_uri_reference {
public:
    svg_uri_reference(svg_element* el);

    const svg_string& href() const { return m_href; }
    const std::string& hrefString() const { return m_href.value(); }
    svg_element* getTargetElement(const document* dom) const;

private:
    svg_string m_href;
};

class svg_paint_server {
public:
    svg_paint_server() = default;
    svg_paint_server(const svg_paint_element* el, const stb::color& color, float opacity)
        : m_element(el), m_color(color), m_opacity(opacity)
    {}

    bool isRenderable() const { return m_opacity > 0.f && (m_element || m_color.a() > 0); }

    const svg_paint_element* el() const { return m_element; }
    const stb::color& color() const { return  m_color; }
    float opacity() const { return m_opacity; }

    bool applyPaint(svg_render_state& state) const;

private:
    const svg_paint_element* m_element = nullptr;
    stb::color m_color = stb::color::transparent();
    float m_opacity = 0.f;
};

class svg_graphics_element : public svg_element {
public:
    svg_graphics_element(document* dom, ElementID id);

    bool isGraphicsElement() const final { return true; }

    const svg_transform& transform() const { return m_transform; }
    Transform local_transform() const override;

    svg_paint_server getPaintServer(const Paint& paint, float opacity) const;
    StrokeData getStrokeData(const svg_layout_state& state) const;

private:
    svg_transform m_transform;
    svg_transform_origin m_transformOrigin;
    svg_transform_box m_transformBox;
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

    float intrinsic_width() const { return m_intrinsicWidth; }
    float intrinsic_height() const { return m_intrinsicHeight; }

    void setNeedsLayout() { m_intrinsicWidth = -1.f; }
    bool needsLayout() const { return m_intrinsicWidth == -1.f; }

    svg_root_element* layout_if_needed();

    svg_element* element_by_id(std::string_view id) const;
    void addElementById(const std::string& id, svg_element* el);
    void layout(svg_layout_state& state) final;

    void force_layout();

    // Every stylesheet applied to the dom -- the <style> text found while parsing, then anything
    // passed to document::apply_style_sheet() -- kept verbatim and in order. The HTML inside a
    // <foreignObject> sits outside the svg el tree and so never goes through the resolver that sets
    // svg presentation properties; it is styled from these (see resolveForeignObjectStyle()).
    const std::vector<std::string>& styleSheets() const { return m_styleSheets; }
    void addStyleSheet(std::string text) { m_styleSheets.push_back(std::move(text)); }

private:
    std::map<std::string, svg_element*, std::less<>> m_idCache;
    float m_intrinsicWidth{-1.f};
    float m_intrinsicHeight{-1.f};
    std::vector<std::string> m_styleSheets;
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
    std::unique_ptr<svg_element> cloneTargetElement(svg_element* targetElement);
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
    const svg_preserve_aspect_ratio& preserveAspectRatio() const { return m_preserveAspectRatio; }
    const pixmap& image() const { return m_image; }

    Rect fillBoundingBox() const final;
    Rect strokeBoundingBox() const final;
    void render(svg_render_state& state) const final;
    void parseAttribute(PropertyID id, const std::string& value) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_preserve_aspect_ratio m_preserveAspectRatio;
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
class ForeignObjectRenderer {
public:
    virtual ~ForeignObjectRenderer() = default;
    virtual void render(const svg_foreign_object_element* el, svg_render_state& state) const = 0;
};

/**
 * @brief Default ForeignObjectRenderer: tag-stripped plain text, centered.
 */
class ForeignObjectSimple final : public ForeignObjectRenderer {
public:
    static ForeignObjectSimple* instance();

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
struct ForeignObjectFonts {
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
    unsigned char baseFontStyle() const { return m_baseFontStyle; }
    /// font() plus the variants named by `usedStyles` (a bit mask: bit n set = style n is needed). Nothing
    /// is looked up for styles that are not in the mask, so unstyled text costs nothing extra.
    ForeignObjectFonts fontsFor(unsigned usedStyles) const;

    /**
     * @brief Raw markup captured between <foreignObject ...> and
     * </foreignObject> (whatever HTML/XML the dom embedded there),
     * exactly as it appeared in the source -- untouched, unparsed. What a
     * ForeignObjectRenderer does with it is entirely up to that renderer.
     */
    const std::string& rawContent() const { return m_rawContent; }
    void setRawContent(std::string content) { m_rawContent = std::move(content); }

    ForeignObjectRenderer* renderer() const { return m_renderer; }
    void setRenderer(ForeignObjectRenderer* renderer) { m_renderer = renderer; }

    /**
     * The text colour and the box colour the dom's CSS gives this el's HTML, worked out when a
     * stylesheet is applied (see resolveForeignObjectStyle()). Unset = nothing in the CSS asked for one
     * (text is then black, and no box is painted).
     */
    const std::optional<stb::color>& htmlTextColor() const { return m_htmlTextColor; }
    const std::optional<stb::color>& htmlBackgroundColor() const { return m_htmlBackgroundColor; }
    void setHtmlStyle(std::optional<stb::color> text, std::optional<stb::color> background)
    {
        m_htmlTextColor = text;
        m_htmlBackgroundColor = background;
    }

    Rect fillBoundingBox() const final;
    Rect strokeBoundingBox() const final;
    void layoutElement(const svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    Font m_font;
    std::string m_fontFamily;
    unsigned char m_baseFontStyle = 0;
    svg_paint_server m_fill;
    std::string m_rawContent;
    ForeignObjectRenderer* m_renderer = nullptr;
    std::optional<stb::color> m_htmlTextColor;
    std::optional<stb::color> m_htmlBackgroundColor;
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
    const svg_element* selectedChild() const;
};

class svg_defs_element final : public svg_graphics_element {
public:
    svg_defs_element(document* dom);
};

class svg_marker_element final : public svg_element, public svg_fit_to_view_box {
public:
    svg_marker_element(document* dom);

    const svg_length& refX() const { return m_refX; }
    const svg_length& refY() const { return m_refY; }
    const svg_length& markerWidth() const { return m_markerWidth; }
    const svg_length& markerHeight() const { return m_markerHeight; }
    const svg_enumeration<MarkerUnits>& markerUnits() const { return m_markerUnits; }
    const svg_angle& orient() const { return m_orient; }

    Point refPoint() const;
    Size markerSize() const;

    Transform markerTransform(const Point& origin, float angle, float strokeWidth) const;
    Rect markerBoundingBox(const Point& origin, float angle, float strokeWidth) const;
    void renderMarker(svg_render_state& state, const Point& origin, float angle, float strokeWidth) const;

    Transform local_transform() const final;

private:
    svg_length m_refX;
    svg_length m_refY;
    svg_length m_markerWidth;
    svg_length m_markerHeight;
    svg_enumeration<MarkerUnits> m_markerUnits;
    svg_angle m_orient;
};

class svg_clip_path_element final : public svg_graphics_element {
public:
    svg_clip_path_element(document* dom);

    const svg_enumeration<Units>& clipPathUnits() const { return m_clipPathUnits; }
    Rect clipBoundingBox(const svg_element* el) const;

    void applyClipMask(svg_render_state& state) const;
    void applyClipPath(svg_render_state& state) const;

    bool requiresMasking() const;

private:
    svg_enumeration<Units> m_clipPathUnits;
};

class svg_mask_element final : public svg_element {
public:
    svg_mask_element(document* dom);

    const svg_length& x() const { return m_x; }
    const svg_length& y() const { return m_y; }
    const svg_length& width() const { return m_width; }
    const svg_length& height() const { return m_height; }
    const svg_enumeration<Units>& maskUnits() const { return m_maskUnits; }
    const svg_enumeration<Units>& maskContentUnits() const { return m_maskContentUnits; }

    Rect maskRect(const svg_element* el) const;
    Rect maskBoundingBox(const svg_element* el) const;
    void applyMask(svg_render_state& state) const;

    void layoutElement(const svg_layout_state& state) final;

private:
    svg_length m_x;
    svg_length m_y;
    svg_length m_width;
    svg_length m_height;
    svg_enumeration<Units> m_maskUnits;
    svg_enumeration<Units> m_maskContentUnits;
    MaskType m_mask_type = MaskType::Luminance;
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
static float parseFirstFloat(std::string_view input, float fallback)
{
    if(input.empty())
        return fallback;
    return std::strtof(std::string(input).c_str(), nullptr);
}

class svg_filter_context {
public:
    svg_filter_context(const std::shared_ptr<raster_canvas>& sourceGraphic, float scale)
        : m_sourceGraphic(sourceGraphic), m_lastResult(sourceGraphic), m_scale(scale)
    {}

    float scale() const { return m_scale; }

    // A blank transparent canvas matching SourceGraphic's position/size --
    // every primitive's output shares this geometry (ponytail: real UAs
    // allow per-primitive filter regions; one shared region for the whole
    // chain is a deliberate simplification, see checklist.md).
    std::shared_ptr<raster_canvas> newCanvas() const
    {
        return raster_canvas::create(float(m_sourceGraphic->x()), float(m_sourceGraphic->y()), float(m_sourceGraphic->width()), float(m_sourceGraphic->height()));
    }

    std::shared_ptr<raster_canvas> cloneCanvas(const raster_canvas& source) const
    {
        auto canvas = newCanvas();
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
    std::shared_ptr<raster_canvas> resolveInput(std::string_view in) const
    {
        if(in.empty())
            return m_lastResult;
        if(in == "SourceGraphic")
            return m_sourceGraphic;
        if(in == "SourceAlpha") {
            auto alpha = cloneCanvas(*m_sourceGraphic);
            alpha->tint_to_flood_color(stb::color(0, 0, 0), 1.f);
            return alpha;
        }
        auto it = m_namedResults.find(std::string(in));
        if(it != m_namedResults.end())
            return it->second;
        return m_lastResult;
    }

    // Records a primitive's output as the new implicit chain head, and
    // additionally under `name` if it declared `result=`.
    void setResult(std::string_view name, std::shared_ptr<raster_canvas> canvas)
    {
        if(!name.empty())
            m_namedResults.emplace(std::string(name), canvas);
        m_lastResult = std::move(canvas);
    }

    const std::shared_ptr<raster_canvas>& lastResult() const { return m_lastResult; }

private:
    std::shared_ptr<raster_canvas> m_sourceGraphic;
    std::shared_ptr<raster_canvas> m_lastResult;
    std::unordered_map<std::string, std::shared_ptr<raster_canvas>> m_namedResults;
    float m_scale;
};

class svg_fe_gaussian_blur_element final : public svg_element {
public:
    svg_fe_gaussian_blur_element(document* dom) : svg_element(dom, ElementID::FeGaussianBlur) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto sigma = parseFirstFloat(attribute(PropertyID::StdDeviation), 0.f) * context.scale();
        auto output = context.cloneCanvas(*context.resolveInput(attribute(PropertyID::In)));
        output->box_blur(sigma, sigma);
        return output;
    }
};

class svg_fe_offset_element final : public svg_element {
public:
    svg_fe_offset_element(document* dom) : svg_element(dom, ElementID::FeOffset) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto dx = parseFirstFloat(attribute(PropertyID::Dx), 0.f) * context.scale();
        auto dy = parseFirstFloat(attribute(PropertyID::Dy), 0.f) * context.scale();
        auto output = context.cloneCanvas(*context.resolveInput(attribute(PropertyID::In)));
        output->shift(dx, dy);
        return output;
    }
};

class svg_fe_flood_element final : public svg_element {
public:
    svg_fe_flood_element(document* dom) : svg_element(dom, ElementID::FeFlood) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto floodColorAttr = attribute(PropertyID::Flood_Color);
        stb::color floodColor = floodColorAttr.empty() ? stb::color(0, 0, 0) : stb::color(floodColorAttr);
        auto floodOpacity = parseFirstFloat(attribute(PropertyID::Flood_Opacity), 1.f);

        auto output = context.newCanvas();
        output->fill_opaque_white();
        output->tint_to_flood_color(floodColor, floodOpacity);
        return output;
    }
};

class svg_fe_merge_element final : public svg_element {
public:
    svg_fe_merge_element(document* dom) : svg_element(dom, ElementID::FeMerge) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto output = context.newCanvas();
        for(const auto& child : children()) {
            auto el = toSVGElement(child);
            if(el && el->id() == ElementID::FeMergeNode)
                output->composite_over(*context.resolveInput(el->attribute(PropertyID::In)));
        }

        return output;
    }
};

class svg_fe_composite_element final : public svg_element {
public:
    svg_fe_composite_element(document* dom) : svg_element(dom, ElementID::FeComposite) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto input = context.resolveInput(attribute(PropertyID::In));
        auto input2 = context.resolveInput(attribute(PropertyID::In2));
        auto output = context.cloneCanvas(*input2);

        auto op = attribute(PropertyID::Operator);
        if(op == "arithmetic") {
            auto k1 = parseFirstFloat(attribute(PropertyID::K1), 0.f);
            auto k2 = parseFirstFloat(attribute(PropertyID::K2), 0.f);
            auto k3 = parseFirstFloat(attribute(PropertyID::K3), 0.f);
            auto k4 = parseFirstFloat(attribute(PropertyID::K4), 0.f);
            output->composite_arithmetic(*input, k1, k2, k3, k4);
            return output;
        }

        auto mode = raster_canvas::porter_duff::Over;
        if(op == "in") mode = raster_canvas::porter_duff::In;
        else if(op == "out") mode = raster_canvas::porter_duff::Out;
        else if(op == "atop") mode = raster_canvas::porter_duff::Atop;
        else if(op == "xor") mode = raster_canvas::porter_duff::Xor;

        output->composite_with(*input, mode);
        return output;
    }
};

class svg_fe_drop_shadow_element final : public svg_element {
public:
    svg_fe_drop_shadow_element(document* dom) : svg_element(dom, ElementID::FeDropShadow) {}

    std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const final
    {
        auto sigma = parseFirstFloat(attribute(PropertyID::StdDeviation), 2.f) * context.scale();
        auto dx = parseFirstFloat(attribute(PropertyID::Dx), 2.f) * context.scale();
        auto dy = parseFirstFloat(attribute(PropertyID::Dy), 2.f) * context.scale();
        auto floodColorAttr = attribute(PropertyID::Flood_Color);
        stb::color floodColor = floodColorAttr.empty() ? stb::color(0, 0, 0) : stb::color(floodColorAttr);
        auto floodOpacity = parseFirstFloat(attribute(PropertyID::Flood_Opacity), 1.f);

        auto input = context.resolveInput(attribute(PropertyID::In));
        auto shadow = context.cloneCanvas(*input);
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
    float pixelMargin() const;

    // Runs every child primitive in dom order, threading each
    // result through svg_filter_context, then writes the final result back
    // into `state`'s current (already-rendered) canvas. `scale` converts
    // user-space stdDeviation/dx/dy to device pixels.
    void applyFilter(svg_render_state& state, float scale) const;
};

// ---- svgrenderstate ----

} // namespace nova::svg
