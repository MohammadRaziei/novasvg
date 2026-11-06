#pragma once

// pdf_canvas: the second backend of the abstract `canvas` (ADR-0008, step F). It writes one PDF page of vector
// drawing commands instead of pixels. 1 user unit is 1 CSS px (1/96 in), so a 100x50 canvas is a 75x37.5 pt page.
//
// Vector: solid and gradient paints, fills and strokes of paths and text (text is written as glyph outlines, so no
// font embedding), clipping, images, save/restore, and transparency groups (begin_group/end_group: opacity and
// blend mode). Raster inside the vector page: what PDF cannot express (masks, filters, clip masks) is rendered by
// the svg layer to a bitmap at raster_scale() and placed with draw_raster_group(); pattern tiles come as bitmaps too
// (set_tiled_image()).
#include "canvas.h"

#include <map>
#include <string>

namespace nova::vg {

class pdf_canvas final : public canvas {
public:
    // A page of width x height CSS px.
    static std::shared_ptr<pdf_canvas> create(float width, float height);

    void set_color(const stb::color& color) override;
    void set_color(float r, float g, float b, float a) override;
    void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;
    void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;

    void fill_path(const stb::outline& path, FillRule fill_rule, const Transform& transform) override;
    void stroke_path(const stb::outline& path, const StrokeData& stroke_data, const Transform& transform) override;

    void fill_text(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform) override;
    void stroke_text(const std::u32string_view& text, float stroke_width, const Font& font, const Point& origin, const Transform& transform) override;

    void clip_path(const stb::outline& path, FillRule clip_rule, const Transform& transform) override;
    void clip_rect(const Rect& rect, FillRule clip_rule, const Transform& transform) override;

    void draw_image(const pixmap& image, const Rect& dst_rect, const Rect& src_rect, const Transform& transform) override;

    // ---- beyond the `canvas` interface; the svg layer calls these when its target is a pdf_canvas ----

    // The page in user units: where a group's bitmap may be placed.
    Rect extents() const { return Rect(0, 0, m_width, m_height); }

    // Pixels per user unit used for the bitmaps the svg layer makes for this page (96 dpi * raster_scale()).
    float raster_scale() const { return m_raster_scale; }
    void set_raster_scale(float scale) { m_raster_scale = scale > 0.f ? scale : 1.f; }

    // Everything drawn between begin_group() and end_group() is one transparency group, composited
    // with `opacity` and `blend_mode` (Form XObject with a /Group). Groups nest.
    void begin_group();
    void end_group(BlendMode blend_mode, float opacity);

    // A finished raster group (premultiplied ARGB32) placed at `dst` (user units).
    void draw_raster_group(const pixmap& image, const Rect& dst, BlendMode blend_mode, float opacity);

    // Makes the current paint a tiled image: `tile` repeated, `transform` mapping tile pixels to user space.
    void set_tiled_image(const pixmap& tile, const Transform& transform, float opacity);

    void save() override;
    void restore() override;

    // The finished document. Closes any save() still open; the canvas can keep drawing afterwards.
    std::vector<uint8_t> data() const;
    bool write_to_file(const char* filename) const;

private:
    pdf_canvas(float width, float height);

    struct paint {
        enum class kind { solid, linear, radial, tiled } type = kind::solid;
        int tile_object = 0;
        int tile_w = 0, tile_h = 0;
        float r = 0, g = 0, b = 0, a = 1;
        float p[6] = {0, 0, 0, 0, 0, 0}; // linear: x1 y1 x2 y2; radial: cx cy r fx fy
        SpreadMethod spread = SpreadMethod::pad;
        GradientStops stops;
        matrix_t matrix = {1, 0, 0, 1, 0, 0};
    };

    struct object {
        std::string dict;   // the dictionary (or whole body when there is no stream)
        std::string stream; // empty: no stream
        bool has_stream = false;
    };

    int add_object(std::string dict);
    int add_stream(std::string dict, const std::string& bytes, bool compress);
    void begin_paint(bool stroke, const matrix_t& transform);
    void emit_path(const stb::outline& path, const matrix_t* transform);
    void emit_rect_path(const Rect& rect, const matrix_t* transform);
    void emit_text_path(const std::u32string_view& text, const Font& font, const Point& origin, const matrix_t* transform);
    std::string alpha_state(float alpha, BlendMode blend_mode = BlendMode::src_over);
    int shading_pattern(const matrix_t& transform);
    int tiling_pattern(const matrix_t& transform);
    // Adds `image` as an Image XObject (Flate RGB, plus an SMask when it has alpha). Returns its object number.
    int add_image(const pixmap& image, int* width, int* height);
    void place_image(int object, int width, int height, const Rect& dst, const Rect& src, const matrix_t* transform, float alpha, BlendMode blend_mode);

    float m_width, m_height;
    std::vector<object> m_objects; // object number = index + 1
    std::string m_content;
    std::vector<paint> m_paints;   // top = current paint; one entry per open save()
    int m_open_saves = 0;
    struct ext_gstate {
        std::string name;
        std::string alpha;
        std::string blend;
    };
    std::map<std::string, ext_gstate> m_ext_gstates; // key: alpha + blend mode
    float m_raster_scale = 1.f;
    struct group_frame {
        std::string outer_content; // what was being written before begin_group()
        int open_saves;
    };
    std::vector<group_frame> m_groups;
    std::vector<std::pair<std::string, int>> m_patterns;  // resource name -> object
    std::vector<std::pair<std::string, int>> m_xobjects;  // resource name -> object
};

} // namespace nova::vg
