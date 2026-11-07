// The built-in foreignObject renderer: the HTML inside is drawn as styled plain text, centered in the box.
// It is written against nova/svg/foreign_object.h only (context, painter, css_query), like any other
// renderer would be (ADR-0010).

#include "nova/svg/foreign_object.h"

#include "html_style.h"
#include "html_text.h"

namespace nova::svg {
using namespace vg;

namespace {

// Only understands a small, deliberately shallow slice of HTML5: it strips every tag (div/span/p/i/b/...) and
// draws the remaining text, with bold and italic runs, centered in the foreignObject's box, using the font
// inherited through the normal svg/CSS cascade. That covers what tools like Mermaid emit, without pretending
// to be a real HTML/CSS layout engine.
class foreign_object_simple final : public foreign_object_renderer {
public:
    std::shared_ptr<void> prepare(std::string_view markup, const css_query& css) const override
    {
        return std::make_shared<html_style>(resolve_html_style(markup, css));
    }

    void render(const foreign_object_context& context, foreign_object_painter& painter) const override;

    std::optional<foreign_object_size> measure(const foreign_object_context& context, const foreign_object_painter& painter,
                                               float max_width) const override;
};

unsigned char style_bits(const foreign_object_font& font_)
{
    return (unsigned char)((font_.bold ? foreign_object_fonts::bold : 0) | (font_.italic ? foreign_object_fonts::italic : 0));
}

void foreign_object_simple::render(const foreign_object_context& context, foreign_object_painter& painter) const
{
    const auto base_style = style_bits(context.font);
    auto segments = foreign_object_styled_text(std::string(context.markup), base_style);
    if(segments.empty())
        return;

    const auto fonts = foreign_object_fonts::of(context.font, painter);
    const auto& font_ = fonts.get(base_style);
    if(font_.is_null())
        return;

    const auto& box_ = context.box;
    // A browser wraps HTML text at its box's edge unless CSS says nowrap/pre.
    auto lines = layout_foreign_object_text(segments, fonts, foreign_object_allows_wrap(std::string(context.markup)) ? box_.w : 0.f);

    // Real line-height from the HTML's own `line-height:` (mermaid's content usually says 1.5) when present and
    // parseable; falls back to a 1.2x default (roughly a browser's own "normal") otherwise -- e.g. for %/em
    // values or "normal" itself, which aren't handled.
    auto line_height = foreign_object_line_height(std::string(context.markup), font_.size_()).value_or(font_.height() * 1.2f);
    auto total_height = line_height * float(lines.size());
    auto top_y = box_.y + (box_.h - total_height) / 2.f;

    const auto* style = static_cast<const html_style*>(context.prepared);
    if(style != nullptr && style->background_color)
        painter.fill_rect(box_, *style->background_color);
    const stb::color text_color = (style != nullptr && style->text_color) ? *style->text_color : stb::color(0, 0, 0);

    for(size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        auto line_width = measure_foreign_object_line(line, fonts);

        // A line is as tall as its tallest face (bold or italic can differ a little from the regular one); a line
        // of one style, the usual case, is simply that style's face.
        float ascent = font_.ascent();
        float font_height = font_.height();
        if(!line.empty()) {
            ascent = 0.f;
            font_height = 0.f;
            for(const auto& run : line) {
                ascent = STB_MAX(ascent, fonts.get(run.style).ascent());
                font_height = STB_MAX(font_height, fonts.get(run.style).height());
            }
        }

        // A real Trebuchet-MS-metrics render always fits each wrapped line with room to spare -- so overflow here
        // only ever means OUR substitute font is wider than the one mermaid measured with (this environment lacks
        // Trebuchet MS). There's no reliable way to know how much wider without the real font, so rather than
        // truncating (tried before, still didn't match a real Chromium render) we condense that one line
        // horizontally.
        float scale = 1.f;
        if(line_width > box_.w && line_width > 0.f)
            scale = box_.w / line_width;

        const float origin_x = box_.x + (box_.w - line_width * scale) / 2.f;
        // Baseline for this line = top of the text block + however many line-heights precede it + half the leading
        // (the extra space line-height adds beyond the font's own ascent+descent, split evenly above and below --
        // standard CSS line-box behavior) + this font's ascent. Without the half-leading term, text sits noticeably
        // above center (font.height() is usually well under a 1.5x line-height, so the omitted term isn't a
        // rounding-sized error).
        const float baseline = top_y + float(i) * line_height + (line_height - font_height) / 2.f + ascent;

        // Condensed about the line's own left edge so it still lands at `origin_x`; only x is scaled, so glyph
        // height (and therefore this line's vertical position) is untouched.
        text_squeeze squeeze;
        if(scale < 1.f)
            squeeze = {scale, origin_x};

        // The runs of the line side by side on the one baseline, each in its own face.
        float x = origin_x;
        for(const auto& run : line) {
            const std::u32string_view run_view(run.text);
            const auto& run_face = fonts.get(run.style);
            painter.fill_text(run_view, run_face.spec, x, baseline, text_color, squeeze);
            x += run_face.measure_text(run_view);
        }
    }
}

std::optional<foreign_object_size> foreign_object_simple::measure(const foreign_object_context& context,
                                                                  const foreign_object_painter& painter, float max_width) const
{
    const auto base_style = style_bits(context.font);
    const auto fonts = foreign_object_fonts::of(context.font, painter);
    const auto metrics = measure_foreign_object_content(context.markup, fonts, max_width, base_style);
    foreign_object_size size_;
    size_.width = metrics.width;
    size_.height = metrics.height;
    size_.line_count = metrics.line_count;
    size_.line_height = metrics.line_height;
    return size_;
}

} // namespace

std::shared_ptr<foreign_object_renderer> default_foreign_object_renderer()
{
    static const std::shared_ptr<foreign_object_renderer> renderer = std::make_shared<foreign_object_simple>();
    return renderer;
}

} // namespace nova::svg
