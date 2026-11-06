#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

svg_image_element::svg_image_element(document* dom)
    : svg_graphics_element(dom, ElementID::Image)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 100.f, LengthUnits::Percent)
    , m_preserveAspectRatio(PropertyID::PreserveAspectRatio)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
    addProperty(m_preserveAspectRatio);
}

Rect svg_image_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    const Rect viewportRect = {
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    };

    return viewportRect;
}

Rect svg_image_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

void svg_image_element::render(svg_render_state& state) const
{
    if(m_image.is_null() || isDisplayNone() || isVisibilityHidden())
        return;
    Rect dstRect(fillBoundingBox());
    Rect srcRect(0, 0, m_image.width(), m_image.height());
    if(dstRect.empty() || srcRect.empty())
        return;
    m_preserveAspectRatio.transformRect(dstRect, srcRect);

    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    newState->draw_image(m_image, dstRect, srcRect, newState.currentTransform());
    newState.endGroup(blendInfo);
}

pixmap loadImageResource(const std::string& href)
{
    if(href.compare(0, 5, "data:") == 0) {
        std::string_view input(href);
        auto index = input.find(',', 5);
        if(index == std::string_view::npos)
            return pixmap();
        input.remove_prefix(index + 1);
        return surface_load_from_image_base64(input.data(), input.length());
    }

    return surface_load_from_image_file(href.data());
}

void svg_image_element::parseAttribute(PropertyID id, const std::string& value)
{
    if(id == PropertyID::Href) {
        m_image = loadImageResource(value);
    } else {
        svg_graphics_element::parseAttribute(id, value);
    }
}

svg_foreign_object_element::svg_foreign_object_element(document* dom)
    : svg_graphics_element(dom, ElementID::ForeignObject)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow, 0.f, LengthUnits::None)
    , m_width(PropertyID::Width, LengthDirection::Horizontal, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
    , m_height(PropertyID::Height, LengthDirection::Vertical, LengthNegativeMode::Forbid, 0.f, LengthUnits::None)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_width);
    addProperty(m_height);
}

Rect svg_foreign_object_element::fillBoundingBox() const
{
    LengthContext lengthContext(this);
    return Rect(
        lengthContext.valueForLength(m_x),
        lengthContext.valueForLength(m_y),
        lengthContext.valueForLength(m_width),
        lengthContext.valueForLength(m_height)
    );
}

Rect svg_foreign_object_element::strokeBoundingBox() const
{
    return fillBoundingBox();
}

ForeignObjectFonts svg_foreign_object_element::fontsFor(unsigned usedStyles) const
{
    ForeignObjectFonts fonts;
    for(unsigned style = 0; style < 4; ++style) {
        if(!(usedStyles & (1u << style)))
            continue;
        if(style == m_baseFontStyle) {
            fonts.face[style] = m_font;
            continue;
        }
        auto variant = makeFont(m_fontFamily, m_font.size(), (style & ForeignObjectFonts::Bold) != 0, (style & ForeignObjectFonts::Italic) != 0);
        // A variant the machine can't provide at all must not blank the text: use the el's own face.
        fonts.face[style] = variant.is_null() ? m_font : variant;
    }
    return fonts;
}

void svg_foreign_object_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fontFamily = state.font_family();
    m_baseFontStyle = (unsigned char)((state.font_weight() == FontWeight::Bold ? ForeignObjectFonts::Bold : 0)
                                      | (state.font_style() == FontStyle::Italic ? ForeignObjectFonts::Italic : 0));
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    svg_graphics_element::layoutElement(state);
}

void svg_foreign_object_element::render(svg_render_state& state) const
{
    if(m_rawContent.empty() || isDisplayNone() || isVisibilityHidden())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    auto renderer = m_renderer ? m_renderer : ForeignObjectSimple::instance();
    renderer->render(this, newState);
    newState.endGroup(blendInfo);
}

ForeignObjectSimple* ForeignObjectSimple::instance()
{
    static ForeignObjectSimple s_instance;
    return &s_instance;
}

void ForeignObjectSimple::render(const svg_foreign_object_element* el, svg_render_state& state) const
{
    auto segments = foreignObjectStyledText(el->rawContent(), el->baseFontStyle());
    if(segments.empty())
        return;

    const auto& font = el->font();
    if(font.is_null())
        return;

    // One face per style that actually occurs (just font() for unstyled text, nothing else is looked up).
    const auto fonts = el->fontsFor(foreignObjectStylesUsed(segments));

    auto box = el->fillBoundingBox();
    // A browser wraps HTML text at its box's edge unless CSS says nowrap/pre.
    auto lines = layoutForeignObjectText(segments, fonts, foreignObjectAllowsWrap(el->rawContent()) ? box.w : 0.f);

    // Real line-height from the HTML's own `line-height:` (mermaid's
    // content usually says 1.5) when present and parseable; falls back
    // to a 1.2x default (roughly a browser's own "normal") otherwise --
    // e.g. for %/em values or "normal" itself, which aren't handled.
    auto lineHeight = foreignObjectLineHeight(el->rawContent(), font.size()).value_or(font.height() * 1.2f);
    auto totalHeight = lineHeight * float(lines.size());
    auto topY = box.y + (box.h - totalHeight) / 2.f;

    if(auto backgroundColor = el->htmlBackgroundColor()) {
        state->set_color(*backgroundColor);
        stb::outline backgroundPath;
        backgroundPath.add_rect(box.x, box.y, box.w, box.h);
        state->fill_path(backgroundPath, FillRule::NonZero, state.currentTransform());
    }

    state->set_color(el->htmlTextColor().value_or(stb::color(0, 0, 0)));

    for(size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        auto lineWidth = measureForeignObjectLine(line, fonts);

        // A line is as tall as its tallest face (bold or italic can differ a little from the regular one);
        // a line of one style, the usual case, is simply that style's face.
        float ascent = font.ascent();
        float fontHeight = font.height();
        if(!line.empty()) {
            ascent = 0.f;
            fontHeight = 0.f;
            for(const auto& run : line) {
                ascent = STB_MAX(ascent, fonts.get(run.style).ascent());
                fontHeight = STB_MAX(fontHeight, fonts.get(run.style).height());
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
            topY + float(i) * lineHeight + (lineHeight - fontHeight) / 2.f + ascent
        );

        auto textTransform = state.currentTransform();
        if(scale < 1.f) {
            // Condense horizontally about the line's own left edge so it
            // still lands at `origin`; only x is scaled, so glyph height
            // (and therefore this line's vertical position) is untouched.
            textTransform = textTransform * Transform::translated(origin.x, 0.f) * Transform::scaled(scale, 1.f) * Transform::translated(-origin.x, 0.f);
        }

        // The runs of the line side by side on the one baseline, each in its own face.
        float x = origin.x;
        for(const auto& run : line) {
            const std::u32string_view runView(run.text);
            const auto& runFont = fonts.get(run.style);
            state->fill_text(runView, runFont, Point(x, origin.y), textTransform);
            x += runFont.measureText(runView);
        }
    }
}

} // namespace nova::svg
