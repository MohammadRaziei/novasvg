#ifndef NOVA_FOREIGN_OBJECT_H
#define NOVA_FOREIGN_OBJECT_H

// The extension point for <foreignObject> (ADR-0010).
//
// novasvg does not know what is inside a <foreignObject>: it has a box and a piece of markup, and a
// foreign_object_renderer that draws it. The built-in renderer ("simple": HTML as styled text) is one such
// renderer, written against exactly this header. Give a document your own with
// document::set_foreign_object_renderer(), e.g. one that rasterizes the markup with a real HTML engine and
// hands the pixels to the painter.

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <stb/color.hpp>
#include <stb/image.hpp>

#include "../config.h"
#include "../vg/geometry.h"

namespace nova::svg {

struct foreign_object_font {
    std::string family;
    float size = 16.f; ///< user units
    bool bold = false;
    bool italic = false;
};

struct foreign_object_font_metrics {
    float ascent = 0.f;
    float height = 0.f; ///< ascent + descent
};

struct foreign_object_size {
    float width = 0.f;
    float height = 0.f;
    int line_count = 0;
    float line_height = 0.f;
};

/// One element of the markup, as the renderer parsed it; `parent` indexes the same list (-1: a direct child
/// of the foreignObject).
struct html_tag {
    std::string tag;        ///< lower case
    std::string id;
    std::string class_names;
    std::string style;      ///< the style="" attribute, verbatim
    int parent = -1;
};

struct css_declaration {
    std::string value;
    int specificity = 0; ///< the larger one wins; a later declaration wins a tie
};

/// The document's CSS, for markup the document did not parse itself. A selector reaches through the
/// foreignObject into the svg around it (mermaid's `.section-0 span { color: black }`), so the answer
/// depends on the svg ancestors of the foreignObject, which only the document knows.
class css_query {
public:
    virtual ~css_query() = default;
    /// The declaration of `property` ("color", "background-color", ...) that wins for `nodes[index]`
    /// among the document's style sheets, or nothing. Declarations in a style="" attribute are not
    /// included: they are in the markup, the renderer reads them (they beat any rule unless !important).
    virtual std::optional<css_declaration> declared(const std::vector<html_tag>& nodes, int index, std::string_view property) const = 0;
};

/// Condense text horizontally about `about_x` (a line that is wider than its box).
struct text_squeeze {
    float factor = 1.f;
    float about_x = 0.f;
};

/// What a renderer may draw with; implemented by the document over its canvas, in the foreignObject's
/// coordinate system (user units, y down).
class foreign_object_painter {
public:
    virtual ~foreign_object_painter() = default;
    virtual void fill_rect(const vg::box& box_, const stb::color& color_) = 0;
    /// `x`, `y`: the left end of the baseline.
    virtual void fill_text(std::u32string_view text, const foreign_object_font& font_, float x, float y,
                           const stb::color& color_, const text_squeeze& squeeze = {}) = 0;
    /// Straight (not premultiplied) RGBA, stretched to `dest`. For renderers that rasterize.
    virtual void draw_image(const stb::image& image, const vg::box& dest) = 0;
    virtual foreign_object_font_metrics metrics(const foreign_object_font& font_) const = 0;
    virtual float measure_text(std::u32string_view text, const foreign_object_font& font_) const = 0;
};

struct foreign_object_context {
    std::string_view markup; ///< untouched, as in the source
    vg::box box;             ///< the foreignObject's x, y, width, height
    foreign_object_font font; ///< inherited through the svg cascade
    float device_scale = 1.f; ///< device pixels per user unit (the larger axis), for renderers that rasterize
    const void* prepared = nullptr; ///< what prepare() returned for this element
};

/// Called concurrently from `const` methods on one document: keep no mutable state without a lock.
class foreign_object_renderer {
public:
    virtual ~foreign_object_renderer() = default;

    /// Once per foreignObject, after the style sheets are applied (and again when the sheets or the
    /// renderer change): work out what depends on the CSS. The result is handed back in
    /// foreign_object_context::prepared.
    virtual std::shared_ptr<void> prepare(std::string_view markup, const css_query& css) const
    {
        (void)markup;
        (void)css;
        return nullptr;
    }

    virtual void render(const foreign_object_context& context, foreign_object_painter& painter) const = 0;

    /// The size the content wants when wrapped at `max_width` (<= 0: the markup's own width, else no
    /// wrapping). Optional.
    virtual std::optional<foreign_object_size> measure(const foreign_object_context& context, const foreign_object_painter& painter,
                                                       float max_width) const
    {
        (void)context;
        (void)painter;
        (void)max_width;
        return std::nullopt;
    }
};

/// The renderer a document starts with: the built-in simple one, or null when novasvg was built with
/// NOVA_FOREIGN_OBJECT_SIMPLE=OFF.
NOVA_API std::shared_ptr<foreign_object_renderer> default_foreign_object_renderer();

} // namespace nova::svg

#endif // NOVA_FOREIGN_OBJECT_H
