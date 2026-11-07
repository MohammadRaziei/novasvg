#pragma once

// svg_node, attributes, ElementID and the svg_element base class.

#include "svg_property.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

#include <cassert>

// ---- svgelement ----

class document;
class svg_element;
class svg_root_element;

class svg_node {
public:
    svg_node(document* dom)
        : m_document(dom)
    {}

    virtual ~svg_node() = default;
    virtual bool is_text_node() const { return false; }
    virtual bool is_element() const { return false; }
    virtual bool is_paint_element() const { return false; }
    virtual bool is_graphics_element() const { return false; }
    virtual bool is_geometry_element() const { return false; }
    virtual bool is_text_positioning_element() const { return false; }

    document* dom() const { return m_document; }
    svg_root_element* root_element() const { return m_document->root_element(); }

    svg_element* parent_element() const { return m_parent_element; }
    void set_parent_element(svg_element* parent) { m_parent_element = parent; }

    bool is_root_element() const { return m_parent_element == nullptr; }

    virtual std::unique_ptr<svg_node> clone(bool deep) const = 0;

private:
    svg_node(const svg_node&) = delete;
    svg_node& operator=(const svg_node&) = delete;
    document* m_document;
    svg_element* m_parent_element = nullptr;
};

class svg_text_node final : public svg_node {
public:
    svg_text_node(document* dom);

    bool is_text_node() const final { return true; }

    const std::string& data() const { return m_data; }
    void set_data(const std::string& data);

    std::unique_ptr<svg_node> clone(bool deep) const final;

private:
    std::string m_data;
};

/**
 * Where an attribute value came from, as a number: when two sources set the same property the higher one
 * wins, and the later one wins a tie. This is CSS's cascade, lowest to highest:
 * presentation attribute, stylesheet, inline `style=""`, stylesheet `!important`, inline `!important`,
 * and finally a value set through the API (a program that loads a dom and then changes a property
 * means it, whatever CSS the file carried).
 */
namespace specificity {
constexpr int presentation_attribute = 0x1;
constexpr int stylesheet = 0x10;
constexpr int InlineStyle = 0x100;
constexpr int stylesheet_important = 0x1000;
constexpr int inline_important = 0x2000;
constexpr int api = 0x4000;
} // namespace Specificity

class attribute {
public:
    attribute() = default;
    attribute(int specificity, property_id id, std::string value)
        : m_specificity(specificity), m_id(id), m_value(std::move(value))
    {}

    int specificity() const { return m_specificity; }
    property_id id() const { return m_id; }
    const std::string& value() const { return m_value; }

private:
    int m_specificity;
    property_id m_id;
    std::string m_value;
};

using attribute_list = std::forward_list<attribute>;

enum class element_id : uint8_t {
    unknown = 0,
    star,
    circle,
    clip_path,
    defs,
    ellipse,
    fe_composite,
    fe_drop_shadow,
    fe_flood,
    fe_gaussian_blur,
    fe_merge,
    fe_merge_node,
    fe_offset,
    filter,
    ForeignObject,
    G,
    image,
    line,
    linear_gradient,
    marker,
    mask,
    path,
    pattern,
    polygon,
    polyline,
    radial_gradient,
    rect,
    stop,
    style,
    svg,
    switch_,
    symbol,
    text,
    tspan,
    use
};

element_id elementid(std::string_view name);

using svg_node_list = std::list<std::unique_ptr<svg_node>>;
using svg_property_list = std::forward_list<svg_property*>;

class svg_marker_element;
class svg_clip_path_element;
class svg_mask_element;
class svg_filter_element;
class svg_filter_context;
class svg_paint_element;
class svg_layout_state;
class svg_render_state;

extern const std::string empty_string;

class svg_element : public svg_node {
public:
    static std::unique_ptr<svg_element> create(document* dom, element_id id);

    svg_element(document* dom, element_id id);
    virtual ~svg_element() = default;

    bool has_attribute(std::string_view name) const;
    const std::string& attribute_(std::string_view name) const;
    bool set_attribute(std::string_view name, const std::string& value);

    const attribute* find_attribute(property_id id) const;
    bool has_attribute(property_id id) const;
    const std::string& attribute_(property_id id) const;

    // Attributes novasvg does not know (`data-*`, `aria-*`, ...). They paint nothing, but CSS attribute
    // selectors (`[data-k]`, `[data-k="1"]`) match on them.
    const std::string* find_custom_attribute(const std::string& name) const;
    void set_custom_attribute(const std::string& name, const std::string& value);
    bool set_attribute(int specificity, property_id id, const std::string& value);
    void set_attributes(const attribute_list& attributes);
    bool set_attribute(const attribute& attribute_);

    virtual void parse_attribute(property_id id, const std::string& value);

    // CSS custom properties (`--name: value`, from `style=""` or a stylesheet) declared on this el: the
    // higher cascade priority wins, a tie goes to the later one. Names are case-sensitive.
    void set_custom_property(int specificity, const std::string& name, const std::string& value);
    // The el (this one or the nearest ancestor: custom properties inherit) that gives `name` a value.
    const svg_element* find_custom_property_owner(const std::string& name) const;
    const std::string& custom_property(const std::string& name) const;
    // Gives every attribute whose value uses `var()` its value, now that the custom properties are known.
    // An attribute whose `var()` cannot be resolved (no such property and no fallback) is left as it was.
    void resolve_variables();

    svg_element* previous_element() const;
    svg_element* next_element() const;

    svg_node* add_child(std::unique_ptr<svg_node> child);
    svg_node* firstChild() const;
    svg_node* lastChild() const;

    element_id id() const { return m_id; }
    const attribute_list& attributes() const { return m_attributes; }
    const svg_property_list& properties() const { return m_properties; }
    const svg_node_list& children() const { return m_children; }

    virtual transform local_transform() const { return transform::identity; }
    virtual rect fill_bounding_box() const;
    virtual rect stroke_bounding_box() const;
    virtual rect paint_bounding_box() const;

    svg_marker_element* get_marker(std::string_view id) const;
    svg_clip_path_element* get_clipper(std::string_view id) const;
    svg_mask_element* get_masker(std::string_view id) const;
    svg_filter_element* get_filter_element(std::string_view id) const;
    svg_paint_element* get_painter(std::string_view id) const;

    svg_element* element_from_point(float x, float y);

    template<typename T>
    void transverse(T callback);

    void add_property(svg_property& value);
    svg_property* get_property(property_id id) const;
    size current_viewport_size() const;
    float font_size() const { return m_font_size; }

    void clone_children(svg_element* parent_element) const;
    std::unique_ptr<svg_node> clone(bool deep) const final;

    virtual void build();

    virtual void layout_element(const svg_layout_state& state);
    void layout_children(svg_layout_state& state);
    virtual void layout(svg_layout_state& state);

    void render_children(svg_render_state& state) const;
    virtual void render(svg_render_state& state) const;

    // Template Method hook for filter-primitive elements (feGaussianBlur,
    // feOffset, feFlood, feComposite, feMerge, feDropShadow, ...): given
    // the running filter pipeline context, compute and return this
    // primitive's output. Default (every non-primitive el,
    // including feMergeNode which only makes sense as feMerge's child)
    // is "not a primitive" -- returns nullptr, which svg_filter_element's
    // pipeline simply skips over.
    virtual std::shared_ptr<raster_canvas> apply_filter_primitive(svg_filter_context& context) const { return nullptr; }

    bool is_display_none() const { return m_display == display::none; }
    bool is_overflow_hidden() const { return m_overflow == overflow::hidden; }
    bool is_visibility_hidden() const { return m_visibility != visibility::visible; }

    bool is_hidden_element() const;
    bool is_pointable_element() const;

    const svg_clip_path_element* clipper() const { return m_clipper; }
    const svg_mask_element* masker() const { return m_masker; }
    const svg_filter_element* filter_element() const { return m_filter; }
    float opacity() const { return m_opacity; }
    mix_blend_mode mix_blend_mode_() const { return m_mix_blend_mode; }

    bool is_element() const final { return true; }

private:
    mutable rect m_paint_bounding_box = rect::invalid;
    const svg_clip_path_element* m_clipper = nullptr;
    const svg_mask_element* m_masker = nullptr;
    const svg_filter_element* m_filter = nullptr;
    float m_opacity = 1.f;
    mix_blend_mode m_mix_blend_mode = mix_blend_mode::normal;

    float m_font_size = 12.f;
    display m_display = display::inline_;
    overflow m_overflow = overflow::visible;
    visibility m_visibility = visibility::visible;
    pointer_events m_pointer_events = pointer_events::auto_;

    element_id m_id;
    attribute_list m_attributes;
    // Attributes with a name that is not a property_id, as written; only read by CSS attribute selectors.
    std::map<std::string, std::string> m_custom_attributes;
    svg_property_list m_properties;
    svg_node_list m_children;
    // `--name` -> (cascade priority, value as written, possibly with var() of its own)
    std::map<std::string, std::pair<int, std::string>> m_custom_properties;
    // Attributes whose value uses var(), as written. Their value in m_attributes is the resolved one (or this
    // very text while it is unresolved), so that a <use> copies what was computed; resolveVariables() starts
    // from the text kept here, so a stylesheet applied later can still change the result.
    std::map<property_id, std::string> m_variable_sources;
};

inline const svg_element* to_svg_element(const svg_node* nd)
{
    if(nd && nd->is_element())
        return static_cast<const svg_element*>(nd);
    return nullptr;
}

inline svg_element* to_svg_element(svg_node* nd)
{
    if(nd && nd->is_element())
        return static_cast<svg_element*>(nd);
    return nullptr;
}

inline svg_element* to_svg_element(const std::unique_ptr<svg_node>& nd)
{
    return to_svg_element(nd.get());
}

template<typename T>
inline void svg_element::transverse(T callback)
{
    callback(this);
    for(const auto& child : m_children) {
        if(auto el = to_svg_element(child)) {
            el->transverse(callback);
        }
    }
}

} // namespace nova::svg
