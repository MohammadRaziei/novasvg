#pragma once

// The HTML subset a <foreignObject> understands: parser, wrapping, measuring. Part of the built-in simple
// renderer (foreign_object/simple.cpp); the core does not include it (ADR-0010).

#include "utils/text.h"
#include "../svg_css_utils.h"
#include "nova/svg/foreign_object.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

/// One face as the renderer sees it: a font spec, measured through the painter.
struct foreign_object_face {
    const foreign_object_painter* painter = nullptr;
    foreign_object_font spec;

    /// No usable face (the machine has no font at all): text in it is not drawn.
    bool is_null() const { return painter == nullptr || painter->metrics(spec).height <= 0.f; }
    float size_() const { return spec.size; }
    float ascent() const { return painter->metrics(spec).ascent; }
    float height() const { return painter->metrics(spec).height; }
    float measure_text(std::u32string_view text) const { return painter->measure_text(text, spec); }
};

/// The faces a <foreignObject> can paint with: the inherited one plus the bold / italic variants that <b>,
/// <strong>, <i>, <em> and inline `font-weight` / `font-style` ask for. Indexed by style bits (bit 0 bold,
/// bit 1 italic). What a variant looks like is up to the painter.
struct foreign_object_fonts {
    static constexpr unsigned char bold = 1;
    static constexpr unsigned char italic = 2;

    foreign_object_face face[4];
    const foreign_object_face& get(unsigned char style) const { return face[style & 3]; }

    static foreign_object_fonts of(const foreign_object_font& base, const foreign_object_painter& painter)
    {
        foreign_object_fonts fonts;
        for(unsigned style = 0; style < 4; ++style) {
            fonts.face[style].painter = &painter;
            fonts.face[style].spec = base;
            fonts.face[style].spec.bold = (style & bold) != 0;
            fonts.face[style].spec.italic = (style & italic) != 0;
        }

        return fonts;
    }
};

// ---------------------------------------------------------------
// svg_foreign_object_element / ForeignObjectSimple
// ---------------------------------------------------------------



inline std::optional<std::string_view> html_attribute(std::string_view tag, std::string_view name)
{
    auto pos = tag.find(name);
    if(pos == std::string_view::npos)
        return std::nullopt;
    auto value_start = pos + name.size();
    if(value_start >= tag.size() || (tag[value_start] != '"' && tag[value_start] != '\''))
        return std::nullopt;
    auto quote = tag[value_start];
    ++value_start;
    auto value_end = tag.find(quote, value_start);
    if(value_end == std::string_view::npos)
        return std::nullopt;
    return tag.substr(value_start, value_end - value_start);
}

// ---------------------------------------------------------------------------------------------------
// <foreignObject> text as runs of one style each.
//
// Roughly "element.innerText" for the tiny subset of HTML this cares about, but keeping what a reader
// can SEE of the markup: which stretches are bold and which are italic. Deliberately not a real HTML
// engine -- no box model, no per-tag display, margins or sizes -- because mermaid (and most
// svg-generating tools) only put one short run of text, perhaps with <br>, <b>/<strong> and <i>/<em>
// in it.
//
// Style bits (ForeignObjectFonts::Bold / Italic) come from, in this order:
//   * the style the foreignObject itself inherits from the svg around it (the `baseStyle` argument);
//   * the tag: <b> <strong> = bold, <i> <em> <cite> <var> <dfn> = italic;
//   * an inline style="font-weight: ..." / style="font-style: ...", which wins over the tag and may
//     also turn a style OFF again (`font-weight: normal` inside a bold stretch).
// Styles nest and are inherited by everything inside the el, and end with its closing tag.
// ---------------------------------------------------------------------------------------------------
struct foreign_object_segment {
    std::string text; ///< UTF-8; '\n' marks a forced line break (<br>, <p>, <div>, ...)
    unsigned char style = 0;
};

inline bool foreign_object_is_void_tag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"area", "base", "br", "col", "embed", "hr", "img",
                                                    "input", "link", "meta", "source", "track", "wbr"};
    return tags.count(tag) > 0;
}

// Tags that sit INSIDE a line of text, so a boundary with them is not a boundary between words:
// "un<b>believ</b>able" is one word. Every tag NOT in this set, and not a block tag, still counts as
// a word boundary (a space), exactly as before styles existed -- see foreignObjectStyledText().
inline bool foreign_object_is_inline_tag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"a", "abbr", "b", "bdi", "bdo", "big", "cite", "code", "data", "del",
                                                    "dfn", "em", "font", "i", "ins", "kbd", "label", "mark", "q", "s",
                                                    "samp", "small", "span", "strike", "strong", "sub", "sup", "time",
                                                    "tt", "u", "var"};
    return tags.count(tag) > 0;
}

// The value of the style="..." attribute inside a tag's text (after its name), or empty.
inline std::string_view foreign_object_style_attribute(std::string_view attributes)
{
    size_t pos = 0;
    while(pos < attributes.size()) {
        while(pos < attributes.size() && (utils::is_space(attributes[pos]) || attributes[pos] == '/'))
            ++pos;
        auto name_start = pos;
        while(pos < attributes.size() && !utils::is_space(attributes[pos]) && attributes[pos] != '=' && attributes[pos] != '/')
            ++pos;
        auto name = attributes.substr(name_start, pos - name_start);
        while(pos < attributes.size() && utils::is_space(attributes[pos]))
            ++pos;
        std::string_view value;
        if(pos < attributes.size() && attributes[pos] == '=') {
            ++pos;
            while(pos < attributes.size() && utils::is_space(attributes[pos]))
                ++pos;
            if(pos < attributes.size() && (attributes[pos] == '"' || attributes[pos] == '\'')) {
                auto quote = attributes[pos++];
                auto end = attributes.find(quote, pos);
                value = attributes.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
                pos = end == std::string_view::npos ? attributes.size() : end + 1;
            } else {
                auto value_start = pos;
                while(pos < attributes.size() && !utils::is_space(attributes[pos]))
                    ++pos;
                value = attributes.substr(value_start, pos - value_start);
            }
        }
        if(name.size() == 5 && (name[0] | 0x20) == 's' && (name[1] | 0x20) == 't' && (name[2] | 0x20) == 'y' && (name[3] | 0x20) == 'l' && (name[4] | 0x20) == 'e')
            return value;
    }
    return {};
}

// The lower-cased value of one declaration, without a trailing `!important`; empty if it is absent.
inline std::string foreign_object_declaration(std::string_view block, std::string_view property)
{
    auto value = find_declaration_value(block, property);
    if(!value)
        return {};
    std::string result = utils::to_lower(*value);
    if(auto bang = result.find('!'); bang != std::string::npos)
        result.erase(bang);
    result = std::string(utils::rtrim(result));
    return result;
}

// The style in effect inside an el that is being opened.
inline unsigned char foreign_object_style_of_opening(const std::string& tag, std::string_view attributes, unsigned char inherited)
{
    unsigned char style = inherited;
    if(tag == "b" || tag == "strong")
        style |= foreign_object_fonts::bold;
    else if(tag == "i" || tag == "em" || tag == "cite" || tag == "var" || tag == "dfn")
        style |= foreign_object_fonts::italic;

    auto inline_style = foreign_object_style_attribute(attributes);
    if(!inline_style.empty()) {
        if(auto weight = foreign_object_declaration(inline_style, "font-weight"); !weight.empty()) {
            style &= (unsigned char)~foreign_object_fonts::bold;
            if(parse_font_weight(weight) == font_weight::bold)
                style |= foreign_object_fonts::bold;
        }
        if(auto font_style_ = foreign_object_declaration(inline_style, "font-style"); !font_style_.empty()) {
            style &= (unsigned char)~foreign_object_fonts::italic;
            if(parse_font_style(font_style_) == font_style::italic)
                style |= foreign_object_fonts::italic;
        }
    }
    return style;
}

inline std::vector<foreign_object_segment> foreign_object_styled_text(std::string_view html, unsigned char base_style = 0)
{
    // Tag names that force a line break, matching their real-browser default of `display: block` (so a
    // `<br>`/`<p>`/`<div>` boundary becomes '\n', preserved through the whitespace-collapse pass below,
    // instead of collapsing into an ordinary space like other tags do). Mermaid's own labels are
    // `white-space: nowrap` by design (each is laid out to fit on one line at the font it expects) and
    // only ever span multiple lines where it inserted an explicit `<br>` itself at a deliberate break
    // point -- so honoring just these forced breaks reproduces real multi-line labels without needing
    // general width-based word-wrap.
    static const std::set<std::string_view> block_tags = {"br", "p", "div", "tr", "li"};

    std::vector<foreign_object_segment> raw;
    auto put = [&](char c, unsigned char style) {
        if(raw.empty() || raw.back().style != style)
            raw.push_back({std::string(), style});
        raw.back().text += c;
    };

    struct open_element {
        std::string tag;
        unsigned char style;
    };
    std::vector<open_element> open;
    unsigned char style = base_style;

    for(size_t i = 0; i < html.size();) {
        char c = html[i];

        if(c == '<') {
            if(html.compare(i, 4, "<!--") == 0) {
                auto end = html.find("-->", i + 4);
                i = end == std::string_view::npos ? html.size() : end + 3;
                put(' ', style);
                continue;
            }

            // the end of the tag: the first '>' that is not inside a quoted attribute value
            size_t gt = i + 1;
            char quote = 0;
            while(gt < html.size() && (quote || html[gt] != '>')) {
                if(quote) {
                    if(html[gt] == quote)
                        quote = 0;
                } else if(html[gt] == '"' || html[gt] == '\'') {
                    quote = html[gt];
                }
                ++gt;
            }
            auto inside = html.substr(i + 1, gt - i - 1);
            i = STB_MIN(gt + 1, html.size());

            const bool closing = !inside.empty() && inside.front() == '/';
            const bool self_closing = !inside.empty() && inside.back() == '/';
            auto rest = closing ? inside.substr(1) : inside;
            size_t name_length = 0;
            while(name_length < rest.size() && std::isalnum(static_cast<unsigned char>(rest[name_length])))
                ++name_length;
            std::string name(rest.substr(0, name_length));
            for(auto& ch : name)
                ch = utils::to_lower(ch);

            // Every tag boundary is a word boundary (a space), so "Hello<br/>world" and "a</td><td>b"
            // never glue together; block tags force a line break instead; and the tags that sit inside
            // a line of text (<b>, <span>, ...) are no boundary at all.
            auto boundary = [&](unsigned char at) {
                if(block_tags.count(name))
                    put('\n', at);
                else if(!foreign_object_is_inline_tag(name))
                    put(' ', at);
            };

            if(closing) {
                for(size_t k = open.size(); k-- > 0;) {
                    if(open[k].tag == name) {
                        open.resize(k);
                        break;
                    }
                }
                style = open.empty() ? base_style : open.back().style;
                boundary(style);
            } else {
                boundary(style);
                if(!self_closing && !foreign_object_is_void_tag(name)) {
                    style = foreign_object_style_of_opening(name, rest.substr(name_length), style);
                    open.push_back({name, style});
                }
            }
            continue;
        }

        if(c == '&') {
            if(html.compare(i, 5, "&amp;") == 0) { put('&', style); i += 5; continue; }
            if(html.compare(i, 4, "&lt;") == 0) { put('<', style); i += 4; continue; }
            if(html.compare(i, 4, "&gt;") == 0) { put('>', style); i += 4; continue; }
            if(html.compare(i, 6, "&quot;") == 0) { put('"', style); i += 6; continue; }
            if(html.compare(i, 6, "&apos;") == 0) { put('\'', style); i += 6; continue; }
            if(html.compare(i, 6, "&nbsp;") == 0) { put(' ', style); i += 6; continue; }
        }

        put(c, style);
        ++i;
    }

    // Collapse runs of ordinary whitespace into a single space (trimming leading/trailing) -- across
    // run boundaries, so "<b>a </b> b" has one space, not two -- but '\n' is preserved as its own
    // line-break marker rather than being folded into that, and spaces immediately touching a '\n' are
    // dropped rather than kept as a redundant space before/after the break.
    std::vector<foreign_object_segment> collapsed;
    auto append = [&](char c, unsigned char at) {
        if(collapsed.empty() || collapsed.back().style != at)
            collapsed.push_back({std::string(), at});
        collapsed.back().text += c;
    };
    auto drop_last = [&]() {
        collapsed.back().text.pop_back();
        if(collapsed.back().text.empty())
            collapsed.pop_back();
    };
    bool last_was_space = true; // trims leading whitespace too
    for(const auto& segment : raw) {
        for(char c : segment.text) {
            if(c == '\t' || c == '\r')
                c = ' ';
            if(c == '\n') {
                while(!collapsed.empty() && collapsed.back().text.back() == ' ')
                    drop_last();
                append('\n', segment.style);
                last_was_space = true;
                continue;
            }
            if(c == ' ') {
                if(last_was_space)
                    continue;
                last_was_space = true;
            } else {
                last_was_space = false;
            }
            append(c, segment.style);
        }
    }
    while(!collapsed.empty() && (collapsed.back().text.back() == ' ' || collapsed.back().text.back() == '\n'))
        drop_last();
    return collapsed;
}

// The same text with every style dropped: the plain "innerText" of the HTML ('\n' at forced breaks).
inline std::string foreign_object_plain_text(std::string_view html)
{
    std::string text;
    for(const auto& segment : foreign_object_styled_text(html))
        text += segment.text;
    return text;
}

// Real line-height for foreignObject text, read from the HTML's own
// inline style="line-height:..." (mermaid always sets this directly on
// the wrapping <div> -- never via a class the way it sometimes does for
// color/background-color -- so this only needs to check inline style=,
// not walk the stylesheet the way resolveForeignObjectStyle() does).
// Handles a bare multiplier ("1.5", the common case, including
// mermaid's own default) or an absolute "Npx" length; anything else
// (%, em, "normal", ...) or no line-height at all falls back to the
// caller's own default.
inline std::optional<float> foreign_object_line_height(std::string_view raw_html, float font_size)
{
    size_t pos = 0;
    while(pos < raw_html.size()) {
        auto tag_start = raw_html.find('<', pos);
        if(tag_start == std::string_view::npos)
            break;
        if(tag_start + 1 < raw_html.size() && raw_html[tag_start + 1] == '/') {
            pos = tag_start + 2;
            continue;
        }
        auto tag_end = raw_html.find('>', tag_start);
        if(tag_end == std::string_view::npos)
            break;
        auto tag = raw_html.substr(tag_start, tag_end - tag_start);
        if(auto style = html_attribute(tag, "style=")) {
            if(auto declaration = parse_numeric_declaration(*style, "line-height"))
                return declaration->is_pixels ? declaration->raw : declaration->raw * font_size;
        }
        pos = tag_end + 1;
    }

    return std::nullopt;
}

// Whether the HTML's own CSS lets its text wrap at the box edge. Decided
// exactly like a browser would: the OUTERMOST el that declares
// `white-space` wins, and `nowrap` / `pre` mean "never soft-wrap" -- that is
// what mermaid's default labels say (`display:table-cell; white-space:nowrap`),
// so those keep laying out on one line (an overflowing line is handled by
// render()'s own horizontal condense, unchanged). Anything else --
// `normal`, `break-spaces` (mermaid's wrap mode: `display:table;
// white-space:break-spaces; width:200px`), `pre-wrap`, `pre-line` -- may
// wrap. No `white-space` declared at all is CSS's initial value, `normal`.
inline bool foreign_object_allows_wrap(std::string_view raw_html)
{
    size_t pos = 0;
    while(pos < raw_html.size()) {
        auto tag_start = raw_html.find('<', pos);
        if(tag_start == std::string_view::npos)
            break;
        if(tag_start + 1 < raw_html.size() && raw_html[tag_start + 1] == '/') {
            pos = tag_start + 2;
            continue;
        }
        auto tag_end = raw_html.find('>', tag_start);
        if(tag_end == std::string_view::npos)
            break;
        auto tag = raw_html.substr(tag_start, tag_end - tag_start);
        if(auto style = html_attribute(tag, "style=")) {
            if(auto value = find_declaration_value(*style, "white-space"))
                return !(*value == "nowrap" || *value == "pre");
        }
        pos = tag_end + 1;
    }
    return true;
}

// The px width the HTML itself constrains its text to, when it says so:
// the outermost el declaring `width` and/or `max-width` in px (both ->
// the smaller, as in CSS). Only px is understood; anything else (%, em, auto)
// means "no declared width" and the caller falls back to the box it has.
inline std::optional<float> foreign_object_declared_width(std::string_view raw_html)
{
    size_t pos = 0;
    while(pos < raw_html.size()) {
        auto tag_start = raw_html.find('<', pos);
        if(tag_start == std::string_view::npos)
            break;
        if(tag_start + 1 < raw_html.size() && raw_html[tag_start + 1] == '/') {
            pos = tag_start + 2;
            continue;
        }
        auto tag_end = raw_html.find('>', tag_start);
        if(tag_end == std::string_view::npos)
            break;
        auto tag = raw_html.substr(tag_start, tag_end - tag_start);
        if(auto style = html_attribute(tag, "style=")) {
            std::optional<float> width;
            if(auto d = parse_numeric_declaration(*style, "width"); d && d->is_pixels)
                width = d->raw;
            if(auto d = parse_numeric_declaration(*style, "max-width"); d && d->is_pixels)
                width = width ? STB_MIN(*width, d->raw) : d->raw;
            if(width)
                return width;
        }
        pos = tag_end + 1;
    }
    return std::nullopt;
}

// Greedy word-wrap of ONE paragraph to `maxWidth`: a line takes the next word
// while the whole candidate line ("a b c", measured as one string so kerning
// across the space counts, like a browser's shaping) still fits. A single
// word wider than maxWidth stays alone on an overflowing line -- CSS never
// breaks inside a word by default; render() condenses that one line.
inline void wrap_foreign_object_paragraph(const std::u32string& paragraph, const foreign_object_face& font_, float max_width, std::vector<std::u32string>& lines)
{
    // Browsers lay out in 1/64px units, so "fits" tolerates that much slack.
    const float limit = max_width + 1.f / 128.f;
    std::u32string current;
    size_t i = 0;
    while(i < paragraph.size()) {
        while(i < paragraph.size() && (paragraph[i] == U' ' || paragraph[i] == U'\t'))
            ++i;
        auto word_end = i;
        while(word_end < paragraph.size() && paragraph[word_end] != U' ' && paragraph[word_end] != U'\t')
            ++word_end;
        if(word_end == i)
            break;
        auto word = paragraph.substr(i, word_end - i);
        i = word_end;

        if(current.empty()) {
            current = std::move(word);
            continue;
        }
        auto candidate = current;
        candidate += U' ';
        candidate += word;
        if(font_.measure_text(std::u32string_view(candidate)) <= limit) {
            current = std::move(candidate);
        } else {
            lines.push_back(std::move(current));
            current = std::move(word);
        }
    }
    if(!current.empty())
        lines.push_back(std::move(current));
}

// Splits `text` (already using '\n' for the forced breaks
// foreignObjectPlainText() inserts at <br>/<p>/<div> boundaries) into lines.
// With maxWidth <= 0 (the caller's HTML is `white-space: nowrap`/`pre`, or
// there is no width to wrap at) it only splits on those forced breaks, as
// mermaid's nowrap labels want. With a positive maxWidth each paragraph is
// additionally greedy word-wrapped to it, using the same Font::measureText()
// the paint path uses.
inline std::vector<std::u32string> wrap_foreign_object_text(const std::string& text, const foreign_object_face* font_ = nullptr, float max_width = 0.f)
{
    std::vector<std::u32string> lines;
    const bool soft_wrap = font_ != nullptr && !font_->is_null() && max_width > 0.f;
    size_t pos = 0;
    while(pos <= text.size()) {
        auto nl = text.find('\n', pos);
        auto paragraph = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if(paragraph.empty())
            continue;
        if(soft_wrap)
            wrap_foreign_object_paragraph(utf8_to_u32(paragraph), *font_, max_width, lines);
        else
            lines.push_back(utf8_to_u32(paragraph));
    }

    if(lines.empty())
        lines.emplace_back();
    return lines;
}

// One stretch of a line in a single style; a line is a few of them in a row.
struct foreign_object_run {
    std::u32string text;
    unsigned char style = 0;
};
using foreign_object_line = std::vector<foreign_object_run>;

// The text of a segment list split at its forced breaks ('\n'), each paragraph as segments again.
inline std::vector<std::vector<foreign_object_segment>> split_foreign_object_paragraphs(const std::vector<foreign_object_segment>& segments)
{
    std::vector<std::vector<foreign_object_segment>> paragraphs(1);
    for(const auto& segment : segments) {
        size_t pos = 0;
        while(pos <= segment.text.size()) {
            auto newline = segment.text.find('\n', pos);
            auto piece = segment.text.substr(pos, newline == std::string::npos ? std::string::npos : newline - pos);
            if(!piece.empty())
                paragraphs.back().push_back({std::move(piece), segment.style});
            if(newline == std::string::npos)
                break;
            paragraphs.emplace_back();
            pos = newline + 1;
        }
    }
    return paragraphs;
}

// Width of text whose characters each have a style: every stretch of one style is measured on its own
// with that style's face (kerning is not applied ACROSS a style change, which no face could do anyway).
inline float measure_foreign_object_cells(const std::u32string& chars, const std::vector<unsigned char>& styles, const foreign_object_fonts& fonts)
{
    float width = 0.f;
    size_t begin = 0;
    while(begin < chars.size()) {
        size_t end = begin + 1;
        while(end < chars.size() && styles[end] == styles[begin])
            ++end;
        width += fonts.get(styles[begin]).measure_text(std::u32string_view(chars).substr(begin, end - begin));
        begin = end;
    }
    return width;
}

inline foreign_object_line make_foreign_object_line(const std::u32string& chars, const std::vector<unsigned char>& styles)
{
    foreign_object_line line;
    for(size_t i = 0; i < chars.size(); ++i) {
        if(line.empty() || line.back().style != styles[i])
            line.push_back({std::u32string(), styles[i]});
        line.back().text += chars[i];
    }
    return line;
}

// Lays ONE paragraph out into lines (appended to `lines`). With maxWidth <= 0 it never soft-wraps.
inline void wrap_foreign_object_styled_paragraph(const std::vector<foreign_object_segment>& paragraph, const foreign_object_fonts& fonts, float max_width, std::vector<foreign_object_line>& lines)
{
    bool uniform = true;
    for(const auto& segment : paragraph)
        uniform = uniform && segment.style == paragraph.front().style;

    if(uniform) {
        // One style all the way: exactly the layout text had before it could carry styles (a candidate
        // line is measured as ONE string, so kerning across the space counts).
        const auto style = paragraph.front().style;
        std::string text;
        for(const auto& segment : paragraph)
            text += segment.text;
        auto chars = utf8_to_u32(text);
        const auto& font_ = fonts.get(style);
        if(max_width > 0.f && !font_.is_null()) {
            std::vector<std::u32string> wrapped;
            wrap_foreign_object_paragraph(chars, font_, max_width, wrapped);
            for(auto& line : wrapped)
                lines.push_back(foreign_object_line{foreign_object_run{std::move(line), style}});
        } else {
            lines.push_back(foreign_object_line{foreign_object_run{std::move(chars), style}});
        }
        return;
    }

    std::u32string chars;
    std::vector<unsigned char> styles;
    for(const auto& segment : paragraph) {
        auto piece = utf8_to_u32(segment.text);
        chars += piece;
        styles.insert(styles.end(), piece.size(), segment.style);
    }
    if(!(max_width > 0.f)) {
        lines.push_back(make_foreign_object_line(chars, styles));
        return;
    }

    // Same greedy word-wrap as wrapForeignObjectParagraph(), a word at a time, but a candidate line is
    // measured style stretch by style stretch. A space takes the style it has in the source.
    const float limit = max_width + 1.f / 128.f;
    std::u32string current;
    std::vector<unsigned char> current_styles;
    size_t i = 0;
    while(i < chars.size()) {
        unsigned char space_style = 0;
        while(i < chars.size() && (chars[i] == U' ' || chars[i] == U'\t')) {
            space_style = styles[i];
            ++i;
        }
        auto word_end = i;
        while(word_end < chars.size() && chars[word_end] != U' ' && chars[word_end] != U'\t')
            ++word_end;
        if(word_end == i)
            break;
        auto word = chars.substr(i, word_end - i);
        std::vector<unsigned char> word_styles(styles.begin() + long(i), styles.begin() + long(word_end));
        i = word_end;

        if(current.empty()) {
            current = std::move(word);
            current_styles = std::move(word_styles);
            continue;
        }
        auto candidate = current;
        auto candidate_styles = current_styles;
        candidate += U' ';
        candidate_styles.push_back(space_style);
        candidate += word;
        candidate_styles.insert(candidate_styles.end(), word_styles.begin(), word_styles.end());
        if(measure_foreign_object_cells(candidate, candidate_styles, fonts) <= limit) {
            current = std::move(candidate);
            current_styles = std::move(candidate_styles);
        } else {
            lines.push_back(make_foreign_object_line(current, current_styles));
            current = std::move(word);
            current_styles = std::move(word_styles);
        }
    }
    if(!current.empty())
        lines.push_back(make_foreign_object_line(current, current_styles));
}

// Lines of a styled text, honouring its forced breaks; wrapWidth <= 0 = no soft wrapping. Never empty.
inline std::vector<foreign_object_line> layout_foreign_object_text(const std::vector<foreign_object_segment>& segments, const foreign_object_fonts& fonts, float wrap_width)
{
    std::vector<foreign_object_line> lines;
    for(const auto& paragraph : split_foreign_object_paragraphs(segments)) {
        if(!paragraph.empty())
            wrap_foreign_object_styled_paragraph(paragraph, fonts, wrap_width, lines);
    }
    if(lines.empty())
        lines.emplace_back();
    return lines;
}

inline float measure_foreign_object_line(const foreign_object_line& line, const foreign_object_fonts& fonts)
{
    float width = 0.f;
    for(const auto& run : line)
        width += fonts.get(run.style).measure_text(std::u32string_view(run.text));
    return width;
}

// Bit n set = style n appears somewhere in the text.
inline unsigned foreign_object_styles_used(const std::vector<foreign_object_segment>& segments)
{
    unsigned used = 0;
    for(const auto& segment : segments)
        used |= 1u << (segment.style & 3);
    return used;
}


/**
 * @brief Metrics for how ForeignObjectSimple::render() will lay out and
 * size a <foreignObject>'s content, computed WITHOUT actually rendering it.
 *
 * Exists so a caller building the svg in the first place (i.e. before any
 * svg_foreign_object_element exists to query) -- e.g. mermaid.js's own layout
 * pass, run headless inside mermaidx's QuickJS/V8 DOM shim, computing a
 * nd box's height to fit a multi-line HTML label -- can ask novasvg
 * itself what height that content will need, rather than reimplementing
 * (and risking drifting out of sync with) the line-counting/line-height
 * logic ForeignObjectSimple::render() uses at actual paint time. See
 * measureForeignObjectContent() below.
 */
struct foreign_object_metrics {
    float height = 0.f;   ///< Total height ForeignObjectSimple::render() will use for this content, in pixels.
    float width = 0.f;    ///< Widest line, per Font::measureText() -- see this struct's own field-level note above measureForeignObjectContent() for why this is NOT what render() itself uses to size anything.
    int line_count = 0;    ///< Number of lines wrapForeignObjectText() splits this content into.
    float line_height = 0.f; ///< Per-line height (from the content's own inline `line-height:`, or `font.height() * 1.2`).
};

/**
 * @brief Measures what ForeignObjectMetrics::height (etc.) will be for
 * `rawHtml` painted with `font`, using the exact same
 * foreignObjectPlainText() / wrapForeignObjectText() / foreignObjectLineHeight()
 * logic ForeignObjectSimple::render() itself calls -- so a value computed
 * here and a box later painted by novasvg with the same (rawHtml, font)
 * can never disagree, short of a bug in this shared logic itself.
 *
 * `maxWidth` (px, optional): the width text may wrap at. When omitted it is
 * read from the HTML's own `width`/`max-width` (px). Wrapping happens only if
 * the HTML's `white-space` allows it (see foreignObjectAllowsWrap()): mermaid's
 * `nowrap` labels stay one line, its wrap mode (`white-space:break-spaces;
 * width:200px`) wraps -- exactly what render() does at the box's own width.
 *
 * ForeignObjectMetrics::width is the widest resulting line. render() never
 * grows a line's box to fit its content horizontally; it only ever
 * horizontally condenses a line that overflows the box it's given. It's
 * computed and returned anyway (via the same per-line Font::measureText()
 * render() itself calls) because a caller BUILDING that box in the first
 * place -- this function's whole reason to exist -- needs some width to
 * size it to, and the widest line at this exact font is the only figure
 * that guarantees render() never needs to condense anything afterward.
 */
inline foreign_object_metrics measure_foreign_object_segments(std::string_view raw_html, const std::vector<foreign_object_segment>& segments, const foreign_object_fonts& fonts, unsigned char base_style, float max_width)
{
    foreign_object_metrics metrics;
    const auto& font_ = fonts.get(base_style);
    if(font_.is_null())
        return metrics;
    // Same rule render() applies, so layout and paint agree: soft-wrap only if
    // the HTML's own `white-space` allows it, at the explicit maxWidth if one
    // was given, else at the width the HTML itself declares (px).
    float wrap_width = 0.f;
    if(foreign_object_allows_wrap(raw_html)) {
        if(max_width > 0.f)
            wrap_width = max_width;
        else if(auto declared = foreign_object_declared_width(raw_html))
            wrap_width = *declared;
    }
    auto lines = layout_foreign_object_text(segments, fonts, wrap_width);
    metrics.line_count = int(lines.size());
    metrics.line_height = foreign_object_line_height(raw_html, font_.size_()).value_or(font_.height() * 1.2f);
    metrics.height = metrics.line_height * float(metrics.line_count);
    for(const auto& line : lines)
        // STB_MAX(), not std::max(): see novasvg.h's own
        // comment on why this codebase avoids std::max/std::min entirely
        // rather than special-casing each call site against the
        // Windows.h max/min macro collision.
        metrics.width = STB_MAX(metrics.width, measure_foreign_object_line(line, fonts));
    return metrics;
}

/**
 * @brief measureForeignObjectContent() for text that may carry bold and italic: `fonts` supplies the face
 * for each style (see ForeignObjectFonts; svg_foreign_object_element::fontsFor() builds it the way painting
 * does), `baseStyle` is the style the foreignObject itself has. This is exactly what painting does, so the
 * metrics and the painted box cannot disagree about a bold label being wider than a regular one.
 */
inline foreign_object_metrics measure_foreign_object_content(std::string_view raw_html, const foreign_object_fonts& fonts, float max_width = -1.f, unsigned char base_style = 0)
{
    return measure_foreign_object_segments(raw_html, foreign_object_styled_text(raw_html, base_style), fonts, base_style, max_width);
}

// svg conditional processing for one switch child. `requiredExtensions` must be non-empty
// and name an extension we implement (the XHTML namespace -- <foreignObject> content);
// `systemLanguage` must list a tag matching the user language ("en"); no attribute -> true.
// (requiredFeatures was removed in svg 2 and browsers treat it as always true.)

} // namespace nova::svg
