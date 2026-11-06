#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svg/svgelement.h"
#include "html_text.h"

namespace nova::svg {
using namespace vg;

foreign_object_simple* foreign_object_simple::instance()
{
    static foreign_object_simple s_instance;
    return &s_instance;
}

void foreign_object_simple::render(const svg_foreign_object_element* el, svg_render_state& state) const
{
    auto segments = foreign_object_styled_text(el->raw_content(), el->base_font_style());
    if(segments.empty())
        return;

    const auto& font = el->font();
    if(font.is_null())
        return;

    // One face per style that actually occurs (just font() for unstyled text, nothing else is looked up).
    const auto fonts = el->fonts_for(foreign_object_styles_used(segments));

    auto box = el->fill_bounding_box();
    // A browser wraps HTML text at its box's edge unless CSS says nowrap/pre.
    auto lines = layout_foreign_object_text(segments, fonts, foreign_object_allows_wrap(el->raw_content()) ? box.w : 0.f);

    // Real line-height from the HTML's own `line-height:` (mermaid's
    // content usually says 1.5) when present and parseable; falls back
    // to a 1.2x default (roughly a browser's own "normal") otherwise --
    // e.g. for %/em values or "normal" itself, which aren't handled.
    auto line_height = foreign_object_line_height(el->raw_content(), font.size()).value_or(font.height() * 1.2f);
    auto total_height = line_height * float(lines.size());
    auto top_y = box.y + (box.h - total_height) / 2.f;

    if(auto backgroundColor = el->html_background_color()) {
        state->set_color(*backgroundColor);
        stb::outline background_path;
        background_path.add_rect(box.x, box.y, box.w, box.h);
        state->fill_path(background_path, FillRule::NonZero, state.current_transform());
    }

    state->set_color(el->html_text_color().value_or(stb::color(0, 0, 0)));

    for(size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        auto lineWidth = measure_foreign_object_line(line, fonts);

        // A line is as tall as its tallest face (bold or italic can differ a little from the regular one);
        // a line of one style, the usual case, is simply that style's face.
        float ascent = font.ascent();
        float font_height = font.height();
        if(!line.empty()) {
            ascent = 0.f;
            font_height = 0.f;
            for(const auto& run : line) {
                ascent = STB_MAX(ascent, fonts.get(run.style).ascent());
                font_height = STB_MAX(font_height, fonts.get(run.style).height());
            }
        }

        // A real Trebuchet-MS-metrics render always fits each wrapped
        // line with room to spare -- so overflow here only ever means
        // OUR substitute font is wider than the one mermaid measured
        // with (this environment lacks Trebuchet MS). There's no
        // reliable way to know how much wider without the real font, so
        // rather than truncating (tried before, still didn't match a
        // real Chromium render) we condense that one line horizontally.
        float scale = 1.f;
        if(lineWidth > box.w && lineWidth > 0.f)
            scale = box.w / lineWidth;

        Point origin(
            box.x + (box.w - lineWidth * scale) / 2.f,
            // Baseline for this line = top of the text block + however
            // many line-heights precede it + half the leading (the extra
            // space line-height adds beyond the font's own ascent+descent,
            // split evenly above and below -- standard CSS line-box
            // behavior) + this font's ascent. Without the half-leading
            // term, text sits noticeably above center (font.height() is
            // usually well under a 1.5x line-height, so the omitted term
            // isn't a rounding-sized error).
            top_y + float(i) * line_height + (line_height - font_height) / 2.f + ascent
        );

        auto text_transform = state.current_transform();
        if(scale < 1.f) {
            // Condense horizontally about the line's own left edge so it
            // still lands at `origin`; only x is scaled, so glyph height
            // (and therefore this line's vertical position) is untouched.
            text_transform = text_transform * Transform::translated(origin.x, 0.f) * Transform::scaled(scale, 1.f) * Transform::translated(-origin.x, 0.f);
        }

        // The runs of the line side by side on the one baseline, each in its own face.
        float x = origin.x;
        for(const auto& run : line) {
            const std::u32string_view run_view(run.text);
            const auto& run_font = fonts.get(run.style);
            state->fill_text(run_view, run_font, Point(x, origin.y), text_transform);
            x += run_font.measure_text(run_view);
        }
    }
}

} // namespace nova::svg
