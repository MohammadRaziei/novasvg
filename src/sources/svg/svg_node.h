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
    virtual bool isPaintElement() const { return false; }
    virtual bool isGraphicsElement() const { return false; }
    virtual bool isGeometryElement() const { return false; }
    virtual bool isTextPositioningElement() const { return false; }

    document* dom() const { return m_document; }
    svg_root_element* root_element() const { return m_document->root_element(); }

    svg_element* parent_element() const { return m_parentElement; }
    void setParentElement(svg_element* parent) { m_parentElement = parent; }

    bool isRootElement() const { return m_parentElement == nullptr; }

    virtual std::unique_ptr<svg_node> clone(bool deep) const = 0;

private:
    svg_node(const svg_node&) = delete;
    svg_node& operator=(const svg_node&) = delete;
    document* m_document;
    svg_element* m_parentElement = nullptr;
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
namespace Specificity {
constexpr int PresentationAttribute = 0x1;
constexpr int Stylesheet = 0x10;
constexpr int InlineStyle = 0x100;
constexpr int StylesheetImportant = 0x1000;
constexpr int InlineImportant = 0x2000;
constexpr int Api = 0x4000;
} // namespace Specificity

class Attribute {
public:
    Attribute() = default;
    Attribute(int specificity, PropertyID id, std::string value)
        : m_specificity(specificity), m_id(id), m_value(std::move(value))
    {}

    int specificity() const { return m_specificity; }
    PropertyID id() const { return m_id; }
    const std::string& value() const { return m_value; }

private:
    int m_specificity;
    PropertyID m_id;
    std::string m_value;
};

using AttributeList = std::forward_list<Attribute>;

enum class ElementID : uint8_t {
    Unknown = 0,
    Star,
    Circle,
    ClipPath,
    Defs,
    Ellipse,
    FeComposite,
    FeDropShadow,
    FeFlood,
    FeGaussianBlur,
    FeMerge,
    FeMergeNode,
    FeOffset,
    Filter,
    ForeignObject,
    G,
    Image,
    Line,
    LinearGradient,
    Marker,
    Mask,
    Path,
    Pattern,
    Polygon,
    Polyline,
    RadialGradient,
    Rect,
    Stop,
    Style,
    Svg,
    Switch,
    Symbol,
    Text,
    Tspan,
    Use
};

ElementID elementid(std::string_view name);

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

extern const std::string emptyString;

class svg_element : public svg_node {
public:
    static std::unique_ptr<svg_element> create(document* dom, ElementID id);

    svg_element(document* dom, ElementID id);
    virtual ~svg_element() = default;

    bool has_attribute(std::string_view name) const;
    const std::string& attribute(std::string_view name) const;
    bool set_attribute(std::string_view name, const std::string& value);

    const Attribute* findAttribute(PropertyID id) const;
    bool has_attribute(PropertyID id) const;
    const std::string& attribute(PropertyID id) const;
    bool set_attribute(int specificity, PropertyID id, const std::string& value);
    void setAttributes(const AttributeList& attributes);
    bool set_attribute(const Attribute& attribute);

    virtual void parseAttribute(PropertyID id, const std::string& value);

    // CSS custom properties (`--name: value`, from `style=""` or a stylesheet) declared on this el: the
    // higher cascade priority wins, a tie goes to the later one. Names are case-sensitive.
    void setCustomProperty(int specificity, const std::string& name, const std::string& value);
    // The el (this one or the nearest ancestor: custom properties inherit) that gives `name` a value.
    const svg_element* findCustomPropertyOwner(const std::string& name) const;
    const std::string& customProperty(const std::string& name) const;
    // Gives every attribute whose value uses `var()` its value, now that the custom properties are known.
    // An attribute whose `var()` cannot be resolved (no such property and no fallback) is left as it was.
    void resolveVariables();

    svg_element* previousElement() const;
    svg_element* nextElement() const;

    svg_node* addChild(std::unique_ptr<svg_node> child);
    svg_node* firstChild() const;
    svg_node* lastChild() const;

    ElementID id() const { return m_id; }
    const AttributeList& attributes() const { return m_attributes; }
    const svg_property_list& properties() const { return m_properties; }
    const svg_node_list& children() const { return m_children; }

    virtual Transform local_transform() const { return Transform::Identity; }
    virtual Rect fillBoundingBox() const;
    virtual Rect strokeBoundingBox() const;
    virtual Rect paint_bounding_box() const;

    svg_marker_element* getMarker(std::string_view id) const;
    svg_clip_path_element* getClipper(std::string_view id) const;
    svg_mask_element* getMasker(std::string_view id) const;
    svg_filter_element* getFilterElement(std::string_view id) const;
    svg_paint_element* getPainter(std::string_view id) const;

    svg_element* element_from_point(float x, float y);

    template<typename T>
    void transverse(T callback);

    void addProperty(svg_property& value);
    svg_property* getProperty(PropertyID id) const;
    Size currentViewportSize() const;
    float font_size() const { return m_font_size; }

    void cloneChildren(svg_element* parent_element) const;
    std::unique_ptr<svg_node> clone(bool deep) const final;

    virtual void build();

    virtual void layoutElement(const svg_layout_state& state);
    void layoutChildren(svg_layout_state& state);
    virtual void layout(svg_layout_state& state);

    void renderChildren(svg_render_state& state) const;
    virtual void render(svg_render_state& state) const;

    // Template Method hook for filter-primitive elements (feGaussianBlur,
    // feOffset, feFlood, feComposite, feMerge, feDropShadow, ...): given
    // the running filter pipeline context, compute and return this
    // primitive's output. Default (every non-primitive el,
    // including feMergeNode which only makes sense as feMerge's child)
    // is "not a primitive" -- returns nullptr, which svg_filter_element's
    // pipeline simply skips over.
    virtual std::shared_ptr<raster_canvas> applyFilterPrimitive(svg_filter_context& context) const { return nullptr; }

    bool isDisplayNone() const { return m_display == Display::None; }
    bool isOverflowHidden() const { return m_overflow == Overflow::Hidden; }
    bool isVisibilityHidden() const { return m_visibility != Visibility::Visible; }

    bool isHiddenElement() const;
    bool isPointableElement() const;

    const svg_clip_path_element* clipper() const { return m_clipper; }
    const svg_mask_element* masker() const { return m_masker; }
    const svg_filter_element* filterElement() const { return m_filter; }
    float opacity() const { return m_opacity; }
    MixBlendMode mixBlendMode() const { return m_mix_blend_mode; }

    bool is_element() const final { return true; }

private:
    mutable Rect m_paintBoundingBox = Rect::Invalid;
    const svg_clip_path_element* m_clipper = nullptr;
    const svg_mask_element* m_masker = nullptr;
    const svg_filter_element* m_filter = nullptr;
    float m_opacity = 1.f;
    MixBlendMode m_mix_blend_mode = MixBlendMode::Normal;

    float m_font_size = 12.f;
    Display m_display = Display::Inline;
    Overflow m_overflow = Overflow::Visible;
    Visibility m_visibility = Visibility::Visible;
    PointerEvents m_pointer_events = PointerEvents::Auto;

    ElementID m_id;
    AttributeList m_attributes;
    svg_property_list m_properties;
    svg_node_list m_children;
    // `--name` -> (cascade priority, value as written, possibly with var() of its own)
    std::map<std::string, std::pair<int, std::string>> m_customProperties;
    // Attributes whose value uses var(), as written. Their value in m_attributes is the resolved one (or this
    // very text while it is unresolved), so that a <use> copies what was computed; resolveVariables() starts
    // from the text kept here, so a stylesheet applied later can still change the result.
    std::map<PropertyID, std::string> m_variableSources;
};

inline const svg_element* toSVGElement(const svg_node* nd)
{
    if(nd && nd->is_element())
        return static_cast<const svg_element*>(nd);
    return nullptr;
}

inline svg_element* toSVGElement(svg_node* nd)
{
    if(nd && nd->is_element())
        return static_cast<svg_element*>(nd);
    return nullptr;
}

inline svg_element* toSVGElement(const std::unique_ptr<svg_node>& nd)
{
    return toSVGElement(nd.get());
}

template<typename T>
inline void svg_element::transverse(T callback)
{
    callback(this);
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->transverse(callback);
        }
    }
}

} // namespace nova::svg
