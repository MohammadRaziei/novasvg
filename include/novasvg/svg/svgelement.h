#ifndef NOVASVG_SVGELEMENT_H
#define NOVASVG_SVGELEMENT_H

#include <string>
#include <forward_list>
#include <list>
#include <map>
#include <unordered_map>
#include <optional>
#include <vector>
#include <cmath>
#include <cctype>
#include <set>
#include <cassert>
#include "svgparserutils.h"

// ---------------------------------------------------------------
// svg_element and its family (svg_paint_element, svg_geometry_element,
// svg_text_element, svg_layout_state) used to be 5 declaration/
// implementation pairs (10 files). They're merged into this one
// file because they're circularly dependent -- svg_element's own
// implementation needs the derived classes fully declared, and the
// derived classes need svg_element fully declared to inherit from it
// -- so a clean 1:1 .h/.hpp split was never actually possible for
// this group; only a shared declare-everything-then-define-
// everything file is. Order matters: declarations below follow the
// dependency order (base class first), implementations follow
// (order doesn't matter there since all declarations above are
// already visible).
// ---------------------------------------------------------------

namespace novasvg {
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

// ---- svgproperty ----
enum class PropertyID : uint8_t {
    Unknown = 0,
    Alignment_Baseline,
    Background_Color,
    Baseline_Shift,
    Class,
    Clip_Path,
    Clip_Rule,
    ClipPathUnits,
    Color,
    Cx,
    Cy,
    D,
    Direction,
    Display,
    Dominant_Baseline,
    Dx,
    Dy,
    Fill,
    Fill_Opacity,
    Fill_Rule,
    Filter,
    Flood_Color,
    Flood_Opacity,
    Font_Family,
    Font_Size,
    Font_Style,
    Font_Weight,
    Fx,
    Fy,
    GradientTransform,
    GradientUnits,
    Height,
    Href,
    Id,
    In,
    In2,
    K1,
    K2,
    K3,
    K4,
    LengthAdjust,
    Letter_Spacing,
    Marker_End,
    Marker_Mid,
    Marker_Start,
    MarkerHeight,
    MarkerUnits,
    MarkerWidth,
    Mask,
    Mask_Type,
    MaskContentUnits,
    MaskUnits,
    Mix_Blend_Mode,
    Offset,
    Opacity,
    Operator,
    Orient,
    Overflow,
    PatternContentUnits,
    PatternTransform,
    PatternUnits,
    Pointer_Events,
    Points,
    PreserveAspectRatio,
    R,
    RefX,
    RefY,
    RequiredExtensions,
    Result,
    Rotate,
    Rx,
    Ry,
    SpreadMethod,
    StdDeviation,
    Stop_Color,
    Stop_Opacity,
    Stroke,
    Stroke_Dasharray,
    Stroke_Dashoffset,
    Stroke_Linecap,
    Stroke_Linejoin,
    Stroke_Miterlimit,
    Stroke_Opacity,
    Stroke_Width,
    Style,
    SystemLanguage,
    Text_Anchor,
    Text_Orientation,
    TextLength,
    Transform,
    Transform_Box,
    Transform_Origin,
    ViewBox,
    Visibility,
    White_Space,
    Width,
    Word_Spacing,
    Writing_Mode,
    X,
    X1,
    X2,
    Y,
    Y1,
    Y2
};

PropertyID propertyid(std::string_view name);
PropertyID csspropertyid(std::string_view name);

class svg_element;

class svg_property {
public:
    svg_property(PropertyID id);
    virtual ~svg_property() = default;
    PropertyID id() const { return m_id; }

    virtual bool parse(std::string_view input) = 0;

private:
    svg_property(const svg_property&) = delete;
    svg_property& operator=(const svg_property&) = delete;
    PropertyID m_id;
};

class svg_string final : public svg_property {
public:
    explicit svg_string(PropertyID id)
        : svg_property(id)
    {}

    const std::string& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    std::string m_value;
};

class Paint {
public:
    Paint() = default;
    explicit Paint(const Color& color) : m_color(color) {}
    Paint(const std::string& id, const Color& color)
        : m_id(id), m_color(color)
    {}

    const Color& color() const { return m_color; }
    const std::string& id() const { return m_id; }
    bool isNone() const { return m_id.empty() && !m_color.isVisible(); }

private:
    std::string m_id;
    Color m_color = Color::Transparent;
};

enum class Display : uint8_t {
    Inline,
    None
};

enum class Visibility : uint8_t {
    Visible,
    Hidden,
    Collapse
};

enum class Overflow : uint8_t {
    Visible,
    Hidden
};

// CSS mix-blend-mode. Normal = ordinary painting. Of the other keywords the separable modes below are
// supported; overlay, color-dodge, color-burn, hard-light, soft-light and the non-separable ones (hue,
// saturation, color, luminosity) are not yet and paint as Normal, like a value this library doesn't know.
enum class MixBlendMode : uint8_t {
    Normal,
    Multiply,
    Screen,
    Darken,
    Lighten,
    Difference,
    Exclusion
};

enum class PointerEvents : uint8_t {
    None,
    Auto,
    Stroke,
    Fill,
    Painted,
    Visible,
    VisibleStroke,
    VisibleFill,
    VisiblePainted,
    BoundingBox,
    All
};

enum class FontStyle : uint8_t {
    Normal,
    Italic
};

enum class FontWeight : uint8_t {
    Normal,
    Bold
};

enum class AlignmentBaseline : uint8_t {
    Auto,
    Baseline,
    BeforeEdge,
    TextBeforeEdge,
    Middle,
    Central,
    AfterEdge,
    TextAfterEdge,
    Ideographic,
    Alphabetic,
    Hanging,
    Mathematical
};

enum class DominantBaseline : uint8_t {
    Auto,
    UseScript,
    NoChange,
    ResetSize,
    Ideographic,
    Alphabetic,
    Hanging,
    Mathematical,
    Central,
    Middle,
    TextAfterEdge,
    TextBeforeEdge
};

enum class TextAnchor : uint8_t {
    Start,
    Middle,
    End
};

enum class WhiteSpace : uint8_t {
    Default,
    Preserve
};

enum class WritingMode : uint8_t {
    Horizontal,
    Vertical
};

enum class TextOrientation : uint8_t {
    Mixed,
    Upright
};

enum class Direction : uint8_t {
    Ltr,
    Rtl
};

enum class MaskType : uint8_t {
    Luminance,
    Alpha
};

enum class Units : uint8_t {
    UserSpaceOnUse,
    ObjectBoundingBox
};

enum class MarkerUnits : uint8_t {
    StrokeWidth,
    UserSpaceOnUse
};

enum class LengthAdjust : uint8_t {
    Spacing,
    SpacingAndGlyphs
};

template<typename Enum>
using svg_enumeration_entry = std::pair<Enum, std::string_view>;

template<typename Enum>
class svg_enumeration final : public svg_property {
public:
    explicit svg_enumeration(PropertyID id, Enum value)
        : svg_property(id)
        , m_value(value)
    {}

    Enum value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    template<unsigned int N>
    bool parseEnum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N]);
    Enum m_value;
};

class svg_angle final : public svg_property {
public:
    enum class OrientType {
        Auto,
        AutoStartReverse,
        Angle
    };

    explicit svg_angle(PropertyID id)
        : svg_property(id)
    {}

    float value() const { return m_value; }
    OrientType orientType() const { return m_orientType; }
    bool parse(std::string_view input) final;

private:
    float m_value = 0;
    OrientType m_orientType = OrientType::Angle;
};

enum class LengthUnits : uint8_t {
    None,
    Percent,
    Px,
    Em,
    Ex
};

enum class LengthDirection : uint8_t {
    Horizontal,
    Vertical,
    Diagonal
};

enum class LengthNegativeMode : uint8_t {
    Allow,
    Forbid
};

class Length {
public:
    Length() = default;
    Length(float value, LengthUnits units)
        : m_value(value), m_units(units)
    {}

    float value() const { return m_value; }
    LengthUnits units() const { return m_units; }

    bool parse(std::string_view input, LengthNegativeMode mode);

private:
    float m_value = 0.f;
    LengthUnits m_units = LengthUnits::None;
};

class svg_length final : public svg_property {
public:
    svg_length(PropertyID id, LengthDirection direction, LengthNegativeMode negativeMode, float value = 0, LengthUnits units = LengthUnits::None)
        : svg_property(id)
        , m_direction(direction)
        , m_negativeMode(negativeMode)
        , m_value(value, units)
    {}

    bool isPercent() const { return m_value.units() == LengthUnits::Percent; }

    LengthDirection direction() const { return m_direction; }
    LengthNegativeMode negativeMode() const { return m_negativeMode; }
    const Length& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    const LengthDirection m_direction;
    const LengthNegativeMode m_negativeMode;
    Length m_value;
};

class LengthContext {
public:
    LengthContext(const svg_element* el, Units units = Units::UserSpaceOnUse)
        : m_element(el), m_units(units)
    {}

    float valueForLength(const Length& length, LengthDirection direction) const;
    float valueForLength(const svg_length& length) const { return valueForLength(length.value(), length.direction()); }

private:
    float viewportDimension(LengthDirection direction) const;
    const svg_element* m_element;
    const Units m_units;
};

using LengthList = std::vector<Length>;

class svg_length_list final : public svg_property {
public:
    svg_length_list(PropertyID id, LengthDirection direction, LengthNegativeMode negativeMode)
        : svg_property(id)
        , m_direction(direction)
        , m_negativeMode(negativeMode)
    {}

    LengthDirection direction() const { return m_direction; }
    LengthNegativeMode negativeMode() const { return m_negativeMode; }
    const LengthList& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    const LengthDirection m_direction;
    const LengthNegativeMode m_negativeMode;
    LengthList m_values;
};

class BaselineShift {
public:
    enum class Type {
        Baseline,
        Sub,
        Super,
        Length
    };

    BaselineShift() = default;
    BaselineShift(Type type) : m_type(type) {}
    BaselineShift(const Length& length) : m_type(Type::Length), m_length(length) {}

    Type type() const { return m_type; }
    const Length& length() const { return m_length; }

private:
    Type m_type{Type::Baseline};
    Length m_length;
};

class svg_number : public svg_property {
public:
    svg_number(PropertyID id, float value)
        : svg_property(id)
        , m_value(value)
    {}

    float value() const { return m_value; }
    bool parse(std::string_view input) override;

private:
    float m_value;
};

class svg_number_percentage final : public svg_property {
public:
    svg_number_percentage(PropertyID id, float value)
        : svg_property(id)
        , m_value(value)
    {}

    float value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    float m_value;
};

using NumberList = std::vector<float>;

class svg_number_list final : public svg_property {
public:
    explicit svg_number_list(PropertyID id)
        : svg_property(id)
    {}

    const NumberList& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    NumberList m_values;
};

class svg_path final : public svg_property {
public:
    explicit svg_path(PropertyID id)
        : svg_property(id)
    {}

    const Path& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Path m_value;
};

class svg_point final : public svg_property {
public:
   explicit svg_point(PropertyID id)
        : svg_property(id)
    {}

    const Point& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Point m_value;
};

using PointList = std::vector<Point>;

class svg_point_list final : public svg_property {
public:
    explicit svg_point_list(PropertyID id)
        : svg_property(id)
    {}

    const PointList& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    PointList m_values;
};

class svg_rect final : public svg_property {
public:
    explicit svg_rect(PropertyID id)
        : svg_property(id)
        , m_value(Rect::Invalid)
    {}

    const Rect& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Rect m_value;
};

class svg_transform final : public svg_property {
public:
    explicit svg_transform(PropertyID id)
        : svg_property(id)
    {}

    const Transform& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Transform m_value;
};

enum class TransformBoxType : uint8_t {
    ViewBox,
    FillBox
};

// CSS `transform-box`. Only distinguishes fill-box (and content-box, treated
// the same here) from the default view-box -- stroke-box and the various
// *-box refinements aren't worth the extra bookkeeping for what this is
// used for (resolving `transform-origin`'s reference box).
class svg_transform_box final : public svg_property {
public:
    explicit svg_transform_box(PropertyID id)
        : svg_property(id)
    {}

    TransformBoxType value() const { return m_value; }
    bool specified() const { return m_specified; }
    bool parse(std::string_view input) final;

private:
    TransformBoxType m_value = TransformBoxType::ViewBox;
    bool m_specified = false;
};

// CSS `transform-origin`, 2-value form (x y; an optional 3rd z-offset token
// is skipped if present, this renderer is 2D only). Supports the keywords
// (left/center/right/top/bottom), percentages, and plain numbers (treated
// as user-unit lengths) -- covers the common cases without pulling in the
// full CSS <length-percentage> grammar.
class svg_transform_origin final : public svg_property {
public:
    explicit svg_transform_origin(PropertyID id)
        : svg_property(id)
    {}

    bool specified() const { return m_specified; }
    Point resolve(const Rect& box) const
    {
        auto ox = m_xIsPercent ? box.x + box.w * m_x : box.x + m_x;
        auto oy = m_yIsPercent ? box.y + box.h * m_y : box.y + m_y;
        return Point(ox, oy);
    }
    bool parse(std::string_view input) final;

private:
    float m_x = 0.5f;
    float m_y = 0.5f;
    bool m_xIsPercent = true;
    bool m_yIsPercent = true;
    bool m_specified = false;
};

class svg_preserve_aspect_ratio final : public svg_property {
public:
    enum class AlignType {
        None,
        xMinYMin,
        xMidYMin,
        xMaxYMin,
        xMinYMid,
        xMidYMid,
        xMaxYMid,
        xMinYMax,
        xMidYMax,
        xMaxYMax
    };

    enum class MeetOrSlice {
        Meet,
        Slice
    };

    explicit svg_preserve_aspect_ratio(PropertyID id)
        : svg_property(id)
    {}

    AlignType alignType() const { return m_alignType; }
    MeetOrSlice meetOrSlice() const { return m_meetOrSlice; }
    bool parse(std::string_view input) final;

    Rect getClipRect(const Rect& viewBoxRect, const Size& viewportSize) const;
    Transform getTransform(const Rect& viewBoxRect, const Size& viewportSize) const;
    void transformRect(Rect& dstRect, Rect& srcRect) const;

private:
    AlignType m_alignType = AlignType::xMidYMid;
    MeetOrSlice m_meetOrSlice = MeetOrSlice::Meet;
};

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
    svg_paint_server(const svg_paint_element* el, const Color& color, float opacity)
        : m_element(el), m_color(color), m_opacity(opacity)
    {}

    bool isRenderable() const { return m_opacity > 0.f && (m_element || m_color.getA() > 0); }

    const svg_paint_element* el() const { return m_element; }
    const Color& color() const { return  m_color; }
    float opacity() const { return m_opacity; }

    bool applyPaint(svg_render_state& state) const;

private:
    const svg_paint_element* m_element = nullptr;
    Color m_color = Color::Transparent;
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
    const Bitmap& image() const { return m_image; }

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
    Bitmap m_image;
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
    const std::optional<Color>& htmlTextColor() const { return m_htmlTextColor; }
    const std::optional<Color>& htmlBackgroundColor() const { return m_htmlBackgroundColor; }
    void setHtmlStyle(std::optional<Color> text, std::optional<Color> background)
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
    std::optional<Color> m_htmlTextColor;
    std::optional<Color> m_htmlBackgroundColor;
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
            alpha->tint_to_flood_color(Color(0, 0, 0), 1.f);
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
        Color floodColor = floodColorAttr.empty() ? Color(0, 0, 0) : Color(floodColorAttr);
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
        Color floodColor = floodColorAttr.empty() ? Color(0, 0, 0) : Color(floodColorAttr);
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
        , m_mode(parent.mode()), m_canvas(parent.raster())
    {}

    svg_render_state(const svg_element* el, const svg_render_state* parent, const Transform& currentTransform, svg_render_mode mode, std::shared_ptr<raster_canvas> canvas)
        : m_element(el), m_parent(parent), m_currentTransform(currentTransform), m_mode(mode), m_canvas(std::move(canvas))
    {}

    canvas& operator*() const { return *m_canvas; }
    canvas* operator->() const { return &*m_canvas; }

    const svg_element* el() const { return m_element; }
    const svg_render_state* parent() const { return m_parent; }
    const Transform& currentTransform() const { return m_currentTransform; }
    const svg_render_mode mode() const { return m_mode; }
    const std::shared_ptr<raster_canvas>& raster() const { return m_canvas; }

    Rect fillBoundingBox() const { return m_element->fillBoundingBox(); }
    Rect paint_bounding_box() const { return m_element->paint_bounding_box(); }

    bool hasCycleReference(const svg_element* el) const;

    void beginGroup(const svg_blend_info& blendInfo);
    void endGroup(const svg_blend_info& blendInfo);

private:
    const svg_element* m_element;
    const svg_render_state* m_parent;
    const Transform m_currentTransform;
    const svg_render_mode m_mode;
    std::shared_ptr<raster_canvas> m_canvas;
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
    Color m_stop_color = Color::Black;
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

class svg_geometry_element : public svg_graphics_element {
public:
    svg_geometry_element(document* dom, ElementID id);

    bool isGeometryElement() const final { return true; }

    Rect fillBoundingBox() const override { return m_fillBoundingBox; }
    Rect strokeBoundingBox() const override;
    void layoutElement(const svg_layout_state& state) override;

    bool isRenderable() const { return !m_path.is_null() && !isDisplayNone() && !isVisibilityHidden(); }

    FillRule fill_rule() const { return m_fill_rule; }
    FillRule clip_rule() const { return m_clip_rule; }

    virtual Rect updateShape(Path& path) = 0;

    void updateMarkerPositions(svg_marker_position_list& positions, const svg_layout_state& state);
    void render(svg_render_state& state) const override;

    const Path& path() const { return m_path; }

private:
    Path m_path;
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

    Rect updateShape(Path& path) final;

private:
    svg_length m_x1;
    svg_length m_y1;
    svg_length m_x2;
    svg_length m_y2;
};

class svg_rect_element final : public svg_geometry_element {
public:
    svg_rect_element(document* dom);

    Rect updateShape(Path& path) final;

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

    Rect updateShape(Path& path) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_rx;
    svg_length m_ry;
};

class svg_circle_element final : public svg_geometry_element {
public:
    svg_circle_element(document* dom);

    Rect updateShape(Path& path) final;

private:
    svg_length m_cx;
    svg_length m_cy;
    svg_length m_r;
};

class svg_poly_element final : public svg_geometry_element {
public:
    svg_poly_element(document* dom, ElementID id);

    Rect updateShape(Path& path) final;

private:
    svg_point_list m_points;
};

class svg_path_element final : public svg_geometry_element {
public:
    svg_path_element(document* dom);

    Rect updateShape(Path& path) final;

private:
    svg_path m_d;
};

// ---- svgtextelement ----

class svg_text_positioning_element;
class svg_text_element;

struct svg_character_position {
    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> dx;
    std::optional<float> dy;
    std::optional<float> rotate;
};

using svg_character_positions = std::map<size_t, svg_character_position>;

struct svg_text_position {
    svg_text_position(const svg_node* nd, size_t startOffset, size_t endOffset)
        : nd(nd), startOffset(startOffset), endOffset(endOffset)
    {}

    const svg_node* nd;
    size_t startOffset;
    size_t endOffset;
};

using svg_text_position_list = std::vector<svg_text_position>;

struct svg_text_fragment {
    explicit svg_text_fragment(const svg_text_positioning_element* el) : el(el) {}
    const svg_text_positioning_element* el;
    Transform lengthAdjustTransform;
    size_t offset = 0;
    size_t length = 0;
    bool startsNewTextChunk = false;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float angle = 0;
};

using svg_text_fragment_list = std::vector<svg_text_fragment>;

class svg_text_fragments_builder {
public:
    svg_text_fragments_builder(std::u32string& text, svg_text_fragment_list& fragments);

    void build(const svg_text_element* textElement);

private:
    void handleText(const svg_text_node* nd);
    void handleElement(const svg_text_positioning_element* el);
    void fillCharacterPositions(const svg_text_position& position);
    std::u32string& m_text;
    svg_text_fragment_list& m_fragments;
    svg_character_positions m_characterPositions;
    svg_text_position_list m_textPositions;
    size_t m_characterOffset = 0;
    float m_x = 0;
    float m_y = 0;
};

class svg_text_positioning_element : public svg_graphics_element {
public:
    svg_text_positioning_element(document* dom, ElementID id);

    bool isTextPositioningElement() const final { return true; }

    const LengthList& x() const { return m_x.values(); }
    const LengthList& y() const { return m_y.values(); }
    const LengthList& dx() const { return m_dx.values(); }
    const LengthList& dy() const { return m_dy.values(); }
    const NumberList& rotate() const { return m_rotate.values(); }

    const svg_length& textLength() const { return m_textLength; }
    LengthAdjust lengthAdjust() const { return m_lengthAdjust.value(); }

    const Font& font() const { return m_font; }
    const svg_paint_server& fill() const { return m_fill; }
    const svg_paint_server& stroke() const { return m_stroke; }

    bool isVerticalWritingMode() const { return m_writing_mode == WritingMode::Vertical; }
    bool isUprightTextOrientation() const { return m_text_orientation == TextOrientation::Upright; }

    float stroke_width() const { return m_stroke_width; }
    float letter_spacing() const { return m_letter_spacing; }
    float word_spacing() const { return m_word_spacing; }
    float baseline_offset() const { return m_baseline_offset; }
    AlignmentBaseline alignment_baseline() const { return m_alignment_baseline; }
    DominantBaseline dominant_baseline() const { return m_dominant_baseline; }
    TextAnchor text_anchor() const { return m_text_anchor; }
    WhiteSpace white_space() const { return m_white_space; }
    Direction direction() const { return m_direction; }

    void layoutElement(const svg_layout_state& state) override;

private:
    float convertBaselineOffset(const BaselineShift& baselineShift) const;
    svg_length_list m_x;
    svg_length_list m_y;
    svg_length_list m_dx;
    svg_length_list m_dy;
    svg_number_list m_rotate;

    svg_length m_textLength;
    svg_enumeration<LengthAdjust> m_lengthAdjust;

    Font m_font;
    svg_paint_server m_fill;
    svg_paint_server m_stroke;

    float m_stroke_width = 1.f;
    float m_letter_spacing = 0.f;
    float m_word_spacing = 0.f;
    float m_baseline_offset = 0.f;
    AlignmentBaseline m_alignment_baseline = AlignmentBaseline::Auto;
    DominantBaseline m_dominant_baseline = DominantBaseline::Auto;
    TextAnchor m_text_anchor = TextAnchor::Start;
    WhiteSpace m_white_space = WhiteSpace::Default;
    WritingMode m_writing_mode = WritingMode::Horizontal;
    TextOrientation m_text_orientation = TextOrientation::Mixed;
    Direction m_direction = Direction::Ltr;
};

class svg_t_span_element final : public svg_text_positioning_element {
public:
    svg_t_span_element(document* dom);
};

class svg_text_element final : public svg_text_positioning_element {
public:
    svg_text_element(document* dom);

    Rect fillBoundingBox() const final { return bounding_box(false); }
    Rect strokeBoundingBox() const final { return bounding_box(true); }

    void layout(svg_layout_state& state) final;
    void render(svg_render_state& state) const final;

private:
    Rect bounding_box(bool includeStroke) const;
    svg_text_fragment_list m_fragments;
    std::u32string m_text;
};

// ---- svglayoutstate ----

class svg_layout_state {
public:
    svg_layout_state() = default;
    svg_layout_state(const svg_layout_state& parent, const svg_element* el);

    const svg_layout_state* parent() const { return m_parent; }
    const svg_element* el() const { return m_element; }

    const Paint& fill() const { return m_fill; }
    const Paint& stroke() const { return m_stroke; }

    const Color& color() const { return m_color; }
    const Color& stop_color() const { return m_stop_color; }

    float opacity() const { return m_opacity; }
    MixBlendMode mix_blend_mode() const { return m_mix_blend_mode; }
    float stop_opacity() const { return m_stop_opacity; }
    float fill_opacity() const { return m_fill_opacity; }
    float stroke_opacity() const { return m_stroke_opacity; }
    float stroke_miterlimit() const { return m_stroke_miterlimit; }
    float font_size() const { return m_font_size; }

    const Length& letter_spacing() const { return m_letter_spacing; }
    const Length& word_spacing() const { return m_word_spacing; }

    const BaselineShift& baseline_shift() const { return m_baseline_shift; }
    const Length& stroke_width() const { return m_stroke_width; }
    const Length& stroke_dashoffset() const { return m_stroke_dashoffset; }
    const LengthList& stroke_dasharray() const { return m_stroke_dasharray; }

    LineCap stroke_linecap() const { return m_stroke_linecap; }
    LineJoin stroke_linejoin() const { return m_stroke_linejoin; }

    FillRule fill_rule() const { return m_fill_rule; }
    FillRule clip_rule() const { return m_clip_rule; }

    FontWeight font_weight() const { return m_font_weight; }
    FontStyle font_style() const { return m_font_style; }

    AlignmentBaseline alignment_baseline() const { return m_alignment_baseline; }
    DominantBaseline dominant_baseline() const { return m_dominant_baseline; }

    TextAnchor text_anchor() const { return m_text_anchor; }
    WhiteSpace white_space() const { return m_white_space; }
    WritingMode writing_mode() const { return m_writing_mode; }
    TextOrientation text_orientation() const { return m_text_orientation; }
    Direction direction() const { return m_direction; }

    Display display() const { return m_display; }
    Visibility visibility() const { return m_visibility; }
    Overflow overflow() const { return m_overflow; }
    PointerEvents pointer_events() const { return m_pointer_events; }
    MaskType mask_type() const { return m_mask_type; }

    const std::string& mask() const { return m_mask; }
    const std::string& clip_path() const { return m_clip_path; }
    const std::string& filter() const { return m_filter; }
    const std::string& marker_start() const { return m_marker_start; }
    const std::string& marker_mid() const { return m_marker_mid; }
    const std::string& marker_end() const { return m_marker_end; }
    const std::string& font_family() const { return m_font_family; }

    Font font() const;

private:
    const svg_layout_state* m_parent = nullptr;
    const svg_element* m_element = nullptr;

    Paint m_fill{Color::Black};
    Paint m_stroke{Color::Transparent};

    Color m_color = Color::Black;
    Color m_stop_color = Color::Black;

    float m_opacity = 1.f;
    MixBlendMode m_mix_blend_mode = MixBlendMode::Normal; // like opacity: not inherited
    float m_fill_opacity = 1.f;
    float m_stroke_opacity = 1.f;
    float m_stop_opacity = 1.f;
    float m_stroke_miterlimit = 4.f;
    float m_font_size = 12.f;

    Length m_letter_spacing{0.f, LengthUnits::None};
    Length m_word_spacing{0.f, LengthUnits::None};

    BaselineShift m_baseline_shift;
    Length m_stroke_width{1.f, LengthUnits::None};
    Length m_stroke_dashoffset{0.f, LengthUnits::None};
    LengthList m_stroke_dasharray;

    LineCap m_stroke_linecap = LineCap::Butt;
    LineJoin m_stroke_linejoin = LineJoin::Miter;

    FillRule m_fill_rule = FillRule::NonZero;
    FillRule m_clip_rule = FillRule::NonZero;

    FontWeight m_font_weight = FontWeight::Normal;
    FontStyle m_font_style = FontStyle::Normal;

    AlignmentBaseline m_alignment_baseline = AlignmentBaseline::Auto;
    DominantBaseline m_dominant_baseline = DominantBaseline::Auto;

    TextAnchor m_text_anchor = TextAnchor::Start;
    WhiteSpace m_white_space = WhiteSpace::Default;
    WritingMode m_writing_mode = WritingMode::Horizontal;
    TextOrientation m_text_orientation = TextOrientation::Mixed;
    Direction m_direction = Direction::Ltr;

    Display m_display = Display::Inline;
    Visibility m_visibility = Visibility::Visible;
    Overflow m_overflow = Overflow::Visible;
    PointerEvents m_pointer_events = PointerEvents::Auto;
    MaskType m_mask_type = MaskType::Luminance;

    std::string m_mask;
    std::string m_clip_path;
    std::string m_filter;
    std::string m_marker_start;
    std::string m_marker_mid;
    std::string m_marker_end;
    std::string m_font_family;
};

// ================================================================
// Implementation
// ================================================================

// ---- svgproperty (impl) ----
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE PropertyID propertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        PropertyID value;
    } table[] = {
        {"class", PropertyID::Class},
        {"clipPathUnits", PropertyID::ClipPathUnits},
        {"cx", PropertyID::Cx},
        {"cy", PropertyID::Cy},
        {"d", PropertyID::D},
        {"dx", PropertyID::Dx},
        {"dy", PropertyID::Dy},
        {"fx", PropertyID::Fx},
        {"fy", PropertyID::Fy},
        {"gradientTransform", PropertyID::GradientTransform},
        {"gradientUnits", PropertyID::GradientUnits},
        {"height", PropertyID::Height},
        {"href", PropertyID::Href},
        {"id", PropertyID::Id},
        {"in", PropertyID::In},
        {"in2", PropertyID::In2},
        {"k1", PropertyID::K1},
        {"k2", PropertyID::K2},
        {"k3", PropertyID::K3},
        {"k4", PropertyID::K4},
        {"lengthAdjust", PropertyID::LengthAdjust},
        {"markerHeight", PropertyID::MarkerHeight},
        {"markerUnits", PropertyID::MarkerUnits},
        {"markerWidth", PropertyID::MarkerWidth},
        {"maskContentUnits", PropertyID::MaskContentUnits},
        {"maskUnits", PropertyID::MaskUnits},
        {"offset", PropertyID::Offset},
        {"operator", PropertyID::Operator},
        {"orient", PropertyID::Orient},
        {"patternContentUnits", PropertyID::PatternContentUnits},
        {"patternTransform", PropertyID::PatternTransform},
        {"patternUnits", PropertyID::PatternUnits},
        {"points", PropertyID::Points},
        {"preserveAspectRatio", PropertyID::PreserveAspectRatio},
        {"r", PropertyID::R},
        {"refX", PropertyID::RefX},
        {"refY", PropertyID::RefY},
        {"requiredExtensions", PropertyID::RequiredExtensions},
        {"result", PropertyID::Result},
        {"rotate", PropertyID::Rotate},
        {"rx", PropertyID::Rx},
        {"ry", PropertyID::Ry},
        {"spreadMethod", PropertyID::SpreadMethod},
        {"stdDeviation", PropertyID::StdDeviation},
        {"style", PropertyID::Style},
        {"systemLanguage", PropertyID::SystemLanguage},
        {"textLength", PropertyID::TextLength},
        {"transform", PropertyID::Transform},
        {"viewBox", PropertyID::ViewBox},
        {"width", PropertyID::Width},
        {"x", PropertyID::X},
        {"x1", PropertyID::X1},
        {"x2", PropertyID::X2},
        {"xlink:href", PropertyID::Href},
        {"xml:space", PropertyID::White_Space},
        {"y", PropertyID::Y},
        {"y1", PropertyID::Y1},
        {"y2", PropertyID::Y2}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return csspropertyid(name);
    return it->value;
}

NOVASVG_INLINE PropertyID csspropertyid(std::string_view name)
{
    static const struct {
        std::string_view name;
        PropertyID value;
    } table[] = {
        {"alignment-baseline", PropertyID::Alignment_Baseline},
        {"background-color", PropertyID::Background_Color},
        {"baseline-shift", PropertyID::Baseline_Shift},
        {"clip-path", PropertyID::Clip_Path},
        {"clip-rule", PropertyID::Clip_Rule},
        {"color", PropertyID::Color},
        {"direction", PropertyID::Direction},
        {"display", PropertyID::Display},
        {"dominant-baseline", PropertyID::Dominant_Baseline},
        {"fill", PropertyID::Fill},
        {"fill-opacity", PropertyID::Fill_Opacity},
        {"fill-rule", PropertyID::Fill_Rule},
        {"filter", PropertyID::Filter},
        {"flood-color", PropertyID::Flood_Color},
        {"flood-opacity", PropertyID::Flood_Opacity},
        {"font-family", PropertyID::Font_Family},
        {"font-size", PropertyID::Font_Size},
        {"font-style", PropertyID::Font_Style},
        {"font-weight", PropertyID::Font_Weight},
        {"letter-spacing", PropertyID::Letter_Spacing},
        {"marker-end", PropertyID::Marker_End},
        {"marker-mid", PropertyID::Marker_Mid},
        {"marker-start", PropertyID::Marker_Start},
        {"mask", PropertyID::Mask},
        {"mask-type", PropertyID::Mask_Type},
        {"mix-blend-mode", PropertyID::Mix_Blend_Mode},
        {"opacity", PropertyID::Opacity},
        {"overflow", PropertyID::Overflow},
        {"pointer-events", PropertyID::Pointer_Events},
        {"stop-color", PropertyID::Stop_Color},
        {"stop-opacity", PropertyID::Stop_Opacity},
        {"stroke", PropertyID::Stroke},
        {"stroke-dasharray", PropertyID::Stroke_Dasharray},
        {"stroke-dashoffset", PropertyID::Stroke_Dashoffset},
        {"stroke-linecap", PropertyID::Stroke_Linecap},
        {"stroke-linejoin", PropertyID::Stroke_Linejoin},
        {"stroke-miterlimit", PropertyID::Stroke_Miterlimit},
        {"stroke-opacity", PropertyID::Stroke_Opacity},
        {"stroke-width", PropertyID::Stroke_Width},
        {"text-anchor", PropertyID::Text_Anchor},
        {"text-orientation", PropertyID::Text_Orientation},
        {"transform-box", PropertyID::Transform_Box},
        {"transform-origin", PropertyID::Transform_Origin},
        {"visibility", PropertyID::Visibility},
        {"white-space", PropertyID::White_Space},
        {"word-spacing", PropertyID::Word_Spacing},
        {"writing-mode", PropertyID::Writing_Mode}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return PropertyID::Unknown;
    return it->value;
}

NOVASVG_INLINE svg_property::svg_property(PropertyID id)
    : m_id(id)
{
}

NOVASVG_INLINE bool svg_string::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    m_value.assign(input);
    return true;
}

template<>
NOVASVG_INLINE bool svg_enumeration<SpreadMethod>::parse(std::string_view input)
{
    static const svg_enumeration_entry<SpreadMethod> entries[] = {
        {SpreadMethod::Pad, "pad"},
        {SpreadMethod::Reflect, "reflect"},
        {SpreadMethod::Repeat, "repeat"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool svg_enumeration<Units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<Units> entries[] = {
        {Units::UserSpaceOnUse, "userSpaceOnUse"},
        {Units::ObjectBoundingBox, "objectBoundingBox"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool svg_enumeration<MarkerUnits>::parse(std::string_view input)
{
    static const svg_enumeration_entry<MarkerUnits> entries[] = {
        {MarkerUnits::StrokeWidth, "strokeWidth"},
        {MarkerUnits::UserSpaceOnUse, "userSpaceOnUse"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool svg_enumeration<LengthAdjust>::parse(std::string_view input)
{
    static const svg_enumeration_entry<LengthAdjust> entries[] = {
        {LengthAdjust::Spacing, "spacing"},
        {LengthAdjust::SpacingAndGlyphs, "spacingAndGlyphs"}
    };

    return parseEnum(input, entries);
}

template<typename Enum>
template<unsigned int N>
NOVASVG_INLINE bool svg_enumeration<Enum>::parseEnum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N])
{
    stripLeadingAndTrailingSpaces(input);
    for(const auto& entry : entries) {
        if(input == entry.second) {
            m_value = entry.first;
            return true;
        }
    }

    return false;
}

NOVASVG_INLINE bool svg_angle::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    if(input == "auto") {
        m_value = 0.f;
        m_orientType = OrientType::Auto;
        return true;
    }

    if(input == "auto-start-reverse") {
        m_value = 0.f;
        m_orientType = OrientType::AutoStartReverse;
        return true;
    }

    float value = 0.f;
    if(!parseNumber(input, value))
        return false;
    if(!input.empty()) {
        if(input == "rad")
            value *= 180.f / NOVASVG_PI;
        else if(input == "grad")
            value *= 360.f / 400.f;
        else if(input == "turn")
            value *= 360.f;
        else if(input != "deg") {
            return false;
        }
    }

    m_value = value;
    m_orientType = OrientType::Angle;
    return true;
}

NOVASVG_INLINE bool Length::parse(std::string_view input, LengthNegativeMode mode)
{
    float value = 0.f;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value))
        return false;
    if(value < 0.f && mode == LengthNegativeMode::Forbid)
        return false;
    if(input.empty()) {
        m_value = value;
        m_units = LengthUnits::None;
        return true;
    }

    constexpr auto dpi = 96.f;
    switch(input.front()) {
    case '%':
        m_value = value;
        m_units = LengthUnits::Percent;
        input.remove_prefix(1);
        break;
    case 'p':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'x')
            m_value = value;
        else if(input.front() == 'c')
            m_value = value * dpi / 6.f;
        else if(input.front() == 't')
            m_value = value * dpi / 72.f;
        else
            return false;
        m_units = LengthUnits::Px;
        input.remove_prefix(1);
        break;
    case 'i':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'n')
            m_value = value * dpi;
        else
            return false;
        m_units = LengthUnits::Px;
        input.remove_prefix(1);
        break;
    case 'c':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_value = value * dpi / 2.54f;
        else
            return false;
        m_units = LengthUnits::Px;
        input.remove_prefix(1);
        break;
    case 'm':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_value = value * dpi / 25.4f;
        else
            return false;
        m_units = LengthUnits::Px;
        input.remove_prefix(1);
        break;
    case 'e':
        input.remove_prefix(1);
        if(input.empty())
            return false;
        else if(input.front() == 'm')
            m_units = LengthUnits::Em;
        else if(input.front() == 'x')
            m_units = LengthUnits::Ex;
        else
            return false;
        m_value = value;
        input.remove_prefix(1);
        break;
    default:
        return false;
    }

    return input.empty();
}

NOVASVG_INLINE float LengthContext::valueForLength(const Length& length, LengthDirection direction) const
{
    if(length.units() == LengthUnits::Percent) {
        if(m_units == Units::UserSpaceOnUse)
            return length.value() * viewportDimension(direction) / 100.f;
        return length.value() / 100.f;
    }

    if(length.units() == LengthUnits::Ex)
        return length.value() * m_element->font_size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_element->font_size();
    return length.value();
}

NOVASVG_INLINE float LengthContext::viewportDimension(LengthDirection direction) const
{
    auto viewportSize = m_element->currentViewportSize();
    switch(direction) {
    case LengthDirection::Horizontal:
        return viewportSize.w;
    case LengthDirection::Vertical:
        return viewportSize.h;
    default:
        return std::sqrt(viewportSize.w * viewportSize.w + viewportSize.h * viewportSize.h) / NOVASVG_SQRT2;
    }
}

NOVASVG_INLINE bool svg_length::parse(std::string_view input)
{
    return m_value.parse(input, m_negativeMode);
}

NOVASVG_INLINE bool svg_length_list::parse(std::string_view input)
{
    m_values.clear();
    while(!input.empty()) {
        size_t count = 0;
        while(count < input.length() && input[count] != ',' && !IS_WS(input[count]))
            ++count;
        if(count == 0)
            break;
        Length value(0, LengthUnits::None);
        if(!value.parse(input.substr(0, count), m_negativeMode))
            return false;
        input.remove_prefix(count);
        skipOptionalSpacesOrComma(input);
        m_values.push_back(value);
    }

    return true;
}

NOVASVG_INLINE bool svg_number::parse(std::string_view input)
{
    float value = 0.f;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value))
        return false;
    if(!input.empty())
        return false;
    m_value = value;
    return true;
}

NOVASVG_INLINE bool svg_number_percentage::parse(std::string_view input)
{
    float value = 0.f;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value))
        return false;
    if(!input.empty() && input.front() == '%') {
        value /= 100.f;
        input.remove_prefix(1);
    }

    if(!input.empty())
        return false;
    m_value = std::clamp(value, 0.f, 1.f);
    return true;
}

NOVASVG_INLINE bool svg_number_list::parse(std::string_view input)
{
    m_values.clear();
    stripLeadingSpaces(input);
    while(!input.empty()) {
        float value = 0.f;
        if(!parseNumber(input, value))
            return false;
        skipOptionalSpacesOrComma(input);
        m_values.push_back(value);
    }

    return true;
}

NOVASVG_INLINE bool svg_path::parse(std::string_view input)
{
    return m_value.parse(input.data(), input.length());
}

NOVASVG_INLINE bool svg_point::parse(std::string_view input)
{
    Point value;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value.x)
        || !skipOptionalSpaces(input)
        || !parseNumber(input, value.y)
        || !input.empty()) {
        return false;
    }

    m_value = value;
    return true;
}

NOVASVG_INLINE bool svg_point_list::parse(std::string_view input)
{
    m_values.clear();
    stripLeadingSpaces(input);
    while(!input.empty()) {
        Point value;
        if(!parseNumber(input, value.x)
            || !skipOptionalSpacesOrComma(input)
            || !parseNumber(input, value.y)) {
            return false;
        }

        m_values.push_back(value);
        skipOptionalSpacesOrComma(input);
    }

    return true;
}

NOVASVG_INLINE bool svg_rect::parse(std::string_view input)
{
    Rect value;
    stripLeadingAndTrailingSpaces(input);
    if(!parseNumber(input, value.x)
        || !skipOptionalSpacesOrComma(input)
        || !parseNumber(input, value.y)
        || !skipOptionalSpacesOrComma(input)
        || !parseNumber(input, value.w)
        || !skipOptionalSpacesOrComma(input)
        || !parseNumber(input, value.h)
        || !input.empty()) {
        return false;
    }

    if(value.w < 0.f || value.h < 0.f)
        return false;
    m_value = value;
    return true;
}

NOVASVG_INLINE bool svg_transform::parse(std::string_view input)
{
    return m_value.parse(input.data(), input.length());
}

NOVASVG_INLINE bool svg_transform_box::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    if(input == "fill-box" || input == "content-box") {
        m_value = TransformBoxType::FillBox;
    } else if(input == "view-box" || input == "border-box" || input == "stroke-box") {
        // border-box/stroke-box aren't distinguished from view-box here (see
        // the class comment) -- treated as "not fill-box" for origin purposes.
        m_value = TransformBoxType::ViewBox;
    } else {
        return false;
    }

    m_specified = true;
    return true;
}

NOVASVG_INLINE bool svg_transform_origin::parse(std::string_view input)
{
    auto parseComponent = [](std::string_view token, float& value, bool& isPercent) {
        if(token == "left" || token == "top") { value = 0.f; isPercent = true; return true; }
        if(token == "center") { value = 0.5f; isPercent = true; return true; }
        if(token == "right" || token == "bottom") { value = 1.f; isPercent = true; return true; }
        float number = 0.f;
        if(!parseNumber(token, number))
            return false;
        if(!token.empty() && token.front() == '%') {
            value = number / 100.f;
            isPercent = true;
            token.remove_prefix(1);
        } else {
            value = number;
            isPercent = false;
        }
        return token.empty();
    };

    std::vector<std::string_view> tokens;
    stripLeadingAndTrailingSpaces(input);
    while(!input.empty()) {
        size_t count = 0;
        while(count < input.length() && !IS_WS(input[count]))
            ++count;
        tokens.push_back(input.substr(0, count));
        input.remove_prefix(count);
        skipOptionalSpaces(input);
    }

    if(tokens.empty() || tokens.size() > 2)
        return false;

    float x = 0.5f, y = 0.5f;
    bool xIsPercent = true, yIsPercent = true;
    if(tokens.size() == 1) {
        if(tokens[0] == "top" || tokens[0] == "bottom") {
            if(!parseComponent(tokens[0], y, yIsPercent))
                return false;
        } else if(!parseComponent(tokens[0], x, xIsPercent)) {
            return false;
        }
    } else {
        if(!parseComponent(tokens[0], x, xIsPercent) || !parseComponent(tokens[1], y, yIsPercent))
            return false;
    }

    m_x = x;
    m_y = y;
    m_xIsPercent = xIsPercent;
    m_yIsPercent = yIsPercent;
    m_specified = true;
    return true;
}

NOVASVG_INLINE bool svg_preserve_aspect_ratio::parse(std::string_view input)
{
    auto alignType = AlignType::xMidYMid;
    stripLeadingSpaces(input);
    if(skipString(input, "none"))
        alignType = AlignType::None;
    else if(skipString(input, "xMinYMin"))
        alignType = AlignType::xMinYMin;
    else if(skipString(input, "xMidYMin"))
        alignType = AlignType::xMidYMin;
    else if(skipString(input, "xMaxYMin"))
        alignType = AlignType::xMaxYMin;
    else if(skipString(input, "xMinYMid"))
        alignType = AlignType::xMinYMid;
    else if(skipString(input, "xMidYMid"))
        alignType = AlignType::xMidYMid;
    else if(skipString(input, "xMaxYMid"))
        alignType = AlignType::xMaxYMid;
    else if(skipString(input, "xMinYMax"))
        alignType = AlignType::xMinYMax;
    else if(skipString(input, "xMidYMax"))
        alignType = AlignType::xMidYMax;
    else if(skipString(input, "xMaxYMax"))
        alignType = AlignType::xMaxYMax;
    else {
        return false;
    }

    auto meetOrSlice = MeetOrSlice::Meet;
    skipOptionalSpaces(input);
    if(skipString(input, "meet")) {
        meetOrSlice = MeetOrSlice::Meet;
    } else if(skipString(input, "slice")) {
        meetOrSlice = MeetOrSlice::Slice;
    }

    if(alignType == AlignType::None)
        meetOrSlice = MeetOrSlice::Meet;
    skipOptionalSpaces(input);
    if(!input.empty())
        return false;
    m_alignType = alignType;
    m_meetOrSlice = meetOrSlice;
    return true;
}

NOVASVG_INLINE Rect svg_preserve_aspect_ratio::getClipRect(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.empty() && !viewportSize.empty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Rect(viewBoxRect.x, viewBoxRect.y, viewportSize.w / xScale, viewportSize.h / yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? STB_MIN(xScale, yScale) : STB_MAX(xScale, yScale);
    auto xOffset = -viewBoxRect.x * scale;
    auto yOffset = -viewBoxRect.y * scale;
    auto viewWidth = viewBoxRect.w * scale;
    auto viewHeight = viewBoxRect.h * scale;
    switch(m_alignType) {
    case AlignType::xMidYMin:
    case AlignType::xMidYMid:
    case AlignType::xMidYMax:
        xOffset += (viewportSize.w - viewWidth) * 0.5f;
        break;
    case AlignType::xMaxYMin:
    case AlignType::xMaxYMid:
    case AlignType::xMaxYMax:
        xOffset += (viewportSize.w - viewWidth);
        break;
    default:
        break;
    }

    switch(m_alignType) {
    case AlignType::xMinYMid:
    case AlignType::xMidYMid:
    case AlignType::xMaxYMid:
        yOffset += (viewportSize.h - viewHeight) * 0.5f;
        break;
    case AlignType::xMinYMax:
    case AlignType::xMidYMax:
    case AlignType::xMaxYMax:
        yOffset += (viewportSize.h - viewHeight);
        break;
    default:
        break;
    }

    return Rect(-xOffset / scale, -yOffset / scale, viewportSize.w / scale, viewportSize.h / scale);
}

NOVASVG_INLINE Transform svg_preserve_aspect_ratio::getTransform(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.empty() && !viewportSize.empty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Transform(xScale, 0, 0, yScale, -viewBoxRect.x * xScale, -viewBoxRect.y * yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? STB_MIN(xScale, yScale) : STB_MAX(xScale, yScale);
    auto xOffset = -viewBoxRect.x * scale;
    auto yOffset = -viewBoxRect.y * scale;
    auto viewWidth = viewBoxRect.w * scale;
    auto viewHeight = viewBoxRect.h * scale;
    switch(m_alignType) {
    case AlignType::xMidYMin:
    case AlignType::xMidYMid:
    case AlignType::xMidYMax:
        xOffset += (viewportSize.w - viewWidth) * 0.5f;
        break;
    case AlignType::xMaxYMin:
    case AlignType::xMaxYMid:
    case AlignType::xMaxYMax:
        xOffset += (viewportSize.w - viewWidth);
        break;
    default:
        break;
    }

    switch(m_alignType) {
    case AlignType::xMinYMid:
    case AlignType::xMidYMid:
    case AlignType::xMaxYMid:
        yOffset += (viewportSize.h - viewHeight) * 0.5f;
        break;
    case AlignType::xMinYMax:
    case AlignType::xMidYMax:
    case AlignType::xMaxYMax:
        yOffset += (viewportSize.h - viewHeight);
        break;
    default:
        break;
    }

    return Transform(scale, 0, 0, scale, xOffset, yOffset);
}

NOVASVG_INLINE void svg_preserve_aspect_ratio::transformRect(Rect& dstRect, Rect& srcRect) const
{
    if(m_alignType == AlignType::None)
        return;
    auto viewSize = dstRect.size();
    auto imageSize = srcRect.size();
    if(m_meetOrSlice == MeetOrSlice::Meet) {
        auto scale = imageSize.h / imageSize.w;
        if(viewSize.h > viewSize.w * scale) {
            dstRect.h = viewSize.w * scale;
            switch(m_alignType) {
            case AlignType::xMinYMid:
            case AlignType::xMidYMid:
            case AlignType::xMaxYMid:
                dstRect.y += (viewSize.h - dstRect.h) * 0.5f;
                break;
            case AlignType::xMinYMax:
            case AlignType::xMidYMax:
            case AlignType::xMaxYMax:
                dstRect.y += viewSize.h - dstRect.h;
                break;
            default:
                break;
            }
        }

        if(viewSize.w > viewSize.h / scale) {
            dstRect.w = viewSize.h / scale;
            switch(m_alignType) {
            case AlignType::xMidYMin:
            case AlignType::xMidYMid:
            case AlignType::xMidYMax:
                dstRect.x += (viewSize.w - dstRect.w) * 0.5f;
                break;
            case AlignType::xMaxYMin:
            case AlignType::xMaxYMid:
            case AlignType::xMaxYMax:
                dstRect.x += viewSize.w - dstRect.w;
                break;
            default:
                break;
            }
        }
    } else if(m_meetOrSlice == MeetOrSlice::Slice) {
        auto scale = imageSize.h / imageSize.w;
        if(viewSize.h < viewSize.w * scale) {
            srcRect.h = viewSize.h * (imageSize.w / viewSize.w);
            switch(m_alignType) {
            case AlignType::xMinYMid:
            case AlignType::xMidYMid:
            case AlignType::xMaxYMid:
                srcRect.y += (imageSize.h - srcRect.h) * 0.5f;
                break;
            case AlignType::xMinYMax:
            case AlignType::xMidYMax:
            case AlignType::xMaxYMax:
                srcRect.y += imageSize.h - srcRect.h;
                break;
            default:
                break;
            }
        }

        if(viewSize.w < viewSize.h / scale) {
            srcRect.w = viewSize.w * (imageSize.h / viewSize.h);
            switch(m_alignType) {
            case AlignType::xMidYMin:
            case AlignType::xMidYMid:
            case AlignType::xMidYMax:
                srcRect.x += (imageSize.w - srcRect.w) * 0.5f;
                break;
            case AlignType::xMaxYMin:
            case AlignType::xMaxYMid:
            case AlignType::xMaxYMax:
                srcRect.x += imageSize.w - srcRect.w;
                break;
            default:
                break;
            }
        }
    }
}

// ---- svgrenderstate (impl) ----
NOVASVG_INLINE svg_blend_info::svg_blend_info(const svg_element* el)
    : m_clipper(el->clipper())
    , m_masker(el->masker())
    , m_filter(el->filterElement())
    , m_opacity(el->opacity())
    , m_mixBlendMode(el->mixBlendMode())
{
}

NOVASVG_INLINE bool svg_blend_info::requiresCompositing(svg_render_mode mode) const
{
    return (m_clipper && m_clipper->requiresMasking()) || (mode == svg_render_mode::Painting && (m_masker || m_filter || m_opacity < 1.f || m_mixBlendMode != MixBlendMode::Normal));
}

NOVASVG_INLINE bool svg_render_state::hasCycleReference(const svg_element* el) const
{
    auto current = this;
    do {
        if(el == current->el())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

NOVASVG_INLINE void svg_render_state::beginGroup(const svg_blend_info& blendInfo)
{
    auto requiresCompositing = blendInfo.requiresCompositing(m_mode);
    if(requiresCompositing) {
        auto bounding_box = m_currentTransform.map_rect(m_element->paint_bounding_box());
        if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
            bounding_box.inflate(blendInfo.filterElement()->pixelMargin() * m_currentTransform.xScale());
        }
        bounding_box.intersect(m_canvas->extents());
        m_canvas = raster_canvas::create(bounding_box);
    } else {
        m_canvas->save();
    }

    if(!requiresCompositing && blendInfo.clipper()) {
        blendInfo.clipper()->applyClipPath(*this);
    }
}

NOVASVG_INLINE void svg_render_state::endGroup(const svg_blend_info& blendInfo)
{
    if(m_canvas == m_parent->raster()) {
        m_canvas->restore();
        return;
    }

    auto opacity = m_mode == svg_render_mode::Clipping ? 1.f : blendInfo.opacity();
    if(blendInfo.clipper())
        blendInfo.clipper()->applyClipMask(*this);
    if(m_mode == svg_render_mode::Painting && blendInfo.masker()) {
        blendInfo.masker()->applyMask(*this);
    }
    if(m_mode == svg_render_mode::Painting && blendInfo.filterElement()) {
        blendInfo.filterElement()->applyFilter(*this, m_currentTransform.xScale());
    }

    // the finished group is blended into what is already painted under it, with its mix-blend-mode
    BlendMode blendMode = BlendMode::Src_Over;
    if(m_mode == svg_render_mode::Painting) {
        switch(blendInfo.mixBlendMode()) {
        case MixBlendMode::Multiply: blendMode = BlendMode::Multiply; break;
        case MixBlendMode::Screen: blendMode = BlendMode::Screen; break;
        case MixBlendMode::Darken: blendMode = BlendMode::Darken; break;
        case MixBlendMode::Lighten: blendMode = BlendMode::Lighten; break;
        case MixBlendMode::Difference: blendMode = BlendMode::Difference; break;
        case MixBlendMode::Exclusion: blendMode = BlendMode::Exclusion; break;
        case MixBlendMode::Normal: break;
        }
    }

    m_parent->m_canvas->blend_canvas(*m_canvas, blendMode, opacity);
}

// ---- svgpaintelement (impl) ----

NOVASVG_INLINE svg_paint_element::svg_paint_element(document* dom, ElementID id)
    : svg_element(dom, id)
{
}

NOVASVG_INLINE svg_stop_element::svg_stop_element(document* dom)
    : svg_element(dom, ElementID::Stop)
    , m_offset(PropertyID::Offset, 0.f)
{
    addProperty(m_offset);
}

NOVASVG_INLINE void svg_stop_element::layoutElement(const svg_layout_state& state)
{
    m_stop_color = state.stop_color();
    m_stop_opacity = state.stop_opacity();
    svg_element::layoutElement(state);
}

NOVASVG_INLINE GradientStop svg_stop_element::gradientStop(float opacity) const
{
    Color stopColor = m_stop_color.colorWithAlpha(m_stop_opacity * opacity);
    GradientStop gradientStop = {
        m_offset.value(), { stopColor.getRF(), stopColor.getGF(), stopColor.getBF(), stopColor.getAF() }
    };

    return gradientStop;
}

NOVASVG_INLINE svg_gradient_element::svg_gradient_element(document* dom, ElementID id)
    : svg_paint_element(dom, id)
    , svg_uri_reference(this)
    , m_gradientTransform(PropertyID::GradientTransform)
    , m_gradientUnits(PropertyID::GradientUnits, Units::ObjectBoundingBox)
    , m_spreadMethod(PropertyID::SpreadMethod, SpreadMethod::Pad)
{
    addProperty(m_gradientTransform);
    addProperty(m_gradientUnits);
    addProperty(m_spreadMethod);
}

NOVASVG_INLINE void svg_gradient_element::collectGradientAttributes(svg_gradient_attributes& attributes) const
{
    if(!attributes.hasGradientTransform() && has_attribute(PropertyID::GradientTransform))
        attributes.setGradientTransform(this);
    if(!attributes.hasSpreadMethod() && has_attribute(PropertyID::SpreadMethod))
        attributes.setSpreadMethod(this);
    if(!attributes.hasGradientUnits() && has_attribute(PropertyID::GradientUnits))
        attributes.setGradientUnits(this);
    if(!attributes.hasGradientContentElement()) {
        for(const auto& child : children()) {
            if(auto el = toSVGElement(child); el && el->id() == ElementID::Stop) {
                attributes.setGradientContentElement(this);
                break;
            }
        }
    }
}

NOVASVG_INLINE svg_linear_gradient_element::svg_linear_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::LinearGradient)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow, 100.f, LengthUnits::Percent)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::Percent)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

NOVASVG_INLINE svg_linear_gradient_attributes svg_linear_gradient_element::collectGradientAttributes() const
{
    svg_linear_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::LinearGradient) {
            auto el = static_cast<const svg_linear_gradient_element*>(current);
            if(!attributes.hasX1() && el->has_attribute(PropertyID::X1))
                attributes.setX1(el);
            if(!attributes.hasY1() && el->has_attribute(PropertyID::Y1))
                attributes.setY1(el);
            if(!attributes.hasX2() && el->has_attribute(PropertyID::X2))
                attributes.setX2(el);
            if(!attributes.hasY2() && el->has_attribute(PropertyID::Y2)) {
                attributes.setY2(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

NOVASVG_INLINE GradientStops buildGradientStops(const svg_gradient_element* el, float opacity)
{
    GradientStops gradientStops;

    const auto& children = el->children();
    gradientStops.reserve(children.size());
    for(const auto& child : children) {
        auto childElement = toSVGElement(child);
        if(childElement && childElement->id() == ElementID::Stop) {
            auto stopElement = static_cast<svg_stop_element*>(childElement);
            gradientStops.push_back(stopElement->gradientStop(opacity));
        }
    }

    return gradientStops;
}

NOVASVG_INLINE bool svg_linear_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto x1 = lengthContext.valueForLength(attributes.x1());
    auto y1 = lengthContext.valueForLength(attributes.y1());
    auto x2 = lengthContext.valueForLength(attributes.x2());
    auto y2 = lengthContext.valueForLength(attributes.y2());
    if(gradientStops.size() == 1 || (x1 == x2 && y1 == y2)) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_linear_gradient(x1, y1, x2, y2, spreadMethod, gradientStops, gradientTransform);
    return true;
}

NOVASVG_INLINE svg_radial_gradient_element::svg_radial_gradient_element(document* dom)
    : svg_gradient_element(dom, ElementID::RadialGradient)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow, 50.f, LengthUnits::Percent)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid, 50.f, LengthUnits::Percent)
    , m_fx(PropertyID::Fx, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_fy(PropertyID::Fy, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
    addProperty(m_fx);
    addProperty(m_fy);
}

NOVASVG_INLINE bool svg_radial_gradient_element::applyPaint(svg_render_state& state, float opacity) const
{
    auto attributes = collectGradientAttributes();
    auto gradientContentElement = attributes.gradientContentElement();
    auto gradientStops = buildGradientStops(gradientContentElement, opacity);
    if(gradientStops.empty())
        return false;
    LengthContext lengthContext(this, attributes.gradientUnits());
    auto r = lengthContext.valueForLength(attributes.r());
    if(r == 0.f || gradientStops.size() == 1) {
        const auto& lastStop = gradientStops.back();
        state->set_color(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto fx = lengthContext.valueForLength(attributes.fx());
    auto fy = lengthContext.valueForLength(attributes.fy());
    auto cx = lengthContext.valueForLength(attributes.cx());
    auto cy = lengthContext.valueForLength(attributes.cy());

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.post_multiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->set_radial_gradient(cx, cy, r, fx, fy, spreadMethod, gradientStops, gradientTransform);
    return true;
}

NOVASVG_INLINE svg_radial_gradient_attributes svg_radial_gradient_element::collectGradientAttributes() const
{
    svg_radial_gradient_attributes attributes;
    std::set<const svg_gradient_element*> processedGradients;
    const svg_gradient_element* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::RadialGradient) {
            auto el = static_cast<const svg_radial_gradient_element*>(current);
            if(!attributes.hasCx() && el->has_attribute(PropertyID::Cx))
                attributes.setCx(el);
            if(!attributes.hasCy() && el->has_attribute(PropertyID::Cy))
                attributes.setCy(el);
            if(!attributes.hasR() && el->has_attribute(PropertyID::R))
                attributes.setR(el);
            if(!attributes.hasFx() && el->has_attribute(PropertyID::Fx))
                attributes.setFx(el);
            if(!attributes.hasFy() && el->has_attribute(PropertyID::Fy)) {
                attributes.setFy(el);
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const svg_gradient_element*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

NOVASVG_INLINE svg_pattern_element::svg_pattern_element(document* dom)
    : svg_paint_element(dom, ElementID::Pattern)
    , svg_uri_reference(this)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_patternTransform(PropertyID::PatternTransform)
    , m_patternUnits(PropertyID::PatternUnits, Units::ObjectBoundingBox)
    , m_patternContentUnits(PropertyID::PatternContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_patternTransform);
    addProperty(m_patternUnits);
    addProperty(m_patternContentUnits);
}

NOVASVG_INLINE bool svg_pattern_element::applyPaint(svg_render_state& state, float opacity) const
{
    if(state.hasCycleReference(this))
        return false;
    auto attributes = collectPatternAttributes();
    auto patternContentElement = attributes.patternContentElement();
    if(patternContentElement == nullptr)
        return false;
    LengthContext lengthContext(this, attributes.patternUnits());
    Rect patternRect = {
        lengthContext.valueForLength(attributes.x()),
        lengthContext.valueForLength(attributes.y()),
        lengthContext.valueForLength(attributes.width()),
        lengthContext.valueForLength(attributes.height())
    };

    if(attributes.patternUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternRect.x = patternRect.x * bbox.w + bbox.x;
        patternRect.y = patternRect.y * bbox.h + bbox.y;
        patternRect.w = patternRect.w * bbox.w;
        patternRect.h = patternRect.h * bbox.h;
    }

    auto currentTransform = attributes.patternTransform() * state.currentTransform();
    auto xScale = currentTransform.xScale();
    auto yScale = currentTransform.yScale();

    auto patternImage = raster_canvas::create(0, 0, patternRect.w * xScale, patternRect.h * yScale);
    auto patternImageTransform = Transform::scaled(xScale, yScale);

    const auto& viewBoxRect = attributes.viewBox();
    if(viewBoxRect.isValid()) {
        const auto& preserveAspectRatio = attributes.preserveAspectRatio();
        patternImageTransform.multiply(preserveAspectRatio.getTransform(viewBoxRect, patternRect.size()));
    } else if(attributes.patternContentUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternImageTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, patternImageTransform, svg_render_mode::Painting, patternImage);
    patternContentElement->renderChildren(newState);

    auto patternTransform = attributes.patternTransform();
    patternTransform.translate(patternRect.x, patternRect.y);
    patternTransform.scale(patternRect.w / patternImage->width(), patternRect.h / patternImage->height());
    state.raster()->set_texture(*patternImage, TextureType::Tiled, opacity, patternTransform);
    return true;
}

NOVASVG_INLINE svg_pattern_attributes svg_pattern_element::collectPatternAttributes() const
{
    svg_pattern_attributes attributes;
    std::set<const svg_pattern_element*> processedPatterns;
    const svg_pattern_element* current = this;
    while(true) {
        if(!attributes.hasX() && current->has_attribute(PropertyID::X))
            attributes.setX(current);
        if(!attributes.hasY() && current->has_attribute(PropertyID::Y))
            attributes.setY(current);
        if(!attributes.hasWidth() && current->has_attribute(PropertyID::Width))
            attributes.setWidth(current);
        if(!attributes.hasHeight() && current->has_attribute(PropertyID::Height))
            attributes.setHeight(current);
        if(!attributes.hasPatternTransform() && current->has_attribute(PropertyID::PatternTransform))
            attributes.setPatternTransform(current);
        if(!attributes.hasPatternUnits() && current->has_attribute(PropertyID::PatternUnits))
            attributes.setPatternUnits(current);
        if(!attributes.hasPatternContentUnits() && current->has_attribute(PropertyID::PatternContentUnits))
            attributes.setPatternContentUnits(current);
        if(!attributes.hasViewBox() && current->has_attribute(PropertyID::ViewBox))
            attributes.setViewBox(current);
        if(!attributes.hasPreserveAspectRatio() && current->has_attribute(PropertyID::PreserveAspectRatio))
            attributes.setPreserveAspectRatio(current);
        if(!attributes.hasPatternContentElement()) {
            for(const auto& child : current->children()) {
                if(child->is_element()) {
                    attributes.setPatternContentElement(current);
                    break;
                }
            }
        }

        auto targetElement = current->getTargetElement(dom());
        if(!targetElement || targetElement->id() != ElementID::Pattern)
            break;
        processedPatterns.insert(current);
        current = static_cast<const svg_pattern_element*>(targetElement);
        if(processedPatterns.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

// ---- svggeometryelement (impl) ----
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE Rect svg_marker_position::markerBoundingBox(float strokeWidth) const
{
    return m_element->markerBoundingBox(m_origin, m_angle, strokeWidth);
}

NOVASVG_INLINE void svg_marker_position::renderMarker(svg_render_state& state, float strokeWidth) const
{
    m_element->renderMarker(state, m_origin, m_angle, strokeWidth);
}

NOVASVG_INLINE svg_geometry_element::svg_geometry_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
{
}

NOVASVG_INLINE Rect svg_geometry_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = fillBoundingBox();
    if(m_stroke.isRenderable()) {
        float capLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineCap() == LineCap::Square)
            capLimit *= NOVASVG_SQRT2;
        float joinLimit = m_strokeData.lineWidth() / 2.f;
        if(m_strokeData.lineJoin() == LineJoin::Miter) {
            joinLimit *= m_strokeData.miterLimit();
        }

        strokeBoundingBox.inflate(STB_MAX(capLimit, joinLimit));
    }

    for(const auto& markerPosition : m_markerPositions)
        strokeBoundingBox.unite(markerPosition.markerBoundingBox(m_strokeData.lineWidth()));
    return strokeBoundingBox;
}

NOVASVG_INLINE void svg_geometry_element::layoutElement(const svg_layout_state& state)
{
    m_fill_rule = state.fill_rule();
    m_clip_rule = state.clip_rule();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    m_strokeData = getStrokeData(state);
    svg_graphics_element::layoutElement(state);

    m_path.reset();
    m_markerPositions.clear();
    m_fillBoundingBox = updateShape(m_path);
    updateMarkerPositions(m_markerPositions, state);
}

NOVASVG_INLINE void svg_geometry_element::updateMarkerPositions(svg_marker_position_list& positions, const svg_layout_state& state)
{
    if(m_path.empty())
        return;
    auto markerStart = getMarker(state.marker_start());
    auto markerMid = getMarker(state.marker_mid());
    auto markerEnd = getMarker(state.marker_end());
    if(markerStart == nullptr && markerMid == nullptr && markerEnd == nullptr) {
        return;
    }

    Point origin;
    Point startPoint;
    Point inslopePoints[2];
    Point outslopePoints[2];

    int index = 0;
    std::array<Point, 3> points;
    PathIterator it(m_path);
    while(!it.isDone()) {
        switch(it.currentSegment(points)) {
        case PathCommand::MoveTo:
            startPoint = points[0];
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::LineTo:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::CubicTo:
            inslopePoints[0] = points[1];
            inslopePoints[1] = points[2];
            origin = points[2];
            break;
        case PathCommand::Close:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = startPoint;
            startPoint = Point();
            break;
        }

        it.next();

        if(!it.isDone() && (markerStart || markerMid)) {
            it.currentSegment(points);
            outslopePoints[0] = origin;
            outslopePoints[1] = points[0];
            if(index == 0 && markerStart) {
                auto slope = outslopePoints[1] - outslopePoints[0];
                auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVASVG_PI;
                const auto& orient = markerStart->orient();
                if(orient.orientType() == svg_angle::OrientType::AutoStartReverse)
                    angle -= 180.f;
                positions.emplace_back(markerStart, origin, angle);
            }

            if(index > 0 && markerMid) {
                auto inslope = inslopePoints[1] - inslopePoints[0];
                auto outslope = outslopePoints[1] - outslopePoints[0];
                auto inangle = 180.f * std::atan2(inslope.y, inslope.x) / NOVASVG_PI;
                auto outangle = 180.f * std::atan2(outslope.y, outslope.x) / NOVASVG_PI;
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                positions.emplace_back(markerMid, origin, angle);
            }
        }

        if(markerEnd && it.isDone()) {
            auto slope = inslopePoints[1] - inslopePoints[0];
            auto angle = 180.f * std::atan2(slope.y, slope.x) / NOVASVG_PI;
            positions.emplace_back(markerEnd, origin, angle);
        }

        index += 1;
    }
}

NOVASVG_INLINE void svg_geometry_element::render(svg_render_state& state) const
{
    if(!isRenderable())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(Color::White);
        newState->fill_path(m_path, m_clip_rule, newState.currentTransform());
    } else {
        if(m_fill.applyPaint(newState))
            newState->fill_path(m_path, m_fill_rule, newState.currentTransform());
        if(m_stroke.applyPaint(newState)) {
            newState->stroke_path(m_path, m_strokeData, newState.currentTransform());
        }

        for(const auto& markerPosition : m_markerPositions) {
            markerPosition.renderMarker(newState, m_strokeData.lineWidth());
        }
    }

    newState.endGroup(blendInfo);
}

NOVASVG_INLINE svg_line_element::svg_line_element(document* dom)
    : svg_geometry_element(dom, ElementID::Line)
    , m_x1(PropertyID::X1, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y1(PropertyID::Y1, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_x2(PropertyID::X2, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y2(PropertyID::Y2, LengthDirection::Vertical, LengthNegativeMode::Allow)
{
    addProperty(m_x1);
    addProperty(m_y1);
    addProperty(m_x2);
    addProperty(m_y2);
}

NOVASVG_INLINE Rect svg_line_element::updateShape(Path& path)
{
    LengthContext lengthContext(this);
    auto x1 = lengthContext.valueForLength(m_x1);
    auto y1 = lengthContext.valueForLength(m_y1);
    auto x2 = lengthContext.valueForLength(m_x2);
    auto y2 = lengthContext.valueForLength(m_y2);

    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

NOVASVG_INLINE svg_rect_element::svg_rect_element(document* dom)
    : svg_geometry_element(dom, ElementID::Rect)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid)
    , m_rx(PropertyID::Rx, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Vertical, LengthNegativeMode::Forbid)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_rx);
    addProperty(m_ry);
}

NOVASVG_INLINE Rect svg_rect_element::updateShape(Path& path)
{
    LengthContext lengthContext(this);
    auto width = lengthContext.valueForLength(m_width);
    auto height = lengthContext.valueForLength(m_height);
    if(width <= 0.f || height <= 0.f) {
        return Rect::Empty;
    }

    auto x = lengthContext.valueForLength(m_x);
    auto y = lengthContext.valueForLength(m_y);

    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = STB_MIN(rx, width / 2.f);
    ry = STB_MIN(ry, height / 2.f);

    path.addRoundRect(x, y, width, height, rx, ry);
    return Rect(x, y, width, height);
}

NOVASVG_INLINE svg_ellipse_element::svg_ellipse_element(document* dom)
    : svg_geometry_element(dom, ElementID::Ellipse)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rx(PropertyID::Rx, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
    , m_ry(PropertyID::Ry, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_rx);
    addProperty(m_ry);
}

NOVASVG_INLINE Rect svg_ellipse_element::updateShape(Path& path)
{
    LengthContext lengthContext(this);
    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.addEllipse(cx, cy, rx, ry);
    return Rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

NOVASVG_INLINE svg_circle_element::svg_circle_element(document* dom)
    : svg_geometry_element(dom, ElementID::Circle)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
}

NOVASVG_INLINE Rect svg_circle_element::updateShape(Path& path)
{
    LengthContext lengthContext(this);
    auto r = lengthContext.valueForLength(m_r);
    if(r <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.addEllipse(cx, cy, r, r);
    return Rect(cx - r, cy - r, r + r, r + r);
}

NOVASVG_INLINE svg_poly_element::svg_poly_element(document* dom, ElementID id)
    : svg_geometry_element(dom, id)
    , m_points(PropertyID::Points)
{
    addProperty(m_points);
}

NOVASVG_INLINE Rect svg_poly_element::updateShape(Path& path)
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return Rect::Empty;
    }

    path.moveTo(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path.lineTo(points[i].x, points[i].y);
    }

    if(id() == ElementID::Polygon)
        path.close();
    return path.boundingRect();
}

NOVASVG_INLINE svg_path_element::svg_path_element(document* dom)
    : svg_geometry_element(dom, ElementID::Path)
    , m_d(PropertyID::D)
{
    addProperty(m_d);
}

NOVASVG_INLINE Rect svg_path_element::updateShape(Path& path)
{
    path = m_d.value();
    return path.boundingRect();
}

// ---- svgtextelement (impl) ----
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

inline const svg_text_node* toSVGTextNode(const svg_node* nd)
{
    assert(nd && nd->is_text_node());
    return static_cast<const svg_text_node*>(nd);
}

inline const svg_text_positioning_element* toSVGTextPositioningElement(const svg_node* nd)
{
    assert(nd && nd->isTextPositioningElement());
    return static_cast<const svg_text_positioning_element*>(nd);
}

NOVASVG_INLINE AlignmentBaseline resolveDominantBaseline(const svg_text_positioning_element* el)
{
    switch(el->dominant_baseline()) {
    case DominantBaseline::Auto:
        if(el->isVerticalWritingMode())
            return AlignmentBaseline::Central;
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::UseScript:
    case DominantBaseline::NoChange:
    case DominantBaseline::ResetSize:
        return AlignmentBaseline::Auto;
    case DominantBaseline::Ideographic:
        return AlignmentBaseline::Ideographic;
    case DominantBaseline::Alphabetic:
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::Hanging:
        return AlignmentBaseline::Hanging;
    case DominantBaseline::Mathematical:
        return AlignmentBaseline::Mathematical;
    case DominantBaseline::Central:
        return AlignmentBaseline::Central;
    case DominantBaseline::Middle:
        return AlignmentBaseline::Middle;
    case DominantBaseline::TextAfterEdge:
        return AlignmentBaseline::TextAfterEdge;
    case DominantBaseline::TextBeforeEdge:
        return AlignmentBaseline::TextBeforeEdge;
    default:
        assert(false);
    }

    return AlignmentBaseline::Auto;
}

NOVASVG_INLINE float calculateBaselineOffset(const svg_text_positioning_element* el)
{
    auto offset = el->baseline_offset();
    auto parent = el->parent_element();
    while(parent->isTextPositioningElement()) {
        offset += toSVGTextPositioningElement(parent)->baseline_offset();
        parent = parent->parent_element();
    }

    auto baseline = el->alignment_baseline();
    if(baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
        baseline = resolveDominantBaseline(el);
    }

    const auto& font = el->font();
    switch(baseline) {
    case AlignmentBaseline::BeforeEdge:
    case AlignmentBaseline::TextBeforeEdge:
        offset -= font.ascent();
        break;
    case AlignmentBaseline::Middle:
        offset -= font.xHeight() / 2.f;
        break;
    case AlignmentBaseline::Central:
        offset -= (font.ascent() + font.descent()) / 2.f;
        break;
    case AlignmentBaseline::AfterEdge:
    case AlignmentBaseline::TextAfterEdge:
    case AlignmentBaseline::Ideographic:
        offset -= font.descent();
        break;
    case AlignmentBaseline::Hanging:
        offset -= font.ascent() * 8.f / 10.f;
        break;
    case AlignmentBaseline::Mathematical:
        offset -= font.ascent() / 2.f;
        break;
    default:
        break;
    }

    return offset;
}

NOVASVG_INLINE bool needsTextAnchorAdjustment(const svg_text_positioning_element* el)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        return direction == Direction::Rtl;
    case TextAnchor::Middle:
        return true;
    case TextAnchor::End:
        return direction == Direction::Ltr;
    default:
        assert(false);
    }

    return false;
}

NOVASVG_INLINE float calculateTextAnchorOffset(const svg_text_positioning_element* el, float width)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        if(direction == Direction::Ltr)
            return 0.f;
        return -width;
    case TextAnchor::Middle:
        return -width / 2.f;
    case TextAnchor::End:
        if(direction == Direction::Ltr)
            return -width;
        return 0.f;
    default:
        assert(false);
    }

    return 0.f;
}

using svg_text_fragment_iterator = svg_text_fragment_list::iterator;

NOVASVG_INLINE float calculateTextChunkLength(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool isVerticalText)
{
    float chunkLength = 0;
    const svg_text_fragment* lastFragment = nullptr;
    for(auto it = begin; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        chunkLength += isVerticalText ? fragment.height : fragment.width;
        if(!lastFragment) {
            lastFragment = &fragment;
            continue;
        }

        if(isVerticalText) {
            chunkLength += fragment.y - (lastFragment->y + lastFragment->height);
        } else {
            chunkLength += fragment.x - (lastFragment->x + lastFragment->width);
        }

        lastFragment = &fragment;
    }

    return chunkLength;
}

NOVASVG_INLINE void handleTextChunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end)
{
    const svg_text_fragment& firstFragment = *begin;
    const auto isVerticalText = firstFragment.el->isVerticalWritingMode();
    if(firstFragment.el->has_attribute(PropertyID::TextLength)) {
        LengthContext lengthContext(firstFragment.el);
        auto textLength = lengthContext.valueForLength(firstFragment.el->textLength());
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        if(textLength > 0.f && chunkLength > 0.f) {
            size_t numCharacters = 0;
            for(auto it = begin; it != end; ++it) {
                const svg_text_fragment& fragment = *it;
                numCharacters += fragment.length;
            }

            if(firstFragment.el->lengthAdjust() == LengthAdjust::SpacingAndGlyphs) {
                auto textLengthScale = textLength / chunkLength;
                auto lengthAdjustTransform = Transform::translated(firstFragment.x, firstFragment.y);
                if(isVerticalText) {
                    lengthAdjustTransform.scale(1.f, textLengthScale);
                } else {
                    lengthAdjustTransform.scale(textLengthScale, 1.f);
                }

                lengthAdjustTransform.translate(-firstFragment.x, -firstFragment.y);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    fragment.lengthAdjustTransform = lengthAdjustTransform;
                }
            } else if(numCharacters > 1) {
                assert(firstFragment.el->lengthAdjust() == LengthAdjust::Spacing);
                size_t characterOffset = 0;
                auto textLengthShift = (textLength - chunkLength) / (numCharacters - 1);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    if(isVerticalText) {
                        fragment.y += textLengthShift * characterOffset;
                    } else {
                        fragment.x += textLengthShift * characterOffset;
                    }

                    characterOffset += fragment.length;
                }
            }
        }
    }

    if(needsTextAnchorAdjustment(firstFragment.el)) {
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        auto chunkOffset = calculateTextAnchorOffset(firstFragment.el, chunkLength);
        for(auto it = begin; it != end; ++it) {
            svg_text_fragment& fragment = *it;
            if(isVerticalText) {
                fragment.y += chunkOffset;
            } else {
                fragment.x += chunkOffset;
            }
        }
    }
}

NOVASVG_INLINE svg_text_fragments_builder::svg_text_fragments_builder(std::u32string& text, svg_text_fragment_list& fragments)
    : m_text(text), m_fragments(fragments)
{
    m_text.clear();
    m_fragments.clear();
}

NOVASVG_INLINE void svg_text_fragments_builder::build(const svg_text_element* textElement)
{
    handleElement(textElement);
    for(const auto& position : m_textPositions) {
        fillCharacterPositions(position);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& textPosition : m_textPositions) {
        if(!textPosition.nd->is_text_node())
            continue;
        auto el = toSVGTextPositioningElement(textPosition.nd->parent_element());
        const auto isVerticalText = el->isVerticalWritingMode();
        const auto isUprightText = el->isUprightTextOrientation();
        const auto& font = el->font();

        svg_text_fragment fragment(el);
        auto recordTextFragment = [&](auto startOffset, auto endOffset) {
            auto text = wholeText.substr(startOffset, endOffset - startOffset);
            fragment.offset = startOffset;
            fragment.length = text.length();
            fragment.width = font.measureText(text);
            fragment.height = font.height() + font.lineGap();
            if(isVerticalText) {
                m_y += isUprightText ? fragment.height : fragment.width;
            } else {
                m_x += fragment.width;
            }

            m_fragments.push_back(fragment);
        };

        auto needsTextLengthSpacing = el->lengthAdjust() == LengthAdjust::Spacing && el->has_attribute(PropertyID::TextLength);
        auto baselineOffset = calculateBaselineOffset(el);
        auto startOffset = textPosition.startOffset;
        auto textOffset = textPosition.startOffset;
        auto didStartTextFragment = false;
        auto applySpacingToNextCharacter = false;
        auto lastCharacter = 0u;
        auto lastAngle = 0.f;
        while(textOffset < textPosition.endOffset) {
            svg_character_position characterPosition;
            if(auto it = m_characterPositions.find(m_characterOffset); it != m_characterPositions.end()) {
                characterPosition = it->second;
            }

            auto currentCharacter = wholeText.at(textOffset);
            auto angle = characterPosition.rotate.value_or(0);
            auto dx = characterPosition.dx.value_or(0);
            auto dy = characterPosition.dy.value_or(0);

            auto shouldStartNewFragment = needsTextLengthSpacing || isVerticalText || applySpacingToNextCharacter
                || characterPosition.x || characterPosition.y || dx || dy || angle || angle != lastAngle;
            if(shouldStartNewFragment && didStartTextFragment) {
                recordTextFragment(startOffset, textOffset);
                applySpacingToNextCharacter = false;
                startOffset = textOffset;
            }

            auto startsNewTextChunk = (characterPosition.x || characterPosition.y) && textOffset == textPosition.startOffset;
            if(startsNewTextChunk || shouldStartNewFragment || !didStartTextFragment) {
                m_x = dx + characterPosition.x.value_or(m_x);
                m_y = dy + characterPosition.y.value_or(m_y);
                fragment.x = isVerticalText ? m_x + baselineOffset : m_x;
                fragment.y = isVerticalText ? m_y : m_y - baselineOffset;
                fragment.angle = angle;
                if(isVerticalText) {
                    if(isUprightText) {
                        fragment.y += font.height();
                    } else {
                        fragment.angle += 90.f;
                    }
                }

                fragment.startsNewTextChunk = startsNewTextChunk;
                didStartTextFragment = true;
            }

            auto spacing = el->letter_spacing();
            if(currentCharacter && lastCharacter && el->word_spacing()) {
                if(currentCharacter == ' ' && lastCharacter != ' ') {
                    spacing += el->word_spacing();
                }
            }

            if(spacing) {
                applySpacingToNextCharacter = true;
                if(isVerticalText) {
                    m_y += spacing;
                } else {
                    m_x += spacing;
                }
            }

            lastAngle = angle;
            lastCharacter = currentCharacter;
            ++textOffset;
            ++m_characterOffset;
        }

        recordTextFragment(startOffset, textOffset);
    }

    if(m_fragments.empty())
        return;
    auto it = m_fragments.begin();
    auto begin = m_fragments.begin();
    auto end = m_fragments.end();
    for(++it; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        if(!fragment.startsNewTextChunk)
            continue;
        handleTextChunk(begin, it);
        begin = it;
    }

    handleTextChunk(begin, it);
}

NOVASVG_INLINE void svg_text_fragments_builder::handleText(const svg_text_node* nd)
{
    const auto& text = nd->data();
    if(text.empty())
        return;
    auto el = toSVGTextPositioningElement(nd->parent_element());
    const auto startOffset = m_text.length();
    uint32_t lastCharacter = ' ';
    if(!m_text.empty()) {
        lastCharacter = m_text.back();
    }

    text_iterator_t it;
    text_iterator_init(&it, text.data(), text.length(), NOVASVG_TEXT_ENCODING_UTF8);
    while(text_iterator_has_next(&it)) {
        auto currentCharacter = text_iterator_next(&it);
        if(currentCharacter == '\t' || currentCharacter == '\n' || currentCharacter == '\r')
            currentCharacter = ' ';
        if(currentCharacter == ' ' && lastCharacter == ' ' && el->white_space() == WhiteSpace::Default)
            continue;
        m_text.push_back(currentCharacter);
        lastCharacter = currentCharacter;
    }

    if(startOffset < m_text.length()) {
        m_textPositions.emplace_back(nd, startOffset, m_text.length());
    }
}

NOVASVG_INLINE void svg_text_fragments_builder::handleElement(const svg_text_positioning_element* el)
{
    if(el->isDisplayNone())
        return;
    const auto itemIndex = m_textPositions.size();
    m_textPositions.emplace_back(el, m_text.length(), m_text.length());
    for(const auto& child : el->children()) {
        if(child->is_text_node()) {
            handleText(toSVGTextNode(child.get()));
        } else if(child->isTextPositioningElement()) {
            handleElement(toSVGTextPositioningElement(child.get()));
        }
    }

    auto& position = m_textPositions[itemIndex];
    assert(position.nd == el);
    position.endOffset = m_text.length();
}

NOVASVG_INLINE void svg_text_fragments_builder::fillCharacterPositions(const svg_text_position& position)
{
    if(!position.nd->isTextPositioningElement())
        return;
    auto el = toSVGTextPositioningElement(position.nd);
    const auto& xList = el->x();
    const auto& yList = el->y();
    const auto& dxList = el->dx();
    const auto& dyList = el->dy();
    const auto& rotateList = el->rotate();

    auto xListSize = xList.size();
    auto yListSize = yList.size();
    auto dxListSize = dxList.size();
    auto dyListSize = dyList.size();
    auto rotateListSize = rotateList.size();
    if(!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
        return;
    }

    LengthContext lengthContext(el);
    std::optional<float> lastRotation;
    for(auto offset = position.startOffset; offset < position.endOffset; ++offset) {
        auto index = offset - position.startOffset;
        if(index >= xListSize && index >= yListSize && index >= dxListSize && index >= dyListSize && index >= rotateListSize)
            break;
        auto& characterPosition = m_characterPositions[offset];
        if(index < xListSize)
            characterPosition.x = lengthContext.valueForLength(xList[index], LengthDirection::Horizontal);
        if(index < yListSize)
            characterPosition.y = lengthContext.valueForLength(yList[index], LengthDirection::Vertical);
        if(index < dxListSize)
            characterPosition.dx = lengthContext.valueForLength(dxList[index], LengthDirection::Horizontal);
        if(index < dyListSize)
            characterPosition.dy = lengthContext.valueForLength(dyList[index], LengthDirection::Vertical);
        if(index < rotateListSize) {
            characterPosition.rotate = rotateList[index];
            lastRotation = characterPosition.rotate;
        }
    }

    if(lastRotation == std::nullopt)
        return;
    auto offset = position.startOffset + rotateList.size();
    while(offset < position.endOffset) {
        m_characterPositions[offset++].rotate = lastRotation;
    }
}

NOVASVG_INLINE svg_text_positioning_element::svg_text_positioning_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_dx(PropertyID::Dx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_dy(PropertyID::Dy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rotate(PropertyID::Rotate)
    , m_textLength(PropertyID::TextLength, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_lengthAdjust(PropertyID::LengthAdjust, LengthAdjust::Spacing)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_dx);
    addProperty(m_dy);
    addProperty(m_rotate);
    addProperty(m_textLength);
    addProperty(m_lengthAdjust);
}

NOVASVG_INLINE void svg_text_positioning_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    svg_graphics_element::layoutElement(state);

    LengthContext lengthContext(this);
    m_stroke_width = lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal);
    m_letter_spacing = lengthContext.valueForLength(state.letter_spacing(), LengthDirection::Diagonal);
    m_word_spacing = lengthContext.valueForLength(state.word_spacing(), LengthDirection::Diagonal);

    m_baseline_offset = convertBaselineOffset(state.baseline_shift());
    m_alignment_baseline = state.alignment_baseline();
    m_dominant_baseline = state.dominant_baseline();
    m_text_anchor = state.text_anchor();
    m_white_space = state.white_space();
    m_writing_mode = state.writing_mode();
    m_text_orientation = state.text_orientation();
    m_direction = state.direction();
}

NOVASVG_INLINE float svg_text_positioning_element::convertBaselineOffset(const BaselineShift& baselineShift) const
{
    if(baselineShift.type() == BaselineShift::Type::Baseline)
        return 0.f;
    if(baselineShift.type() == BaselineShift::Type::Sub)
        return -m_font.height() / 2.f;
    if(baselineShift.type() == BaselineShift::Type::Super) {
        return m_font.height() / 2.f;
    }

    const auto& length = baselineShift.length();
    if(length.units() == LengthUnits::Percent)
        return length.value() * m_font.size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * m_font.size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_font.size();
    return length.value();
}

NOVASVG_INLINE svg_t_span_element::svg_t_span_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Tspan)
{
}

NOVASVG_INLINE svg_text_element::svg_text_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Text)
{
}

NOVASVG_INLINE void svg_text_element::layout(svg_layout_state& state)
{
    svg_text_positioning_element::layout(state);
    svg_text_fragments_builder(m_text, m_fragments).build(this);
}

NOVASVG_INLINE void svg_text_element::render(svg_render_state& state) const
{
    if(m_fragments.empty() || isVisibilityHidden() || isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(Color::White);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        if(fragment.el->isVisibilityHidden())
            continue;
        auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto text = wholeText.substr(fragment.offset, fragment.length);
        auto origin = Point(fragment.x, fragment.y);

        const auto& font = fragment.el->font();
        if(newState.mode() == svg_render_mode::Clipping) {
            newState->fill_text(text, font, origin, transform);
        } else {
            const auto& fill = fragment.el->fill();
            const auto& stroke = fragment.el->stroke();
            auto stroke_width = fragment.el->stroke_width();
            if(fill.applyPaint(newState))
                newState->fill_text(text, font, origin, transform);
            if(stroke.applyPaint(newState)) {
                newState->stroke_text(text, stroke_width, font, origin, transform);
            }
        }
    }

    newState.endGroup(blendInfo);
}

NOVASVG_INLINE Rect svg_text_element::bounding_box(bool includeStroke) const
{
    auto bounding_box = Rect::Invalid;
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.el->font();
        const auto& stroke = fragment.el->stroke();
        auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto fragmentRect = Rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.height);
        if(includeStroke && stroke.isRenderable())
            fragmentRect.inflate(fragment.el->stroke_width() / 2.f);
        bounding_box.unite(fragmentTranform.map_rect(fragmentRect));
    }

    if(!bounding_box.isValid())
        bounding_box = Rect::Empty;
    return bounding_box;
}

// ---- svglayoutstate (impl) ----
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE std::optional<Color> parseColorValue(std::string_view& input, const svg_layout_state* state)
{
    if(skipString(input, "currentColor")) {
        return state->color();
    }

    color_t color;
    int length = color_parse(&color, input.data(), input.length());
    if(length == 0)
        return std::nullopt;
    input.remove_prefix(length);
    // color_to_argb32() packs as 0xAARRGGBB (alpha high byte) -- a
    // different layout than Color::value()'s 0xRRGGBBAA, so this goes
    // through components, never through the raw packed value.
    auto argb = color_to_argb32(&color);
    return Color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
}

NOVASVG_INLINE Color parseColor(std::string_view input, const svg_layout_state* state, const Color& defaultValue)
{
    auto color = parseColorValue(input, state);
    if(!color || !input.empty())
        color = defaultValue;
    return color.value();
}

NOVASVG_INLINE Color parseColorOrNone(std::string_view input, const svg_layout_state* state, const Color& defaultValue)
{
    if(input.compare("none") == 0)
        return Color::Transparent;
    return parseColor(input, state, defaultValue);
}

NOVASVG_INLINE bool parseUrlValue(std::string_view& input, std::string& value)
{
    if(!skipString(input, "url")
        || !skipOptionalSpaces(input)
        || !skipDelimiter(input, '(')
        || !skipOptionalSpaces(input)) {
        return false;
    }

    switch(input.front()) {
    case '\'':
    case '\"': {
        auto delim = input.front();
        input.remove_prefix(1);
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, '#'))
            return false;
        while(!input.empty() && input.front() != delim) {
            value += input.front();
            input.remove_prefix(1);
        }

        skipOptionalSpaces(input);
        if(!skipDelimiter(input, delim))
            return false;
        break;
    } case '#': {
        input.remove_prefix(1);
        while(!input.empty() && input.front() != ')') {
            value += input.front();
            input.remove_prefix(1);
        }

        break;
    } default:
        return false;
    }

    return skipOptionalSpaces(input) && skipDelimiter(input, ')');
}

NOVASVG_INLINE std::string parseUrl(std::string_view input)
{
    std::string value;
    if(!parseUrlValue(input, value) || !input.empty())
        value.clear();
    return value;
}

NOVASVG_INLINE Paint parsePaint(std::string_view input, const svg_layout_state* state, const Color& defaultValue)
{
    std::string id;
    if(!parseUrlValue(input, id))
        return Paint(parseColorOrNone(input, state, defaultValue));
    if(skipOptionalSpaces(input))
        return Paint(id, parseColorOrNone(input, state, defaultValue));
    return Paint(id, Color::Transparent);
}

NOVASVG_INLINE float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue)
{
    float value;
    if(!parseNumber(input, value))
        return defaultValue;
    if(allowPercentage) {
        if(skipDelimiter(input, '%'))
            value /= 100.f;
        value = std::clamp(value, 0.f, 1.f);
    }

    if(!input.empty())
        return defaultValue;
    return value;
}

NOVASVG_INLINE Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue)
{
    Length value;
    if(!value.parse(input, mode))
        value = defaultValue;
    return value;
}

NOVASVG_INLINE BaselineShift parseBaselineShift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return BaselineShift::Type::Baseline;
    if(input.compare("sub") == 0)
        return BaselineShift::Type::Sub;
    if(input.compare("super") == 0)
        return BaselineShift::Type::Super;
    return parseLength(input, LengthNegativeMode::Allow, Length(0.f, LengthUnits::None));
}

NOVASVG_INLINE LengthList parseDashArray(std::string_view input)
{
    if(input.compare("none") == 0)
        return LengthList();
    LengthList values;
    do {
        size_t count = 0;
        while(count < input.length() && input[count] != ',' && !IS_WS(input[count]))
            ++count;
        Length value(0, LengthUnits::None);
        if(!value.parse(input.substr(0, count), LengthNegativeMode::Forbid))
            return LengthList();
        input.remove_prefix(count);
        values.push_back(std::move(value));
    } while(skipOptionalSpacesOrComma(input));
    return values;
}

NOVASVG_INLINE Length parseLengthOrNormal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return Length(0, LengthUnits::None);
    return parseLength(input, LengthNegativeMode::Allow, Length(0, LengthUnits::None));
}

NOVASVG_INLINE float parseFontSize(std::string_view input, const svg_layout_state* state)
{
    auto length = parseLength(input, LengthNegativeMode::Forbid, Length(12, LengthUnits::None));
    if(length.units() == LengthUnits::Percent)
        return length.value() * state->font_size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * state->font_size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * state->font_size();
    return length.value();
}

template<typename Enum, unsigned int N>
NOVASVG_INLINE Enum parseEnumValue(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N], Enum defaultValue)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return defaultValue;
}

NOVASVG_INLINE Display parseDisplay(std::string_view input)
{
    static const svg_enumeration_entry<Display> entries[] = {
        {Display::Inline, "inline"},
        {Display::None, "none"}
    };

    return parseEnumValue(input, entries, Display::Inline);
}

NOVASVG_INLINE Visibility parseVisibility(std::string_view input)
{
    static const svg_enumeration_entry<Visibility> entries[] = {
        {Visibility::Visible, "visible"},
        {Visibility::Hidden, "hidden"},
        {Visibility::Collapse, "collapse"}
    };

    return parseEnumValue(input, entries, Visibility::Visible);
}

NOVASVG_INLINE Overflow parseOverflow(std::string_view input)
{
    static const svg_enumeration_entry<Overflow> entries[] = {
        {Overflow::Visible, "visible"},
        {Overflow::Hidden, "hidden"}
    };

    return parseEnumValue(input, entries, Overflow::Visible);
}

NOVASVG_INLINE PointerEvents parsePointerEvents(std::string_view input)
{
    static const svg_enumeration_entry<PointerEvents> entries[] = {
        {PointerEvents::None,"none"},
        {PointerEvents::Auto,"auto"},
        {PointerEvents::Stroke,"stroke"},
        {PointerEvents::Fill,"fill"},
        {PointerEvents::Painted,"painted"},
        {PointerEvents::Visible,"visible"},
        {PointerEvents::VisibleStroke,"visibleStroke"},
        {PointerEvents::VisibleFill,"visibleFill"},
        {PointerEvents::VisiblePainted,"visiblePainted"},
        {PointerEvents::BoundingBox,"bounding-box"},
        {PointerEvents::All,"all"},
    };

    return parseEnumValue(input, entries, PointerEvents::Auto);
}

NOVASVG_INLINE FontWeight parseFontWeight(std::string_view input)
{
    static const svg_enumeration_entry<FontWeight> entries[] = {
        {FontWeight::Normal, "normal"},
        {FontWeight::Bold, "bold"},
        {FontWeight::Bold, "bolder"},
        {FontWeight::Normal, "lighter"},
        {FontWeight::Normal, "100"},
        {FontWeight::Normal, "200"},
        {FontWeight::Normal, "300"},
        {FontWeight::Normal, "400"},
        {FontWeight::Normal, "500"},
        {FontWeight::Bold, "600"},
        {FontWeight::Bold, "700"},
        {FontWeight::Bold, "800"},
        {FontWeight::Bold, "900"}
    };

    return parseEnumValue(input, entries, FontWeight::Normal);
}

NOVASVG_INLINE FontStyle parseFontStyle(std::string_view input)
{
    static const svg_enumeration_entry<FontStyle> entries[] = {
        {FontStyle::Normal, "normal"},
        {FontStyle::Italic, "italic"},
        {FontStyle::Italic, "oblique"}
    };

    return parseEnumValue(input, entries, FontStyle::Normal);
}

NOVASVG_INLINE AlignmentBaseline parseAlignmentBaseline(std::string_view input)
{
    static const svg_enumeration_entry<AlignmentBaseline> entries[] = {
        {AlignmentBaseline::Auto, "auto"},
        {AlignmentBaseline::Baseline, "baseline"},
        {AlignmentBaseline::BeforeEdge, "before-edge"},
        {AlignmentBaseline::TextBeforeEdge, "text-before-edge"},
        {AlignmentBaseline::Middle, "middle"},
        {AlignmentBaseline::Central, "central"},
        {AlignmentBaseline::AfterEdge, "after-edge"},
        {AlignmentBaseline::TextAfterEdge, "text-after-edge"},
        {AlignmentBaseline::Ideographic, "ideographic"},
        {AlignmentBaseline::Alphabetic, "alphabetic"},
        {AlignmentBaseline::Hanging, "hanging"},
        {AlignmentBaseline::Mathematical, "mathematical"}
    };

    return parseEnumValue(input, entries, AlignmentBaseline::Auto);
}

NOVASVG_INLINE DominantBaseline parseDominantBaseline(std::string_view input)
{
    static const svg_enumeration_entry<DominantBaseline> entries[] = {
        {DominantBaseline::Auto, "auto"},
        {DominantBaseline::UseScript, "use-script"},
        {DominantBaseline::NoChange, "no-change"},
        {DominantBaseline::ResetSize, "reset-size"},
        {DominantBaseline::Ideographic, "ideographic"},
        {DominantBaseline::Alphabetic, "alphabetic"},
        {DominantBaseline::Hanging, "hanging"},
        {DominantBaseline::Mathematical, "mathematical"},
        {DominantBaseline::Central, "central"},
        {DominantBaseline::Middle, "middle"},
        {DominantBaseline::TextAfterEdge, "text-after-edge"},
        {DominantBaseline::TextBeforeEdge, "text-before-edge"}
    };

    return parseEnumValue(input, entries, DominantBaseline::Auto);
}

NOVASVG_INLINE Direction parseDirection(std::string_view input)
{
    static const svg_enumeration_entry<Direction> entries[] = {
        {Direction::Ltr, "ltr"},
        {Direction::Rtl, "rtl"}
    };

    return parseEnumValue(input, entries, Direction::Ltr);
}

NOVASVG_INLINE WritingMode parseWritingMode(std::string_view input)
{
    static const svg_enumeration_entry<WritingMode> entries[] = {
        {WritingMode::Horizontal, "horizontal-tb"},
        {WritingMode::Vertical, "vertical-rl"},
        {WritingMode::Vertical, "vertical-lr"},
        {WritingMode::Horizontal, "lr-tb"},
        {WritingMode::Horizontal, "lr"},
        {WritingMode::Horizontal, "rl-tb"},
        {WritingMode::Horizontal, "rl"},
        {WritingMode::Vertical, "tb-rl"},
        {WritingMode::Vertical, "tb"}
    };

    return parseEnumValue(input, entries, WritingMode::Horizontal);
}

NOVASVG_INLINE TextOrientation parseTextOrientation(std::string_view input)
{
    static const svg_enumeration_entry<TextOrientation> entries[] = {
        {TextOrientation::Mixed, "mixed"},
        {TextOrientation::Upright, "upright"}
    };

    return parseEnumValue(input, entries, TextOrientation::Mixed);
}

NOVASVG_INLINE TextAnchor parseTextAnchor(std::string_view input)
{
    static const svg_enumeration_entry<TextAnchor> entries[] = {
        {TextAnchor::Start, "start"},
        {TextAnchor::Middle, "middle"},
        {TextAnchor::End, "end"}
    };

    return parseEnumValue(input, entries, TextAnchor::Start);
}

NOVASVG_INLINE WhiteSpace parseWhiteSpace(std::string_view input)
{
    static const svg_enumeration_entry<WhiteSpace> entries[] = {
        {WhiteSpace::Default, "default"},
        {WhiteSpace::Preserve, "preserve"},
        {WhiteSpace::Default, "normal"},
        {WhiteSpace::Default, "nowrap"},
        {WhiteSpace::Default, "pre-line"},
        {WhiteSpace::Preserve, "pre-wrap"},
        {WhiteSpace::Preserve, "pre"}
    };

    return parseEnumValue(input, entries, WhiteSpace::Default);
}

NOVASVG_INLINE MixBlendMode parseMixBlendMode(std::string_view input)
{
    static const svg_enumeration_entry<MixBlendMode> entries[] = {
        {MixBlendMode::Normal, "normal"},
        {MixBlendMode::Multiply, "multiply"},
        {MixBlendMode::Screen, "screen"},
        {MixBlendMode::Darken, "darken"},
        {MixBlendMode::Lighten, "lighten"},
        {MixBlendMode::Difference, "difference"},
        {MixBlendMode::Exclusion, "exclusion"}
    };

    // CSS keywords are ASCII case-insensitive
    std::string keyword;
    for(char ch : input)
        keyword.push_back(char(std::tolower(static_cast<unsigned char>(ch))));
    return parseEnumValue(keyword, entries, MixBlendMode::Normal);
}

NOVASVG_INLINE MaskType parseMaskType(std::string_view input)
{
    static const svg_enumeration_entry<MaskType> entries[] = {
        {MaskType::Luminance, "luminance"},
        {MaskType::Alpha, "alpha"}
    };

    return parseEnumValue(input, entries, MaskType::Luminance);
}

NOVASVG_INLINE FillRule parseFillRule(std::string_view input)
{
    static const svg_enumeration_entry<FillRule> entries[] = {
        {FillRule::NonZero, "nonzero"},
        {FillRule::EvenOdd, "evenodd"}
    };

    return parseEnumValue(input, entries, FillRule::NonZero);
}

NOVASVG_INLINE LineCap parseLineCap(std::string_view input)
{
    static const svg_enumeration_entry<LineCap> entries[] = {
        {LineCap::Butt, "butt"},
        {LineCap::Round, "round"},
        {LineCap::Square, "square"}
    };

    return parseEnumValue(input, entries, LineCap::Butt);
}

NOVASVG_INLINE LineJoin parseLineJoin(std::string_view input)
{
    static const svg_enumeration_entry<LineJoin> entries[] = {
        {LineJoin::Miter, "miter"},
        {LineJoin::Round, "round"},
        {LineJoin::Bevel, "bevel"}
    };

    return parseEnumValue(input, entries, LineJoin::Miter);
}

NOVASVG_INLINE svg_layout_state::svg_layout_state(const svg_layout_state& parent, const svg_element* el)
    : m_parent(&parent)
    , m_element(el)
    , m_fill(parent.fill())
    , m_stroke(parent.stroke())
    , m_color(parent.color())
    , m_fill_opacity(parent.fill_opacity())
    , m_stroke_opacity(parent.stroke_opacity())
    , m_stroke_miterlimit(parent.stroke_miterlimit())
    , m_font_size(parent.font_size())
    , m_letter_spacing(parent.letter_spacing())
    , m_word_spacing(parent.word_spacing())
    , m_stroke_width(parent.stroke_width())
    , m_stroke_dashoffset(parent.stroke_dashoffset())
    , m_stroke_dasharray(parent.stroke_dasharray())
    , m_stroke_linecap(parent.stroke_linecap())
    , m_stroke_linejoin(parent.stroke_linejoin())
    , m_fill_rule(parent.fill_rule())
    , m_clip_rule(parent.clip_rule())
    , m_font_weight(parent.font_weight())
    , m_font_style(parent.font_style())
    , m_dominant_baseline(parent.dominant_baseline())
    , m_text_anchor(parent.text_anchor())
    , m_white_space(parent.white_space())
    , m_writing_mode(parent.writing_mode())
    , m_text_orientation(parent.text_orientation())
    , m_direction(parent.direction())
    , m_visibility(parent.visibility())
    , m_overflow(el->isRootElement() ? Overflow::Visible : Overflow::Hidden)
    , m_pointer_events(parent.pointer_events())
    , m_marker_start(parent.marker_start())
    , m_marker_mid(parent.marker_mid())
    , m_marker_end(parent.marker_end())
    , m_font_family(parent.font_family())
{
    for(const auto& attribute : el->attributes()) {
        std::string_view input(attribute.value());
        stripLeadingAndTrailingSpaces(input);
        if(input.empty() || input.compare("inherit") == 0)
            continue;
        switch(attribute.id()) {
        case PropertyID::Fill:
            m_fill = parsePaint(input, this, Color::Black);
            break;
        case PropertyID::Stroke:
            m_stroke = parsePaint(input, this, Color::Transparent);
            break;
        case PropertyID::Color:
            m_color = parseColor(input, this, Color::Black);
            break;
        case PropertyID::Stop_Color:
            m_stop_color = parseColor(input, this, Color::Black);
            break;
        case PropertyID::Opacity:
            m_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Mix_Blend_Mode:
            m_mix_blend_mode = parseMixBlendMode(input);
            break;
        case PropertyID::Fill_Opacity:
            m_fill_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stroke_Opacity:
            m_stroke_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stop_Opacity:
            m_stop_opacity = parseNumberOrPercentage(input, true, 1.f);
            break;
        case PropertyID::Stroke_Miterlimit:
            m_stroke_miterlimit = parseNumberOrPercentage(input, false, 4.f);
            break;
        case PropertyID::Font_Size:
            m_font_size = parseFontSize(input, this);
            break;
        case PropertyID::Letter_Spacing:
            m_letter_spacing = parseLengthOrNormal(input);
            break;
        case PropertyID::Word_Spacing:
            m_word_spacing = parseLengthOrNormal(input);
            break;
        case PropertyID::Baseline_Shift:
            m_baseline_shift = parseBaselineShift(input);
            break;
        case PropertyID::Stroke_Width:
            m_stroke_width = parseLength(input, LengthNegativeMode::Forbid, Length(1.f, LengthUnits::None));
            break;
        case PropertyID::Stroke_Dashoffset:
            m_stroke_dashoffset = parseLength(input, LengthNegativeMode::Allow, Length(0.f, LengthUnits::None));
            break;
        case PropertyID::Stroke_Dasharray:
            m_stroke_dasharray = parseDashArray(input);
            break;
        case PropertyID::Stroke_Linecap:
            m_stroke_linecap = parseLineCap(input);
            break;
        case PropertyID::Stroke_Linejoin:
            m_stroke_linejoin = parseLineJoin(input);
            break;
        case PropertyID::Fill_Rule:
            m_fill_rule = parseFillRule(input);
            break;
        case PropertyID::Clip_Rule:
            m_clip_rule = parseFillRule(input);
            break;
        case PropertyID::Font_Weight:
            m_font_weight = parseFontWeight(input);
            break;
        case PropertyID::Font_Style:
            m_font_style = parseFontStyle(input);
            break;
        case PropertyID::Alignment_Baseline:
            m_alignment_baseline = parseAlignmentBaseline(input);
            break;
        case PropertyID::Dominant_Baseline:
            m_dominant_baseline = parseDominantBaseline(input);
            break;
        case PropertyID::Direction:
            m_direction = parseDirection(input);
            break;
        case PropertyID::Text_Anchor:
            m_text_anchor = parseTextAnchor(input);
            break;
        case PropertyID::White_Space:
            m_white_space = parseWhiteSpace(input);
            break;
        case PropertyID::Writing_Mode:
            m_writing_mode = parseWritingMode(input);
            break;
        case PropertyID::Text_Orientation:
            m_text_orientation = parseTextOrientation(input);
            break;
        case PropertyID::Display:
            m_display = parseDisplay(input);
            break;
        case PropertyID::Visibility:
            m_visibility = parseVisibility(input);
            break;
        case PropertyID::Overflow:
            m_overflow = parseOverflow(input);
            break;
        case PropertyID::Pointer_Events:
            m_pointer_events = parsePointerEvents(input);
            break;
        case PropertyID::Mask_Type:
            m_mask_type = parseMaskType(input);
            break;
        case PropertyID::Mask:
            m_mask = parseUrl(input);
            break;
        case PropertyID::Clip_Path:
            m_clip_path = parseUrl(input);
            break;
        case PropertyID::Filter:
            m_filter = parseUrl(input);
            break;
        case PropertyID::Marker_Start:
            m_marker_start = parseUrl(input);
            break;
        case PropertyID::Marker_Mid:
            m_marker_mid = parseUrl(input);
            break;
        case PropertyID::Marker_End:
            m_marker_end = parseUrl(input);
            break;
        case PropertyID::Font_Family:
            m_font_family.assign(input);
            break;
        default:
            break;
        }
    }
}

// The one place a (font-family stack, weight, style) triple becomes a Font, shared by svg <text>
// (svg_layout_state::font()) and by the bold/italic runs inside a <foreignObject>, so both always
// pick the very same face for the same request.
NOVASVG_INLINE Font makeFont(const std::string& family, float size, bool bold, bool italic)
{
    // Walk the font-family stack name by name (see
    // FontFaceCache::getFontFaceForFamilyStack()): an @font-face-embedded
    // font wins under its exact declared name, an uninstalled name falls
    // back to a metric-compatible substitute, unresolvable names are skipped.
    FontFace face = fontFaceCache()->getFontFaceForFamilyStack(family, bold, italic);

    // Absolute last resort: the hardcoded generic-family table.
    if(face.is_null())
        face = fontFaceCache()->getFontFace(emptyString, bold, italic);
    return Font(face, size);
}

NOVASVG_INLINE Font svg_layout_state::font() const
{
    return makeFont(m_font_family, m_font_size, m_font_weight == FontWeight::Bold, m_font_style == FontStyle::Italic);
}

// ---- svgelement (impl) ----
using namespace gfx; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE ElementID elementid(std::string_view name)
{
    static const struct {
        std::string_view name;
        ElementID value;
    } table[] = {
        {"a", ElementID::G},
        {"circle", ElementID::Circle},
        {"clipPath", ElementID::ClipPath},
        {"defs", ElementID::Defs},
        {"ellipse", ElementID::Ellipse},
        {"feComposite", ElementID::FeComposite},
        {"feDropShadow", ElementID::FeDropShadow},
        {"feFlood", ElementID::FeFlood},
        {"feGaussianBlur", ElementID::FeGaussianBlur},
        {"feMerge", ElementID::FeMerge},
        {"feMergeNode", ElementID::FeMergeNode},
        {"feOffset", ElementID::FeOffset},
        {"filter", ElementID::Filter},
        {"foreignObject", ElementID::ForeignObject},
        {"g", ElementID::G},
        {"image", ElementID::Image},
        {"line", ElementID::Line},
        {"linearGradient", ElementID::LinearGradient},
        {"marker", ElementID::Marker},
        {"mask", ElementID::Mask},
        {"path", ElementID::Path},
        {"pattern", ElementID::Pattern},
        {"polygon", ElementID::Polygon},
        {"polyline", ElementID::Polyline},
        {"radialGradient", ElementID::RadialGradient},
        {"rect", ElementID::Rect},
        {"stop", ElementID::Stop},
        {"style", ElementID::Style},
        {"svg", ElementID::Svg},
        {"switch", ElementID::Switch},
        {"symbol", ElementID::Symbol},
        {"text", ElementID::Text},
        {"tspan", ElementID::Tspan},
        {"use", ElementID::Use}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](const auto& item, const auto& name) { return item.name < name; });
    if(it == std::end(table) || it->name != name)
        return ElementID::Unknown;
    return it->value;
}

NOVASVG_INLINE svg_text_node::svg_text_node(document* dom)
    : svg_node(dom)
{
}

NOVASVG_INLINE void svg_text_node::set_data(const std::string& data)
{
    root_element()->setNeedsLayout();
    m_data.assign(data);
}

NOVASVG_INLINE std::unique_ptr<svg_node> svg_text_node::clone(bool deep) const
{
    auto nd = std::make_unique<svg_text_node>(dom());
    nd->set_data(m_data);
    return nd;
}

NOVASVG_INLINE const std::string emptyString;

NOVASVG_INLINE std::unique_ptr<svg_element> svg_element::create(document* dom, ElementID id)
{
    switch(id) {
    case ElementID::Svg:
        return std::make_unique<svg_svg_element>(dom);
    case ElementID::Path:
        return std::make_unique<svg_path_element>(dom);
    case ElementID::G:
        return std::make_unique<svg_g_element>(dom);
    case ElementID::Switch:
        return std::make_unique<svg_switch_element>(dom);
    case ElementID::Rect:
        return std::make_unique<svg_rect_element>(dom);
    case ElementID::Circle:
        return std::make_unique<svg_circle_element>(dom);
    case ElementID::Ellipse:
        return std::make_unique<svg_ellipse_element>(dom);
    case ElementID::Line:
        return std::make_unique<svg_line_element>(dom);
    case ElementID::Defs:
        return std::make_unique<svg_defs_element>(dom);
    case ElementID::Polygon:
    case ElementID::Polyline:
        return std::make_unique<svg_poly_element>(dom, id);
    case ElementID::Stop:
        return std::make_unique<svg_stop_element>(dom);
    case ElementID::LinearGradient:
        return std::make_unique<svg_linear_gradient_element>(dom);
    case ElementID::RadialGradient:
        return std::make_unique<svg_radial_gradient_element>(dom);
    case ElementID::Symbol:
        return std::make_unique<svg_symbol_element>(dom);
    case ElementID::Use:
        return std::make_unique<svg_use_element>(dom);
    case ElementID::Pattern:
        return std::make_unique<svg_pattern_element>(dom);
    case ElementID::Mask:
        return std::make_unique<svg_mask_element>(dom);
    case ElementID::ClipPath:
        return std::make_unique<svg_clip_path_element>(dom);
    case ElementID::Filter:
        return std::make_unique<svg_filter_element>(dom);
    case ElementID::FeGaussianBlur:
        return std::make_unique<svg_fe_gaussian_blur_element>(dom);
    case ElementID::FeDropShadow:
        return std::make_unique<svg_fe_drop_shadow_element>(dom);
    case ElementID::FeOffset:
        return std::make_unique<svg_fe_offset_element>(dom);
    case ElementID::FeFlood:
        return std::make_unique<svg_fe_flood_element>(dom);
    case ElementID::FeComposite:
        return std::make_unique<svg_fe_composite_element>(dom);
    case ElementID::FeMerge:
        return std::make_unique<svg_fe_merge_element>(dom);
    case ElementID::FeMergeNode:
        return std::make_unique<svg_element>(dom, id);
    case ElementID::Marker:
        return std::make_unique<svg_marker_element>(dom);
    case ElementID::Image:
        return std::make_unique<svg_image_element>(dom);
    case ElementID::ForeignObject:
        return std::make_unique<svg_foreign_object_element>(dom);
    case ElementID::Style:
        return std::make_unique<svg_style_element>(dom);
    case ElementID::Text:
        return std::make_unique<svg_text_element>(dom);
    case ElementID::Tspan:
        return std::make_unique<svg_t_span_element>(dom);
    default:
        assert(false);
    }

    return nullptr;
}

NOVASVG_INLINE svg_element::svg_element(document* dom, ElementID id)
    : svg_node(dom)
    , m_id(id)
{
}

NOVASVG_INLINE bool svg_element::has_attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return has_attribute(id);
}

NOVASVG_INLINE const std::string& svg_element::attribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return emptyString;
    return attribute(id);
}

NOVASVG_INLINE bool svg_element::set_attribute(std::string_view name, const std::string& value)
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return set_attribute(Specificity::Api, id, value);
}

NOVASVG_INLINE const Attribute* svg_element::findAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return &attribute;
        }
    }

    return nullptr;
}

NOVASVG_INLINE bool svg_element::has_attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return true;
        }
    }

    return false;
}

NOVASVG_INLINE const std::string& svg_element::attribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return attribute.value();
        }
    }

    return emptyString;
}

// Whether `value` has a `var(` function in it. Not for the attributes that are names or references, never CSS values.
inline bool usesVariables(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Id || id == PropertyID::Class || id == PropertyID::Href)
        return false;
    for(size_t pos = value.find("var("); pos != std::string::npos; pos = value.find("var(", pos + 1)) {
        if(pos == 0 || !IS_CSS_NAMECHAR(static_cast<unsigned char>(value[pos - 1])))
            return true;
    }
    return false;
}

NOVASVG_INLINE bool svg_element::set_attribute(int specificity, PropertyID id, const std::string& value)
{
    // A value with var() cannot be parsed before the custom properties it names are all known (a stylesheet
    // can come after the el): it is kept, and parsed by resolveVariables() when the dom is complete.
    const bool deferred = usesVariables(id, value);
    for(auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            if(specificity < attribute.specificity())
                return false;
            if(deferred)
                m_variableSources[id] = value;
            else {
                m_variableSources.erase(id);
                parseAttribute(id, value);
            }
            attribute = Attribute(specificity, id, value);
            return true;
        }
    }

    if(deferred)
        m_variableSources[id] = value;
    else
        parseAttribute(id, value);
    m_attributes.emplace_front(specificity, id, value);
    return true;
}

NOVASVG_INLINE void svg_element::setCustomProperty(int specificity, const std::string& name, const std::string& value)
{
    auto it = m_customProperties.find(name);
    if(it != m_customProperties.end() && specificity < it->second.first)
        return;
    std::string_view trimmed(value);
    stripLeadingAndTrailingSpaces(trimmed);
    m_customProperties[name] = {specificity, std::string(trimmed)};
}

NOVASVG_INLINE const svg_element* svg_element::findCustomPropertyOwner(const std::string& name) const
{
    for(auto el = this; el != nullptr; el = el->parent_element()) {
        if(el->m_customProperties.count(name))
            return el;
    }
    return nullptr;
}

NOVASVG_INLINE const std::string& svg_element::customProperty(const std::string& name) const
{
    if(auto owner = findCustomPropertyOwner(name))
        return owner->m_customProperties.at(name).second;
    return emptyString;
}

// Replaces each `var(--name, fallback)` in `text` with the value `--name` has on `el` -- or the fallback, if
// there is no such property or its own var() cannot be resolved (a cycle, say). False if one cannot be replaced
// at all: the value is then invalid and the caller leaves the attribute alone. `depth` stops a cycle.
inline bool substituteVariables(std::string_view text, const svg_element* el, std::string& out, int depth)
{
    constexpr int MaxDepth = 32;
    if(depth > MaxDepth)
        return false;
    out.clear();
    while(!text.empty()) {
        const auto pos = text.find("var(");
        if(pos == std::string_view::npos) {
            out.append(text);
            break;
        }
        if(pos > 0 && IS_CSS_NAMECHAR(static_cast<unsigned char>(text[pos - 1]))) { // `somevar(`: not ours
            out.append(text.substr(0, pos + 4));
            text.remove_prefix(pos + 4);
            continue;
        }
        out.append(text.substr(0, pos));
        text.remove_prefix(pos + 4);

        // The arguments run to the matching `)`; `name` ends at the first comma outside any nesting.
        size_t nesting = 1, end = 0, comma = std::string_view::npos;
        char quote = 0;
        for(; end < text.size(); ++end) {
            const char c = text[end];
            if(quote) {
                if(c == quote)
                    quote = 0;
            } else if(c == '"' || c == '\'') {
                quote = c;
            } else if(c == '(') {
                ++nesting;
            } else if(c == ')') {
                if(--nesting == 0)
                    break;
            } else if(c == ',' && nesting == 1 && comma == std::string_view::npos) {
                comma = end;
            }
        }
        if(end == text.size())
            return false;

        auto name = text.substr(0, comma == std::string_view::npos ? end : comma);
        stripLeadingAndTrailingSpaces(name);
        if(name.size() < 3 || name[0] != '-' || name[1] != '-')
            return false;

        std::string value;
        bool found = false;
        if(auto owner = el->findCustomPropertyOwner(std::string(name))) {
            // Resolved where it is declared, then inherited as that.
            found = substituteVariables(owner->customProperty(std::string(name)), owner, value, depth + 1);
        }
        if(!found) {
            if(comma == std::string_view::npos)
                return false;
            auto fallback = text.substr(comma + 1, end - comma - 1);
            stripLeadingAndTrailingSpaces(fallback);
            if(!substituteVariables(fallback, el, value, depth + 1))
                return false;
        }
        out.append(value);
        text.remove_prefix(end + 1);
    }
    return true;
}

NOVASVG_INLINE void svg_element::resolveVariables()
{
    for(const auto& [id, source] : m_variableSources) {
        std::string resolved;
        if(!substituteVariables(source, this, resolved, 0))
            continue;
        for(auto& attribute : m_attributes) {
            if(attribute.id() == id) {
                attribute = Attribute(attribute.specificity(), id, resolved);
                break;
            }
        }
        parseAttribute(id, resolved);
    }
}

NOVASVG_INLINE void svg_element::setAttributes(const AttributeList& attributes)
{
    for(const auto& attribute : attributes) {
        set_attribute(attribute);
    }
}

NOVASVG_INLINE bool svg_element::set_attribute(const Attribute& attribute)
{
    return set_attribute(attribute.specificity(), attribute.id(), attribute.value());
}

NOVASVG_INLINE void svg_element::parseAttribute(PropertyID id, const std::string& value)
{
    root_element()->setNeedsLayout();
    if(auto property = getProperty(id)) {
        property->parse(value);
    }
}

NOVASVG_INLINE svg_element* svg_element::previousElement() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.begin();
    auto end = children.end();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

NOVASVG_INLINE svg_element* svg_element::nextElement() const
{
    auto parent = parent_element();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.rbegin();
    auto end = children.rend();
    svg_element* el = nullptr;
    for(; it != end; ++it) {
        svg_node* nd = &**it;
        if(nd->is_text_node())
            continue;
        if(nd == this)
            return el;
        el = static_cast<svg_element*>(nd);
    }

    return nullptr;
}

NOVASVG_INLINE svg_node* svg_element::addChild(std::unique_ptr<svg_node> child)
{
    child->setParentElement(this);
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

NOVASVG_INLINE svg_node* svg_element::firstChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.front();
}

NOVASVG_INLINE svg_node* svg_element::lastChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.back();
}

NOVASVG_INLINE Rect svg_element::fillBoundingBox() const
{
    auto fillBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            fillBoundingBox.unite(el->local_transform().map_rect(el->fillBoundingBox()));
        }
    }

    if(!fillBoundingBox.isValid())
        fillBoundingBox = Rect::Empty;
    return fillBoundingBox;
}

NOVASVG_INLINE Rect svg_element::strokeBoundingBox() const
{
    auto strokeBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child); el && !el->isHiddenElement()) {
            strokeBoundingBox.unite(el->local_transform().map_rect(el->strokeBoundingBox()));
        }
    }

    if(!strokeBoundingBox.isValid())
        strokeBoundingBox = Rect::Empty;
    return strokeBoundingBox;
}

NOVASVG_INLINE Rect svg_element::paint_bounding_box() const
{
    if(m_paintBoundingBox.isValid())
        return m_paintBoundingBox;
    m_paintBoundingBox = Rect::Empty;
    m_paintBoundingBox = strokeBoundingBox();
    assert(m_paintBoundingBox.isValid());
    if(m_clipper) m_paintBoundingBox.intersect(m_clipper->clipBoundingBox(this));
    if(m_masker) m_paintBoundingBox.intersect(m_masker->maskBoundingBox(this));
    return m_paintBoundingBox;
}

NOVASVG_INLINE svg_marker_element* svg_element::getMarker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Marker)
        return static_cast<svg_marker_element*>(el);
    return nullptr;
}

NOVASVG_INLINE svg_clip_path_element* svg_element::getClipper(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::ClipPath)
        return static_cast<svg_clip_path_element*>(el);
    return nullptr;
}

NOVASVG_INLINE svg_mask_element* svg_element::getMasker(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Mask)
        return static_cast<svg_mask_element*>(el);
    return nullptr;
}

NOVASVG_INLINE svg_filter_element* svg_element::getFilterElement(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->id() == ElementID::Filter)
        return static_cast<svg_filter_element*>(el);
    return nullptr;
}

NOVASVG_INLINE svg_paint_element* svg_element::getPainter(std::string_view id) const
{
    auto el = root_element()->element_by_id(id);
    if(el && el->isPaintElement())
        return static_cast<svg_paint_element*>(el);
    return nullptr;
}

NOVASVG_INLINE svg_element* svg_element::element_from_point(float x, float y)
{
    auto it = m_children.rbegin();
    auto end = m_children.rend();
    for(; it != end; ++it) {
        auto child = toSVGElement(*it);
        if(child && !child->isHiddenElement()) {
            if(auto el = child->element_from_point(x, y)) {
                return el;
            }
        }
    }

    if(isPointableElement()) {
        auto transform = local_transform();
        for(auto parent = parent_element(); parent; parent = parent->parent_element())
            transform.post_multiply(parent->local_transform());
        auto bbox = transform.map_rect(paint_bounding_box());
        if(bbox.contains(x, y)) {
            return this;
        }
    }

    return nullptr;
}

NOVASVG_INLINE void svg_element::addProperty(svg_property& value)
{
    m_properties.push_front(&value);
}

NOVASVG_INLINE svg_property* svg_element::getProperty(PropertyID id) const
{
    for(auto property : m_properties) {
        if(id == property->id()) {
            return property;
        }
    }

    return nullptr;
}

NOVASVG_INLINE Size svg_element::currentViewportSize() const
{
    auto parent = parent_element();
    if(parent == nullptr) {
        auto el = static_cast<const svg_svg_element*>(this);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        return Size(300, 150);
    }

    if(parent->id() == ElementID::Svg) {
        auto el = static_cast<const svg_svg_element*>(parent);
        const auto& viewBox = el->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        LengthContext lengthContext(el);
        auto width = lengthContext.valueForLength(el->width());
        auto height = lengthContext.valueForLength(el->height());
        return Size(width, height);
    }

    return parent->currentViewportSize();
}

NOVASVG_INLINE void svg_element::cloneChildren(svg_element* parent_element) const
{
    for(const auto& child : m_children) {
        parent_element->addChild(child->clone(true));
    }
}

NOVASVG_INLINE std::unique_ptr<svg_node> svg_element::clone(bool deep) const
{
    auto el = svg_element::create(dom(), m_id);
    el->setAttributes(m_attributes);
    if(deep) { cloneChildren(el.get()); }
    return el;
}

NOVASVG_INLINE void svg_element::build()
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->build();
        }
    }
}

NOVASVG_INLINE void svg_element::layoutElement(const svg_layout_state& state)
{
    m_paintBoundingBox = Rect::Invalid;
    m_clipper = getClipper(state.clip_path());
    m_masker = getMasker(state.mask());
    m_filter = getFilterElement(state.filter());
    m_opacity = state.opacity();
    m_mix_blend_mode = state.mix_blend_mode();

    m_font_size = state.font_size();
    m_display = state.display();
    m_overflow = state.overflow();
    m_visibility = state.visibility();
    m_pointer_events = state.pointer_events();
}

NOVASVG_INLINE void svg_element::layoutChildren(svg_layout_state& state)
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->layout(state);
        }
    }
}

NOVASVG_INLINE void svg_element::layout(svg_layout_state& state)
{
    svg_layout_state newState(state, this);
    layoutElement(newState);
    layoutChildren(newState);
}

NOVASVG_INLINE void svg_element::renderChildren(svg_render_state& state) const
{
    for(const auto& child : m_children) {
        if(auto el = toSVGElement(child)) {
            el->render(state);
        }
    }
}

NOVASVG_INLINE void svg_element::render(svg_render_state& state) const
{
}

NOVASVG_INLINE bool svg_element::isHiddenElement() const
{
    if(isDisplayNone())
        return true;
    switch(m_id) {
    case ElementID::Defs:
    case ElementID::Symbol:
    case ElementID::Marker:
    case ElementID::ClipPath:
    case ElementID::Mask:
    case ElementID::Filter:
    case ElementID::FeGaussianBlur:
    case ElementID::FeDropShadow:
    case ElementID::FeOffset:
    case ElementID::FeFlood:
    case ElementID::FeComposite:
    case ElementID::FeMerge:
    case ElementID::FeMergeNode:
    case ElementID::LinearGradient:
    case ElementID::RadialGradient:
    case ElementID::Pattern:
    case ElementID::Stop:
        return true;
    default:
        return false;
    }
}

NOVASVG_INLINE bool svg_element::isPointableElement() const
{
    if(m_pointer_events != PointerEvents::None
        && m_visibility != Visibility::Hidden
        && m_display != Display::None
        && m_opacity != 0.f) {
        switch(m_id) {
        case ElementID::Line:
        case ElementID::Rect:
        case ElementID::Ellipse:
        case ElementID::Circle:
        case ElementID::Polyline:
        case ElementID::Polygon:
        case ElementID::Path:
        case ElementID::Text:
        case ElementID::Image:
        case ElementID::ForeignObject:
            return true;
        default:
            break;
        }
    }

    return false;
}

NOVASVG_INLINE svg_style_element::svg_style_element(document* dom)
    : svg_element(dom, ElementID::Style)
{
}

NOVASVG_INLINE svg_fit_to_view_box::svg_fit_to_view_box(svg_element* el)
    : m_viewBox(PropertyID::ViewBox)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    el->addProperty(m_viewBox);
    el->addProperty(m_preserveAspectRatio);
}

NOVASVG_INLINE Transform svg_fit_to_view_box::viewBoxToViewTransform(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Transform::Identity;
    return m_preserveAspectRatio.getTransform(viewBoxRect, viewportSize);
}

NOVASVG_INLINE Rect svg_fit_to_view_box::getClipRect(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.empty() || viewportSize.empty())
        return Rect(0, 0, viewportSize.w, viewportSize.h);
    return m_preserveAspectRatio.getClipRect(viewBoxRect, viewportSize);
}

NOVASVG_INLINE svg_uri_reference::svg_uri_reference(svg_element* el)
    : m_href(PropertyID::Href)
{
    el->addProperty(m_href);
}

NOVASVG_INLINE svg_element* svg_uri_reference::getTargetElement(const document* dom) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return dom->root_element()->element_by_id(value.substr(1));
}

NOVASVG_INLINE bool svg_paint_server::applyPaint(svg_render_state& state) const
{
    if(!isRenderable())
        return false;
    if(m_element) return m_element->applyPaint(state, m_opacity);
    state->set_color(m_color.colorWithAlpha(m_opacity));
    return true;
}

NOVASVG_INLINE svg_graphics_element::svg_graphics_element(document* dom, ElementID id)
    : svg_element(dom, id)
    , m_transform(PropertyID::Transform)
    , m_transformOrigin(PropertyID::Transform_Origin)
    , m_transformBox(PropertyID::Transform_Box)
{
    addProperty(m_transform);
    addProperty(m_transformOrigin);
    addProperty(m_transformBox);
}

NOVASVG_INLINE Transform svg_graphics_element::local_transform() const
{
    // Only shift the pivot when transform-origin was actually specified --
    // matches legacy behavior (origin at the current user-space (0,0)) for
    // every el that doesn't use it, i.e. everything except elements
    // that explicitly opt in the way feature-css-use-symbol.svg's `.rot`
    // class does.
    if(!m_transformOrigin.specified())
        return m_transform.value();

    Rect box;
    if(m_transformBox.specified() && m_transformBox.value() == TransformBoxType::FillBox) {
        box = fillBoundingBox();
    } else {
        auto size = currentViewportSize();
        box = Rect(0.f, 0.f, size.w, size.h);
    }

    if(box.empty())
        return m_transform.value();

    auto origin = m_transformOrigin.resolve(box);
    return Transform::translated(origin.x, origin.y) * m_transform.value() * Transform::translated(-origin.x, -origin.y);
}

NOVASVG_INLINE svg_paint_server svg_graphics_element::getPaintServer(const Paint& paint, float opacity) const
{
    if(paint.isNone())
        return svg_paint_server();
    if(auto el = getPainter(paint.id()))
        return svg_paint_server(el, paint.color(), opacity);
    return svg_paint_server(nullptr, paint.color(), opacity);
}

NOVASVG_INLINE StrokeData svg_graphics_element::getStrokeData(const svg_layout_state& state) const
{
    LengthContext lengthContext(this);
    StrokeData strokeData(lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal));
    strokeData.setMiterLimit(state.stroke_miterlimit());
    strokeData.setLineCap(state.stroke_linecap());
    strokeData.setLineJoin(state.stroke_linejoin());
    strokeData.setDashOffset(lengthContext.valueForLength(state.stroke_dashoffset(), LengthDirection::Diagonal));

    DashArray dashArray;
    for(const auto& dash : state.stroke_dasharray())
        dashArray.push_back(lengthContext.valueForLength(dash, LengthDirection::Diagonal));
    strokeData.setDashArray(std::move(dashArray));
    return strokeData;
}

NOVASVG_INLINE svg_svg_element::svg_svg_element(document* dom)
    : svg_graphics_element(dom, ElementID::Svg)
    , svg_fit_to_view_box(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

NOVASVG_INLINE Transform svg_svg_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(isRootElement())
        return viewBoxToViewTransform(viewportRect.size());
    return svg_graphics_element::local_transform() * Transform::translated(viewportRect.x, viewportRect.y) * viewBoxToViewTransform(viewportRect.size());
}

NOVASVG_INLINE void svg_svg_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    LengthContext lengthContext(this);
    const Size viewportSize = {
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(viewportSize.empty())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(viewportSize), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE svg_root_element::svg_root_element(document* dom)
    : svg_svg_element(dom)
{
}

NOVASVG_INLINE svg_root_element* svg_root_element::layout_if_needed()
{
    if(needsLayout())
        force_layout();
    return this;
}

NOVASVG_INLINE svg_element* svg_root_element::element_by_id(std::string_view id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

NOVASVG_INLINE void svg_root_element::addElementById(const std::string& id, svg_element* el)
{
    m_idCache.emplace(id, el);
}

NOVASVG_INLINE void svg_root_element::layout(svg_layout_state& state)
{
    svg_svg_element::layout(state);

    LengthContext lengthContext(this);
    if(!width().isPercent()) {
        m_intrinsicWidth = lengthContext.valueForLength(width());
    } else {
        m_intrinsicWidth = 0.f;
    }

    if(!height().isPercent()) {
        m_intrinsicHeight = lengthContext.valueForLength(height());
    } else {
        m_intrinsicHeight = 0.f;
    }

    const auto& viewBoxRect = viewBox().value();
    if(!viewBoxRect.empty() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        auto intrinsicRatio = viewBoxRect.w / viewBoxRect.h;
        if(!m_intrinsicWidth && m_intrinsicHeight)
            m_intrinsicWidth = m_intrinsicHeight * intrinsicRatio;
        else if(m_intrinsicWidth && !m_intrinsicHeight) {
            m_intrinsicHeight = m_intrinsicWidth / intrinsicRatio;
        }
    }

    if(viewBoxRect.isValid() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
        m_intrinsicWidth = viewBoxRect.w;
        m_intrinsicHeight = viewBoxRect.h;
    }

    if(!m_intrinsicWidth || !m_intrinsicHeight) {
        auto bounding_box = paint_bounding_box();
        if(!m_intrinsicWidth)
            m_intrinsicWidth = bounding_box.right();
        if(!m_intrinsicHeight) {
            m_intrinsicHeight = bounding_box.bottom();
        }
    }
}

NOVASVG_INLINE void svg_root_element::force_layout()
{
    svg_layout_state state;
    layout(state);
}

NOVASVG_INLINE svg_use_element::svg_use_element(document* dom)
    : svg_graphics_element(dom, ElementID::Use)
    , svg_uri_reference(this)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

NOVASVG_INLINE Transform svg_use_element::local_transform() const
{
    LengthContext lengthContext(this);
    const Point translation = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y)
    };

    return svg_graphics_element::local_transform() * Transform::translated(translation.x, translation.y);
}

NOVASVG_INLINE void svg_use_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE void svg_use_element::build()
{
    if(auto targetElement = getTargetElement(dom())) {
        if(auto newElement = cloneTargetElement(targetElement)) {
            addChild(std::move(newElement));
        }
    }

    svg_graphics_element::build();
}

inline bool isDisallowedElement(const svg_element* el)
{
    switch(el->id()) {
    case ElementID::Circle:
    case ElementID::Ellipse:
    case ElementID::ForeignObject:
    case ElementID::G:
    case ElementID::Image:
    case ElementID::Line:
    case ElementID::Path:
    case ElementID::Polygon:
    case ElementID::Polyline:
    case ElementID::Rect:
    case ElementID::Svg:
    case ElementID::Switch:
    case ElementID::Symbol:
    case ElementID::Text:
    case ElementID::Tspan:
    case ElementID::Use:
        return false;
    default:
        return true;
    }
}

NOVASVG_INLINE std::unique_ptr<svg_element> svg_use_element::cloneTargetElement(svg_element* targetElement)
{
    if(targetElement == this || isDisallowedElement(targetElement))
        return nullptr;
    const auto& idAttr = targetElement->attribute(PropertyID::Id);
    auto parent = parent_element();
    while(parent) {
        auto attribute = parent->findAttribute(PropertyID::Id);
        if(attribute && idAttr == attribute->value())
            return nullptr;
        parent = parent->parent_element();
    }

    auto tagId = targetElement->id();
    if(tagId == ElementID::Symbol) {
        tagId = ElementID::Svg;
    }

    auto newElement = svg_element::create(dom(), tagId);
    newElement->setAttributes(targetElement->attributes());
    if(newElement->id() == ElementID::Svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == PropertyID::Width || attribute.id() == PropertyID::Height) {
                newElement->set_attribute(attribute);
            }
        }
    }

    if(newElement->id() != ElementID::Use)
        targetElement->cloneChildren(newElement.get());
    return newElement;
}

NOVASVG_INLINE svg_image_element::svg_image_element(document* dom)
    : svg_graphics_element(dom, ElementID::Image)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_preserveAspectRatio);
}

NOVASVG_INLINE Rect svg_image_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    return viewportRect;
}

NOVASVG_INLINE Rect svg_image_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

NOVASVG_INLINE void svg_image_element::render(svg_render_state& state) const
{
    if(m_image.is_null() || isDisplayNone() || isVisibilityHidden())
        return;
    Rect dstRect(fillBoundingBox());
    Rect srcRect(0, 0, m_image.width(), m_image.height());
    if(dstRect.empty() || srcRect.empty())
        return;
    m_preserveAspectRatio.transformRect(dstRect, srcRect);

    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    newState->draw_image(m_image, dstRect, srcRect, newState.currentTransform());
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE Bitmap loadImageResource(const std::string& href)
{
    if(href.compare(0, 5, "data:") == 0) {
        std::string_view input(href);
        auto index = input.find(',', 5);
        if(index == std::string_view::npos)
            return Bitmap();
        input.remove_prefix(index + 1);
        return surface_load_from_image_base64(input.data(), input.length());
    }

    return surface_load_from_image_file(href.data());
}

NOVASVG_INLINE void svg_image_element::parseAttribute(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Href) {
        m_image = loadImageResource(value);
    } else {
        svg_graphics_element::parseAttribute(id, value);
    }
}

// ---------------------------------------------------------------
// svg_foreign_object_element / ForeignObjectSimple
// ---------------------------------------------------------------

NOVASVG_INLINE svg_foreign_object_element::svg_foreign_object_element(document* dom)
    : svg_graphics_element(dom, ElementID::ForeignObject)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

NOVASVG_INLINE Rect svg_foreign_object_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    return Rect(
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    );
}

NOVASVG_INLINE Rect svg_foreign_object_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

NOVASVG_INLINE ForeignObjectFonts svg_foreign_object_element::fontsFor(unsigned usedStyles) const
{
    ForeignObjectFonts fonts;
    for(unsigned style = 0; style < 4; ++style) {
        if(!(usedStyles & (1u << style)))
            continue;
        if(style == m_baseFontStyle) {
            fonts.face[style] = m_font;
            continue;
        }
        auto variant = makeFont(m_fontFamily, m_font.size(), (style & ForeignObjectFonts::Bold) != 0, (style & ForeignObjectFonts::Italic) != 0);
        // A variant the machine can't provide at all must not blank the text: use the el's own face.
        fonts.face[style] = variant.is_null() ? m_font : variant;
    }
    return fonts;
}

NOVASVG_INLINE void svg_foreign_object_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fontFamily = state.font_family();
    m_baseFontStyle = (unsigned char)((state.font_weight() == FontWeight::Bold ? ForeignObjectFonts::Bold : 0)
                                      | (state.font_style() == FontStyle::Italic ? ForeignObjectFonts::Italic : 0));
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    svg_graphics_element::layoutElement(state);
}

NOVASVG_INLINE void svg_foreign_object_element::render(svg_render_state& state) const
{
    if(m_rawContent.empty() || isDisplayNone() || isVisibilityHidden())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    auto renderer = m_renderer ? m_renderer : ForeignObjectSimple::instance();
    renderer->render(this, newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE ForeignObjectSimple* ForeignObjectSimple::instance()
{
    static ForeignObjectSimple s_instance;
    return &s_instance;
}

namespace {

inline std::u32string utf8ToU32(const std::string& text)
{
    std::u32string result;
    result.reserve(text.size());
    text_iterator_t it;
    text_iterator_init(&it, text.data(), (int)text.length(), NOVASVG_TEXT_ENCODING_UTF8);
    while(text_iterator_has_next(&it))
        result.push_back(text_iterator_next(&it));
    return result;
}

// Pulls a "<property>: <value>" declaration's raw value text out of a CSS
// declaration block (the inside of a style="..." attribute, or the
// inside of a rule's { ... }). Not a general CSS-value parser -- just
// finds and trims the text between the property name and the next
// `;`/`}`/quote. `property` match is boundary-checked (not
// preceded/followed by identifier characters) so searching for "color"
// doesn't false-match inside "background-color", or "height" inside
// "line-height". Used by parseNumericDeclaration() below and by the
// white-space lookup.
inline std::optional<std::string_view> findDeclarationValue(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto matchStart = pos;
        auto after = pos + property.size();
        pos = after;
        if(matchStart > 0) {
            auto prev = block[matchStart - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue; // part of a longer property name
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue; // property name continues past what we matched, e.g. "line-height" vs "height"

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;
        auto end = block.find_first_of(";}\"'", after);
        auto value = block.substr(after, (end == std::string_view::npos ? block.size() : end) - after);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// Same idea as findDeclarationValue(), but scans to the next *top-level*
// `;`/`}` instead of stopping at the first quote -- needed for
// `@font-face`'s `src:` value, which legitimately contains both quotes
// and commas (`url(...) format("truetype"), url(...) format("woff2")`).
// Parens are tracked so a `;`/`}` inside a url()/format() call (neither
// occurs in valid CSS, but malformed input shouldn't cut the scan short)
// doesn't end the value early either.
inline std::optional<std::string_view> findRawDeclarationValue(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto matchStart = pos;
        auto after = pos + property.size();
        pos = after;
        if(matchStart > 0) {
            auto prev = block[matchStart - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue;
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue;

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;

        auto valueStart = after;
        auto end = after;
        int parenDepth = 0;
        while(end < block.size()) {
            auto ch = block[end];
            if(ch == '(') {
                // `url(data:...)` holds the whole embedded font. Nothing inside it ends the value, so when
                // the run up to the next `)` has no `(` in it, jump over it in one go.
                if(parenDepth == 0) {
                    const char* base = block.data() + end + 1;
                    const size_t remaining = block.size() - end - 1;
                    if(auto* close = static_cast<const char*>(std::memchr(base, ')', remaining))) {
                        if(std::memchr(base, '(', size_t(close - base)) == nullptr) {
                            end = size_t(close - block.data()) + 1;
                            continue;
                        }
                    }
                }
                ++parenDepth;
            } else if(ch == ')') {
                if(parenDepth > 0)
                    --parenDepth;
            } else if((ch == ';' || ch == '}') && parenDepth == 0) {
                break;
            }
            ++end;
        }

        auto value = block.substr(valueStart, end - valueStart);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// A CSS colour value (`#f9f`, `rgb(..)`, `hsl(..)`, `rgba(..)`, a name) -> Color; nullopt if it isn't one.
inline std::optional<Color> parseCssColor(std::string_view value)
{
    color_t color;
    int length = color_parse(&color, value.data(), (int)value.length());
    if(length == 0)
        return std::nullopt;
    auto argb = color_to_argb32(&color);
    return Color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
}

// A parsed numeric CSS value: `raw` is the number itself, `isPixels`
// says whether it carried a "px" suffix (an absolute length) as opposed
// to a bare multiplier (e.g. line-height's "1.5"). Anything else
// (%, em, "normal", ...) isn't handled -- callers fall back to their own
// default for those, same as when the property is missing entirely.
struct NumericDeclaration {
    float raw;
    bool isPixels;
};

inline std::optional<NumericDeclaration> parseNumericDeclaration(std::string_view block, std::string_view property)
{
    auto value = findDeclarationValue(block, property);
    if(!value)
        return std::nullopt;

    char* end = nullptr;
    std::string valueStr(*value);
    auto number = std::strtof(valueStr.c_str(), &end);
    if(end == valueStr.c_str())
        return std::nullopt; // no leading number at all, e.g. "normal"

    auto unit = valueStr.c_str() + (end - valueStr.c_str());
    while(*unit == ' ')
        ++unit;
    return NumericDeclaration{number, std::string_view(unit) == "px"};
}

inline std::optional<std::string_view> htmlAttribute(std::string_view tag, std::string_view name)
{
    auto pos = tag.find(name);
    if(pos == std::string_view::npos)
        return std::nullopt;
    auto valueStart = pos + name.size();
    if(valueStart >= tag.size() || (tag[valueStart] != '"' && tag[valueStart] != '\''))
        return std::nullopt;
    auto quote = tag[valueStart];
    ++valueStart;
    auto valueEnd = tag.find(quote, valueStart);
    if(valueEnd == std::string_view::npos)
        return std::nullopt;
    return tag.substr(valueStart, valueEnd - valueStart);
}

// ---------------------------------------------------------------------------------------------------
// <foreignObject> text as runs of one style each.
//
// Roughly "element.innerText" for the tiny subset of HTML this cares about, but keeping what a reader
// can SEE of the markup: which stretches are bold and which are italic. Deliberately not a real HTML
// engine -- no box model, no per-tag display, margins or sizes -- because mermaid (and most
// svg-generating tools) only put one short run of text, perhaps with <br>, <b>/<strong> and <i>/<em>
// in it.
//
// Style bits (ForeignObjectFonts::Bold / Italic) come from, in this order:
//   * the style the foreignObject itself inherits from the svg around it (the `baseStyle` argument);
//   * the tag: <b> <strong> = bold, <i> <em> <cite> <var> <dfn> = italic;
//   * an inline style="font-weight: ..." / style="font-style: ...", which wins over the tag and may
//     also turn a style OFF again (`font-weight: normal` inside a bold stretch).
// Styles nest and are inherited by everything inside the el, and end with its closing tag.
// ---------------------------------------------------------------------------------------------------
struct ForeignObjectSegment {
    std::string text; ///< UTF-8; '\n' marks a forced line break (<br>, <p>, <div>, ...)
    unsigned char style = 0;
};

inline bool foreignObjectIsVoidTag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"area", "base", "br", "col", "embed", "hr", "img",
                                                    "input", "link", "meta", "source", "track", "wbr"};
    return tags.count(tag) > 0;
}

// Tags that sit INSIDE a line of text, so a boundary with them is not a boundary between words:
// "un<b>believ</b>able" is one word. Every tag NOT in this set, and not a block tag, still counts as
// a word boundary (a space), exactly as before styles existed -- see foreignObjectStyledText().
inline bool foreignObjectIsInlineTag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"a", "abbr", "b", "bdi", "bdo", "big", "cite", "code", "data", "del",
                                                    "dfn", "em", "font", "i", "ins", "kbd", "label", "mark", "q", "s",
                                                    "samp", "small", "span", "strike", "strong", "sub", "sup", "time",
                                                    "tt", "u", "var"};
    return tags.count(tag) > 0;
}

// The value of the style="..." attribute inside a tag's text (after its name), or empty.
inline std::string_view foreignObjectStyleAttribute(std::string_view attributes)
{
    size_t pos = 0;
    while(pos < attributes.size()) {
        while(pos < attributes.size() && (std::isspace(static_cast<unsigned char>(attributes[pos])) || attributes[pos] == '/'))
            ++pos;
        auto nameStart = pos;
        while(pos < attributes.size() && !std::isspace(static_cast<unsigned char>(attributes[pos])) && attributes[pos] != '=' && attributes[pos] != '/')
            ++pos;
        auto name = attributes.substr(nameStart, pos - nameStart);
        while(pos < attributes.size() && std::isspace(static_cast<unsigned char>(attributes[pos])))
            ++pos;
        std::string_view value;
        if(pos < attributes.size() && attributes[pos] == '=') {
            ++pos;
            while(pos < attributes.size() && std::isspace(static_cast<unsigned char>(attributes[pos])))
                ++pos;
            if(pos < attributes.size() && (attributes[pos] == '"' || attributes[pos] == '\'')) {
                auto quote = attributes[pos++];
                auto end = attributes.find(quote, pos);
                value = attributes.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
                pos = end == std::string_view::npos ? attributes.size() : end + 1;
            } else {
                auto valueStart = pos;
                while(pos < attributes.size() && !std::isspace(static_cast<unsigned char>(attributes[pos])))
                    ++pos;
                value = attributes.substr(valueStart, pos - valueStart);
            }
        }
        if(name.size() == 5 && (name[0] | 0x20) == 's' && (name[1] | 0x20) == 't' && (name[2] | 0x20) == 'y' && (name[3] | 0x20) == 'l' && (name[4] | 0x20) == 'e')
            return value;
    }
    return {};
}

// The lower-cased value of one declaration, without a trailing `!important`; empty if it is absent.
inline std::string foreignObjectDeclaration(std::string_view block, std::string_view property)
{
    auto value = findDeclarationValue(block, property);
    if(!value)
        return {};
    std::string result;
    for(char c : *value)
        result += char(std::tolower(static_cast<unsigned char>(c)));
    if(auto bang = result.find('!'); bang != std::string::npos)
        result.erase(bang);
    while(!result.empty() && std::isspace(static_cast<unsigned char>(result.back())))
        result.pop_back();
    return result;
}

// The style in effect inside an el that is being opened.
inline unsigned char foreignObjectStyleOfOpening(const std::string& tag, std::string_view attributes, unsigned char inherited)
{
    unsigned char style = inherited;
    if(tag == "b" || tag == "strong")
        style |= ForeignObjectFonts::Bold;
    else if(tag == "i" || tag == "em" || tag == "cite" || tag == "var" || tag == "dfn")
        style |= ForeignObjectFonts::Italic;

    auto inlineStyle = foreignObjectStyleAttribute(attributes);
    if(!inlineStyle.empty()) {
        if(auto weight = foreignObjectDeclaration(inlineStyle, "font-weight"); !weight.empty()) {
            style &= (unsigned char)~ForeignObjectFonts::Bold;
            if(parseFontWeight(weight) == FontWeight::Bold)
                style |= ForeignObjectFonts::Bold;
        }
        if(auto fontStyle = foreignObjectDeclaration(inlineStyle, "font-style"); !fontStyle.empty()) {
            style &= (unsigned char)~ForeignObjectFonts::Italic;
            if(parseFontStyle(fontStyle) == FontStyle::Italic)
                style |= ForeignObjectFonts::Italic;
        }
    }
    return style;
}

inline std::vector<ForeignObjectSegment> foreignObjectStyledText(std::string_view html, unsigned char baseStyle = 0)
{
    // Tag names that force a line break, matching their real-browser default of `display: block` (so a
    // `<br>`/`<p>`/`<div>` boundary becomes '\n', preserved through the whitespace-collapse pass below,
    // instead of collapsing into an ordinary space like other tags do). Mermaid's own labels are
    // `white-space: nowrap` by design (each is laid out to fit on one line at the font it expects) and
    // only ever span multiple lines where it inserted an explicit `<br>` itself at a deliberate break
    // point -- so honoring just these forced breaks reproduces real multi-line labels without needing
    // general width-based word-wrap.
    static const std::set<std::string_view> block_tags = {"br", "p", "div", "tr", "li"};

    std::vector<ForeignObjectSegment> raw;
    auto put = [&](char c, unsigned char style) {
        if(raw.empty() || raw.back().style != style)
            raw.push_back({std::string(), style});
        raw.back().text += c;
    };

    struct OpenElement {
        std::string tag;
        unsigned char style;
    };
    std::vector<OpenElement> open;
    unsigned char style = baseStyle;

    for(size_t i = 0; i < html.size();) {
        char c = html[i];

        if(c == '<') {
            if(html.compare(i, 4, "<!--") == 0) {
                auto end = html.find("-->", i + 4);
                i = end == std::string_view::npos ? html.size() : end + 3;
                put(' ', style);
                continue;
            }

            // the end of the tag: the first '>' that is not inside a quoted attribute value
            size_t gt = i + 1;
            char quote = 0;
            while(gt < html.size() && (quote || html[gt] != '>')) {
                if(quote) {
                    if(html[gt] == quote)
                        quote = 0;
                } else if(html[gt] == '"' || html[gt] == '\'') {
                    quote = html[gt];
                }
                ++gt;
            }
            auto inside = html.substr(i + 1, gt - i - 1);
            i = STB_MIN(gt + 1, html.size());

            const bool closing = !inside.empty() && inside.front() == '/';
            const bool selfClosing = !inside.empty() && inside.back() == '/';
            auto rest = closing ? inside.substr(1) : inside;
            size_t nameLength = 0;
            while(nameLength < rest.size() && std::isalnum(static_cast<unsigned char>(rest[nameLength])))
                ++nameLength;
            std::string name(rest.substr(0, nameLength));
            for(auto& ch : name)
                ch = char(std::tolower(static_cast<unsigned char>(ch)));

            // Every tag boundary is a word boundary (a space), so "Hello<br/>world" and "a</td><td>b"
            // never glue together; block tags force a line break instead; and the tags that sit inside
            // a line of text (<b>, <span>, ...) are no boundary at all.
            auto boundary = [&](unsigned char at) {
                if(block_tags.count(name))
                    put('\n', at);
                else if(!foreignObjectIsInlineTag(name))
                    put(' ', at);
            };

            if(closing) {
                for(size_t k = open.size(); k-- > 0;) {
                    if(open[k].tag == name) {
                        open.resize(k);
                        break;
                    }
                }
                style = open.empty() ? baseStyle : open.back().style;
                boundary(style);
            } else {
                boundary(style);
                if(!selfClosing && !foreignObjectIsVoidTag(name)) {
                    style = foreignObjectStyleOfOpening(name, rest.substr(nameLength), style);
                    open.push_back({name, style});
                }
            }
            continue;
        }

        if(c == '&') {
            if(html.compare(i, 5, "&amp;") == 0) { put('&', style); i += 5; continue; }
            if(html.compare(i, 4, "&lt;") == 0) { put('<', style); i += 4; continue; }
            if(html.compare(i, 4, "&gt;") == 0) { put('>', style); i += 4; continue; }
            if(html.compare(i, 6, "&quot;") == 0) { put('"', style); i += 6; continue; }
            if(html.compare(i, 6, "&apos;") == 0) { put('\'', style); i += 6; continue; }
            if(html.compare(i, 6, "&nbsp;") == 0) { put(' ', style); i += 6; continue; }
        }

        put(c, style);
        ++i;
    }

    // Collapse runs of ordinary whitespace into a single space (trimming leading/trailing) -- across
    // run boundaries, so "<b>a </b> b" has one space, not two -- but '\n' is preserved as its own
    // line-break marker rather than being folded into that, and spaces immediately touching a '\n' are
    // dropped rather than kept as a redundant space before/after the break.
    std::vector<ForeignObjectSegment> collapsed;
    auto append = [&](char c, unsigned char at) {
        if(collapsed.empty() || collapsed.back().style != at)
            collapsed.push_back({std::string(), at});
        collapsed.back().text += c;
    };
    auto dropLast = [&]() {
        collapsed.back().text.pop_back();
        if(collapsed.back().text.empty())
            collapsed.pop_back();
    };
    bool lastWasSpace = true; // trims leading whitespace too
    for(const auto& segment : raw) {
        for(char c : segment.text) {
            if(c == '\t' || c == '\r')
                c = ' ';
            if(c == '\n') {
                while(!collapsed.empty() && collapsed.back().text.back() == ' ')
                    dropLast();
                append('\n', segment.style);
                lastWasSpace = true;
                continue;
            }
            if(c == ' ') {
                if(lastWasSpace)
                    continue;
                lastWasSpace = true;
            } else {
                lastWasSpace = false;
            }
            append(c, segment.style);
        }
    }
    while(!collapsed.empty() && (collapsed.back().text.back() == ' ' || collapsed.back().text.back() == '\n'))
        dropLast();
    return collapsed;
}

// The same text with every style dropped: the plain "innerText" of the HTML ('\n' at forced breaks).
inline std::string foreignObjectPlainText(std::string_view html)
{
    std::string text;
    for(const auto& segment : foreignObjectStyledText(html))
        text += segment.text;
    return text;
}

// Real line-height for foreignObject text, read from the HTML's own
// inline style="line-height:..." (mermaid always sets this directly on
// the wrapping <div> -- never via a class the way it sometimes does for
// color/background-color -- so this only needs to check inline style=,
// not walk the stylesheet the way resolveForeignObjectStyle() does).
// Handles a bare multiplier ("1.5", the common case, including
// mermaid's own default) or an absolute "Npx" length; anything else
// (%, em, "normal", ...) or no line-height at all falls back to the
// caller's own default.
inline std::optional<float> foreignObjectLineHeight(std::string_view rawHtml, float fontSize)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            if(auto declaration = parseNumericDeclaration(*style, "line-height"))
                return declaration->isPixels ? declaration->raw : declaration->raw * fontSize;
        }
        pos = tagEnd + 1;
    }

    return std::nullopt;
}

// Whether the HTML's own CSS lets its text wrap at the box edge. Decided
// exactly like a browser would: the OUTERMOST el that declares
// `white-space` wins, and `nowrap` / `pre` mean "never soft-wrap" -- that is
// what mermaid's default labels say (`display:table-cell; white-space:nowrap`),
// so those keep laying out on one line (an overflowing line is handled by
// render()'s own horizontal condense, unchanged). Anything else --
// `normal`, `break-spaces` (mermaid's wrap mode: `display:table;
// white-space:break-spaces; width:200px`), `pre-wrap`, `pre-line` -- may
// wrap. No `white-space` declared at all is CSS's initial value, `normal`.
inline bool foreignObjectAllowsWrap(std::string_view rawHtml)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            if(auto value = findDeclarationValue(*style, "white-space"))
                return !(*value == "nowrap" || *value == "pre");
        }
        pos = tagEnd + 1;
    }
    return true;
}

// The px width the HTML itself constrains its text to, when it says so:
// the outermost el declaring `width` and/or `max-width` in px (both ->
// the smaller, as in CSS). Only px is understood; anything else (%, em, auto)
// means "no declared width" and the caller falls back to the box it has.
inline std::optional<float> foreignObjectDeclaredWidth(std::string_view rawHtml)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            std::optional<float> width;
            if(auto d = parseNumericDeclaration(*style, "width"); d && d->isPixels)
                width = d->raw;
            if(auto d = parseNumericDeclaration(*style, "max-width"); d && d->isPixels)
                width = width ? STB_MIN(*width, d->raw) : d->raw;
            if(width)
                return width;
        }
        pos = tagEnd + 1;
    }
    return std::nullopt;
}

// Greedy word-wrap of ONE paragraph to `maxWidth`: a line takes the next word
// while the whole candidate line ("a b c", measured as one string so kerning
// across the space counts, like a browser's shaping) still fits. A single
// word wider than maxWidth stays alone on an overflowing line -- CSS never
// breaks inside a word by default; render() condenses that one line.
inline void wrapForeignObjectParagraph(const std::u32string& paragraph, const Font& font, float maxWidth, std::vector<std::u32string>& lines)
{
    // Browsers lay out in 1/64px units, so "fits" tolerates that much slack.
    const float limit = maxWidth + 1.f / 128.f;
    std::u32string current;
    size_t i = 0;
    while(i < paragraph.size()) {
        while(i < paragraph.size() && (paragraph[i] == U' ' || paragraph[i] == U'\t'))
            ++i;
        auto wordEnd = i;
        while(wordEnd < paragraph.size() && paragraph[wordEnd] != U' ' && paragraph[wordEnd] != U'\t')
            ++wordEnd;
        if(wordEnd == i)
            break;
        auto word = paragraph.substr(i, wordEnd - i);
        i = wordEnd;

        if(current.empty()) {
            current = std::move(word);
            continue;
        }
        auto candidate = current;
        candidate += U' ';
        candidate += word;
        if(font.measureText(std::u32string_view(candidate)) <= limit) {
            current = std::move(candidate);
        } else {
            lines.push_back(std::move(current));
            current = std::move(word);
        }
    }
    if(!current.empty())
        lines.push_back(std::move(current));
}

// Splits `text` (already using '\n' for the forced breaks
// foreignObjectPlainText() inserts at <br>/<p>/<div> boundaries) into lines.
// With maxWidth <= 0 (the caller's HTML is `white-space: nowrap`/`pre`, or
// there is no width to wrap at) it only splits on those forced breaks, as
// mermaid's nowrap labels want. With a positive maxWidth each paragraph is
// additionally greedy word-wrapped to it, using the same Font::measureText()
// the paint path uses.
inline std::vector<std::u32string> wrapForeignObjectText(const std::string& text, const Font* font = nullptr, float maxWidth = 0.f)
{
    std::vector<std::u32string> lines;
    const bool softWrap = font != nullptr && !font->is_null() && maxWidth > 0.f;
    size_t pos = 0;
    while(pos <= text.size()) {
        auto nl = text.find('\n', pos);
        auto paragraph = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if(paragraph.empty())
            continue;
        if(softWrap)
            wrapForeignObjectParagraph(utf8ToU32(paragraph), *font, maxWidth, lines);
        else
            lines.push_back(utf8ToU32(paragraph));
    }

    if(lines.empty())
        lines.emplace_back();
    return lines;
}

// One stretch of a line in a single style; a line is a few of them in a row.
struct ForeignObjectRun {
    std::u32string text;
    unsigned char style = 0;
};
using ForeignObjectLine = std::vector<ForeignObjectRun>;

// The text of a segment list split at its forced breaks ('\n'), each paragraph as segments again.
inline std::vector<std::vector<ForeignObjectSegment>> splitForeignObjectParagraphs(const std::vector<ForeignObjectSegment>& segments)
{
    std::vector<std::vector<ForeignObjectSegment>> paragraphs(1);
    for(const auto& segment : segments) {
        size_t pos = 0;
        while(pos <= segment.text.size()) {
            auto newline = segment.text.find('\n', pos);
            auto piece = segment.text.substr(pos, newline == std::string::npos ? std::string::npos : newline - pos);
            if(!piece.empty())
                paragraphs.back().push_back({std::move(piece), segment.style});
            if(newline == std::string::npos)
                break;
            paragraphs.emplace_back();
            pos = newline + 1;
        }
    }
    return paragraphs;
}

// Width of text whose characters each have a style: every stretch of one style is measured on its own
// with that style's face (kerning is not applied ACROSS a style change, which no face could do anyway).
inline float measureForeignObjectCells(const std::u32string& chars, const std::vector<unsigned char>& styles, const ForeignObjectFonts& fonts)
{
    float width = 0.f;
    size_t begin = 0;
    while(begin < chars.size()) {
        size_t end = begin + 1;
        while(end < chars.size() && styles[end] == styles[begin])
            ++end;
        width += fonts.get(styles[begin]).measureText(std::u32string_view(chars).substr(begin, end - begin));
        begin = end;
    }
    return width;
}

inline ForeignObjectLine makeForeignObjectLine(const std::u32string& chars, const std::vector<unsigned char>& styles)
{
    ForeignObjectLine line;
    for(size_t i = 0; i < chars.size(); ++i) {
        if(line.empty() || line.back().style != styles[i])
            line.push_back({std::u32string(), styles[i]});
        line.back().text += chars[i];
    }
    return line;
}

// Lays ONE paragraph out into lines (appended to `lines`). With maxWidth <= 0 it never soft-wraps.
inline void wrapForeignObjectStyledParagraph(const std::vector<ForeignObjectSegment>& paragraph, const ForeignObjectFonts& fonts, float maxWidth, std::vector<ForeignObjectLine>& lines)
{
    bool uniform = true;
    for(const auto& segment : paragraph)
        uniform = uniform && segment.style == paragraph.front().style;

    if(uniform) {
        // One style all the way: exactly the layout text had before it could carry styles (a candidate
        // line is measured as ONE string, so kerning across the space counts).
        const auto style = paragraph.front().style;
        std::string text;
        for(const auto& segment : paragraph)
            text += segment.text;
        auto chars = utf8ToU32(text);
        const auto& font = fonts.get(style);
        if(maxWidth > 0.f && !font.is_null()) {
            std::vector<std::u32string> wrapped;
            wrapForeignObjectParagraph(chars, font, maxWidth, wrapped);
            for(auto& line : wrapped)
                lines.push_back(ForeignObjectLine{ForeignObjectRun{std::move(line), style}});
        } else {
            lines.push_back(ForeignObjectLine{ForeignObjectRun{std::move(chars), style}});
        }
        return;
    }

    std::u32string chars;
    std::vector<unsigned char> styles;
    for(const auto& segment : paragraph) {
        auto piece = utf8ToU32(segment.text);
        chars += piece;
        styles.insert(styles.end(), piece.size(), segment.style);
    }
    if(!(maxWidth > 0.f)) {
        lines.push_back(makeForeignObjectLine(chars, styles));
        return;
    }

    // Same greedy word-wrap as wrapForeignObjectParagraph(), a word at a time, but a candidate line is
    // measured style stretch by style stretch. A space takes the style it has in the source.
    const float limit = maxWidth + 1.f / 128.f;
    std::u32string current;
    std::vector<unsigned char> currentStyles;
    size_t i = 0;
    while(i < chars.size()) {
        unsigned char spaceStyle = 0;
        while(i < chars.size() && (chars[i] == U' ' || chars[i] == U'\t')) {
            spaceStyle = styles[i];
            ++i;
        }
        auto wordEnd = i;
        while(wordEnd < chars.size() && chars[wordEnd] != U' ' && chars[wordEnd] != U'\t')
            ++wordEnd;
        if(wordEnd == i)
            break;
        auto word = chars.substr(i, wordEnd - i);
        std::vector<unsigned char> wordStyles(styles.begin() + long(i), styles.begin() + long(wordEnd));
        i = wordEnd;

        if(current.empty()) {
            current = std::move(word);
            currentStyles = std::move(wordStyles);
            continue;
        }
        auto candidate = current;
        auto candidateStyles = currentStyles;
        candidate += U' ';
        candidateStyles.push_back(spaceStyle);
        candidate += word;
        candidateStyles.insert(candidateStyles.end(), wordStyles.begin(), wordStyles.end());
        if(measureForeignObjectCells(candidate, candidateStyles, fonts) <= limit) {
            current = std::move(candidate);
            currentStyles = std::move(candidateStyles);
        } else {
            lines.push_back(makeForeignObjectLine(current, currentStyles));
            current = std::move(word);
            currentStyles = std::move(wordStyles);
        }
    }
    if(!current.empty())
        lines.push_back(makeForeignObjectLine(current, currentStyles));
}

// Lines of a styled text, honouring its forced breaks; wrapWidth <= 0 = no soft wrapping. Never empty.
inline std::vector<ForeignObjectLine> layoutForeignObjectText(const std::vector<ForeignObjectSegment>& segments, const ForeignObjectFonts& fonts, float wrapWidth)
{
    std::vector<ForeignObjectLine> lines;
    for(const auto& paragraph : splitForeignObjectParagraphs(segments)) {
        if(!paragraph.empty())
            wrapForeignObjectStyledParagraph(paragraph, fonts, wrapWidth, lines);
    }
    if(lines.empty())
        lines.emplace_back();
    return lines;
}

inline float measureForeignObjectLine(const ForeignObjectLine& line, const ForeignObjectFonts& fonts)
{
    float width = 0.f;
    for(const auto& run : line)
        width += fonts.get(run.style).measureText(std::u32string_view(run.text));
    return width;
}

// Bit n set = style n appears somewhere in the text.
inline unsigned foreignObjectStylesUsed(const std::vector<ForeignObjectSegment>& segments)
{
    unsigned used = 0;
    for(const auto& segment : segments)
        used |= 1u << (segment.style & 3);
    return used;
}

} // namespace

/**
 * @brief Metrics for how ForeignObjectSimple::render() will lay out and
 * size a <foreignObject>'s content, computed WITHOUT actually rendering it.
 *
 * Exists so a caller building the svg in the first place (i.e. before any
 * svg_foreign_object_element exists to query) -- e.g. mermaid.js's own layout
 * pass, run headless inside mermaidx's QuickJS/V8 DOM shim, computing a
 * nd box's height to fit a multi-line HTML label -- can ask novasvg
 * itself what height that content will need, rather than reimplementing
 * (and risking drifting out of sync with) the line-counting/line-height
 * logic ForeignObjectSimple::render() uses at actual paint time. See
 * measureForeignObjectContent() below.
 */
struct ForeignObjectMetrics {
    float height = 0.f;   ///< Total height ForeignObjectSimple::render() will use for this content, in pixels.
    float width = 0.f;    ///< Widest line, per Font::measureText() -- see this struct's own field-level note above measureForeignObjectContent() for why this is NOT what render() itself uses to size anything.
    int lineCount = 0;    ///< Number of lines wrapForeignObjectText() splits this content into.
    float lineHeight = 0.f; ///< Per-line height (from the content's own inline `line-height:`, or `font.height() * 1.2`).
};

/**
 * @brief Measures what ForeignObjectMetrics::height (etc.) will be for
 * `rawHtml` painted with `font`, using the exact same
 * foreignObjectPlainText() / wrapForeignObjectText() / foreignObjectLineHeight()
 * logic ForeignObjectSimple::render() itself calls -- so a value computed
 * here and a box later painted by novasvg with the same (rawHtml, font)
 * can never disagree, short of a bug in this shared logic itself.
 *
 * `maxWidth` (px, optional): the width text may wrap at. When omitted it is
 * read from the HTML's own `width`/`max-width` (px). Wrapping happens only if
 * the HTML's `white-space` allows it (see foreignObjectAllowsWrap()): mermaid's
 * `nowrap` labels stay one line, its wrap mode (`white-space:break-spaces;
 * width:200px`) wraps -- exactly what render() does at the box's own width.
 *
 * ForeignObjectMetrics::width is the widest resulting line. render() never
 * grows a line's box to fit its content horizontally; it only ever
 * horizontally condenses a line that overflows the box it's given. It's
 * computed and returned anyway (via the same per-line Font::measureText()
 * render() itself calls) because a caller BUILDING that box in the first
 * place -- this function's whole reason to exist -- needs some width to
 * size it to, and the widest line at this exact font is the only figure
 * that guarantees render() never needs to condense anything afterward.
 */
inline ForeignObjectMetrics measureForeignObjectSegments(std::string_view rawHtml, const std::vector<ForeignObjectSegment>& segments, const ForeignObjectFonts& fonts, unsigned char baseStyle, float maxWidth)
{
    ForeignObjectMetrics metrics;
    const auto& font = fonts.get(baseStyle);
    if(font.is_null())
        return metrics;
    // Same rule render() applies, so layout and paint agree: soft-wrap only if
    // the HTML's own `white-space` allows it, at the explicit maxWidth if one
    // was given, else at the width the HTML itself declares (px).
    float wrapWidth = 0.f;
    if(foreignObjectAllowsWrap(rawHtml)) {
        if(maxWidth > 0.f)
            wrapWidth = maxWidth;
        else if(auto declared = foreignObjectDeclaredWidth(rawHtml))
            wrapWidth = *declared;
    }
    auto lines = layoutForeignObjectText(segments, fonts, wrapWidth);
    metrics.lineCount = int(lines.size());
    metrics.lineHeight = foreignObjectLineHeight(rawHtml, font.size()).value_or(font.height() * 1.2f);
    metrics.height = metrics.lineHeight * float(metrics.lineCount);
    for(const auto& line : lines)
        // STB_MAX(), not std::max(): see novasvg.h's own
        // comment on why this codebase avoids std::max/std::min entirely
        // rather than special-casing each call site against the
        // Windows.h max/min macro collision.
        metrics.width = STB_MAX(metrics.width, measureForeignObjectLine(line, fonts));
    return metrics;
}

/**
 * @brief measureForeignObjectContent() for text that may carry bold and italic: `fonts` supplies the face
 * for each style (see ForeignObjectFonts; svg_foreign_object_element::fontsFor() builds it the way painting
 * does), `baseStyle` is the style the foreignObject itself has. This is exactly what painting does, so the
 * metrics and the painted box cannot disagree about a bold label being wider than a regular one.
 */
inline ForeignObjectMetrics measureForeignObjectContent(std::string_view rawHtml, const ForeignObjectFonts& fonts, float maxWidth = -1.f, unsigned char baseStyle = 0)
{
    return measureForeignObjectSegments(rawHtml, foreignObjectStyledText(rawHtml, baseStyle), fonts, baseStyle, maxWidth);
}

/**
 * @brief measureForeignObjectContent() with a single face: every style is measured with `font`, i.e. as
 * if the text had no bold or italic in it. Line breaks and heights are unaffected by bold or italic;
 * only the widths of styled stretches can differ from what painting produces, by however much the real
 * bold/italic faces differ from `font`. Use the ForeignObjectFonts overload when that matters.
 */
inline ForeignObjectMetrics measureForeignObjectContent(std::string_view rawHtml, const Font& font, float maxWidth = -1.f)
{
    auto segments = foreignObjectStyledText(rawHtml);
    for(auto& segment : segments)
        segment.style = 0; // one face for everything: measured exactly like plain text
    ForeignObjectFonts fonts;
    fonts.face[0] = font;
    return measureForeignObjectSegments(rawHtml, segments, fonts, 0, maxWidth);
}

NOVASVG_INLINE void ForeignObjectSimple::render(const svg_foreign_object_element* el, svg_render_state& state) const
{
    auto segments = foreignObjectStyledText(el->rawContent(), el->baseFontStyle());
    if(segments.empty())
        return;

    const auto& font = el->font();
    if(font.is_null())
        return;

    // One face per style that actually occurs (just font() for unstyled text, nothing else is looked up).
    const auto fonts = el->fontsFor(foreignObjectStylesUsed(segments));

    auto box = el->fillBoundingBox();
    // A browser wraps HTML text at its box's edge unless CSS says nowrap/pre.
    auto lines = layoutForeignObjectText(segments, fonts, foreignObjectAllowsWrap(el->rawContent()) ? box.w : 0.f);

    // Real line-height from the HTML's own `line-height:` (mermaid's
    // content usually says 1.5) when present and parseable; falls back
    // to a 1.2x default (roughly a browser's own "normal") otherwise --
    // e.g. for %/em values or "normal" itself, which aren't handled.
    auto lineHeight = foreignObjectLineHeight(el->rawContent(), font.size()).value_or(font.height() * 1.2f);
    auto totalHeight = lineHeight * float(lines.size());
    auto topY = box.y + (box.h - totalHeight) / 2.f;

    if(auto backgroundColor = el->htmlBackgroundColor()) {
        state->set_color(*backgroundColor);
        Path backgroundPath;
        backgroundPath.addRect(box);
        state->fill_path(backgroundPath, FillRule::NonZero, state.currentTransform());
    }

    state->set_color(el->htmlTextColor().value_or(Color(0, 0, 0)));

    for(size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        auto lineWidth = measureForeignObjectLine(line, fonts);

        // A line is as tall as its tallest face (bold or italic can differ a little from the regular one);
        // a line of one style, the usual case, is simply that style's face.
        float ascent = font.ascent();
        float fontHeight = font.height();
        if(!line.empty()) {
            ascent = 0.f;
            fontHeight = 0.f;
            for(const auto& run : line) {
                ascent = STB_MAX(ascent, fonts.get(run.style).ascent());
                fontHeight = STB_MAX(fontHeight, fonts.get(run.style).height());
            }
        }

        // A real Trebuchet-MS-metrics render always fits each wrapped
        // line with room to spare -- so overflow here only ever means
        // OUR substitute font is wider than the one mermaid measured
        // with (this environment lacks Trebuchet MS). There's no
        // reliable way to know how much wider without the real font, so
        // rather than truncating (tried before, still didn't match a
        // real Chromium render) we condense that one line horizontally.
        float scale = 1.f;
        if(lineWidth > box.w && lineWidth > 0.f)
            scale = box.w / lineWidth;

        Point origin(
            box.x + (box.w - lineWidth * scale) / 2.f,
            // Baseline for this line = top of the text block + however
            // many line-heights precede it + half the leading (the extra
            // space line-height adds beyond the font's own ascent+descent,
            // split evenly above and below -- standard CSS line-box
            // behavior) + this font's ascent. Without the half-leading
            // term, text sits noticeably above center (font.height() is
            // usually well under a 1.5x line-height, so the omitted term
            // isn't a rounding-sized error).
            topY + float(i) * lineHeight + (lineHeight - fontHeight) / 2.f + ascent
        );

        auto textTransform = state.currentTransform();
        if(scale < 1.f) {
            // Condense horizontally about the line's own left edge so it
            // still lands at `origin`; only x is scaled, so glyph height
            // (and therefore this line's vertical position) is untouched.
            textTransform = textTransform * Transform::translated(origin.x, 0.f) * Transform::scaled(scale, 1.f) * Transform::translated(-origin.x, 0.f);
        }

        // The runs of the line side by side on the one baseline, each in its own face.
        float x = origin.x;
        for(const auto& run : line) {
            const std::u32string_view runView(run.text);
            const auto& runFont = fonts.get(run.style);
            state->fill_text(runView, runFont, Point(x, origin.y), textTransform);
            x += runFont.measureText(runView);
        }
    }
}

NOVASVG_INLINE svg_symbol_element::svg_symbol_element(document* dom)
    : svg_graphics_element(dom, ElementID::Symbol)
    , svg_fit_to_view_box(this)
{
}

NOVASVG_INLINE svg_g_element::svg_g_element(document* dom)
    : svg_graphics_element(dom, ElementID::G)
{
}

NOVASVG_INLINE void svg_g_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE svg_switch_element::svg_switch_element(document* dom)
    : svg_graphics_element(dom, ElementID::Switch)
{
}

// svg conditional processing for one switch child. `requiredExtensions` must be non-empty
// and name an extension we implement (the XHTML namespace -- <foreignObject> content);
// `systemLanguage` must list a tag matching the user language ("en"); no attribute -> true.
// (requiredFeatures was removed in svg 2 and browsers treat it as always true.)
inline bool switchChildPasses(const svg_element* child)
{
    auto trim = [](std::string_view v) {
        while(!v.empty() && std::isspace(static_cast<unsigned char>(v.front())))
            v.remove_prefix(1);
        while(!v.empty() && std::isspace(static_cast<unsigned char>(v.back())))
            v.remove_suffix(1);
        return v;
    };
    if(child->has_attribute(PropertyID::RequiredExtensions)) {
        std::string_view value = trim(child->attribute(PropertyID::RequiredExtensions));
        bool supported = false;
        size_t pos = 0;
        while(pos < value.size() && !supported) {
            while(pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos])))
                ++pos;
            auto end = pos;
            while(end < value.size() && !std::isspace(static_cast<unsigned char>(value[end])))
                ++end;
            supported = value.substr(pos, end - pos) == "http://www.w3.org/1999/xhtml";
            pos = end;
        }
        if(!supported)
            return false;
    }
    if(child->has_attribute(PropertyID::SystemLanguage)) {
        std::string_view value = child->attribute(PropertyID::SystemLanguage);
        bool matches = false;
        size_t pos = 0;
        while(pos <= value.size() && !matches) {
            auto comma = value.find(',', pos);
            auto tag = trim(value.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos));
            std::string lower(tag);
            for(auto& c : lower)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            matches = lower == "en" || lower.rfind("en-", 0) == 0;
            if(comma == std::string_view::npos)
                break;
            pos = comma + 1;
        }
        if(!matches)
            return false;
    }
    return true;
}

NOVASVG_INLINE const svg_element* svg_switch_element::selectedChild() const
{
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        // only graphics elements take part; <title>, <desc>, <defs>... are skipped
        if(!el || isDisallowedElement(el))
            continue;
        if(switchChildPasses(el))
            return el;
    }
    return nullptr;
}

NOVASVG_INLINE void svg_switch_element::render(svg_render_state& state) const
{
    if(isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(auto child = selectedChild())
        child->render(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE svg_defs_element::svg_defs_element(document* dom)
    : svg_graphics_element(dom, ElementID::Defs)
{
}

NOVASVG_INLINE svg_marker_element::svg_marker_element(document* dom)
    : svg_element(dom, ElementID::Marker)
    , svg_fit_to_view_box(this)
    , m_refX(PropertyID::RefX, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_refY(PropertyID::RefY, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_markerWidth(PropertyID::MarkerWidth, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerHeight(PropertyID::MarkerHeight, LengthDirection::Vertical, LengthNegativeMode::Forbid, 3.f, LengthUnits::None)
    , m_markerUnits(PropertyID::MarkerUnits, MarkerUnits::StrokeWidth)
    , m_orient(PropertyID::Orient)
{
    addProperty(m_refX);
    addProperty(m_refY);
    addProperty(m_markerWidth);
    addProperty(m_markerHeight);
    addProperty(m_markerUnits);
    addProperty(m_orient);
}

NOVASVG_INLINE Point svg_marker_element::refPoint() const
{
    LengthContext lengthContext(this);
    const Point refPoint = {
        lengthContext.valueForLength(m_refX),
        lengthContext.valueForLength(m_refY)
    };

    return refPoint;
}

NOVASVG_INLINE Size svg_marker_element::markerSize() const
{
    LengthContext lengthContext(this);
    const Size markerSize = {
        lengthContext.valueForLength(m_markerWidth),
        lengthContext.valueForLength(m_markerHeight)
    };

    return markerSize;
}

NOVASVG_INLINE Transform svg_marker_element::markerTransform(const Point& origin, float angle, float strokeWidth) const
{
    auto transform = Transform::translated(origin.x, origin.y);
    if(m_orient.orientType() == svg_angle::OrientType::Angle) {
        transform.rotate(m_orient.value());
    } else {
        transform.rotate(angle);
    }

    auto viewTransform = viewBoxToViewTransform(markerSize());
    auto refOrigin = viewTransform.mapPoint(refPoint());
    if(m_markerUnits.value() == MarkerUnits::StrokeWidth)
        transform.scale(strokeWidth, strokeWidth);
    transform.translate(-refOrigin.x, -refOrigin.y);
    return transform * viewTransform;
}

NOVASVG_INLINE Rect svg_marker_element::markerBoundingBox(const Point& origin, float angle, float strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).map_rect(paint_bounding_box());
}

NOVASVG_INLINE void svg_marker_element::renderMarker(svg_render_state& state, const Point& origin, float angle, float strokeWidth) const
{
    if(state.hasCycleReference(this))
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, markerTransform(origin, angle, strokeWidth));
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clip_rect(getClipRect(markerSize()), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE Transform svg_marker_element::local_transform() const
{
    return viewBoxToViewTransform(markerSize());
}

NOVASVG_INLINE svg_clip_path_element::svg_clip_path_element(document* dom)
    : svg_graphics_element(dom, ElementID::ClipPath)
    , m_clipPathUnits(PropertyID::ClipPathUnits, Units::UserSpaceOnUse)
{
    addProperty(m_clipPathUnits);
}

NOVASVG_INLINE Rect svg_clip_path_element::clipBoundingBox(const svg_element* el) const
{
    auto clipBoundingBox = paint_bounding_box();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        clipBoundingBox.x = clipBoundingBox.x * bbox.w + bbox.x;
        clipBoundingBox.y = clipBoundingBox.y * bbox.h + bbox.y;
        clipBoundingBox.w = clipBoundingBox.w * bbox.w;
        clipBoundingBox.h = clipBoundingBox.h * bbox.h;
    }

    return local_transform().map_rect(clipBoundingBox);
}

NOVASVG_INLINE void svg_clip_path_element::applyClipMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Clipping, maskImage);
    renderChildren(newState);
    if(clipper()) {
        clipper()->applyClipMask(newState);
    }

    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

inline const svg_geometry_element* toSVGGeometryElement(const svg_node* nd)
{
    if(nd && nd->isGeometryElement())
        return static_cast<const svg_geometry_element*>(nd);
    return nullptr;
}

NOVASVG_INLINE void svg_clip_path_element::applyClipPath(svg_render_state& state) const
{
    auto currentTransform = state.currentTransform() * local_transform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        Transform clipTransform(currentTransform);
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->id() != ElementID::Use)
                continue;
            clipTransform.multiply(el->local_transform());
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        state->clip_path(shapeElement->path(), shapeElement->clip_rule(), clipTransform * shapeElement->local_transform());
        return;
    }

    state->clip_rect(Rect::Empty, FillRule::NonZero, Transform::Identity);
}

NOVASVG_INLINE bool svg_clip_path_element::requiresMasking() const
{
    if(clipper())
        return true;
    const svg_geometry_element* prevShapeElement = nullptr;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(el == nullptr || el->isDisplayNone())
            continue;
        auto shapeElement = toSVGGeometryElement(el);
        if(shapeElement == nullptr) {
            if(el->isTextPositioningElement())
                return true;
            if(el->id() != ElementID::Use)
                continue;
            if(el->clipper())
                return true;
            shapeElement = toSVGGeometryElement(el->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        if(prevShapeElement || shapeElement->clipper())
            return true;
        prevShapeElement = shapeElement;
    }

    return false;
}

NOVASVG_INLINE svg_mask_element::svg_mask_element(document* dom)
    : svg_element(dom, ElementID::Mask)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, -10.f, LengthUnits::Percent)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 120.f, LengthUnits::Percent)
    , m_maskUnits(PropertyID::MaskUnits, Units::ObjectBoundingBox)
    , m_maskContentUnits(PropertyID::MaskContentUnits, Units::UserSpaceOnUse)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_maskUnits);
    addProperty(m_maskContentUnits);
}

NOVASVG_INLINE Rect svg_mask_element::maskRect(const svg_element* el) const
{
    LengthContext lengthContext(this, m_maskUnits.value());
    Rect maskRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(m_maskUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    return maskRect;
}

NOVASVG_INLINE Rect svg_mask_element::maskBoundingBox(const svg_element* el) const
{
    auto maskBoundingBox = paint_bounding_box();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = el->fillBoundingBox();
        maskBoundingBox.x = maskBoundingBox.x * bbox.w + bbox.x;
        maskBoundingBox.y = maskBoundingBox.y * bbox.h + bbox.y;
        maskBoundingBox.w = maskBoundingBox.w * bbox.w;
        maskBoundingBox.h = maskBoundingBox.h * bbox.h;
    }

    return maskBoundingBox.intersected(maskRect(el));
}

NOVASVG_INLINE void svg_mask_element::applyMask(svg_render_state& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = raster_canvas::create(state.currentTransform().map_rect(state.paint_bounding_box()));
    maskImage->clip_rect(maskRect(state.el()), FillRule::NonZero, state.currentTransform());

    auto currentTransform = state.currentTransform();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    svg_render_state newState(this, &state, currentTransform, svg_render_mode::Painting, maskImage);
    renderChildren(newState);
    if(clipper())
        clipper()->applyClipMask(newState);
    if(masker()) {
        masker()->applyMask(newState);
    }

    if(m_mask_type == MaskType::Luminance)
        maskImage->convert_to_luminance_mask();
    state.raster()->blend_canvas(*maskImage, BlendMode::Dst_In, 1.f);
}

// ---- svgfilterelement (impl) ----
NOVASVG_INLINE svg_filter_element::svg_filter_element(document* dom)
    : svg_element(dom, ElementID::Filter)
{
}

NOVASVG_INLINE float svg_filter_element::pixelMargin() const
{
    float margin = 0.f;
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        if(el->id() == ElementID::FeGaussianBlur) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 0.f);
            margin = STB_MAX(margin, sigma * 3.f);
        } else if(el->id() == ElementID::FeDropShadow) {
            auto sigma = parseFirstFloat(el->attribute(PropertyID::StdDeviation), 2.f);
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 2.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 2.f);
            margin = STB_MAX(margin, sigma * 3.f + STB_MAX(std::abs(dx), std::abs(dy)));
        } else if(el->id() == ElementID::FeOffset) {
            auto dx = parseFirstFloat(el->attribute(PropertyID::Dx), 0.f);
            auto dy = parseFirstFloat(el->attribute(PropertyID::Dy), 0.f);
            margin = STB_MAX(margin, STB_MAX(std::abs(dx), std::abs(dy)));
        }
        // feFlood/feComposite/feMerge don't themselves need extra margin;
        // if they combine an already-blurred/offset named result, that
        // primitive already grew the margin.
    }

    return margin;
}

NOVASVG_INLINE void svg_filter_element::applyFilter(svg_render_state& state, float scale) const
{
    svg_filter_context context(state.raster(), scale);
    for(const auto& child : children()) {
        auto el = toSVGElement(child);
        if(!el)
            continue;
        auto output = el->applyFilterPrimitive(context);
        if(!output)
            continue; // not a primitive (e.g. a stray feMergeNode outside feMerge)
        context.setResult(el->attribute(PropertyID::Result), std::move(output));
    }

    state.raster()->copy_pixels_from(*context.lastResult());
}

NOVASVG_INLINE void svg_mask_element::layoutElement(const svg_layout_state& state)
{
    m_mask_type = state.mask_type();
    svg_element::layoutElement(state);
}

} // namespace novasvg

#endif // NOVASVG_SVGELEMENT_H
