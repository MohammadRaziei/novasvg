#pragma once

// pdf_canvas: the second backend of the abstract `canvas` (ADR-0008, step F). It writes one PDF page of vector
// drawing commands instead of pixels. 1 user unit is 1 CSS px (1/96 in), so a 100x50 canvas is a 75x37.5 pt page.
//
// What it can do today is exactly what the `canvas` interface expresses: solid and gradient paints, fills and
// strokes of paths and text (text is written as glyph outlines, so no font embedding), clipping, images and
// save/restore. Not yet: layers (group opacity, blend modes), masks, patterns and filters; those need the
// interface to grow first (see REFACTOR_CHECKLIST.md, C2).
#include "gfx/canvas.h"

#include <map>
#include <string>

namespace novasvg {

class pdf_canvas final : public canvas {
public:
    // A page of width x height CSS px.
    static std::shared_ptr<pdf_canvas> create(float width, float height);

    void set_color(const Color& color) override;
    void set_color(float r, float g, float b, float a) override;
    void set_linear_gradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;
    void set_radial_gradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform) override;

    void fill_path(const Path& path, FillRule fill_rule, const Transform& transform) override;
    void stroke_path(const Path& path, const StrokeData& stroke_data, const Transform& transform) override;

    void fill_text(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform) override;
    void stroke_text(const std::u32string_view& text, float stroke_width, const Font& font, const Point& origin, const Transform& transform) override;

    void clip_path(const Path& path, FillRule clip_rule, const Transform& transform) override;
    void clip_rect(const Rect& rect, FillRule clip_rule, const Transform& transform) override;

    void draw_image(const Bitmap& image, const Rect& dst_rect, const Rect& src_rect, const Transform& transform) override;

    void save() override;
    void restore() override;

    // The finished document. Closes any save() still open; the canvas can keep drawing afterwards.
    std::vector<uint8_t> data() const;
    bool write_to_file(const char* filename) const;

private:
    pdf_canvas(float width, float height);

    struct paint {
        enum class kind { solid, linear, radial } type = kind::solid;
        float r = 0, g = 0, b = 0, a = 1;
        float p[6] = {0, 0, 0, 0, 0, 0}; // linear: x1 y1 x2 y2; radial: cx cy r fx fy
        SpreadMethod spread = SpreadMethod::Pad;
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
    void emit_path(const Path& path, const matrix_t* transform);
    void emit_rect_path(const Rect& rect, const matrix_t* transform);
    void emit_text_path(const std::u32string_view& text, const Font& font, const Point& origin, const matrix_t* transform);
    std::string alpha_state(float alpha);
    int shading_pattern(const matrix_t& transform);

    float m_width, m_height;
    std::vector<object> m_objects; // object number = index + 1
    std::string m_content;
    std::vector<paint> m_paints;   // top = current paint; one entry per open save()
    int m_open_saves = 0;
    std::map<std::string, std::string> m_ext_gstates; // key "0.5000" -> resource name
    std::vector<std::pair<std::string, int>> m_patterns;  // resource name -> object
    std::vector<std::pair<std::string, int>> m_xobjects;  // resource name -> object
};

} // namespace novasvg
