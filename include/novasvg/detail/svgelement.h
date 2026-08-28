#ifndef NOVASVG_SVGELEMENT_H
#define NOVASVG_SVGELEMENT_H

#include <string>
#include <forward_list>
#include <list>
#include <map>
#include <optional>
#include <cmath>
#include <set>
#include <cassert>
#include "svgparserutils.h"

// ---------------------------------------------------------------
// SVGElement and its family (SVGPaintElement, SVGGeometryElement,
// SVGTextElement, SVGLayoutState) used to be 5 declaration/
// implementation pairs (10 files). They're merged into this one
// file because they're circularly dependent -- SVGElement's own
// implementation needs the derived classes fully declared, and the
// derived classes need SVGElement fully declared to inherit from it
// -- so a clean 1:1 .h/.hpp split was never actually possible for
// this group; only a shared declare-everything-then-define-
// everything file is. Order matters: declarations below follow the
// dependency order (base class first), implementations follow
// (order doesn't matter there since all declarations above are
// already visible).
// ---------------------------------------------------------------

namespace novasvg {
using namespace render; // render/ layer (novasvg::render), the former plutovg

// ---- svgproperty ----
enum class PropertyID : uint8_t {
    Unknown = 0,
    Alignment_Baseline,
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
    Offset,
    Opacity,
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
    Rotate,
    Rx,
    Ry,
    SpreadMethod,
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
    Text_Anchor,
    Text_Orientation,
    TextLength,
    Transform,
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

class SVGElement;

class SVGProperty {
public:
    SVGProperty(PropertyID id);
    virtual ~SVGProperty() = default;
    PropertyID id() const { return m_id; }

    virtual bool parse(std::string_view input) = 0;

private:
    SVGProperty(const SVGProperty&) = delete;
    SVGProperty& operator=(const SVGProperty&) = delete;
    PropertyID m_id;
};

class SVGString final : public SVGProperty {
public:
    explicit SVGString(PropertyID id)
        : SVGProperty(id)
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
using SVGEnumerationEntry = std::pair<Enum, std::string_view>;

template<typename Enum>
class SVGEnumeration final : public SVGProperty {
public:
    explicit SVGEnumeration(PropertyID id, Enum value)
        : SVGProperty(id)
        , m_value(value)
    {}

    Enum value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    template<unsigned int N>
    bool parseEnum(std::string_view input, const SVGEnumerationEntry<Enum>(&entries)[N]);
    Enum m_value;
};

class SVGAngle final : public SVGProperty {
public:
    enum class OrientType {
        Auto,
        AutoStartReverse,
        Angle
    };

    explicit SVGAngle(PropertyID id)
        : SVGProperty(id)
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

class SVGLength final : public SVGProperty {
public:
    SVGLength(PropertyID id, LengthDirection direction, LengthNegativeMode negativeMode, float value = 0, LengthUnits units = LengthUnits::None)
        : SVGProperty(id)
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
    LengthContext(const SVGElement* element, Units units = Units::UserSpaceOnUse)
        : m_element(element), m_units(units)
    {}

    float valueForLength(const Length& length, LengthDirection direction) const;
    float valueForLength(const SVGLength& length) const { return valueForLength(length.value(), length.direction()); }

private:
    float viewportDimension(LengthDirection direction) const;
    const SVGElement* m_element;
    const Units m_units;
};

using LengthList = std::vector<Length>;

class SVGLengthList final : public SVGProperty {
public:
    SVGLengthList(PropertyID id, LengthDirection direction, LengthNegativeMode negativeMode)
        : SVGProperty(id)
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

class SVGNumber : public SVGProperty {
public:
    SVGNumber(PropertyID id, float value)
        : SVGProperty(id)
        , m_value(value)
    {}

    float value() const { return m_value; }
    bool parse(std::string_view input) override;

private:
    float m_value;
};

class SVGNumberPercentage final : public SVGProperty {
public:
    SVGNumberPercentage(PropertyID id, float value)
        : SVGProperty(id)
        , m_value(value)
    {}

    float value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    float m_value;
};

using NumberList = std::vector<float>;

class SVGNumberList final : public SVGProperty {
public:
    explicit SVGNumberList(PropertyID id)
        : SVGProperty(id)
    {}

    const NumberList& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    NumberList m_values;
};

class SVGPath final : public SVGProperty {
public:
    explicit SVGPath(PropertyID id)
        : SVGProperty(id)
    {}

    const Path& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Path m_value;
};

class SVGPoint final : public SVGProperty {
public:
   explicit SVGPoint(PropertyID id)
        : SVGProperty(id)
    {}

    const Point& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Point m_value;
};

using PointList = std::vector<Point>;

class SVGPointList final : public SVGProperty {
public:
    explicit SVGPointList(PropertyID id)
        : SVGProperty(id)
    {}

    const PointList& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    PointList m_values;
};

class SVGRect final : public SVGProperty {
public:
    explicit SVGRect(PropertyID id)
        : SVGProperty(id)
        , m_value(Rect::Invalid)
    {}

    const Rect& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Rect m_value;
};

class SVGTransform final : public SVGProperty {
public:
    explicit SVGTransform(PropertyID id)
        : SVGProperty(id)
    {}

    const Transform& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Transform m_value;
};

class SVGPreserveAspectRatio final : public SVGProperty {
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

    explicit SVGPreserveAspectRatio(PropertyID id)
        : SVGProperty(id)
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

class Document;
class SVGElement;
class SVGRootElement;

class SVGNode {
public:
    SVGNode(Document* document)
        : m_document(document)
    {}

    virtual ~SVGNode() = default;
    virtual bool isTextNode() const { return false; }
    virtual bool isElement() const { return false; }
    virtual bool isPaintElement() const { return false; }
    virtual bool isGraphicsElement() const { return false; }
    virtual bool isGeometryElement() const { return false; }
    virtual bool isTextPositioningElement() const { return false; }

    Document* document() const { return m_document; }
    SVGRootElement* rootElement() const { return m_document->rootElement(); }

    SVGElement* parentElement() const { return m_parentElement; }
    void setParentElement(SVGElement* parent) { m_parentElement = parent; }

    bool isRootElement() const { return m_parentElement == nullptr; }

    virtual std::unique_ptr<SVGNode> clone(bool deep) const = 0;

private:
    SVGNode(const SVGNode&) = delete;
    SVGNode& operator=(const SVGNode&) = delete;
    Document* m_document;
    SVGElement* m_parentElement = nullptr;
};

class SVGTextNode final : public SVGNode {
public:
    SVGTextNode(Document* document);

    bool isTextNode() const final { return true; }

    const std::string& data() const { return m_data; }
    void setData(const std::string& data);

    std::unique_ptr<SVGNode> clone(bool deep) const final;

private:
    std::string m_data;
};

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
    Symbol,
    Text,
    Tspan,
    Use
};

ElementID elementid(std::string_view name);

using SVGNodeList = std::list<std::unique_ptr<SVGNode>>;
using SVGPropertyList = std::forward_list<SVGProperty*>;

class SVGMarkerElement;
class SVGClipPathElement;
class SVGMaskElement;
class SVGPaintElement;
class SVGLayoutState;
class SVGRenderState;

extern const std::string emptyString;

class SVGElement : public SVGNode {
public:
    static std::unique_ptr<SVGElement> create(Document* document, ElementID id);

    SVGElement(Document* document, ElementID id);
    virtual ~SVGElement() = default;

    bool hasAttribute(std::string_view name) const;
    const std::string& getAttribute(std::string_view name) const;
    bool setAttribute(std::string_view name, const std::string& value);

    const Attribute* findAttribute(PropertyID id) const;
    bool hasAttribute(PropertyID id) const;
    const std::string& getAttribute(PropertyID id) const;
    bool setAttribute(int specificity, PropertyID id, const std::string& value);
    void setAttributes(const AttributeList& attributes);
    bool setAttribute(const Attribute& attribute);

    virtual void parseAttribute(PropertyID id, const std::string& value);

    SVGElement* previousElement() const;
    SVGElement* nextElement() const;

    SVGNode* addChild(std::unique_ptr<SVGNode> child);
    SVGNode* firstChild() const;
    SVGNode* lastChild() const;

    ElementID id() const { return m_id; }
    const AttributeList& attributes() const { return m_attributes; }
    const SVGPropertyList& properties() const { return m_properties; }
    const SVGNodeList& children() const { return m_children; }

    virtual Transform localTransform() const { return Transform::Identity; }
    virtual Rect fillBoundingBox() const;
    virtual Rect strokeBoundingBox() const;
    virtual Rect paintBoundingBox() const;

    SVGMarkerElement* getMarker(std::string_view id) const;
    SVGClipPathElement* getClipper(std::string_view id) const;
    SVGMaskElement* getMasker(std::string_view id) const;
    SVGPaintElement* getPainter(std::string_view id) const;

    SVGElement* elementFromPoint(float x, float y);

    template<typename T>
    void transverse(T callback);

    void addProperty(SVGProperty& value);
    SVGProperty* getProperty(PropertyID id) const;
    Size currentViewportSize() const;
    float font_size() const { return m_font_size; }

    void cloneChildren(SVGElement* parentElement) const;
    std::unique_ptr<SVGNode> clone(bool deep) const final;

    virtual void build();

    virtual void layoutElement(const SVGLayoutState& state);
    void layoutChildren(SVGLayoutState& state);
    virtual void layout(SVGLayoutState& state);

    void renderChildren(SVGRenderState& state) const;
    virtual void render(SVGRenderState& state) const;

    bool isDisplayNone() const { return m_display == Display::None; }
    bool isOverflowHidden() const { return m_overflow == Overflow::Hidden; }
    bool isVisibilityHidden() const { return m_visibility != Visibility::Visible; }

    bool isHiddenElement() const;
    bool isPointableElement() const;

    const SVGClipPathElement* clipper() const { return m_clipper; }
    const SVGMaskElement* masker() const { return m_masker; }
    float opacity() const { return m_opacity; }

    bool isElement() const final { return true; }

private:
    mutable Rect m_paintBoundingBox = Rect::Invalid;
    const SVGClipPathElement* m_clipper = nullptr;
    const SVGMaskElement* m_masker = nullptr;
    float m_opacity = 1.f;

    float m_font_size = 12.f;
    Display m_display = Display::Inline;
    Overflow m_overflow = Overflow::Visible;
    Visibility m_visibility = Visibility::Visible;
    PointerEvents m_pointer_events = PointerEvents::Auto;

    ElementID m_id;
    AttributeList m_attributes;
    SVGPropertyList m_properties;
    SVGNodeList m_children;
};

inline const SVGElement* toSVGElement(const SVGNode* node)
{
    if(node && node->isElement())
        return static_cast<const SVGElement*>(node);
    return nullptr;
}

inline SVGElement* toSVGElement(SVGNode* node)
{
    if(node && node->isElement())
        return static_cast<SVGElement*>(node);
    return nullptr;
}

inline SVGElement* toSVGElement(const std::unique_ptr<SVGNode>& node)
{
    return toSVGElement(node.get());
}

template<typename T>
inline void SVGElement::transverse(T callback)
{
    callback(this);
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child)) {
            element->transverse(callback);
        }
    }
}

class SVGStyleElement final : public SVGElement {
public:
    SVGStyleElement(Document* document);
};

class SVGFitToViewBox {
public:
    SVGFitToViewBox(SVGElement* element);

    const SVGRect& viewBox() const { return m_viewBox; }
    const SVGPreserveAspectRatio& preserveAspectRatio() const { return m_preserveAspectRatio; }
    Transform viewBoxToViewTransform(const Size& viewportSize) const;
    Rect getClipRect(const Size& viewportSize) const;

private:
    SVGRect m_viewBox;
    SVGPreserveAspectRatio m_preserveAspectRatio;
};

class SVGURIReference {
public:
    SVGURIReference(SVGElement* element);

    const SVGString& href() const { return m_href; }
    const std::string& hrefString() const { return m_href.value(); }
    SVGElement* getTargetElement(const Document* document) const;

private:
    SVGString m_href;
};

class SVGPaintServer {
public:
    SVGPaintServer() = default;
    SVGPaintServer(const SVGPaintElement* element, const Color& color, float opacity)
        : m_element(element), m_color(color), m_opacity(opacity)
    {}

    bool isRenderable() const { return m_opacity > 0.f && (m_element || m_color.getA() > 0); }

    const SVGPaintElement* element() const { return m_element; }
    const Color& color() const { return  m_color; }
    float opacity() const { return m_opacity; }

    bool applyPaint(SVGRenderState& state) const;

private:
    const SVGPaintElement* m_element = nullptr;
    Color m_color = Color::Transparent;
    float m_opacity = 0.f;
};

class SVGGraphicsElement : public SVGElement {
public:
    SVGGraphicsElement(Document* document, ElementID id);

    bool isGraphicsElement() const final { return true; }

    const SVGTransform& transform() const { return m_transform; }
    Transform localTransform() const override { return m_transform.value(); }

    SVGPaintServer getPaintServer(const Paint& paint, float opacity) const;
    StrokeData getStrokeData(const SVGLayoutState& state) const;

private:
    SVGTransform m_transform;
};

class SVGSVGElement : public SVGGraphicsElement, public SVGFitToViewBox {
public:
    SVGSVGElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }

    Transform localTransform() const override;
    void render(SVGRenderState& state) const override;

private:
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
};

class SVGRootElement final : public SVGSVGElement {
public:
    SVGRootElement(Document* document);

    float intrinsicWidth() const { return m_intrinsicWidth; }
    float intrinsicHeight() const { return m_intrinsicHeight; }

    void setNeedsLayout() { m_intrinsicWidth = -1.f; }
    bool needsLayout() const { return m_intrinsicWidth == -1.f; }

    SVGRootElement* layoutIfNeeded();

    SVGElement* getElementById(std::string_view id) const;
    void addElementById(const std::string& id, SVGElement* element);
    void layout(SVGLayoutState& state) final;

    void forceLayout();

private:
    std::map<std::string, SVGElement*, std::less<>> m_idCache;
    float m_intrinsicWidth{-1.f};
    float m_intrinsicHeight{-1.f};
};

class SVGUseElement final : public SVGGraphicsElement, public SVGURIReference {
public:
    SVGUseElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }

    Transform localTransform() const final;
    void render(SVGRenderState& state) const final;
    void build() final;

private:
    std::unique_ptr<SVGElement> cloneTargetElement(SVGElement* targetElement);
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
};

class SVGImageElement final : public SVGGraphicsElement {
public:
    SVGImageElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }
    const SVGPreserveAspectRatio& preserveAspectRatio() const { return m_preserveAspectRatio; }
    const Bitmap& image() const { return m_image; }

    Rect fillBoundingBox() const final;
    Rect strokeBoundingBox() const final;
    void render(SVGRenderState& state) const final;
    void parseAttribute(PropertyID id, const std::string& value) final;

private:
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    SVGPreserveAspectRatio m_preserveAspectRatio;
    Bitmap m_image;
};

class SVGForeignObjectElement;

/**
 * @brief Renders the (HTML-ish) content captured inside a <foreignObject>.
 *
 * This is an extension point on purpose: ForeignObjectSimple below only
 * understands a small, deliberately shallow slice of HTML5 -- it strips
 * every tag (div/span/p/i/b/...) and just draws the remaining plain text
 * centered in the foreignObject's box, using the font/fill already
 * inherited through the normal SVG/CSS cascade. That's enough to cover
 * what tools like Mermaid actually emit (a single short, non-wrapping,
 * centered text run per foreignObject), without pretending to be a real
 * HTML/CSS layout engine.
 *
 * A more complete renderer (e.g. one backed by litehtml) can be hooked in
 * later without touching SVGForeignObjectElement itself: implement this
 * interface and call SVGForeignObjectElement::setRenderer() (or replace
 * ForeignObjectSimple::instance() below) to swap it in.
 */
class ForeignObjectRenderer {
public:
    virtual ~ForeignObjectRenderer() = default;
    virtual void render(const SVGForeignObjectElement* element, SVGRenderState& state) const = 0;
};

/**
 * @brief Default ForeignObjectRenderer: tag-stripped plain text, centered.
 */
class ForeignObjectSimple final : public ForeignObjectRenderer {
public:
    static ForeignObjectSimple* instance();

    void render(const SVGForeignObjectElement* element, SVGRenderState& state) const final;
};

class SVGForeignObjectElement final : public SVGGraphicsElement {
public:
    SVGForeignObjectElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }
    const Font& font() const { return m_font; }
    const SVGPaintServer& fill() const { return m_fill; }

    /**
     * @brief Raw markup captured between <foreignObject ...> and
     * </foreignObject> (whatever HTML/XML the document embedded there),
     * exactly as it appeared in the source -- untouched, unparsed. What a
     * ForeignObjectRenderer does with it is entirely up to that renderer.
     */
    const std::string& rawContent() const { return m_rawContent; }
    void setRawContent(std::string content) { m_rawContent = std::move(content); }

    ForeignObjectRenderer* renderer() const { return m_renderer; }
    void setRenderer(ForeignObjectRenderer* renderer) { m_renderer = renderer; }

    Rect fillBoundingBox() const final;
    Rect strokeBoundingBox() const final;
    void layoutElement(const SVGLayoutState& state) final;
    void render(SVGRenderState& state) const final;

private:
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    Font m_font;
    SVGPaintServer m_fill;
    std::string m_rawContent;
    ForeignObjectRenderer* m_renderer = nullptr;
};

class SVGSymbolElement final : public SVGGraphicsElement, public SVGFitToViewBox {
public:
    SVGSymbolElement(Document* document);
};

class SVGGElement final : public SVGGraphicsElement {
public:
    SVGGElement(Document* document);

    void render(SVGRenderState& state) const final;
};

class SVGDefsElement final : public SVGGraphicsElement {
public:
    SVGDefsElement(Document* document);
};

class SVGMarkerElement final : public SVGElement, public SVGFitToViewBox {
public:
    SVGMarkerElement(Document* document);

    const SVGLength& refX() const { return m_refX; }
    const SVGLength& refY() const { return m_refY; }
    const SVGLength& markerWidth() const { return m_markerWidth; }
    const SVGLength& markerHeight() const { return m_markerHeight; }
    const SVGEnumeration<MarkerUnits>& markerUnits() const { return m_markerUnits; }
    const SVGAngle& orient() const { return m_orient; }

    Point refPoint() const;
    Size markerSize() const;

    Transform markerTransform(const Point& origin, float angle, float strokeWidth) const;
    Rect markerBoundingBox(const Point& origin, float angle, float strokeWidth) const;
    void renderMarker(SVGRenderState& state, const Point& origin, float angle, float strokeWidth) const;

    Transform localTransform() const final;

private:
    SVGLength m_refX;
    SVGLength m_refY;
    SVGLength m_markerWidth;
    SVGLength m_markerHeight;
    SVGEnumeration<MarkerUnits> m_markerUnits;
    SVGAngle m_orient;
};

class SVGClipPathElement final : public SVGGraphicsElement {
public:
    SVGClipPathElement(Document* document);

    const SVGEnumeration<Units>& clipPathUnits() const { return m_clipPathUnits; }
    Rect clipBoundingBox(const SVGElement* element) const;

    void applyClipMask(SVGRenderState& state) const;
    void applyClipPath(SVGRenderState& state) const;

    bool requiresMasking() const;

private:
    SVGEnumeration<Units> m_clipPathUnits;
};

class SVGMaskElement final : public SVGElement {
public:
    SVGMaskElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }
    const SVGEnumeration<Units>& maskUnits() const { return m_maskUnits; }
    const SVGEnumeration<Units>& maskContentUnits() const { return m_maskContentUnits; }

    Rect maskRect(const SVGElement* element) const;
    Rect maskBoundingBox(const SVGElement* element) const;
    void applyMask(SVGRenderState& state) const;

    void layoutElement(const SVGLayoutState& state) final;

private:
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    SVGEnumeration<Units> m_maskUnits;
    SVGEnumeration<Units> m_maskContentUnits;
    MaskType m_mask_type = MaskType::Luminance;
};

// ---- svgrenderstate ----
enum class SVGRenderMode {
    Painting,
    Clipping
};

class SVGBlendInfo {
public:
    explicit SVGBlendInfo(const SVGElement* element);
    SVGBlendInfo(const SVGClipPathElement* clipper, const SVGMaskElement* masker, float opacity)
        : m_clipper(clipper), m_masker(masker), m_opacity(opacity)
    {}

    bool requiresCompositing(SVGRenderMode mode) const;
    const SVGClipPathElement* clipper() const { return m_clipper; }
    const SVGMaskElement* masker() const { return m_masker; }
    float opacity() const { return m_opacity; }

private:
    const SVGClipPathElement* m_clipper;
    const SVGMaskElement* m_masker;
    const float m_opacity;
};

class SVGRenderState {
public:
    SVGRenderState(const SVGElement* element, const SVGRenderState& parent, const Transform& localTransform)
        : m_element(element), m_parent(&parent), m_currentTransform(parent.currentTransform() * localTransform)
        , m_mode(parent.mode()), m_canvas(parent.canvas())
    {}

    SVGRenderState(const SVGElement* element, const SVGRenderState* parent, const Transform& currentTransform, SVGRenderMode mode, std::shared_ptr<Canvas> canvas)
        : m_element(element), m_parent(parent), m_currentTransform(currentTransform), m_mode(mode), m_canvas(std::move(canvas))
    {}

    Canvas& operator*() const { return *m_canvas; }
    Canvas* operator->() const { return &*m_canvas; }

    const SVGElement* element() const { return m_element; }
    const SVGRenderState* parent() const { return m_parent; }
    const Transform& currentTransform() const { return m_currentTransform; }
    const SVGRenderMode mode() const { return m_mode; }
    const std::shared_ptr<Canvas>& canvas() const { return m_canvas; }

    Rect fillBoundingBox() const { return m_element->fillBoundingBox(); }
    Rect paintBoundingBox() const { return m_element->paintBoundingBox(); }

    bool hasCycleReference(const SVGElement* element) const;

    void beginGroup(const SVGBlendInfo& blendInfo);
    void endGroup(const SVGBlendInfo& blendInfo);

private:
    const SVGElement* m_element;
    const SVGRenderState* m_parent;
    const Transform m_currentTransform;
    const SVGRenderMode m_mode;
    std::shared_ptr<Canvas> m_canvas;
};

// ---- svgpaintelement ----

class SVGPaintElement : public SVGElement {
public:
    SVGPaintElement(Document* document, ElementID id);

    bool isPaintElement() const final { return true; }

    virtual bool applyPaint(SVGRenderState& state, float opacity) const = 0;
};

class SVGStopElement final : public SVGElement {
public:
    SVGStopElement(Document* document);

    void layoutElement(const SVGLayoutState& state) final;
    const SVGNumberPercentage& offset() const { return m_offset; }
    GradientStop gradientStop(float opacity) const;

private:
    SVGNumberPercentage m_offset;
    Color m_stop_color = Color::Black;
    float m_stop_opacity = 1.f;
};

class SVGGradientAttributes;

class SVGGradientElement : public SVGPaintElement, public SVGURIReference {
public:
    SVGGradientElement(Document* document, ElementID id);

    const SVGTransform& gradientTransform() const { return m_gradientTransform; }
    const SVGEnumeration<Units>& gradientUnits() const { return m_gradientUnits; }
    const SVGEnumeration<SpreadMethod>& spreadMethod() const { return m_spreadMethod; }
    void collectGradientAttributes(SVGGradientAttributes& attributes) const;

private:
    SVGTransform m_gradientTransform;
    SVGEnumeration<Units> m_gradientUnits;
    SVGEnumeration<SpreadMethod> m_spreadMethod;
};

class SVGGradientAttributes {
public:
    SVGGradientAttributes() = default;

    const Transform& gradientTransform() const { return m_gradientTransform->gradientTransform().value(); }
    SpreadMethod spreadMethod() const { return m_spreadMethod->spreadMethod().value(); }
    Units gradientUnits() const { return m_gradientUnits->gradientUnits().value(); }
    const SVGGradientElement* gradientContentElement() const { return m_gradientContentElement; }

    bool hasGradientTransform() const { return m_gradientTransform; }
    bool hasSpreadMethod() const { return m_spreadMethod; }
    bool hasGradientUnits() const { return m_gradientUnits; }
    bool hasGradientContentElement() const { return m_gradientContentElement; }

    void setGradientTransform(const SVGGradientElement* value) { m_gradientTransform = value; }
    void setSpreadMethod(const SVGGradientElement* value) { m_spreadMethod = value; }
    void setGradientUnits(const SVGGradientElement* value) { m_gradientUnits = value; }
    void setGradientContentElement(const SVGGradientElement* value) { m_gradientContentElement = value; }

    void setDefaultValues(const SVGGradientElement* element) {
        if(!m_gradientTransform) { m_gradientTransform = element; }
        if(!m_spreadMethod) { m_spreadMethod = element; }
        if(!m_gradientUnits) { m_gradientUnits = element; }
        if(!m_gradientContentElement) { m_gradientContentElement = element; }
    }

private:
    const SVGGradientElement* m_gradientTransform{nullptr};
    const SVGGradientElement* m_spreadMethod{nullptr};
    const SVGGradientElement* m_gradientUnits{nullptr};
    const SVGGradientElement* m_gradientContentElement{nullptr};
};

class SVGLinearGradientAttributes;

class SVGLinearGradientElement final : public SVGGradientElement {
public:
    SVGLinearGradientElement(Document* document);

    const SVGLength& x1() const { return m_x1; }
    const SVGLength& y1() const { return m_y1; }
    const SVGLength& x2() const { return m_x2; }
    const SVGLength& y2() const { return m_y2; }

    bool applyPaint(SVGRenderState& state, float opacity) const final;

private:
    SVGLinearGradientAttributes collectGradientAttributes() const;
    SVGLength m_x1;
    SVGLength m_y1;
    SVGLength m_x2;
    SVGLength m_y2;
};

class SVGLinearGradientAttributes : public SVGGradientAttributes {
public:
    SVGLinearGradientAttributes() = default;

    const SVGLength& x1() const { return m_x1->x1(); }
    const SVGLength& y1() const { return m_y1->y1(); }
    const SVGLength& x2() const { return m_x2->x2(); }
    const SVGLength& y2() const { return m_y2->y2(); }

    bool hasX1() const { return m_x1; }
    bool hasY1() const { return m_y1; }
    bool hasX2() const { return m_x2; }
    bool hasY2() const { return m_y2; }

    void setX1(const SVGLinearGradientElement* value) { m_x1 = value; }
    void setY1(const SVGLinearGradientElement* value) { m_y1 = value; }
    void setX2(const SVGLinearGradientElement* value) { m_x2 = value; }
    void setY2(const SVGLinearGradientElement* value) { m_y2 = value; }

    void setDefaultValues(const SVGLinearGradientElement* element) {
        SVGGradientAttributes::setDefaultValues(element);
        if(!m_x1) { m_x1 = element; }
        if(!m_y1) { m_y1 = element; }
        if(!m_x2) { m_x2 = element; }
        if(!m_y2) { m_y2 = element; }
    }

private:
    const SVGLinearGradientElement* m_x1{nullptr};
    const SVGLinearGradientElement* m_y1{nullptr};
    const SVGLinearGradientElement* m_x2{nullptr};
    const SVGLinearGradientElement* m_y2{nullptr};
};

class SVGRadialGradientAttributes;

class SVGRadialGradientElement final : public SVGGradientElement {
public:
    SVGRadialGradientElement(Document* document);

    const SVGLength& cx() const { return m_cx; }
    const SVGLength& cy() const { return m_cy; }
    const SVGLength& r() const { return m_r; }
    const SVGLength& fx() const { return m_fx; }
    const SVGLength& fy() const { return m_fy; }

    bool applyPaint(SVGRenderState& state, float opacity) const final;

private:
    SVGRadialGradientAttributes collectGradientAttributes() const;
    SVGLength m_cx;
    SVGLength m_cy;
    SVGLength m_r;
    SVGLength m_fx;
    SVGLength m_fy;
};

class SVGRadialGradientAttributes : public SVGGradientAttributes {
public:
    SVGRadialGradientAttributes() = default;

    const SVGLength& cx() const { return m_cx->cx(); }
    const SVGLength& cy() const { return m_cy->cy(); }
    const SVGLength& r() const { return m_r->r(); }
    const SVGLength& fx() const { return m_fx ? m_fx->fx() : m_cx->cx(); }
    const SVGLength& fy() const { return m_fy ? m_fy->fy() : m_cy->cy(); }

    bool hasCx() const { return m_cx; }
    bool hasCy() const { return m_cy; }
    bool hasR() const { return m_r; }
    bool hasFx() const { return m_fx; }
    bool hasFy() const { return m_fy; }

    void setCx(const SVGRadialGradientElement* value) { m_cx = value; }
    void setCy(const SVGRadialGradientElement* value) { m_cy = value; }
    void setR(const SVGRadialGradientElement* value) { m_r = value; }
    void setFx(const SVGRadialGradientElement* value) { m_fx = value; }
    void setFy(const SVGRadialGradientElement* value) { m_fy = value; }

    void setDefaultValues(const SVGRadialGradientElement* element) {
        SVGGradientAttributes::setDefaultValues(element);
        if(!m_cx) { m_cx = element; }
        if(!m_cy) { m_cy = element; }
        if(!m_r) { m_r = element; }
    }

private:
    const SVGRadialGradientElement* m_cx{nullptr};
    const SVGRadialGradientElement* m_cy{nullptr};
    const SVGRadialGradientElement* m_r{nullptr};
    const SVGRadialGradientElement* m_fx{nullptr};
    const SVGRadialGradientElement* m_fy{nullptr};
};

class SVGPatternAttributes;

class SVGPatternElement final : public SVGPaintElement, public SVGURIReference, public SVGFitToViewBox {
public:
    SVGPatternElement(Document* document);

    const SVGLength& x() const { return m_x; }
    const SVGLength& y() const { return m_y; }
    const SVGLength& width() const { return m_width; }
    const SVGLength& height() const { return m_height; }
    const SVGTransform& patternTransform() const { return m_patternTransform; }
    const SVGEnumeration<Units>& patternUnits() const { return m_patternUnits; }
    const SVGEnumeration<Units>& patternContentUnits() const { return m_patternContentUnits; }

    bool applyPaint(SVGRenderState& state, float opacity) const final;

private:
    SVGPatternAttributes collectPatternAttributes() const;
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    SVGTransform m_patternTransform;
    SVGEnumeration<Units> m_patternUnits;
    SVGEnumeration<Units> m_patternContentUnits;
};

class SVGPatternAttributes {
public:
    SVGPatternAttributes() = default;

    const SVGLength& x() const { return m_x->x(); }
    const SVGLength& y() const { return m_y->y(); }
    const SVGLength& width() const { return m_width->width(); }
    const SVGLength& height() const { return m_height->height(); }
    const Transform& patternTransform() const { return m_patternTransform->patternTransform().value(); }
    Units patternUnits() const { return m_patternUnits->patternUnits().value(); }
    Units patternContentUnits() const { return m_patternContentUnits->patternContentUnits().value(); }
    const Rect& viewBox() const { return m_viewBox->viewBox().value(); }
    const SVGPreserveAspectRatio& preserveAspectRatio() const { return m_preserveAspectRatio->preserveAspectRatio(); }
    const SVGPatternElement* patternContentElement() const { return m_patternContentElement; }

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

    void setX(const SVGPatternElement* value) { m_x = value; }
    void setY(const SVGPatternElement* value) { m_y = value; }
    void setWidth(const SVGPatternElement* value) { m_width = value; }
    void setHeight(const SVGPatternElement* value) { m_height = value; }
    void setPatternTransform(const SVGPatternElement* value) { m_patternTransform = value; }
    void setPatternUnits(const SVGPatternElement* value) { m_patternUnits = value; }
    void setPatternContentUnits(const SVGPatternElement* value) { m_patternContentUnits = value; }
    void setViewBox(const SVGPatternElement* value) { m_viewBox = value; }
    void setPreserveAspectRatio(const SVGPatternElement* value) { m_preserveAspectRatio = value; }
    void setPatternContentElement(const SVGPatternElement* value) { m_patternContentElement = value; }

    void setDefaultValues(const SVGPatternElement* element) {
        if(!m_x) { m_x = element; }
        if(!m_y) { m_y = element; }
        if(!m_width) { m_width = element; }
        if(!m_height) { m_height = element; }
        if(!m_patternTransform) { m_patternTransform = element; }
        if(!m_patternUnits) { m_patternUnits = element; }
        if(!m_patternContentUnits) { m_patternContentUnits = element; }
        if(!m_viewBox) { m_viewBox = element; }
        if(!m_preserveAspectRatio) { m_preserveAspectRatio = element; }
        if(!m_patternContentElement) { m_patternContentElement = element; }
    }

private:
    const SVGPatternElement* m_x{nullptr};
    const SVGPatternElement* m_y{nullptr};
    const SVGPatternElement* m_width{nullptr};
    const SVGPatternElement* m_height{nullptr};
    const SVGPatternElement* m_patternTransform{nullptr};
    const SVGPatternElement* m_patternUnits{nullptr};
    const SVGPatternElement* m_patternContentUnits{nullptr};
    const SVGPatternElement* m_viewBox{nullptr};
    const SVGPatternElement* m_preserveAspectRatio{nullptr};
    const SVGPatternElement* m_patternContentElement{nullptr};
};

// ---- svggeometryelement ----

class SVGMarkerPosition {
public:
    SVGMarkerPosition(const SVGMarkerElement* element, const Point& origin, float angle)
        : m_element(element), m_origin(origin), m_angle(angle)
    {}

    const SVGMarkerElement* element() const { return m_element; }
    const Point& origin() const { return m_origin; }
    float angle() const { return m_angle; }

    Rect markerBoundingBox(float strokeWidth) const;
    void renderMarker(SVGRenderState& state, float strokeWidth) const;

private:
    const SVGMarkerElement* m_element;
    Point m_origin;
    float m_angle;
};

using SVGMarkerPositionList = std::vector<SVGMarkerPosition>;

class SVGGeometryElement : public SVGGraphicsElement {
public:
    SVGGeometryElement(Document* document, ElementID id);

    bool isGeometryElement() const final { return true; }

    Rect fillBoundingBox() const override { return m_fillBoundingBox; }
    Rect strokeBoundingBox() const override;
    void layoutElement(const SVGLayoutState& state) override;

    bool isRenderable() const { return !m_path.isNull() && !isDisplayNone() && !isVisibilityHidden(); }

    FillRule fill_rule() const { return m_fill_rule; }
    FillRule clip_rule() const { return m_clip_rule; }

    virtual Rect updateShape(Path& path) = 0;

    void updateMarkerPositions(SVGMarkerPositionList& positions, const SVGLayoutState& state);
    void render(SVGRenderState& state) const override;

    const Path& path() const { return m_path; }

private:
    Path m_path;
    Rect m_fillBoundingBox;
    StrokeData m_strokeData;

    SVGPaintServer m_fill;
    SVGPaintServer m_stroke;
    SVGMarkerPositionList m_markerPositions;

    FillRule m_fill_rule = FillRule::NonZero;
    FillRule m_clip_rule = FillRule::NonZero;
};

class SVGLineElement final : public SVGGeometryElement {
public:
    SVGLineElement(Document* document);

    Rect updateShape(Path& path) final;

private:
    SVGLength m_x1;
    SVGLength m_y1;
    SVGLength m_x2;
    SVGLength m_y2;
};

class SVGRectElement final : public SVGGeometryElement {
public:
    SVGRectElement(Document* document);

    Rect updateShape(Path& path) final;

private:
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    SVGLength m_rx;
    SVGLength m_ry;
};

class SVGEllipseElement final : public SVGGeometryElement {
public:
    SVGEllipseElement(Document* document);

    Rect updateShape(Path& path) final;

private:
    SVGLength m_cx;
    SVGLength m_cy;
    SVGLength m_rx;
    SVGLength m_ry;
};

class SVGCircleElement final : public SVGGeometryElement {
public:
    SVGCircleElement(Document* document);

    Rect updateShape(Path& path) final;

private:
    SVGLength m_cx;
    SVGLength m_cy;
    SVGLength m_r;
};

class SVGPolyElement final : public SVGGeometryElement {
public:
    SVGPolyElement(Document* document, ElementID id);

    Rect updateShape(Path& path) final;

private:
    SVGPointList m_points;
};

class SVGPathElement final : public SVGGeometryElement {
public:
    SVGPathElement(Document* document);

    Rect updateShape(Path& path) final;

private:
    SVGPath m_d;
};

// ---- svgtextelement ----

class SVGTextPositioningElement;
class SVGTextElement;

struct SVGCharacterPosition {
    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> dx;
    std::optional<float> dy;
    std::optional<float> rotate;
};

using SVGCharacterPositions = std::map<size_t, SVGCharacterPosition>;

struct SVGTextPosition {
    SVGTextPosition(const SVGNode* node, size_t startOffset, size_t endOffset)
        : node(node), startOffset(startOffset), endOffset(endOffset)
    {}

    const SVGNode* node;
    size_t startOffset;
    size_t endOffset;
};

using SVGTextPositionList = std::vector<SVGTextPosition>;

struct SVGTextFragment {
    explicit SVGTextFragment(const SVGTextPositioningElement* element) : element(element) {}
    const SVGTextPositioningElement* element;
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

using SVGTextFragmentList = std::vector<SVGTextFragment>;

class SVGTextFragmentsBuilder {
public:
    SVGTextFragmentsBuilder(std::u32string& text, SVGTextFragmentList& fragments);

    void build(const SVGTextElement* textElement);

private:
    void handleText(const SVGTextNode* node);
    void handleElement(const SVGTextPositioningElement* element);
    void fillCharacterPositions(const SVGTextPosition& position);
    std::u32string& m_text;
    SVGTextFragmentList& m_fragments;
    SVGCharacterPositions m_characterPositions;
    SVGTextPositionList m_textPositions;
    size_t m_characterOffset = 0;
    float m_x = 0;
    float m_y = 0;
};

class SVGTextPositioningElement : public SVGGraphicsElement {
public:
    SVGTextPositioningElement(Document* document, ElementID id);

    bool isTextPositioningElement() const final { return true; }

    const LengthList& x() const { return m_x.values(); }
    const LengthList& y() const { return m_y.values(); }
    const LengthList& dx() const { return m_dx.values(); }
    const LengthList& dy() const { return m_dy.values(); }
    const NumberList& rotate() const { return m_rotate.values(); }

    const SVGLength& textLength() const { return m_textLength; }
    LengthAdjust lengthAdjust() const { return m_lengthAdjust.value(); }

    const Font& font() const { return m_font; }
    const SVGPaintServer& fill() const { return m_fill; }
    const SVGPaintServer& stroke() const { return m_stroke; }

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

    void layoutElement(const SVGLayoutState& state) override;

private:
    float convertBaselineOffset(const BaselineShift& baselineShift) const;
    SVGLengthList m_x;
    SVGLengthList m_y;
    SVGLengthList m_dx;
    SVGLengthList m_dy;
    SVGNumberList m_rotate;

    SVGLength m_textLength;
    SVGEnumeration<LengthAdjust> m_lengthAdjust;

    Font m_font;
    SVGPaintServer m_fill;
    SVGPaintServer m_stroke;

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

class SVGTSpanElement final : public SVGTextPositioningElement {
public:
    SVGTSpanElement(Document* document);
};

class SVGTextElement final : public SVGTextPositioningElement {
public:
    SVGTextElement(Document* document);

    Rect fillBoundingBox() const final { return boundingBox(false); }
    Rect strokeBoundingBox() const final { return boundingBox(true); }

    void layout(SVGLayoutState& state) final;
    void render(SVGRenderState& state) const final;

private:
    Rect boundingBox(bool includeStroke) const;
    SVGTextFragmentList m_fragments;
    std::u32string m_text;
};

// ---- svglayoutstate ----

class SVGLayoutState {
public:
    SVGLayoutState() = default;
    SVGLayoutState(const SVGLayoutState& parent, const SVGElement* element);

    const SVGLayoutState* parent() const { return m_parent; }
    const SVGElement* element() const { return m_element; }

    const Paint& fill() const { return m_fill; }
    const Paint& stroke() const { return m_stroke; }

    const Color& color() const { return m_color; }
    const Color& stop_color() const { return m_stop_color; }

    float opacity() const { return m_opacity; }
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
    const std::string& marker_start() const { return m_marker_start; }
    const std::string& marker_mid() const { return m_marker_mid; }
    const std::string& marker_end() const { return m_marker_end; }
    const std::string& font_family() const { return m_font_family; }

    Font font() const;

private:
    const SVGLayoutState* m_parent = nullptr;
    const SVGElement* m_element = nullptr;

    Paint m_fill{Color::Black};
    Paint m_stroke{Color::Transparent};

    Color m_color = Color::Black;
    Color m_stop_color = Color::Black;

    float m_opacity = 1.f;
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
    std::string m_marker_start;
    std::string m_marker_mid;
    std::string m_marker_end;
    std::string m_font_family;
};

// ================================================================
// Implementation
// ================================================================

// ---- svgproperty (impl) ----
using namespace render; // render/ layer (novasvg::render), the former plutovg

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
        {"lengthAdjust", PropertyID::LengthAdjust},
        {"markerHeight", PropertyID::MarkerHeight},
        {"markerUnits", PropertyID::MarkerUnits},
        {"markerWidth", PropertyID::MarkerWidth},
        {"maskContentUnits", PropertyID::MaskContentUnits},
        {"maskUnits", PropertyID::MaskUnits},
        {"offset", PropertyID::Offset},
        {"orient", PropertyID::Orient},
        {"patternContentUnits", PropertyID::PatternContentUnits},
        {"patternTransform", PropertyID::PatternTransform},
        {"patternUnits", PropertyID::PatternUnits},
        {"points", PropertyID::Points},
        {"preserveAspectRatio", PropertyID::PreserveAspectRatio},
        {"r", PropertyID::R},
        {"refX", PropertyID::RefX},
        {"refY", PropertyID::RefY},
        {"rotate", PropertyID::Rotate},
        {"rx", PropertyID::Rx},
        {"ry", PropertyID::Ry},
        {"spreadMethod", PropertyID::SpreadMethod},
        {"style", PropertyID::Style},
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

NOVASVG_INLINE SVGProperty::SVGProperty(PropertyID id)
    : m_id(id)
{
}

NOVASVG_INLINE bool SVGString::parse(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    m_value.assign(input);
    return true;
}

template<>
NOVASVG_INLINE bool SVGEnumeration<SpreadMethod>::parse(std::string_view input)
{
    static const SVGEnumerationEntry<SpreadMethod> entries[] = {
        {SpreadMethod::Pad, "pad"},
        {SpreadMethod::Reflect, "reflect"},
        {SpreadMethod::Repeat, "repeat"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool SVGEnumeration<Units>::parse(std::string_view input)
{
    static const SVGEnumerationEntry<Units> entries[] = {
        {Units::UserSpaceOnUse, "userSpaceOnUse"},
        {Units::ObjectBoundingBox, "objectBoundingBox"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool SVGEnumeration<MarkerUnits>::parse(std::string_view input)
{
    static const SVGEnumerationEntry<MarkerUnits> entries[] = {
        {MarkerUnits::StrokeWidth, "strokeWidth"},
        {MarkerUnits::UserSpaceOnUse, "userSpaceOnUse"}
    };

    return parseEnum(input, entries);
}

template<>
NOVASVG_INLINE bool SVGEnumeration<LengthAdjust>::parse(std::string_view input)
{
    static const SVGEnumerationEntry<LengthAdjust> entries[] = {
        {LengthAdjust::Spacing, "spacing"},
        {LengthAdjust::SpacingAndGlyphs, "spacingAndGlyphs"}
    };

    return parseEnum(input, entries);
}

template<typename Enum>
template<unsigned int N>
NOVASVG_INLINE bool SVGEnumeration<Enum>::parseEnum(std::string_view input, const SVGEnumerationEntry<Enum>(&entries)[N])
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

NOVASVG_INLINE bool SVGAngle::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGLength::parse(std::string_view input)
{
    return m_value.parse(input, m_negativeMode);
}

NOVASVG_INLINE bool SVGLengthList::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGNumber::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGNumberPercentage::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGNumberList::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGPath::parse(std::string_view input)
{
    return m_value.parse(input.data(), input.length());
}

NOVASVG_INLINE bool SVGPoint::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGPointList::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGRect::parse(std::string_view input)
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

NOVASVG_INLINE bool SVGTransform::parse(std::string_view input)
{
    return m_value.parse(input.data(), input.length());
}

NOVASVG_INLINE bool SVGPreserveAspectRatio::parse(std::string_view input)
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

NOVASVG_INLINE Rect SVGPreserveAspectRatio::getClipRect(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.isEmpty() && !viewportSize.isEmpty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Rect(viewBoxRect.x, viewBoxRect.y, viewportSize.w / xScale, viewportSize.h / yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? std::min(xScale, yScale) : std::max(xScale, yScale);
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

NOVASVG_INLINE Transform SVGPreserveAspectRatio::getTransform(const Rect& viewBoxRect, const Size& viewportSize) const
{
    assert(!viewBoxRect.isEmpty() && !viewportSize.isEmpty());
    auto xScale = viewportSize.w / viewBoxRect.w;
    auto yScale = viewportSize.h / viewBoxRect.h;
    if(m_alignType == AlignType::None) {
        return Transform(xScale, 0, 0, yScale, -viewBoxRect.x * xScale, -viewBoxRect.y * yScale);
    }

    auto scale = (m_meetOrSlice == MeetOrSlice::Meet) ? std::min(xScale, yScale) : std::max(xScale, yScale);
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

NOVASVG_INLINE void SVGPreserveAspectRatio::transformRect(Rect& dstRect, Rect& srcRect) const
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
NOVASVG_INLINE SVGBlendInfo::SVGBlendInfo(const SVGElement* element)
    : m_clipper(element->clipper())
    , m_masker(element->masker())
    , m_opacity(element->opacity())
{
}

NOVASVG_INLINE bool SVGBlendInfo::requiresCompositing(SVGRenderMode mode) const
{
    return (m_clipper && m_clipper->requiresMasking()) || (mode == SVGRenderMode::Painting && (m_masker || m_opacity < 1.f));
}

NOVASVG_INLINE bool SVGRenderState::hasCycleReference(const SVGElement* element) const
{
    auto current = this;
    do {
        if(element == current->element())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

NOVASVG_INLINE void SVGRenderState::beginGroup(const SVGBlendInfo& blendInfo)
{
    auto requiresCompositing = blendInfo.requiresCompositing(m_mode);
    if(requiresCompositing) {
        auto boundingBox = m_currentTransform.mapRect(m_element->paintBoundingBox());
        boundingBox.intersect(m_canvas->extents());
        m_canvas = Canvas::create(boundingBox);
    } else {
        m_canvas->save();
    }

    if(!requiresCompositing && blendInfo.clipper()) {
        blendInfo.clipper()->applyClipPath(*this);
    }
}

NOVASVG_INLINE void SVGRenderState::endGroup(const SVGBlendInfo& blendInfo)
{
    if(m_canvas == m_parent->canvas()) {
        m_canvas->restore();
        return;
    }

    auto opacity = m_mode == SVGRenderMode::Clipping ? 1.f : blendInfo.opacity();
    if(blendInfo.clipper())
        blendInfo.clipper()->applyClipMask(*this);
    if(m_mode == SVGRenderMode::Painting && blendInfo.masker()) {
        blendInfo.masker()->applyMask(*this);
    }

    m_parent->m_canvas->blendCanvas(*m_canvas, BlendMode::Src_Over, opacity);
}

// ---- svgpaintelement (impl) ----

NOVASVG_INLINE SVGPaintElement::SVGPaintElement(Document* document, ElementID id)
    : SVGElement(document, id)
{
}

NOVASVG_INLINE SVGStopElement::SVGStopElement(Document* document)
    : SVGElement(document, ElementID::Stop)
    , m_offset(PropertyID::Offset, 0.f)
{
    addProperty(m_offset);
}

NOVASVG_INLINE void SVGStopElement::layoutElement(const SVGLayoutState& state)
{
    m_stop_color = state.stop_color();
    m_stop_opacity = state.stop_opacity();
    SVGElement::layoutElement(state);
}

NOVASVG_INLINE GradientStop SVGStopElement::gradientStop(float opacity) const
{
    Color stopColor = m_stop_color.colorWithAlpha(m_stop_opacity * opacity);
    GradientStop gradientStop = {
        m_offset.value(), { stopColor.getRF(), stopColor.getGF(), stopColor.getBF(), stopColor.getAF() }
    };

    return gradientStop;
}

NOVASVG_INLINE SVGGradientElement::SVGGradientElement(Document* document, ElementID id)
    : SVGPaintElement(document, id)
    , SVGURIReference(this)
    , m_gradientTransform(PropertyID::GradientTransform)
    , m_gradientUnits(PropertyID::GradientUnits, Units::ObjectBoundingBox)
    , m_spreadMethod(PropertyID::SpreadMethod, SpreadMethod::Pad)
{
    addProperty(m_gradientTransform);
    addProperty(m_gradientUnits);
    addProperty(m_spreadMethod);
}

NOVASVG_INLINE void SVGGradientElement::collectGradientAttributes(SVGGradientAttributes& attributes) const
{
    if(!attributes.hasGradientTransform() && hasAttribute(PropertyID::GradientTransform))
        attributes.setGradientTransform(this);
    if(!attributes.hasSpreadMethod() && hasAttribute(PropertyID::SpreadMethod))
        attributes.setSpreadMethod(this);
    if(!attributes.hasGradientUnits() && hasAttribute(PropertyID::GradientUnits))
        attributes.setGradientUnits(this);
    if(!attributes.hasGradientContentElement()) {
        for(const auto& child : children()) {
            if(auto element = toSVGElement(child); element && element->id() == ElementID::Stop) {
                attributes.setGradientContentElement(this);
                break;
            }
        }
    }
}

NOVASVG_INLINE SVGLinearGradientElement::SVGLinearGradientElement(Document* document)
    : SVGGradientElement(document, ElementID::LinearGradient)
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

NOVASVG_INLINE SVGLinearGradientAttributes SVGLinearGradientElement::collectGradientAttributes() const
{
    SVGLinearGradientAttributes attributes;
    std::set<const SVGGradientElement*> processedGradients;
    const SVGGradientElement* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::LinearGradient) {
            auto element = static_cast<const SVGLinearGradientElement*>(current);
            if(!attributes.hasX1() && element->hasAttribute(PropertyID::X1))
                attributes.setX1(element);
            if(!attributes.hasY1() && element->hasAttribute(PropertyID::Y1))
                attributes.setY1(element);
            if(!attributes.hasX2() && element->hasAttribute(PropertyID::X2))
                attributes.setX2(element);
            if(!attributes.hasY2() && element->hasAttribute(PropertyID::Y2)) {
                attributes.setY2(element);
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const SVGGradientElement*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

static GradientStops buildGradientStops(const SVGGradientElement* element, float opacity)
{
    GradientStops gradientStops;

    const auto& children = element->children();
    gradientStops.reserve(children.size());
    for(const auto& child : children) {
        auto childElement = toSVGElement(child);
        if(childElement && childElement->id() == ElementID::Stop) {
            auto stopElement = static_cast<SVGStopElement*>(childElement);
            gradientStops.push_back(stopElement->gradientStop(opacity));
        }
    }

    return gradientStops;
}

NOVASVG_INLINE bool SVGLinearGradientElement::applyPaint(SVGRenderState& state, float opacity) const
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
        state->setColor(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
        return true;
    }

    auto spreadMethod = attributes.spreadMethod();
    auto gradientUnits = attributes.gradientUnits();
    auto gradientTransform = attributes.gradientTransform();
    if(gradientUnits == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        gradientTransform.postMultiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->setLinearGradient(x1, y1, x2, y2, spreadMethod, gradientStops, gradientTransform);
    return true;
}

NOVASVG_INLINE SVGRadialGradientElement::SVGRadialGradientElement(Document* document)
    : SVGGradientElement(document, ElementID::RadialGradient)
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

NOVASVG_INLINE bool SVGRadialGradientElement::applyPaint(SVGRenderState& state, float opacity) const
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
        state->setColor(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a);
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
        gradientTransform.postMultiply(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y));
    }

    state->setRadialGradient(cx, cy, r, fx, fy, spreadMethod, gradientStops, gradientTransform);
    return true;
}

NOVASVG_INLINE SVGRadialGradientAttributes SVGRadialGradientElement::collectGradientAttributes() const
{
    SVGRadialGradientAttributes attributes;
    std::set<const SVGGradientElement*> processedGradients;
    const SVGGradientElement* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->id() == ElementID::RadialGradient) {
            auto element = static_cast<const SVGRadialGradientElement*>(current);
            if(!attributes.hasCx() && element->hasAttribute(PropertyID::Cx))
                attributes.setCx(element);
            if(!attributes.hasCy() && element->hasAttribute(PropertyID::Cy))
                attributes.setCy(element);
            if(!attributes.hasR() && element->hasAttribute(PropertyID::R))
                attributes.setR(element);
            if(!attributes.hasFx() && element->hasAttribute(PropertyID::Fx))
                attributes.setFx(element);
            if(!attributes.hasFy() && element->hasAttribute(PropertyID::Fy)) {
                attributes.setFy(element);
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || !(targetElement->id() == ElementID::LinearGradient || targetElement->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const SVGGradientElement*>(targetElement);
        if(processedGradients.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

NOVASVG_INLINE SVGPatternElement::SVGPatternElement(Document* document)
    : SVGPaintElement(document, ElementID::Pattern)
    , SVGURIReference(this)
    , SVGFitToViewBox(this)
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

NOVASVG_INLINE bool SVGPatternElement::applyPaint(SVGRenderState& state, float opacity) const
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

    auto patternImage = Canvas::create(0, 0, patternRect.w * xScale, patternRect.h * yScale);
    auto patternImageTransform = Transform::scaled(xScale, yScale);

    const auto& viewBoxRect = attributes.viewBox();
    if(viewBoxRect.isValid()) {
        const auto& preserveAspectRatio = attributes.preserveAspectRatio();
        patternImageTransform.multiply(preserveAspectRatio.getTransform(viewBoxRect, patternRect.size()));
    } else if(attributes.patternContentUnits() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        patternImageTransform.scale(bbox.w, bbox.h);
    }

    SVGRenderState newState(this, &state, patternImageTransform, SVGRenderMode::Painting, patternImage);
    patternContentElement->renderChildren(newState);

    auto patternTransform = attributes.patternTransform();
    patternTransform.translate(patternRect.x, patternRect.y);
    patternTransform.scale(patternRect.w / patternImage->width(), patternRect.h / patternImage->height());
    state->setTexture(*patternImage, TextureType::Tiled, opacity, patternTransform);
    return true;
}

NOVASVG_INLINE SVGPatternAttributes SVGPatternElement::collectPatternAttributes() const
{
    SVGPatternAttributes attributes;
    std::set<const SVGPatternElement*> processedPatterns;
    const SVGPatternElement* current = this;
    while(true) {
        if(!attributes.hasX() && current->hasAttribute(PropertyID::X))
            attributes.setX(current);
        if(!attributes.hasY() && current->hasAttribute(PropertyID::Y))
            attributes.setY(current);
        if(!attributes.hasWidth() && current->hasAttribute(PropertyID::Width))
            attributes.setWidth(current);
        if(!attributes.hasHeight() && current->hasAttribute(PropertyID::Height))
            attributes.setHeight(current);
        if(!attributes.hasPatternTransform() && current->hasAttribute(PropertyID::PatternTransform))
            attributes.setPatternTransform(current);
        if(!attributes.hasPatternUnits() && current->hasAttribute(PropertyID::PatternUnits))
            attributes.setPatternUnits(current);
        if(!attributes.hasPatternContentUnits() && current->hasAttribute(PropertyID::PatternContentUnits))
            attributes.setPatternContentUnits(current);
        if(!attributes.hasViewBox() && current->hasAttribute(PropertyID::ViewBox))
            attributes.setViewBox(current);
        if(!attributes.hasPreserveAspectRatio() && current->hasAttribute(PropertyID::PreserveAspectRatio))
            attributes.setPreserveAspectRatio(current);
        if(!attributes.hasPatternContentElement()) {
            for(const auto& child : current->children()) {
                if(child->isElement()) {
                    attributes.setPatternContentElement(current);
                    break;
                }
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || targetElement->id() != ElementID::Pattern)
            break;
        processedPatterns.insert(current);
        current = static_cast<const SVGPatternElement*>(targetElement);
        if(processedPatterns.count(current) > 0) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

// ---- svggeometryelement (impl) ----
using namespace render; // render/ layer (novasvg::render), the former plutovg

NOVASVG_INLINE Rect SVGMarkerPosition::markerBoundingBox(float strokeWidth) const
{
    return m_element->markerBoundingBox(m_origin, m_angle, strokeWidth);
}

NOVASVG_INLINE void SVGMarkerPosition::renderMarker(SVGRenderState& state, float strokeWidth) const
{
    m_element->renderMarker(state, m_origin, m_angle, strokeWidth);
}

NOVASVG_INLINE SVGGeometryElement::SVGGeometryElement(Document* document, ElementID id)
    : SVGGraphicsElement(document, id)
{
}

NOVASVG_INLINE Rect SVGGeometryElement::strokeBoundingBox() const
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

        strokeBoundingBox.inflate(std::max(capLimit, joinLimit));
    }

    for(const auto& markerPosition : m_markerPositions)
        strokeBoundingBox.unite(markerPosition.markerBoundingBox(m_strokeData.lineWidth()));
    return strokeBoundingBox;
}

NOVASVG_INLINE void SVGGeometryElement::layoutElement(const SVGLayoutState& state)
{
    m_fill_rule = state.fill_rule();
    m_clip_rule = state.clip_rule();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    m_strokeData = getStrokeData(state);
    SVGGraphicsElement::layoutElement(state);

    m_path.reset();
    m_markerPositions.clear();
    m_fillBoundingBox = updateShape(m_path);
    updateMarkerPositions(m_markerPositions, state);
}

NOVASVG_INLINE void SVGGeometryElement::updateMarkerPositions(SVGMarkerPositionList& positions, const SVGLayoutState& state)
{
    if(m_path.isEmpty())
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
                if(orient.orientType() == SVGAngle::OrientType::AutoStartReverse)
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

NOVASVG_INLINE void SVGGeometryElement::render(SVGRenderState& state) const
{
    if(!isRenderable())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == SVGRenderMode::Clipping) {
        newState->setColor(Color::White);
        newState->fillPath(m_path, m_clip_rule, newState.currentTransform());
    } else {
        if(m_fill.applyPaint(newState))
            newState->fillPath(m_path, m_fill_rule, newState.currentTransform());
        if(m_stroke.applyPaint(newState)) {
            newState->strokePath(m_path, m_strokeData, newState.currentTransform());
        }

        for(const auto& markerPosition : m_markerPositions) {
            markerPosition.renderMarker(newState, m_strokeData.lineWidth());
        }
    }

    newState.endGroup(blendInfo);
}

NOVASVG_INLINE SVGLineElement::SVGLineElement(Document* document)
    : SVGGeometryElement(document, ElementID::Line)
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

NOVASVG_INLINE Rect SVGLineElement::updateShape(Path& path)
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

NOVASVG_INLINE SVGRectElement::SVGRectElement(Document* document)
    : SVGGeometryElement(document, ElementID::Rect)
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

NOVASVG_INLINE Rect SVGRectElement::updateShape(Path& path)
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

    rx = std::min(rx, width / 2.f);
    ry = std::min(ry, height / 2.f);

    path.addRoundRect(x, y, width, height, rx, ry);
    return Rect(x, y, width, height);
}

NOVASVG_INLINE SVGEllipseElement::SVGEllipseElement(Document* document)
    : SVGGeometryElement(document, ElementID::Ellipse)
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

NOVASVG_INLINE Rect SVGEllipseElement::updateShape(Path& path)
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

NOVASVG_INLINE SVGCircleElement::SVGCircleElement(Document* document)
    : SVGGeometryElement(document, ElementID::Circle)
    , m_cx(PropertyID::Cx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_cy(PropertyID::Cy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_r(PropertyID::R, LengthDirection::Diagonal, LengthNegativeMode::Forbid)
{
    addProperty(m_cx);
    addProperty(m_cy);
    addProperty(m_r);
}

NOVASVG_INLINE Rect SVGCircleElement::updateShape(Path& path)
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

NOVASVG_INLINE SVGPolyElement::SVGPolyElement(Document* document, ElementID id)
    : SVGGeometryElement(document, id)
    , m_points(PropertyID::Points)
{
    addProperty(m_points);
}

NOVASVG_INLINE Rect SVGPolyElement::updateShape(Path& path)
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

NOVASVG_INLINE SVGPathElement::SVGPathElement(Document* document)
    : SVGGeometryElement(document, ElementID::Path)
    , m_d(PropertyID::D)
{
    addProperty(m_d);
}

NOVASVG_INLINE Rect SVGPathElement::updateShape(Path& path)
{
    path = m_d.value();
    return path.boundingRect();
}

// ---- svgtextelement (impl) ----
using namespace render; // render/ layer (novasvg::render), the former plutovg

inline const SVGTextNode* toSVGTextNode(const SVGNode* node)
{
    assert(node && node->isTextNode());
    return static_cast<const SVGTextNode*>(node);
}

inline const SVGTextPositioningElement* toSVGTextPositioningElement(const SVGNode* node)
{
    assert(node && node->isTextPositioningElement());
    return static_cast<const SVGTextPositioningElement*>(node);
}

static AlignmentBaseline resolveDominantBaseline(const SVGTextPositioningElement* element)
{
    switch(element->dominant_baseline()) {
    case DominantBaseline::Auto:
        if(element->isVerticalWritingMode())
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

static float calculateBaselineOffset(const SVGTextPositioningElement* element)
{
    auto offset = element->baseline_offset();
    auto parent = element->parentElement();
    while(parent->isTextPositioningElement()) {
        offset += toSVGTextPositioningElement(parent)->baseline_offset();
        parent = parent->parentElement();
    }

    auto baseline = element->alignment_baseline();
    if(baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
        baseline = resolveDominantBaseline(element);
    }

    const auto& font = element->font();
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

static bool needsTextAnchorAdjustment(const SVGTextPositioningElement* element)
{
    auto direction = element->direction();
    switch(element->text_anchor()) {
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

static float calculateTextAnchorOffset(const SVGTextPositioningElement* element, float width)
{
    auto direction = element->direction();
    switch(element->text_anchor()) {
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

using SVGTextFragmentIterator = SVGTextFragmentList::iterator;

static float calculateTextChunkLength(SVGTextFragmentIterator begin, SVGTextFragmentIterator end, bool isVerticalText)
{
    float chunkLength = 0;
    const SVGTextFragment* lastFragment = nullptr;
    for(auto it = begin; it != end; ++it) {
        const SVGTextFragment& fragment = *it;
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

static void handleTextChunk(SVGTextFragmentIterator begin, SVGTextFragmentIterator end)
{
    const SVGTextFragment& firstFragment = *begin;
    const auto isVerticalText = firstFragment.element->isVerticalWritingMode();
    if(firstFragment.element->hasAttribute(PropertyID::TextLength)) {
        LengthContext lengthContext(firstFragment.element);
        auto textLength = lengthContext.valueForLength(firstFragment.element->textLength());
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        if(textLength > 0.f && chunkLength > 0.f) {
            size_t numCharacters = 0;
            for(auto it = begin; it != end; ++it) {
                const SVGTextFragment& fragment = *it;
                numCharacters += fragment.length;
            }

            if(firstFragment.element->lengthAdjust() == LengthAdjust::SpacingAndGlyphs) {
                auto textLengthScale = textLength / chunkLength;
                auto lengthAdjustTransform = Transform::translated(firstFragment.x, firstFragment.y);
                if(isVerticalText) {
                    lengthAdjustTransform.scale(1.f, textLengthScale);
                } else {
                    lengthAdjustTransform.scale(textLengthScale, 1.f);
                }

                lengthAdjustTransform.translate(-firstFragment.x, -firstFragment.y);
                for(auto it = begin; it != end; ++it) {
                    SVGTextFragment& fragment = *it;
                    fragment.lengthAdjustTransform = lengthAdjustTransform;
                }
            } else if(numCharacters > 1) {
                assert(firstFragment.element->lengthAdjust() == LengthAdjust::Spacing);
                size_t characterOffset = 0;
                auto textLengthShift = (textLength - chunkLength) / (numCharacters - 1);
                for(auto it = begin; it != end; ++it) {
                    SVGTextFragment& fragment = *it;
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

    if(needsTextAnchorAdjustment(firstFragment.element)) {
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        auto chunkOffset = calculateTextAnchorOffset(firstFragment.element, chunkLength);
        for(auto it = begin; it != end; ++it) {
            SVGTextFragment& fragment = *it;
            if(isVerticalText) {
                fragment.y += chunkOffset;
            } else {
                fragment.x += chunkOffset;
            }
        }
    }
}

NOVASVG_INLINE SVGTextFragmentsBuilder::SVGTextFragmentsBuilder(std::u32string& text, SVGTextFragmentList& fragments)
    : m_text(text), m_fragments(fragments)
{
    m_text.clear();
    m_fragments.clear();
}

NOVASVG_INLINE void SVGTextFragmentsBuilder::build(const SVGTextElement* textElement)
{
    handleElement(textElement);
    for(const auto& position : m_textPositions) {
        fillCharacterPositions(position);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& textPosition : m_textPositions) {
        if(!textPosition.node->isTextNode())
            continue;
        auto element = toSVGTextPositioningElement(textPosition.node->parentElement());
        const auto isVerticalText = element->isVerticalWritingMode();
        const auto isUprightText = element->isUprightTextOrientation();
        const auto& font = element->font();

        SVGTextFragment fragment(element);
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

        auto needsTextLengthSpacing = element->lengthAdjust() == LengthAdjust::Spacing && element->hasAttribute(PropertyID::TextLength);
        auto baselineOffset = calculateBaselineOffset(element);
        auto startOffset = textPosition.startOffset;
        auto textOffset = textPosition.startOffset;
        auto didStartTextFragment = false;
        auto applySpacingToNextCharacter = false;
        auto lastCharacter = 0u;
        auto lastAngle = 0.f;
        while(textOffset < textPosition.endOffset) {
            SVGCharacterPosition characterPosition;
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

            auto spacing = element->letter_spacing();
            if(currentCharacter && lastCharacter && element->word_spacing()) {
                if(currentCharacter == ' ' && lastCharacter != ' ') {
                    spacing += element->word_spacing();
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
        const SVGTextFragment& fragment = *it;
        if(!fragment.startsNewTextChunk)
            continue;
        handleTextChunk(begin, it);
        begin = it;
    }

    handleTextChunk(begin, it);
}

NOVASVG_INLINE void SVGTextFragmentsBuilder::handleText(const SVGTextNode* node)
{
    const auto& text = node->data();
    if(text.empty())
        return;
    auto element = toSVGTextPositioningElement(node->parentElement());
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
        if(currentCharacter == ' ' && lastCharacter == ' ' && element->white_space() == WhiteSpace::Default)
            continue;
        m_text.push_back(currentCharacter);
        lastCharacter = currentCharacter;
    }

    if(startOffset < m_text.length()) {
        m_textPositions.emplace_back(node, startOffset, m_text.length());
    }
}

NOVASVG_INLINE void SVGTextFragmentsBuilder::handleElement(const SVGTextPositioningElement* element)
{
    if(element->isDisplayNone())
        return;
    const auto itemIndex = m_textPositions.size();
    m_textPositions.emplace_back(element, m_text.length(), m_text.length());
    for(const auto& child : element->children()) {
        if(child->isTextNode()) {
            handleText(toSVGTextNode(child.get()));
        } else if(child->isTextPositioningElement()) {
            handleElement(toSVGTextPositioningElement(child.get()));
        }
    }

    auto& position = m_textPositions[itemIndex];
    assert(position.node == element);
    position.endOffset = m_text.length();
}

NOVASVG_INLINE void SVGTextFragmentsBuilder::fillCharacterPositions(const SVGTextPosition& position)
{
    if(!position.node->isTextPositioningElement())
        return;
    auto element = toSVGTextPositioningElement(position.node);
    const auto& xList = element->x();
    const auto& yList = element->y();
    const auto& dxList = element->dx();
    const auto& dyList = element->dy();
    const auto& rotateList = element->rotate();

    auto xListSize = xList.size();
    auto yListSize = yList.size();
    auto dxListSize = dxList.size();
    auto dyListSize = dyList.size();
    auto rotateListSize = rotateList.size();
    if(!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
        return;
    }

    LengthContext lengthContext(element);
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

NOVASVG_INLINE SVGTextPositioningElement::SVGTextPositioningElement(Document* document, ElementID id)
    : SVGGraphicsElement(document, id)
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

NOVASVG_INLINE void SVGTextPositioningElement::layoutElement(const SVGLayoutState& state)
{
    m_font = state.font();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    SVGGraphicsElement::layoutElement(state);

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

NOVASVG_INLINE float SVGTextPositioningElement::convertBaselineOffset(const BaselineShift& baselineShift) const
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

NOVASVG_INLINE SVGTSpanElement::SVGTSpanElement(Document* document)
    : SVGTextPositioningElement(document, ElementID::Tspan)
{
}

NOVASVG_INLINE SVGTextElement::SVGTextElement(Document* document)
    : SVGTextPositioningElement(document, ElementID::Text)
{
}

NOVASVG_INLINE void SVGTextElement::layout(SVGLayoutState& state)
{
    SVGTextPositioningElement::layout(state);
    SVGTextFragmentsBuilder(m_text, m_fragments).build(this);
}

NOVASVG_INLINE void SVGTextElement::render(SVGRenderState& state) const
{
    if(m_fragments.empty() || isVisibilityHidden() || isDisplayNone())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == SVGRenderMode::Clipping) {
        newState->setColor(Color::White);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        if(fragment.element->isVisibilityHidden())
            continue;
        auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto text = wholeText.substr(fragment.offset, fragment.length);
        auto origin = Point(fragment.x, fragment.y);

        const auto& font = fragment.element->font();
        if(newState.mode() == SVGRenderMode::Clipping) {
            newState->fillText(text, font, origin, transform);
        } else {
            const auto& fill = fragment.element->fill();
            const auto& stroke = fragment.element->stroke();
            auto stroke_width = fragment.element->stroke_width();
            if(fill.applyPaint(newState))
                newState->fillText(text, font, origin, transform);
            if(stroke.applyPaint(newState)) {
                newState->strokeText(text, stroke_width, font, origin, transform);
            }
        }
    }

    newState.endGroup(blendInfo);
}

NOVASVG_INLINE Rect SVGTextElement::boundingBox(bool includeStroke) const
{
    auto boundingBox = Rect::Invalid;
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.element->font();
        const auto& stroke = fragment.element->stroke();
        auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto fragmentRect = Rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.height);
        if(includeStroke && stroke.isRenderable())
            fragmentRect.inflate(fragment.element->stroke_width() / 2.f);
        boundingBox.unite(fragmentTranform.mapRect(fragmentRect));
    }

    if(!boundingBox.isValid())
        boundingBox = Rect::Empty;
    return boundingBox;
}

// ---- svglayoutstate (impl) ----
using namespace render; // render/ layer (novasvg::render), the former plutovg

static std::optional<Color> parseColorValue(std::string_view& input, const SVGLayoutState* state)
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

static Color parseColor(std::string_view input, const SVGLayoutState* state, const Color& defaultValue)
{
    auto color = parseColorValue(input, state);
    if(!color || !input.empty())
        color = defaultValue;
    return color.value();
}

static Color parseColorOrNone(std::string_view input, const SVGLayoutState* state, const Color& defaultValue)
{
    if(input.compare("none") == 0)
        return Color::Transparent;
    return parseColor(input, state, defaultValue);
}

static bool parseUrlValue(std::string_view& input, std::string& value)
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

static std::string parseUrl(std::string_view input)
{
    std::string value;
    if(!parseUrlValue(input, value) || !input.empty())
        value.clear();
    return value;
}

static Paint parsePaint(std::string_view input, const SVGLayoutState* state, const Color& defaultValue)
{
    std::string id;
    if(!parseUrlValue(input, id))
        return Paint(parseColorOrNone(input, state, defaultValue));
    if(skipOptionalSpaces(input))
        return Paint(id, parseColorOrNone(input, state, defaultValue));
    return Paint(id, Color::Transparent);
}

static float parseNumberOrPercentage(std::string_view input, bool allowPercentage, float defaultValue)
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

static Length parseLength(std::string_view input, LengthNegativeMode mode, const Length& defaultValue)
{
    Length value;
    if(!value.parse(input, mode))
        value = defaultValue;
    return value;
}

static BaselineShift parseBaselineShift(std::string_view input)
{
    if(input.compare("baseline") == 0)
        return BaselineShift::Type::Baseline;
    if(input.compare("sub") == 0)
        return BaselineShift::Type::Sub;
    if(input.compare("super") == 0)
        return BaselineShift::Type::Super;
    return parseLength(input, LengthNegativeMode::Allow, Length(0.f, LengthUnits::None));
}

static LengthList parseDashArray(std::string_view input)
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

static Length parseLengthOrNormal(std::string_view input)
{
    if(input.compare("normal") == 0)
        return Length(0, LengthUnits::None);
    return parseLength(input, LengthNegativeMode::Allow, Length(0, LengthUnits::None));
}

static float parseFontSize(std::string_view input, const SVGLayoutState* state)
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
static Enum parseEnumValue(std::string_view input, const SVGEnumerationEntry<Enum>(&entries)[N], Enum defaultValue)
{
    for(const auto& entry : entries) {
        if(input == entry.second) {
            return entry.first;
        }
    }

    return defaultValue;
}

static Display parseDisplay(std::string_view input)
{
    static const SVGEnumerationEntry<Display> entries[] = {
        {Display::Inline, "inline"},
        {Display::None, "none"}
    };

    return parseEnumValue(input, entries, Display::Inline);
}

static Visibility parseVisibility(std::string_view input)
{
    static const SVGEnumerationEntry<Visibility> entries[] = {
        {Visibility::Visible, "visible"},
        {Visibility::Hidden, "hidden"},
        {Visibility::Collapse, "collapse"}
    };

    return parseEnumValue(input, entries, Visibility::Visible);
}

static Overflow parseOverflow(std::string_view input)
{
    static const SVGEnumerationEntry<Overflow> entries[] = {
        {Overflow::Visible, "visible"},
        {Overflow::Hidden, "hidden"}
    };

    return parseEnumValue(input, entries, Overflow::Visible);
}

static PointerEvents parsePointerEvents(std::string_view input)
{
    static const SVGEnumerationEntry<PointerEvents> entries[] = {
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

static FontWeight parseFontWeight(std::string_view input)
{
    static const SVGEnumerationEntry<FontWeight> entries[] = {
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

static FontStyle parseFontStyle(std::string_view input)
{
    static const SVGEnumerationEntry<FontStyle> entries[] = {
        {FontStyle::Normal, "normal"},
        {FontStyle::Italic, "italic"},
        {FontStyle::Italic, "oblique"}
    };

    return parseEnumValue(input, entries, FontStyle::Normal);
}

static AlignmentBaseline parseAlignmentBaseline(std::string_view input)
{
    static const SVGEnumerationEntry<AlignmentBaseline> entries[] = {
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

static DominantBaseline parseDominantBaseline(std::string_view input)
{
    static const SVGEnumerationEntry<DominantBaseline> entries[] = {
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

static Direction parseDirection(std::string_view input)
{
    static const SVGEnumerationEntry<Direction> entries[] = {
        {Direction::Ltr, "ltr"},
        {Direction::Rtl, "rtl"}
    };

    return parseEnumValue(input, entries, Direction::Ltr);
}

static WritingMode parseWritingMode(std::string_view input)
{
    static const SVGEnumerationEntry<WritingMode> entries[] = {
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

static TextOrientation parseTextOrientation(std::string_view input)
{
    static const SVGEnumerationEntry<TextOrientation> entries[] = {
        {TextOrientation::Mixed, "mixed"},
        {TextOrientation::Upright, "upright"}
    };

    return parseEnumValue(input, entries, TextOrientation::Mixed);
}

static TextAnchor parseTextAnchor(std::string_view input)
{
    static const SVGEnumerationEntry<TextAnchor> entries[] = {
        {TextAnchor::Start, "start"},
        {TextAnchor::Middle, "middle"},
        {TextAnchor::End, "end"}
    };

    return parseEnumValue(input, entries, TextAnchor::Start);
}

static WhiteSpace parseWhiteSpace(std::string_view input)
{
    static const SVGEnumerationEntry<WhiteSpace> entries[] = {
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

static MaskType parseMaskType(std::string_view input)
{
    static const SVGEnumerationEntry<MaskType> entries[] = {
        {MaskType::Luminance, "luminance"},
        {MaskType::Alpha, "alpha"}
    };

    return parseEnumValue(input, entries, MaskType::Luminance);
}

static FillRule parseFillRule(std::string_view input)
{
    static const SVGEnumerationEntry<FillRule> entries[] = {
        {FillRule::NonZero, "nonzero"},
        {FillRule::EvenOdd, "evenodd"}
    };

    return parseEnumValue(input, entries, FillRule::NonZero);
}

static LineCap parseLineCap(std::string_view input)
{
    static const SVGEnumerationEntry<LineCap> entries[] = {
        {LineCap::Butt, "butt"},
        {LineCap::Round, "round"},
        {LineCap::Square, "square"}
    };

    return parseEnumValue(input, entries, LineCap::Butt);
}

static LineJoin parseLineJoin(std::string_view input)
{
    static const SVGEnumerationEntry<LineJoin> entries[] = {
        {LineJoin::Miter, "miter"},
        {LineJoin::Round, "round"},
        {LineJoin::Bevel, "bevel"}
    };

    return parseEnumValue(input, entries, LineJoin::Miter);
}

NOVASVG_INLINE SVGLayoutState::SVGLayoutState(const SVGLayoutState& parent, const SVGElement* element)
    : m_parent(&parent)
    , m_element(element)
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
    , m_overflow(element->isRootElement() ? Overflow::Visible : Overflow::Hidden)
    , m_pointer_events(parent.pointer_events())
    , m_marker_start(parent.marker_start())
    , m_marker_mid(parent.marker_mid())
    , m_marker_end(parent.marker_end())
    , m_font_family(parent.font_family())
{
    for(const auto& attribute : element->attributes()) {
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

NOVASVG_INLINE Font SVGLayoutState::font() const
{
    auto bold = m_font_weight == FontWeight::Bold;
    auto italic = m_font_style == FontStyle::Italic;

    FontFace face;
    std::string_view input(m_font_family);
    while(!input.empty() && face.isNull()) {
        auto family = input.substr(0, input.find(','));
        input.remove_prefix(family.length());
        if(!input.empty() && input.front() == ',')
            input.remove_prefix(1);
        stripLeadingAndTrailingSpaces(family);
        if(!family.empty() && (family.front() == '\'' || family.front() == '"')) {
            auto quote = family.front();
            family.remove_prefix(1);
            if(!family.empty() && family.back() == quote)
                family.remove_suffix(1);
            stripLeadingAndTrailingSpaces(family);
        }

        std::string font_family(family);
        if(!font_family.empty()) {
            face = fontFaceCache()->getFontFace(font_family, bold, italic);
        }
    }

    if(face.isNull())
        face = fontFaceCache()->getFontFace(emptyString, bold, italic);
    return Font(face, m_font_size);
}

// ---- svgelement (impl) ----
using namespace render; // render/ layer (novasvg::render), the former plutovg

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

NOVASVG_INLINE SVGTextNode::SVGTextNode(Document* document)
    : SVGNode(document)
{
}

NOVASVG_INLINE void SVGTextNode::setData(const std::string& data)
{
    rootElement()->setNeedsLayout();
    m_data.assign(data);
}

NOVASVG_INLINE std::unique_ptr<SVGNode> SVGTextNode::clone(bool deep) const
{
    auto node = std::make_unique<SVGTextNode>(document());
    node->setData(m_data);
    return node;
}

NOVASVG_INLINE const std::string emptyString;

NOVASVG_INLINE std::unique_ptr<SVGElement> SVGElement::create(Document* document, ElementID id)
{
    switch(id) {
    case ElementID::Svg:
        return std::make_unique<SVGSVGElement>(document);
    case ElementID::Path:
        return std::make_unique<SVGPathElement>(document);
    case ElementID::G:
        return std::make_unique<SVGGElement>(document);
    case ElementID::Rect:
        return std::make_unique<SVGRectElement>(document);
    case ElementID::Circle:
        return std::make_unique<SVGCircleElement>(document);
    case ElementID::Ellipse:
        return std::make_unique<SVGEllipseElement>(document);
    case ElementID::Line:
        return std::make_unique<SVGLineElement>(document);
    case ElementID::Defs:
        return std::make_unique<SVGDefsElement>(document);
    case ElementID::Polygon:
    case ElementID::Polyline:
        return std::make_unique<SVGPolyElement>(document, id);
    case ElementID::Stop:
        return std::make_unique<SVGStopElement>(document);
    case ElementID::LinearGradient:
        return std::make_unique<SVGLinearGradientElement>(document);
    case ElementID::RadialGradient:
        return std::make_unique<SVGRadialGradientElement>(document);
    case ElementID::Symbol:
        return std::make_unique<SVGSymbolElement>(document);
    case ElementID::Use:
        return std::make_unique<SVGUseElement>(document);
    case ElementID::Pattern:
        return std::make_unique<SVGPatternElement>(document);
    case ElementID::Mask:
        return std::make_unique<SVGMaskElement>(document);
    case ElementID::ClipPath:
        return std::make_unique<SVGClipPathElement>(document);
    case ElementID::Marker:
        return std::make_unique<SVGMarkerElement>(document);
    case ElementID::Image:
        return std::make_unique<SVGImageElement>(document);
    case ElementID::ForeignObject:
        return std::make_unique<SVGForeignObjectElement>(document);
    case ElementID::Style:
        return std::make_unique<SVGStyleElement>(document);
    case ElementID::Text:
        return std::make_unique<SVGTextElement>(document);
    case ElementID::Tspan:
        return std::make_unique<SVGTSpanElement>(document);
    default:
        assert(false);
    }

    return nullptr;
}

NOVASVG_INLINE SVGElement::SVGElement(Document* document, ElementID id)
    : SVGNode(document)
    , m_id(id)
{
}

NOVASVG_INLINE bool SVGElement::hasAttribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return hasAttribute(id);
}

NOVASVG_INLINE const std::string& SVGElement::getAttribute(std::string_view name) const
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return emptyString;
    return getAttribute(id);
}

NOVASVG_INLINE bool SVGElement::setAttribute(std::string_view name, const std::string& value)
{
    auto id = propertyid(name);
    if(id == PropertyID::Unknown)
        return false;
    return setAttribute(0x1000, id, value);
}

NOVASVG_INLINE const Attribute* SVGElement::findAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return &attribute;
        }
    }

    return nullptr;
}

NOVASVG_INLINE bool SVGElement::hasAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return true;
        }
    }

    return false;
}

NOVASVG_INLINE const std::string& SVGElement::getAttribute(PropertyID id) const
{
    for(const auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            return attribute.value();
        }
    }

    return emptyString;
}

NOVASVG_INLINE bool SVGElement::setAttribute(int specificity, PropertyID id, const std::string& value)
{
    for(auto& attribute : m_attributes) {
        if(id == attribute.id()) {
            if(specificity < attribute.specificity())
                return false;
            parseAttribute(id, value);
            attribute = Attribute(specificity, id, value);
            return true;
        }
    }

    parseAttribute(id, value);
    m_attributes.emplace_front(specificity, id, value);
    return true;
}

NOVASVG_INLINE void SVGElement::setAttributes(const AttributeList& attributes)
{
    for(const auto& attribute : attributes) {
        setAttribute(attribute);
    }
}

NOVASVG_INLINE bool SVGElement::setAttribute(const Attribute& attribute)
{
    return setAttribute(attribute.specificity(), attribute.id(), attribute.value());
}

NOVASVG_INLINE void SVGElement::parseAttribute(PropertyID id, const std::string& value)
{
    rootElement()->setNeedsLayout();
    if(auto property = getProperty(id)) {
        property->parse(value);
    }
}

NOVASVG_INLINE SVGElement* SVGElement::previousElement() const
{
    auto parent = parentElement();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.begin();
    auto end = children.end();
    SVGElement* element = nullptr;
    for(; it != end; ++it) {
        SVGNode* node = &**it;
        if(node->isTextNode())
            continue;
        if(node == this)
            return element;
        element = static_cast<SVGElement*>(node);
    }

    return nullptr;
}

NOVASVG_INLINE SVGElement* SVGElement::nextElement() const
{
    auto parent = parentElement();
    if(parent == nullptr)
        return nullptr;
    const auto& children = parent->children();
    auto it = children.rbegin();
    auto end = children.rend();
    SVGElement* element = nullptr;
    for(; it != end; ++it) {
        SVGNode* node = &**it;
        if(node->isTextNode())
            continue;
        if(node == this)
            return element;
        element = static_cast<SVGElement*>(node);
    }

    return nullptr;
}

NOVASVG_INLINE SVGNode* SVGElement::addChild(std::unique_ptr<SVGNode> child)
{
    child->setParentElement(this);
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

NOVASVG_INLINE SVGNode* SVGElement::firstChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.front();
}

NOVASVG_INLINE SVGNode* SVGElement::lastChild() const
{
    if(m_children.empty())
        return nullptr;
    return &*m_children.back();
}

NOVASVG_INLINE Rect SVGElement::fillBoundingBox() const
{
    auto fillBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child); element && !element->isHiddenElement()) {
            fillBoundingBox.unite(element->localTransform().mapRect(element->fillBoundingBox()));
        }
    }

    if(!fillBoundingBox.isValid())
        fillBoundingBox = Rect::Empty;
    return fillBoundingBox;
}

NOVASVG_INLINE Rect SVGElement::strokeBoundingBox() const
{
    auto strokeBoundingBox = Rect::Invalid;
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child); element && !element->isHiddenElement()) {
            strokeBoundingBox.unite(element->localTransform().mapRect(element->strokeBoundingBox()));
        }
    }

    if(!strokeBoundingBox.isValid())
        strokeBoundingBox = Rect::Empty;
    return strokeBoundingBox;
}

NOVASVG_INLINE Rect SVGElement::paintBoundingBox() const
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

NOVASVG_INLINE SVGMarkerElement* SVGElement::getMarker(std::string_view id) const
{
    auto element = rootElement()->getElementById(id);
    if(element && element->id() == ElementID::Marker)
        return static_cast<SVGMarkerElement*>(element);
    return nullptr;
}

NOVASVG_INLINE SVGClipPathElement* SVGElement::getClipper(std::string_view id) const
{
    auto element = rootElement()->getElementById(id);
    if(element && element->id() == ElementID::ClipPath)
        return static_cast<SVGClipPathElement*>(element);
    return nullptr;
}

NOVASVG_INLINE SVGMaskElement* SVGElement::getMasker(std::string_view id) const
{
    auto element = rootElement()->getElementById(id);
    if(element && element->id() == ElementID::Mask)
        return static_cast<SVGMaskElement*>(element);
    return nullptr;
}

NOVASVG_INLINE SVGPaintElement* SVGElement::getPainter(std::string_view id) const
{
    auto element = rootElement()->getElementById(id);
    if(element && element->isPaintElement())
        return static_cast<SVGPaintElement*>(element);
    return nullptr;
}

NOVASVG_INLINE SVGElement* SVGElement::elementFromPoint(float x, float y)
{
    auto it = m_children.rbegin();
    auto end = m_children.rend();
    for(; it != end; ++it) {
        auto child = toSVGElement(*it);
        if(child && !child->isHiddenElement()) {
            if(auto element = child->elementFromPoint(x, y)) {
                return element;
            }
        }
    }

    if(isPointableElement()) {
        auto transform = localTransform();
        for(auto parent = parentElement(); parent; parent = parent->parentElement())
            transform.postMultiply(parent->localTransform());
        auto bbox = transform.mapRect(paintBoundingBox());
        if(bbox.contains(x, y)) {
            return this;
        }
    }

    return nullptr;
}

NOVASVG_INLINE void SVGElement::addProperty(SVGProperty& value)
{
    m_properties.push_front(&value);
}

NOVASVG_INLINE SVGProperty* SVGElement::getProperty(PropertyID id) const
{
    for(auto property : m_properties) {
        if(id == property->id()) {
            return property;
        }
    }

    return nullptr;
}

NOVASVG_INLINE Size SVGElement::currentViewportSize() const
{
    auto parent = parentElement();
    if(parent == nullptr) {
        auto element = static_cast<const SVGSVGElement*>(this);
        const auto& viewBox = element->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        return Size(300, 150);
    }

    if(parent->id() == ElementID::Svg) {
        auto element = static_cast<const SVGSVGElement*>(parent);
        const auto& viewBox = element->viewBox();
        if(viewBox.value().isValid())
            return viewBox.value().size();
        LengthContext lengthContext(element);
        auto width = lengthContext.valueForLength(element->width());
        auto height = lengthContext.valueForLength(element->height());
        return Size(width, height);
    }

    return parent->currentViewportSize();
}

NOVASVG_INLINE void SVGElement::cloneChildren(SVGElement* parentElement) const
{
    for(const auto& child : m_children) {
        parentElement->addChild(child->clone(true));
    }
}

NOVASVG_INLINE std::unique_ptr<SVGNode> SVGElement::clone(bool deep) const
{
    auto element = SVGElement::create(document(), m_id);
    element->setAttributes(m_attributes);
    if(deep) { cloneChildren(element.get()); }
    return element;
}

NOVASVG_INLINE void SVGElement::build()
{
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child)) {
            element->build();
        }
    }
}

NOVASVG_INLINE void SVGElement::layoutElement(const SVGLayoutState& state)
{
    m_paintBoundingBox = Rect::Invalid;
    m_clipper = getClipper(state.clip_path());
    m_masker = getMasker(state.mask());
    m_opacity = state.opacity();

    m_font_size = state.font_size();
    m_display = state.display();
    m_overflow = state.overflow();
    m_visibility = state.visibility();
    m_pointer_events = state.pointer_events();
}

NOVASVG_INLINE void SVGElement::layoutChildren(SVGLayoutState& state)
{
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child)) {
            element->layout(state);
        }
    }
}

NOVASVG_INLINE void SVGElement::layout(SVGLayoutState& state)
{
    SVGLayoutState newState(state, this);
    layoutElement(newState);
    layoutChildren(newState);
}

NOVASVG_INLINE void SVGElement::renderChildren(SVGRenderState& state) const
{
    for(const auto& child : m_children) {
        if(auto element = toSVGElement(child)) {
            element->render(state);
        }
    }
}

NOVASVG_INLINE void SVGElement::render(SVGRenderState& state) const
{
}

NOVASVG_INLINE bool SVGElement::isHiddenElement() const
{
    if(isDisplayNone())
        return true;
    switch(m_id) {
    case ElementID::Defs:
    case ElementID::Symbol:
    case ElementID::Marker:
    case ElementID::ClipPath:
    case ElementID::Mask:
    case ElementID::LinearGradient:
    case ElementID::RadialGradient:
    case ElementID::Pattern:
    case ElementID::Stop:
        return true;
    default:
        return false;
    }
}

NOVASVG_INLINE bool SVGElement::isPointableElement() const
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

NOVASVG_INLINE SVGStyleElement::SVGStyleElement(Document* document)
    : SVGElement(document, ElementID::Style)
{
}

NOVASVG_INLINE SVGFitToViewBox::SVGFitToViewBox(SVGElement* element)
    : m_viewBox(PropertyID::ViewBox)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    element->addProperty(m_viewBox);
    element->addProperty(m_preserveAspectRatio);
}

NOVASVG_INLINE Transform SVGFitToViewBox::viewBoxToViewTransform(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.isEmpty() || viewportSize.isEmpty())
        return Transform::Identity;
    return m_preserveAspectRatio.getTransform(viewBoxRect, viewportSize);
}

NOVASVG_INLINE Rect SVGFitToViewBox::getClipRect(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.isEmpty() || viewportSize.isEmpty())
        return Rect(0, 0, viewportSize.w, viewportSize.h);
    return m_preserveAspectRatio.getClipRect(viewBoxRect, viewportSize);
}

NOVASVG_INLINE SVGURIReference::SVGURIReference(SVGElement* element)
    : m_href(PropertyID::Href)
{
    element->addProperty(m_href);
}

NOVASVG_INLINE SVGElement* SVGURIReference::getTargetElement(const Document* document) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return document->rootElement()->getElementById(value.substr(1));
}

NOVASVG_INLINE bool SVGPaintServer::applyPaint(SVGRenderState& state) const
{
    if(!isRenderable())
        return false;
    if(m_element) return m_element->applyPaint(state, m_opacity);
    state->setColor(m_color.colorWithAlpha(m_opacity));
    return true;
}

NOVASVG_INLINE SVGGraphicsElement::SVGGraphicsElement(Document* document, ElementID id)
    : SVGElement(document, id)
    , m_transform(PropertyID::Transform)
{
    addProperty(m_transform);
}

NOVASVG_INLINE SVGPaintServer SVGGraphicsElement::getPaintServer(const Paint& paint, float opacity) const
{
    if(paint.isNone())
        return SVGPaintServer();
    if(auto element = getPainter(paint.id()))
        return SVGPaintServer(element, paint.color(), opacity);
    return SVGPaintServer(nullptr, paint.color(), opacity);
}

NOVASVG_INLINE StrokeData SVGGraphicsElement::getStrokeData(const SVGLayoutState& state) const
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

NOVASVG_INLINE SVGSVGElement::SVGSVGElement(Document* document)
    : SVGGraphicsElement(document, ElementID::Svg)
    , SVGFitToViewBox(this)
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

NOVASVG_INLINE Transform SVGSVGElement::localTransform() const
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
    return SVGGraphicsElement::localTransform() * Transform::translated(viewportRect.x, viewportRect.y) * viewBoxToViewTransform(viewportRect.size());
}

NOVASVG_INLINE void SVGSVGElement::render(SVGRenderState& state) const
{
    if(isDisplayNone())
        return;
    LengthContext lengthContext(this);
    const Size viewportSize = {
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(viewportSize.isEmpty())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clipRect(getClipRect(viewportSize), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE SVGRootElement::SVGRootElement(Document* document)
    : SVGSVGElement(document)
{
}

NOVASVG_INLINE SVGRootElement* SVGRootElement::layoutIfNeeded()
{
    if(needsLayout())
        forceLayout();
    return this;
}

NOVASVG_INLINE SVGElement* SVGRootElement::getElementById(std::string_view id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

NOVASVG_INLINE void SVGRootElement::addElementById(const std::string& id, SVGElement* element)
{
    m_idCache.emplace(id, element);
}

NOVASVG_INLINE void SVGRootElement::layout(SVGLayoutState& state)
{
    SVGSVGElement::layout(state);

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
    if(!viewBoxRect.isEmpty() && (!m_intrinsicWidth || !m_intrinsicHeight)) {
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
        auto boundingBox = paintBoundingBox();
        if(!m_intrinsicWidth)
            m_intrinsicWidth = boundingBox.right();
        if(!m_intrinsicHeight) {
            m_intrinsicHeight = boundingBox.bottom();
        }
    }
}

NOVASVG_INLINE void SVGRootElement::forceLayout()
{
    SVGLayoutState state;
    layout(state);
}

NOVASVG_INLINE SVGUseElement::SVGUseElement(Document* document)
    : SVGGraphicsElement(document, ElementID::Use)
    , SVGURIReference(this)
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

NOVASVG_INLINE Transform SVGUseElement::localTransform() const
{
    LengthContext lengthContext(this);
    const Point translation = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y)
    };

    return SVGGraphicsElement::localTransform() * Transform::translated(translation.x, translation.y);
}

NOVASVG_INLINE void SVGUseElement::render(SVGRenderState& state) const
{
    if(isDisplayNone())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE void SVGUseElement::build()
{
    if(auto targetElement = getTargetElement(document())) {
        if(auto newElement = cloneTargetElement(targetElement)) {
            addChild(std::move(newElement));
        }
    }

    SVGGraphicsElement::build();
}

inline bool isDisallowedElement(const SVGElement* element)
{
    switch(element->id()) {
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
    case ElementID::Symbol:
    case ElementID::Text:
    case ElementID::Tspan:
    case ElementID::Use:
        return false;
    default:
        return true;
    }
}

NOVASVG_INLINE std::unique_ptr<SVGElement> SVGUseElement::cloneTargetElement(SVGElement* targetElement)
{
    if(targetElement == this || isDisallowedElement(targetElement))
        return nullptr;
    const auto& idAttr = targetElement->getAttribute(PropertyID::Id);
    auto parent = parentElement();
    while(parent) {
        auto attribute = parent->findAttribute(PropertyID::Id);
        if(attribute && idAttr == attribute->value())
            return nullptr;
        parent = parent->parentElement();
    }

    auto tagId = targetElement->id();
    if(tagId == ElementID::Symbol) {
        tagId = ElementID::Svg;
    }

    auto newElement = SVGElement::create(document(), tagId);
    newElement->setAttributes(targetElement->attributes());
    if(newElement->id() == ElementID::Svg) {
        for(const auto& attribute : attributes()) {
            if(attribute.id() == PropertyID::Width || attribute.id() == PropertyID::Height) {
                newElement->setAttribute(attribute);
            }
        }
    }

    if(newElement->id() != ElementID::Use)
        targetElement->cloneChildren(newElement.get());
    return newElement;
}

NOVASVG_INLINE SVGImageElement::SVGImageElement(Document* document)
    : SVGGraphicsElement(document, ElementID::Image)
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

NOVASVG_INLINE Rect SVGImageElement::fillBoundingBox() const
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

NOVASVG_INLINE Rect SVGImageElement::strokeBoundingBox() const
{
    return fillBoundingBox();
}

NOVASVG_INLINE void SVGImageElement::render(SVGRenderState& state) const
{
    if(m_image.isNull() || isDisplayNone() || isVisibilityHidden())
        return;
    Rect dstRect(fillBoundingBox());
    Rect srcRect(0, 0, m_image.width(), m_image.height());
    if(dstRect.isEmpty() || srcRect.isEmpty())
        return;
    m_preserveAspectRatio.transformRect(dstRect, srcRect);

    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    newState->drawImage(m_image, dstRect, srcRect, newState.currentTransform());
    newState.endGroup(blendInfo);
}

static Bitmap loadImageResource(const std::string& href)
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

NOVASVG_INLINE void SVGImageElement::parseAttribute(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Href) {
        m_image = loadImageResource(value);
    } else {
        SVGGraphicsElement::parseAttribute(id, value);
    }
}

// ---------------------------------------------------------------
// SVGForeignObjectElement / ForeignObjectSimple
// ---------------------------------------------------------------

NOVASVG_INLINE SVGForeignObjectElement::SVGForeignObjectElement(Document* document)
    : SVGGraphicsElement(document, ElementID::ForeignObject)
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

NOVASVG_INLINE Rect SVGForeignObjectElement::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    return Rect(
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    );
}

NOVASVG_INLINE Rect SVGForeignObjectElement::strokeBoundingBox() const
{
    return fillBoundingBox();
}

NOVASVG_INLINE void SVGForeignObjectElement::layoutElement(const SVGLayoutState& state)
{
    m_font = state.font();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    SVGGraphicsElement::layoutElement(state);
}

NOVASVG_INLINE void SVGForeignObjectElement::render(SVGRenderState& state) const
{
    if(m_rawContent.empty() || isDisplayNone() || isVisibilityHidden())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
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

// Strips HTML/XML tags, decodes a handful of common entities, and
// collapses runs of whitespace to a single space -- roughly
// "element.innerText" for the tiny subset of HTML this cares about.
// Deliberately not a real parser: no nesting/structure is kept, no
// per-tag CSS (display:block vs inline, margins, ...) is applied. That
// matches what Mermaid (and most SVG-generating tools) actually need,
// since their foreignObject content is always a single short,
// non-wrapping, already-pre-laid-out text run.
inline std::string foreignObjectPlainText(std::string_view html)
{
    std::string out;
    out.reserve(html.size());
    bool inTag = false;
    for(size_t i = 0; i < html.size();) {
        char c = html[i];
        if(inTag) {
            if(c == '>')
                inTag = false;
            ++i;
            continue;
        }

        if(c == '<') {
            inTag = true;
            ++i;
            continue;
        }

        if(c == '&') {
            if(html.compare(i, 5, "&amp;") == 0) { out += '&'; i += 5; continue; }
            if(html.compare(i, 4, "&lt;") == 0) { out += '<'; i += 4; continue; }
            if(html.compare(i, 4, "&gt;") == 0) { out += '>'; i += 4; continue; }
            if(html.compare(i, 6, "&quot;") == 0) { out += '"'; i += 6; continue; }
            if(html.compare(i, 6, "&apos;") == 0) { out += '\''; i += 6; continue; }
            if(html.compare(i, 6, "&nbsp;") == 0) { out += ' '; i += 6; continue; }
        }

        out += c;
        ++i;
    }

    std::string collapsed;
    collapsed.reserve(out.size());
    bool lastWasSpace = true; // trims leading whitespace too
    for(char c : out) {
        if(c == '\t' || c == '\n' || c == '\r')
            c = ' ';
        if(c == ' ') {
            if(lastWasSpace)
                continue;
            lastWasSpace = true;
        } else {
            lastWasSpace = false;
        }
        collapsed += c;
    }
    while(!collapsed.empty() && collapsed.back() == ' ')
        collapsed.pop_back();
    return collapsed;
}

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

} // namespace

NOVASVG_INLINE void ForeignObjectSimple::render(const SVGForeignObjectElement* element, SVGRenderState& state) const
{
    auto text = foreignObjectPlainText(element->rawContent());
    if(text.empty())
        return;

    const auto& font = element->font();
    if(font.isNull())
        return;

    auto u32text = utf8ToU32(text);
    std::u32string_view textView(u32text);

    auto box = element->fillBoundingBox();
    auto textWidth = font.measureText(textView);
    Point origin(
        box.x + (box.w - textWidth) / 2.f,
        box.y + (box.h + font.ascent() - font.descent()) / 2.f
    );

    const auto& fill = element->fill();
    if(fill.applyPaint(state))
        state->fillText(textView, font, origin, state.currentTransform());
}

NOVASVG_INLINE SVGSymbolElement::SVGSymbolElement(Document* document)
    : SVGGraphicsElement(document, ElementID::Symbol)
    , SVGFitToViewBox(this)
{
}

NOVASVG_INLINE SVGGElement::SVGGElement(Document* document)
    : SVGGraphicsElement(document, ElementID::G)
{
}

NOVASVG_INLINE void SVGGElement::render(SVGRenderState& state) const
{
    if(isDisplayNone())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE SVGDefsElement::SVGDefsElement(Document* document)
    : SVGGraphicsElement(document, ElementID::Defs)
{
}

NOVASVG_INLINE SVGMarkerElement::SVGMarkerElement(Document* document)
    : SVGElement(document, ElementID::Marker)
    , SVGFitToViewBox(this)
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

NOVASVG_INLINE Point SVGMarkerElement::refPoint() const
{
    LengthContext lengthContext(this);
    const Point refPoint = {
        lengthContext.valueForLength(m_refX),
        lengthContext.valueForLength(m_refY)
    };

    return refPoint;
}

NOVASVG_INLINE Size SVGMarkerElement::markerSize() const
{
    LengthContext lengthContext(this);
    const Size markerSize = {
        lengthContext.valueForLength(m_markerWidth),
        lengthContext.valueForLength(m_markerHeight)
    };

    return markerSize;
}

NOVASVG_INLINE Transform SVGMarkerElement::markerTransform(const Point& origin, float angle, float strokeWidth) const
{
    auto transform = Transform::translated(origin.x, origin.y);
    if(m_orient.orientType() == SVGAngle::OrientType::Angle) {
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

NOVASVG_INLINE Rect SVGMarkerElement::markerBoundingBox(const Point& origin, float angle, float strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).mapRect(paintBoundingBox());
}

NOVASVG_INLINE void SVGMarkerElement::renderMarker(SVGRenderState& state, const Point& origin, float angle, float strokeWidth) const
{
    if(state.hasCycleReference(this))
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, markerTransform(origin, angle, strokeWidth));
    newState.beginGroup(blendInfo);
    if(isOverflowHidden())
        newState->clipRect(getClipRect(markerSize()), FillRule::NonZero, newState.currentTransform());
    renderChildren(newState);
    newState.endGroup(blendInfo);
}

NOVASVG_INLINE Transform SVGMarkerElement::localTransform() const
{
    return viewBoxToViewTransform(markerSize());
}

NOVASVG_INLINE SVGClipPathElement::SVGClipPathElement(Document* document)
    : SVGGraphicsElement(document, ElementID::ClipPath)
    , m_clipPathUnits(PropertyID::ClipPathUnits, Units::UserSpaceOnUse)
{
    addProperty(m_clipPathUnits);
}

NOVASVG_INLINE Rect SVGClipPathElement::clipBoundingBox(const SVGElement* element) const
{
    auto clipBoundingBox = paintBoundingBox();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = element->fillBoundingBox();
        clipBoundingBox.x = clipBoundingBox.x * bbox.w + bbox.x;
        clipBoundingBox.y = clipBoundingBox.y * bbox.h + bbox.y;
        clipBoundingBox.w = clipBoundingBox.w * bbox.w;
        clipBoundingBox.h = clipBoundingBox.h * bbox.h;
    }

    return localTransform().mapRect(clipBoundingBox);
}

NOVASVG_INLINE void SVGClipPathElement::applyClipMask(SVGRenderState& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = Canvas::create(state.currentTransform().mapRect(state.paintBoundingBox()));
    auto currentTransform = state.currentTransform() * localTransform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    SVGRenderState newState(this, &state, currentTransform, SVGRenderMode::Clipping, maskImage);
    renderChildren(newState);
    if(clipper()) {
        clipper()->applyClipMask(newState);
    }

    state->blendCanvas(*maskImage, BlendMode::Dst_In, 1.f);
}

inline const SVGGeometryElement* toSVGGeometryElement(const SVGNode* node)
{
    if(node && node->isGeometryElement())
        return static_cast<const SVGGeometryElement*>(node);
    return nullptr;
}

NOVASVG_INLINE void SVGClipPathElement::applyClipPath(SVGRenderState& state) const
{
    auto currentTransform = state.currentTransform() * localTransform();
    if(m_clipPathUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    for(const auto& child : children()) {
        auto element = toSVGElement(child);
        if(element == nullptr || element->isDisplayNone())
            continue;
        Transform clipTransform(currentTransform);
        auto shapeElement = toSVGGeometryElement(element);
        if(shapeElement == nullptr) {
            if(element->id() != ElementID::Use)
                continue;
            clipTransform.multiply(element->localTransform());
            shapeElement = toSVGGeometryElement(element->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        state->clipPath(shapeElement->path(), shapeElement->clip_rule(), clipTransform * shapeElement->localTransform());
        return;
    }

    state->clipRect(Rect::Empty, FillRule::NonZero, Transform::Identity);
}

NOVASVG_INLINE bool SVGClipPathElement::requiresMasking() const
{
    if(clipper())
        return true;
    const SVGGeometryElement* prevShapeElement = nullptr;
    for(const auto& child : children()) {
        auto element = toSVGElement(child);
        if(element == nullptr || element->isDisplayNone())
            continue;
        auto shapeElement = toSVGGeometryElement(element);
        if(shapeElement == nullptr) {
            if(element->isTextPositioningElement())
                return true;
            if(element->id() != ElementID::Use)
                continue;
            if(element->clipper())
                return true;
            shapeElement = toSVGGeometryElement(element->firstChild());
        }

        if(shapeElement == nullptr || !shapeElement->isRenderable())
            continue;
        if(prevShapeElement || shapeElement->clipper())
            return true;
        prevShapeElement = shapeElement;
    }

    return false;
}

NOVASVG_INLINE SVGMaskElement::SVGMaskElement(Document* document)
    : SVGElement(document, ElementID::Mask)
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

NOVASVG_INLINE Rect SVGMaskElement::maskRect(const SVGElement* element) const
{
    LengthContext lengthContext(this, m_maskUnits.value());
    Rect maskRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    if(m_maskUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = element->fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    return maskRect;
}

NOVASVG_INLINE Rect SVGMaskElement::maskBoundingBox(const SVGElement* element) const
{
    auto maskBoundingBox = paintBoundingBox();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = element->fillBoundingBox();
        maskBoundingBox.x = maskBoundingBox.x * bbox.w + bbox.x;
        maskBoundingBox.y = maskBoundingBox.y * bbox.h + bbox.y;
        maskBoundingBox.w = maskBoundingBox.w * bbox.w;
        maskBoundingBox.h = maskBoundingBox.h * bbox.h;
    }

    return maskBoundingBox.intersected(maskRect(element));
}

NOVASVG_INLINE void SVGMaskElement::applyMask(SVGRenderState& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = Canvas::create(state.currentTransform().mapRect(state.paintBoundingBox()));
    maskImage->clipRect(maskRect(state.element()), FillRule::NonZero, state.currentTransform());

    auto currentTransform = state.currentTransform();
    if(m_maskContentUnits.value() == Units::ObjectBoundingBox) {
        auto bbox = state.fillBoundingBox();
        currentTransform.translate(bbox.x, bbox.y);
        currentTransform.scale(bbox.w, bbox.h);
    }

    SVGRenderState newState(this, &state, currentTransform, SVGRenderMode::Painting, maskImage);
    renderChildren(newState);
    if(clipper())
        clipper()->applyClipMask(newState);
    if(masker()) {
        masker()->applyMask(newState);
    }

    if(m_mask_type == MaskType::Luminance)
        maskImage->convertToLuminanceMask();
    state->blendCanvas(*maskImage, BlendMode::Dst_In, 1.f);
}

NOVASVG_INLINE void SVGMaskElement::layoutElement(const SVGLayoutState& state)
{
    m_mask_type = state.mask_type();
    SVGElement::layoutElement(state);
}

} // namespace novasvg

#endif // NOVASVG_SVGELEMENT_H
