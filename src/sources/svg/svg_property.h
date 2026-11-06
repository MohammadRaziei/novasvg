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
enum class property_id : uint8_t {
    unknown = 0,
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
    in2,
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
    required_extensions,
    Result,
    Rotate,
    Rx,
    Ry,
    SpreadMethod,
    std_deviation,
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
    system_language,
    Text_Anchor,
    Text_Orientation,
    TextLength,
    Transform,
    transform_box,
    transform_origin,
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

property_id propertyid(std::string_view name);
property_id csspropertyid(std::string_view name);

class svg_element;

class svg_property {
public:
    svg_property(property_id id);
    virtual ~svg_property() = default;
    property_id id() const { return m_id; }

    virtual bool parse(std::string_view input) = 0;

private:
    svg_property(const svg_property&) = delete;
    svg_property& operator=(const svg_property&) = delete;
    property_id m_id;
};

class svg_string final : public svg_property {
public:
    explicit svg_string(property_id id)
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
    visible,
    hidden,
    collapse
};

enum class Overflow : uint8_t {
    visible,
    hidden
};

// CSS mix-blend-mode. Normal = ordinary painting. Of the other keywords the separable modes below are
// supported; overlay, color-dodge, color-burn, hard-light, soft-light and the non-separable ones (hue,
// saturation, color, luminosity) are not yet and paint as Normal, like a value this library doesn't know.
enum class MixBlendMode : uint8_t {
    normal,
    Multiply,
    screen,
    darken,
    lighten,
    difference,
    exclusion
};

enum class PointerEvents : uint8_t {
    None,
    Auto,
    Stroke,
    Fill,
    painted,
    visible,
    visible_stroke,
    visible_fill,
    visible_painted,
    BoundingBox,
    All
};

enum class FontStyle : uint8_t {
    normal,
    Italic
};

enum class FontWeight : uint8_t {
    normal,
    Bold
};

enum class AlignmentBaseline : uint8_t {
    Auto,
    Baseline,
    before_edge,
    text_before_edge,
    middle,
    central,
    after_edge,
    text_after_edge,
    ideographic,
    alphabetic,
    hanging,
    mathematical
};

enum class DominantBaseline : uint8_t {
    Auto,
    use_script,
    no_change,
    reset_size,
    ideographic,
    alphabetic,
    hanging,
    mathematical,
    central,
    middle,
    text_after_edge,
    text_before_edge
};

enum class TextAnchor : uint8_t {
    Start,
    middle,
    End
};

enum class WhiteSpace : uint8_t {
    Default,
    preserve
};

enum class WritingMode : uint8_t {
    Horizontal,
    vertical
};

enum class TextOrientation : uint8_t {
    mixed,
    upright
};

enum class Direction : uint8_t {
    ltr,
    rtl
};

enum class MaskType : uint8_t {
    luminance,
    Alpha
};

enum class Units : uint8_t {
    user_space_on_use,
    object_bounding_box
};

enum class MarkerUnits : uint8_t {
    StrokeWidth,
    user_space_on_use
};

enum class LengthAdjust : uint8_t {
    Spacing,
    spacing_and_glyphs
};

template<typename Enum>
using svg_enumeration_entry = std::pair<Enum, std::string_view>;

template<typename Enum>
class svg_enumeration final : public svg_property {
public:
    explicit svg_enumeration(property_id id, Enum value)
        : svg_property(id)
        , m_value(value)
    {}

    Enum value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    template<unsigned int N>
    bool parse_enum(std::string_view input, const svg_enumeration_entry<Enum>(&entries)[N]);
    Enum m_value;
};

class svg_angle final : public svg_property {
public:
    enum class OrientType {
        Auto,
        auto_start_reverse,
        Angle
    };

    explicit svg_angle(property_id id)
        : svg_property(id)
    {}

    float value() const { return m_value; }
    OrientType orientType() const { return m_orient_type; }
    bool parse(std::string_view input) final;

private:
    float m_value = 0;
    OrientType m_orient_type = OrientType::Angle;
};

enum class length_units : uint8_t {
    None,
    percent,
    Px,
    em,
    ex
};

enum class length_direction : uint8_t {
    Horizontal,
    vertical,
    diagonal
};

enum class length_negative_mode : uint8_t {
    allow,
    forbid
};

class Length {
public:
    Length() = default;
    Length(float value, length_units units)
        : m_value(value), m_units(units)
    {}

    float value() const { return m_value; }
    length_units units() const { return m_units; }

    bool parse(std::string_view input, length_negative_mode mode);

private:
    float m_value = 0.f;
    length_units m_units = length_units::None;
};

class svg_length final : public svg_property {
public:
    svg_length(property_id id, length_direction direction, length_negative_mode negative_mode, float value = 0, length_units units = length_units::None)
        : svg_property(id)
        , m_direction(direction)
        , m_negative_mode(negative_mode)
        , m_value(value, units)
    {}

    bool is_percent() const { return m_value.units() == length_units::percent; }

    length_direction direction() const { return m_direction; }
    length_negative_mode negative_mode() const { return m_negative_mode; }
    const Length& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    const length_direction m_direction;
    const length_negative_mode m_negative_mode;
    Length m_value;
};

class LengthContext {
public:
    LengthContext(const svg_element* el, Units units = Units::user_space_on_use)
        : m_element(el), m_units(units)
    {}

    float value_for_length(const Length& length, length_direction direction) const;
    float value_for_length(const svg_length& length) const { return value_for_length(length.value(), length.direction()); }

private:
    float viewport_dimension(length_direction direction) const;
    const svg_element* m_element;
    const Units m_units;
};

using length_list = std::vector<Length>;

class svg_length_list final : public svg_property {
public:
    svg_length_list(property_id id, length_direction direction, length_negative_mode negative_mode)
        : svg_property(id)
        , m_direction(direction)
        , m_negative_mode(negative_mode)
    {}

    length_direction direction() const { return m_direction; }
    length_negative_mode negative_mode() const { return m_negative_mode; }
    const length_list& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    const length_direction m_direction;
    const length_negative_mode m_negative_mode;
    length_list m_values;
};

class BaselineShift {
public:
    enum class Type {
        Baseline,
        sub,
        super,
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
    svg_number(property_id id, float value)
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
    svg_number_percentage(property_id id, float value)
        : svg_property(id)
        , m_value(value)
    {}

    float value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    float m_value;
};

using number_list = std::vector<float>;

class svg_number_list final : public svg_property {
public:
    explicit svg_number_list(property_id id)
        : svg_property(id)
    {}

    const number_list& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    number_list m_values;
};

class svg_path final : public svg_property {
public:
    explicit svg_path(property_id id)
        : svg_property(id)
    {}

    const stb::outline& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    stb::outline m_value;
};

class svg_point final : public svg_property {
public:
   explicit svg_point(property_id id)
        : svg_property(id)
    {}

    const Point& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Point m_value;
};

using point_list = std::vector<Point>;

class svg_point_list final : public svg_property {
public:
    explicit svg_point_list(property_id id)
        : svg_property(id)
    {}

    const point_list& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    point_list m_values;
};

class svg_rect final : public svg_property {
public:
    explicit svg_rect(property_id id)
        : svg_property(id)
        , m_value(Rect::invalid)
    {}

    const Rect& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Rect m_value;
};

class svg_transform final : public svg_property {
public:
    explicit svg_transform(property_id id)
        : svg_property(id)
    {}

    const Transform& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Transform m_value;
};

enum class transform_box_type : uint8_t {
    ViewBox,
    fill_box
};

// CSS `transform-box`. Only distinguishes fill-box (and content-box, treated
// the same here) from the default view-box -- stroke-box and the various
// *-box refinements aren't worth the extra bookkeeping for what this is
// used for (resolving `transform-origin`'s reference box).
class svg_transform_box final : public svg_property {
public:
    explicit svg_transform_box(property_id id)
        : svg_property(id)
    {}

    transform_box_type value() const { return m_value; }
    bool specified() const { return m_specified; }
    bool parse(std::string_view input) final;

private:
    transform_box_type m_value = transform_box_type::ViewBox;
    bool m_specified = false;
};

// CSS `transform-origin`, 2-value form (x y; an optional 3rd z-offset token
// is skipped if present, this renderer is 2D only). Supports the keywords
// (left/center/right/top/bottom), percentages, and plain numbers (treated
// as user-unit lengths) -- covers the common cases without pulling in the
// full CSS <length-percentage> grammar.
class svg_transform_origin final : public svg_property {
public:
    explicit svg_transform_origin(property_id id)
        : svg_property(id)
    {}

    bool specified() const { return m_specified; }
    Point resolve(const Rect& box) const
    {
        auto ox = m_x_is_percent ? box.x + box.w * m_x : box.x + m_x;
        auto oy = m_y_is_percent ? box.y + box.h * m_y : box.y + m_y;
        return Point(ox, oy);
    }
    bool parse(std::string_view input) final;

private:
    float m_x = 0.5f;
    float m_y = 0.5f;
    bool m_x_is_percent = true;
    bool m_y_is_percent = true;
    bool m_specified = false;
};

class svg_preserve_aspect_ratio final : public svg_property {
public:
    enum class AlignType {
        None,
        x_min_y_min,
        x_mid_y_min,
        x_max_y_min,
        x_min_y_mid,
        x_mid_y_mid,
        x_max_y_mid,
        x_min_y_max,
        x_mid_y_max,
        x_max_y_max
    };

    enum class MeetOrSlice {
        meet,
        slice
    };

    explicit svg_preserve_aspect_ratio(property_id id)
        : svg_property(id)
    {}

    AlignType alignType() const { return m_align_type; }
    MeetOrSlice meetOrSlice() const { return m_meet_or_slice; }
    bool parse(std::string_view input) final;

    Rect get_clip_rect(const Rect& view_box_rect, const Size& viewport_size) const;
    Transform get_transform(const Rect& view_box_rect, const Size& viewport_size) const;
    void transform_rect(Rect& dstRect, Rect& srcRect) const;

private:
    AlignType m_align_type = AlignType::x_mid_y_mid;
    MeetOrSlice m_meet_or_slice = MeetOrSlice::meet;
};

} // namespace nova::svg
