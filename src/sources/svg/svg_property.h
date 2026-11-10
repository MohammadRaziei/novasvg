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
    alignment_baseline_,
    background_color,
    baseline_shift_,
    class_,
    clip_path,
    clip_rule,
    clip_path_units,
    color,
    cx,
    cy,
    D,
    direction,
    display,
    dominant_baseline_,
    dx,
    dy,
    fill,
    fill_opacity,
    fill_rule_,
    filter,
    flood_color,
    flood_opacity,
    font_family,
    font_size,
    font_style_,
    font_weight_,
    fx,
    fy,
    gradient_transform,
    gradient_units,
    height,
    href,
    id,
    in,
    in2,
    K1,
    K2,
    K3,
    K4,
    length_adjust,
    letter_spacing,
    marker_end,
    marker_mid,
    marker_start,
    marker_height,
    marker_units,
    marker_width,
    mask,
    mask_type_,
    mask_content_units,
    mask_units,
    mix_blend_mode_,
    offset,
    opacity,
    operator_,
    orient,
    overflow,
    pattern_content_units,
    pattern_transform,
    pattern_units,
    pointer_events_,
    points,
    preserve_aspect_ratio,
    R,
    ref_x,
    ref_y,
    required_extensions,
    result,
    rotate,
    rx,
    ry,
    spread_method,
    std_deviation,
    stop_color,
    stop_opacity,
    stroke,
    stroke_dasharray,
    stroke_dashoffset,
    stroke_linecap,
    stroke_linejoin,
    stroke_miterlimit,
    stroke_opacity,
    stroke_width,
    style,
    system_language,
    text_anchor_,
    text_orientation_,
    text_length,
    transform,
    transform_box,
    transform_origin,
    view_box,
    visibility,
    white_space_,
    width,
    word_spacing,
    writing_mode_,
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

class paint {
public:
    paint() = default;
    explicit paint(const stb::color& color_) : m_color(color_) {}
    paint(const std::string& id, const stb::color& color_)
        : m_id(id), m_color(color_)
    {}

    const stb::color& color() const { return m_color; }
    const std::string& id() const { return m_id; }
    bool is_none() const { return m_id.empty() && !is_visible(m_color); }

private:
    std::string m_id;
    stb::color m_color = stb::color::transparent();
};

enum class css_display : uint8_t {
    inline_,
    none
};

enum class css_visibility : uint8_t {
    visible,
    hidden,
    collapse
};

enum class css_overflow : uint8_t {
    visible,
    hidden
};

// CSS mix-blend-mode. Normal = ordinary painting. Of the other keywords the separable modes below are
// supported; overlay, color-dodge, color-burn, hard-light, soft-light and the non-separable ones (hue,
// saturation, color, luminosity) are not yet and paint as Normal, like a value this library doesn't know.
enum class css_mix_blend_mode : uint8_t {
    normal,
    multiply,
    screen,
    darken,
    lighten,
    difference,
    exclusion
};

enum class css_pointer_events : uint8_t {
    none,
    auto_,
    stroke,
    fill,
    painted,
    visible,
    visible_stroke,
    visible_fill,
    visible_painted,
    bounding_box,
    all
};

enum class css_font_style : uint8_t {
    normal,
    italic
};

enum class css_font_weight : uint8_t {
    normal,
    bold
};

enum class css_alignment_baseline : uint8_t {
    auto_,
    baseline,
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

enum class css_dominant_baseline : uint8_t {
    auto_,
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

enum class svg_text_anchor : uint8_t {
    start,
    middle,
    end
};

enum class css_white_space : uint8_t {
    default_,
    preserve
};

enum class css_writing_mode : uint8_t {
    horizontal,
    vertical
};

enum class css_text_orientation : uint8_t {
    mixed,
    upright
};

enum class css_direction : uint8_t {
    ltr,
    rtl
};

enum class css_mask_type : uint8_t {
    luminance,
    alpha
};

enum class units : uint8_t {
    user_space_on_use,
    object_bounding_box
};

enum class svg_marker_units : uint8_t {
    stroke_width,
    user_space_on_use
};

enum class svg_length_adjust : uint8_t {
    spacing,
    spacing_and_glyphs
};

template<typename enum_>
using svg_enumeration_entry = std::pair<enum_, std::string_view>;

template<typename enum_>
class svg_enumeration final : public svg_property {
public:
    explicit svg_enumeration(property_id id, enum_ value)
        : svg_property(id)
        , m_value(value)
    {}

    enum_ value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    template<unsigned int N>
    bool parse_enum(std::string_view input, const svg_enumeration_entry<enum_>(&entries)[N]);
    enum_ m_value;
};

class svg_angle final : public svg_property {
public:
    enum class orient_type_value {
        auto_,
        auto_start_reverse,
        angle
    };

    explicit svg_angle(property_id id)
        : svg_property(id)
    {}

    float value() const { return m_value; }
    orient_type_value orient_type() const { return m_orient_type; }
    bool parse(std::string_view input) final;

private:
    float m_value = 0;
    orient_type_value m_orient_type = orient_type_value::angle;
};

enum class length_units : uint8_t {
    none,
    percent,
    px,
    em,
    ex
};

enum class length_direction : uint8_t {
    horizontal,
    vertical,
    diagonal
};

enum class length_negative_mode : uint8_t {
    allow,
    forbid
};

class css_length {
public:
    css_length() = default;
    css_length(float value, length_units units_)
        : m_value(value), m_units(units_)
    {}

    float value() const { return m_value; }
    length_units units() const { return m_units; }

    bool parse(std::string_view input, length_negative_mode mode);

private:
    float m_value = 0.f;
    length_units m_units = length_units::none;
};

class svg_length final : public svg_property {
public:
    svg_length(property_id id, length_direction direction_, length_negative_mode negative_mode, float value = 0, length_units units_ = length_units::none)
        : svg_property(id)
        , m_direction(direction_)
        , m_negative_mode(negative_mode)
        , m_value(value, units_)
    {}

    bool is_percent() const { return m_value.units() == length_units::percent; }

    length_direction direction() const { return m_direction; }
    length_negative_mode negative_mode() const { return m_negative_mode; }
    const css_length& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    const length_direction m_direction;
    const length_negative_mode m_negative_mode;
    css_length m_value;
};

class length_context {
public:
    length_context(const svg_element* el, units units_ = units::user_space_on_use)
        : m_element(el), m_units(units_)
    {}

    float value_for_length(const css_length& length_, length_direction direction_) const;
    float value_for_length(const svg_length& length_) const { return value_for_length(length_.value(), length_.direction()); }

private:
    float viewport_dimension(length_direction direction_) const;
    const svg_element* m_element;
    const units m_units;
};

using length_list = std::vector<css_length>;

class svg_length_list final : public svg_property {
public:
    svg_length_list(property_id id, length_direction direction_, length_negative_mode negative_mode)
        : svg_property(id)
        , m_direction(direction_)
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

class css_baseline_shift {
public:
    enum class shift_type {
        baseline,
        sub,
        super,
        length
    };

    css_baseline_shift() = default;
    css_baseline_shift(shift_type type_) : m_type(type_) {}
    css_baseline_shift(const css_length& length_) : m_type(shift_type::length), m_length(length_) {}

    shift_type type() const { return m_type; }
    const css_length& length() const { return m_length; }

private:
    shift_type m_type{shift_type::baseline};
    css_length m_length;
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

    const point& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    point m_value;
};

using point_list = std::vector<point>;

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
        , m_value(rect::invalid)
    {}

    const rect& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    rect m_value;
};

class svg_transform final : public svg_property {
public:
    explicit svg_transform(property_id id)
        : svg_property(id)
    {}

    const transform& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    transform m_value;
};

enum class transform_box_type : uint8_t {
    view_box,
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
    transform_box_type m_value = transform_box_type::view_box;
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
    point resolve(const rect& box_) const
    {
        auto ox = m_x_is_percent ? box_.x + box_.w * m_x : box_.x + m_x;
        auto oy = m_y_is_percent ? box_.y + box_.h * m_y : box_.y + m_y;
        return point(ox, oy);
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
    enum class align_type_value {
        none,
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

    enum class meet_or_slice_value {
        meet,
        slice
    };

    explicit svg_preserve_aspect_ratio(property_id id)
        : svg_property(id)
    {}

    align_type_value align_type() const { return m_align_type; }
    meet_or_slice_value meet_or_slice() const { return m_meet_or_slice; }
    bool parse(std::string_view input) final;

    rect get_clip_rect(const rect& view_box_rect, const size& viewport_size) const;
    transform get_transform(const rect& view_box_rect, const size& viewport_size) const;
    void transform_rect(rect& dst_rect, rect& src_rect) const;

private:
    align_type_value m_align_type = align_type_value::x_mid_y_mid;
    meet_or_slice_value m_meet_or_slice = meet_or_slice_value::meet;
};

} // namespace nova::svg
