#ifndef NOVA_SVGELEMENT_H
#define NOVA_SVGELEMENT_H

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
#include "novasvg/novasvg.h"
#include "gfx/canvas.h"
#include "gfx/font.h"
#include "novasvg/gfx/parserutils.h"

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

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg), the former plutovg

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
using namespace vg; // the drawing layer (nova::vg), the former plutovg

PropertyID propertyid(std::string_view name);

PropertyID csspropertyid(std::string_view name);



template<>
NOVA_INLINE bool svg_enumeration<SpreadMethod>::parse(std::string_view input)
{
    static const svg_enumeration_entry<SpreadMethod> entries[] = {
        {SpreadMethod::Pad, "pad"},
        {SpreadMethod::Reflect, "reflect"},
        {SpreadMethod::Repeat, "repeat"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<Units>::parse(std::string_view input)
{
    static const svg_enumeration_entry<Units> entries[] = {
        {Units::UserSpaceOnUse, "userSpaceOnUse"},
        {Units::ObjectBoundingBox, "objectBoundingBox"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<MarkerUnits>::parse(std::string_view input)
{
    static const svg_enumeration_entry<MarkerUnits> entries[] = {
        {MarkerUnits::StrokeWidth, "strokeWidth"},
        {MarkerUnits::UserSpaceOnUse, "userSpaceOnUse"}
    };

    return parseEnum(input, entries);
}

template<>
NOVA_INLINE bool svg_enumeration<LengthAdjust>::parse(std::string_view input)
{
    static const svg_enumeration_entry<LengthAdjust> entries[] = {
        {LengthAdjust::Spacing, "spacing"},
        {LengthAdjust::SpacingAndGlyphs, "spacingAndGlyphs"}
    };

    return parseEnum(input, entries);
}

template<typename Enum>
template<unsigned int N>
NOVA_INLINE bool svg_enumeration<Enum>::parseEnum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N])
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





















// ---- svgrenderstate (impl) ----





// ---- svgpaintelement (impl) ----









GradientStops buildGradientStops(const svg_gradient_element* el, float opacity);








// ---- svggeometryelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg




















// ---- svgtextelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

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

AlignmentBaseline resolveDominantBaseline(const svg_text_positioning_element* el);

float calculateBaselineOffset(const svg_text_positioning_element* el);

bool needsTextAnchorAdjustment(const svg_text_positioning_element* el);

float calculateTextAnchorOffset(const svg_text_positioning_element* el, float width);

using svg_text_fragment_iterator = svg_text_fragment_list::iterator;

float calculateTextChunkLength(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool isVerticalText);

void handleTextChunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end);














// ---- svglayoutstate (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg


Color parseColor(std::string_view input, const svg_layout_state* state, const Color& defaultValue);

Color parseColorOrNone(std::string_view input, const svg_layout_state* state, const Color& defaultValue);

bool parseUrlValue(std::string_view& input, std::string& value);


Paint parsePaint(std::string_view input, const svg_layout_state* state, const Color& defaultValue);

float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue);

Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue);

BaselineShift parseBaselineShift(std::string_view input);

LengthList parseDashArray(std::string_view input);

Length parseLengthOrNormal(std::string_view input);

float parseFontSize(std::string_view input, const svg_layout_state* state);

template<typename Enum, unsigned int N>
NOVA_INLINE Enum parseEnumValue(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N], Enum defaultValue)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return defaultValue;
}

Display parseDisplay(std::string_view input);

Visibility parseVisibility(std::string_view input);

Overflow parseOverflow(std::string_view input);

PointerEvents parsePointerEvents(std::string_view input);

FontWeight parseFontWeight(std::string_view input);

FontStyle parseFontStyle(std::string_view input);

AlignmentBaseline parseAlignmentBaseline(std::string_view input);

DominantBaseline parseDominantBaseline(std::string_view input);

Direction parseDirection(std::string_view input);

WritingMode parseWritingMode(std::string_view input);

TextOrientation parseTextOrientation(std::string_view input);

TextAnchor parseTextAnchor(std::string_view input);

WhiteSpace parseWhiteSpace(std::string_view input);

MixBlendMode parseMixBlendMode(std::string_view input);

MaskType parseMaskType(std::string_view input);

FillRule parseFillRule(std::string_view input);

LineCap parseLineCap(std::string_view input);

LineJoin parseLineJoin(std::string_view input);


// The one place a (font-family stack, weight, style) triple becomes a Font, shared by svg <text>
// (svg_layout_state::font()) and by the bold/italic runs inside a <foreignObject>, so both always
// pick the very same face for the same request.
Font makeFont(const std::string& family, float size, bool bold, bool italic);


// ---- svgelement (impl) ----
using namespace vg; // the drawing layer (nova::vg), the former plutovg

ElementID elementid(std::string_view name);




NOVA_INLINE const std::string emptyString;









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






Bitmap loadImageResource(const std::string& href);


// ---------------------------------------------------------------
// svg_foreign_object_element / ForeignObjectSimple
// ---------------------------------------------------------------








namespace {

inline std::u32string utf8ToU32(const std::string& text)
{
    std::u32string result;
    result.reserve(text.size());
    text_iterator_t it;
    text_iterator_init(&it, text.data(), (int)text.length(), NOVA_TEXT_ENCODING_UTF8);
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














inline const svg_geometry_element* toSVGGeometryElement(const svg_node* nd)
{
    if(nd && nd->isGeometryElement())
        return static_cast<const svg_geometry_element*>(nd);
    return nullptr;
}







// ---- svgfilterelement (impl) ----




} // namespace nova::svg

#endif // NOVA_SVGELEMENT_H
