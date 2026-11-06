# ADR-0010: foreignObject is an extension, not part of the core

Status: proposed (design only, no code changed yet)

## Problem
Today the core knows HTML. What is already an extension point is `foreign_object_renderer`, but it is private: it takes `svg_foreign_object_element*` and `svg_render_state&`, so nobody outside the tree can write one. Concretely:

- `svg_foreign_object_text.h` (the HTML subset parser, wrapping, measuring; about 900 lines) is included by the core umbrella header.
- `svg_foreign_object_element` stores `html_text_color` / `html_background_color`, and `resolve_foreign_object_style()` (in `svgparser.hpp`) works out HTML colours with the core's own selector matcher.
- `novasvg.cpp` captures `raw_content` and calls the HTML style resolver for every foreignObject.
- the Python binding re-implements the simple renderer's font choice (`measure_foreign_object`) by reaching into internals.

## Design
**The core knows three things about a foreignObject: its box, its markup (an opaque string) and who renders it.** All HTML lives in one module that the core does not include.

1. **Public interface**, `include/nova/svg/foreign_object.h`, built only from public types (`Box`, `stb::color`, `stb::image`, `element`):

```cpp
struct foreign_object_font { std::string family; float size; bool bold, italic; };

// How a renderer asks the document's CSS about HTML it parsed itself (mermaid's `.section-0 span {...}`
// reaches through the foreignObject into the svg around it).
class css_query {
public:
    virtual ~css_query() = default;
    virtual std::optional<std::string> value(std::span<const html_tag> inside, std::string_view property) const = 0;
};

struct foreign_object_context {
    std::string_view markup;        // untouched, as in the source
    Box box;                        // user units
    foreign_object_font font;       // inherited through the svg cascade
    stb::color fill;
    float device_scale;             // device pixels per user unit, for renderers that rasterize
    const element* node;           // the public DOM element (attributes, ancestors)
    const css_query& css;
};

// What a renderer may draw with. Implemented by the core over the canvas; nothing of it leaks.
class foreign_object_painter {
public:
    virtual ~foreign_object_painter() = default;
    virtual void fill_rect(const Box&, const stb::color&) = 0;
    virtual void fill_text(std::u32string_view, const foreign_object_font&, float x, float y, const stb::color&, float x_scale = 1) = 0;
    virtual void draw_image(const stb::image&, const Box& dest) = 0;   // raster renderers (litehtml, a browser view)
    virtual font_metrics metrics(const foreign_object_font&) const = 0;
    virtual float measure_text(std::u32string_view, const foreign_object_font&) const = 0;
};

class foreign_object_renderer {
public:
    virtual ~foreign_object_renderer() = default;
    virtual void render(const foreign_object_context&, foreign_object_painter&) const = 0;
    // optional: the size the content wants; used by the Python `measure_foreign_object`
    virtual std::optional<foreign_object_size> measure(const foreign_object_context&, float max_width) const { return {}; }
};
```
A renderer must be callable concurrently from `const` methods (documented).

2. **Registration is per document**, no globals: `document::set_foreign_object_renderer(std::shared_ptr<foreign_object_renderer>)`. `nullptr` skips foreignObject entirely (a security and speed switch). The default is the built-in one, below.

3. **The built-in renderer is a plugin written against the same API** (that is what proves the API is enough): `foreign_object_simple` plus the HTML subset helpers and the CSS-to-colour resolution move to `src/sources/svg/foreign_object/` and use only `foreign_object_context` / `foreign_object_painter` / `css_query`. The element loses `html_*_color`; the core provides `css_query` (selector matching over the svg chain) and nothing else.

4. **Build:** CMake option `NOVA_FOREIGN_OBJECT_SIMPLE` (default ON, like `NOVA_EMBED_DEFAULT_FONT`). OFF compiles the core without any HTML code; the default renderer is then "none".

5. **Python:** `measure_foreign_object` calls `renderer.measure(...)`; the binding stops including private headers for this.

## Migration (each step gated: tests, golden at tolerance 0)
1. Move the HTML helpers and `foreign_object_simple` into `foreign_object/`, out of the core umbrella header.
2. Add the public interface and the painter adapter over `svg_render_state` (same calls as today, so pixels do not move).
3. Port `foreign_object_simple` onto context + painter.
4. Move the CSS colour resolution behind `css_query`; remove `html_*_color` and `set_html_style`.
5. `document::set_foreign_object_renderer`, then the CMake option.
6. Python `measure`.

## Open
- `font_metrics` and `foreign_object_size` fields: only what the simple renderer and `measure_foreign_object` use today (ascent, height, width, height).
- Whether `device_scale` should account for a transform with skew (proposal: the larger axis scale).
