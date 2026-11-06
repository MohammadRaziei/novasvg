#pragma once

// PropertyID, the property classes (length, number, path, transform, ...) and the enums they carry.

#include "vg/pixmap.h"

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
#include "nova/svg/svg.h"
#include "vg/canvas.h"
#include "vg/font.h"
#include "vg/parserutils.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)


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
    explicit Paint(const stb::color& color) : m_color(color) {}
    Paint(const std::string& id, const stb::color& color)
        : m_id(id), m_color(color)
    {}

    const stb::color& color() const { return m_color; }
    const std::string& id() const { return m_id; }
    bool isNone() const { return m_id.empty() && !is_visible(m_color); }

private:
    std::string m_id;
    stb::color m_color = stb::color::transparent();
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

    const stb::outline& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    stb::outline m_value;
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

} // namespace nova::svg
