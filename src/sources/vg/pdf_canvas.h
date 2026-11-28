#pragma once

// pdf_canvas: the second backend of the abstract `canvas` (ADR-0008, step F). It writes one PDF page of vector
// drawing commands instead of pixels. 1 user unit is 1 CSS px (1/96 in), so a 100x50 canvas is a 75x37.5 pt page.
//
// Vector: solid and gradient paints, fills and strokes of paths and text (text is written as glyph outlines by default;
// text_embedding says when it is real text in an embedded subset of the font instead, ADR-0019), clipping, images, save/restore, and transparency groups (a layer that needs no pixels: opacity and
// blend mode, and a mask: layer_params::mask is a soft mask of the page, a luminosity one or an alpha one). Raster
// inside the vector page: a layer that needs pixels (filters, and masks that cannot stay a vector) is drawn to a
// bitmap at raster_scale(), run through the caller's effects and placed as an image; pattern tiles come as bitmaps
// too (set_pattern()).
#include "canvas.h"

#include <array>
#include <map>
#include <set>
#include <string>

namespace nova::vg {

class pdf_canvas final : public canvas {
public:
    // A page of width x height CSS px.
    static std::shared_ptr<pdf_canvas> create(float width, float height);

    void set_color(const color& color_) override;
    void set_color(float r, float g, float b, float a) override;
    void set_linear_gradient(float x1, float y1, float x2, float y2, spread_method_value spread, const gradient_stops& stops, const transform& transform_) override;
    void set_radial_gradient(float cx, float cy, float r, float fx, float fy, spread_method_value spread, const gradient_stops& stops, const transform& transform_) override;

    void fill_path(const outline& path_, fill_rule fill_rule_, const transform& transform_) override;
    void stroke_path(const outline& path_, const stroke_data& stroke_data_, const transform& transform_) override;

    void fill_text(const std::u32string_view& text, const sized_font& font_, const point& origin, const transform& transform_, const shaping_context& context = {}) override;
    void stroke_text(const std::u32string_view& text, float stroke_width, const sized_font& font_, const point& origin, const transform& transform_, const shaping_context& context = {}) override;

    void clip_path(const outline& path_, fill_rule clip_rule, const transform& transform_) override;
    void clip_rect(const rect& rect_, fill_rule clip_rule, const transform& transform_) override;

    void draw_image(const pixmap& image, const rect& dst_rect, const rect& src_rect, const transform& transform_) override;

    // `tile` repeated as a PDF tiling pattern, `tile_to_user` mapping tile pixels to user space.
    void set_pattern(const pixmap& tile, const transform& tile_to_user, float opacity) override;

    // A layer that needs no pixels is a transparency group of the page (a Form XObject with a /Group), composited with
    // the layer's opacity and blend mode, and through its mask when it has one (see vector_masks()). One that needs
    // pixels is a bitmap at raster_scale(): it goes through the effects and lands on the page as an image.
    bool vector_masks() const override { return true; }
    layer_scope begin_layer(const layer_params& params) override;
    void end_layer(layer_scope&& scope, const layer_effects& effects = {}) override;

    // Pixels per user unit of the bitmaps made for this page (pattern tiles, layers with effects): 1 is 96 dpi.
    float raster_scale() const override { return m_raster_scale; }
    void set_raster_scale(float scale) { m_raster_scale = scale > 0.f ? scale : 1.f; }

    // The page in user units.
    rect extents() const { return rect(0, 0, m_width, m_height); }

    // How text goes into the file. outlines (the default): glyph outlines, plus the invisible text layer (ADR-0018).
    // embed: real text, shown with a subset of the font embedded in the file (the font's own glyphs, shaped by libstb,
    // one per Tm), unless the font's license (OS/2 fsType) forbids embedding or it cannot be subset (CFF, a variable
    // instance): that text stays outlines and a warning is logged, once per font. embed_ignore_license: the same, but the
    // license bits are not read; whoever asks for this answers for the license.
    enum class text_embedding { outlines, embed, embed_ignore_license };
    void set_text_embedding(text_embedding mode) { m_text_embedding = mode; }

    // What the PDF says about itself (its /Info entry). Fields left empty are not written; the file has no date, so
    // the same drawing always gives the same bytes.
    struct info {
        std::string title;
        std::string producer;
    };
    void set_info(const info& document_info) { m_info = document_info; }

    void save() override;
    void restore() override;

    // The finished document. Closes any save() still open; the canvas can keep drawing afterwards.
    std::vector<uint8_t> data() const;
    bool write_to_file(const char* filename) const;

private:
    pdf_canvas(float width, float height);

    // Everything drawn between begin_group() and end_group() is one transparency group.
    void begin_group();
    // Closes the group begun last and returns its Form XObject; the canvas goes back to the content it had.
    int finish_group();
    // finish_group(), then the form is drawn onto the page with the opacity and blend mode, through `soft_mask` (the
    // object of a mask group, 0 for none) read as luminosity, or as alpha when `mask_alpha`.
    void end_group(blend_mode blend_mode_, float opacity, int soft_mask = 0, bool mask_alpha = false);
    // The invisible text layer (text render mode 3): `text` once more, as characters, placed on the outlines that
    // fill_text() drew, so that the page's text can be selected, searched and extracted. A font without glyphs: the
    // viewer draws nothing, the font only says which Unicode character each code is (/ToUnicode).
    void emit_text_layer(const std::u32string_view& text, const sized_font& font_, const point& origin,
                         const stb::matrix& transform_, const shaping_context& context);
    // Real text in an embedded font (see text_embedding): true if `text` was written that way, false if the caller must
    // draw outlines (mode outlines, or a font that cannot be embedded). `stroke_width` null: a fill, else a stroke.
    bool emit_embedded_text(const std::u32string_view& text, const sized_font& font_, const point& origin, const transform& transform_,
                            const shaping_context& context, const float* stroke_width);
    // The embedded font of `face` (the first time: decided and, when refused, logged), null if its text stays outlines.
    struct embedded_font;
    embedded_font* embedded_font_for(const font_face& face);
    // true when this run, put at this place with this matrix and size, is the one written last (a text that is both
    // filled and stroked is one text, not two); the run is remembered.
    bool repeats_last_text(const std::u32string_view& text, const point& origin, const stb::matrix& m, float size);
    // `mask` painted as a group of its own: the group that a soft mask reads. Returns its Form XObject.
    int mask_group(const layer_mask& mask);
    // A finished bitmap (premultiplied ARGB32) placed at `dst` (user units).
    void draw_raster_group(const pixmap& image, const rect& dst, blend_mode blend_mode_, float opacity);

    struct paint {
        enum class kind { solid, linear, radial, tiled } type_ = kind::solid;
        int tile_object = 0;
        int tile_w = 0, tile_h = 0;
        float r = 0, g = 0, b = 0, a = 1;
        float p[6] = {0, 0, 0, 0, 0, 0}; // linear: x1 y1 x2 y2; radial: cx cy r fx fy
        spread_method_value spread = spread_method_value::pad;
        gradient_stops stops;
        stb::matrix matrix = {1, 0, 0, 1, 0, 0};
    };

    struct object {
        std::string dict;   // the dictionary (or whole body when there is no stream)
        std::string stream; // empty: no stream
        bool has_stream = false;
    };

    int add_object(std::string dict);
    int add_stream(std::string dict, const std::string& bytes, bool compress);
    void begin_paint(bool stroke, const stb::matrix& transform_);
    void emit_path(const outline& path_, const stb::matrix* transform_);
    void emit_rect_path(const rect& rect_, const stb::matrix* transform_);
    void emit_text_path(const std::u32string_view& text, const sized_font& font_, const point& origin, const stb::matrix* transform_, const shaping_context& context);
    // `soft_mask` is the object number of a luminosity soft mask (see fade_mask), or 0 for none: every state names its
    // mask, because an ExtGState that does not would leave the previous one in force.
    std::string alpha_state(float alpha, blend_mode blend_mode_ = blend_mode::normal, int soft_mask = 0, bool mask_alpha = false);
    // `in_form` is for a pattern used inside a Form XObject: its matrix then maps to the form's space, which is the
    // pixel space the page content already works in, not to the page's own (point, y up) space.
    int shading_pattern(const stb::matrix& transform_, const gradient_stops& stops, bool in_form = false);
    // The alpha of a gradient whose stops differ (a fade) as a soft mask: the same gradient geometry painted in
    // grey levels equal to the stop alphas, in a transparency group that /SMask reads as luminosity.
    int fade_mask(const stb::matrix& transform_);
    int tiling_pattern(const stb::matrix& transform_, bool in_form = false);
    // Adds `image` as an Image XObject (Flate RGB, plus an SMask when it has alpha). Returns its object number.
    int add_image(const pixmap& image, int* width, int* height);
    void place_image(int object, int width, int height, const rect& dst, const rect& src, const stb::matrix* transform_, float alpha, blend_mode blend_mode_);

    float m_width, m_height;
    info m_info;
    std::vector<object> m_objects; // object number = index + 1
    std::string m_content;
    std::vector<paint> m_paints;   // top = current paint; one entry per open save()
    int m_open_saves = 0;
    struct ext_gstate {
        std::string name;
        std::string alpha;
        std::string blend;
        int soft_mask = 0;
        bool mask_alpha = false; // the soft mask is read as alpha, not as luminosity
    };
    std::map<std::string, ext_gstate> m_ext_gstates; // key: alpha + blend mode + soft mask + how it is read
    float m_raster_scale = 1.f;
    struct group_frame {
        std::string outer_content; // what was being written before begin_group()
        int open_saves;
    };
    std::vector<group_frame> m_groups;
    std::vector<std::pair<std::string, int>> m_patterns;  // resource name -> object
    std::vector<std::pair<std::string, int>> m_xobjects;  // resource name -> object
    text_embedding m_text_embedding = text_embedding::outlines;
    struct embedded_font {
        font_face face;                              // keeps the font alive; the key is face.get()
        bool usable = false;                         // false: refused, its text is outlines
        std::string name;                            // resource name /FE0, /FE1, ...
        std::set<std::uint32_t> glyphs;              // the glyphs used; a glyph's index is its CID
        std::map<std::uint32_t, char32_t> unicode;   // CID -> a character it stands for (the /ToUnicode of the font)
    };
    std::vector<embedded_font> m_embedded;           // in order of first use
    std::map<char32_t, int> m_text_cids;  // the invisible text layer: code point -> CID, 1.. in order of first use
    int m_mask_depth = 0;                 // > 0 while a mask group is painted: its text is not part of the page's
    std::u32string m_last_text;           // the run of the last text layer and where it was put: a fill and a stroke of
    std::array<float, 9> m_last_text_at{};  // the same run (either order) are the one text, not two
};

} // namespace nova::vg
