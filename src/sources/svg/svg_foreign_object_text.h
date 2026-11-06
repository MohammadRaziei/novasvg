#pragma once

// Foreign object text: the HTML subset parser, wrapping and measuring.

#include "svg_impl.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

// ---------------------------------------------------------------
// svg_foreign_object_element / ForeignObjectSimple
// ---------------------------------------------------------------

namespace {

inline std::u32string utf8ToU32(const std::string& text)
{
    std::u32string result;
    result.reserve(text.size());
    text_iterator_t it;
    text_iterator_init(&it, text.data(), (int)text.length(), NOVA_TEXT_ENCODING_UTF8);
    while(text_iterator_has_next(&it))
        result.push_back(text_iterator_next(&it));
    return result;
}

// Pulls a "<property>: <value>" declaration's raw value text out of a CSS
// declaration block (the inside of a style="..." attribute, or the
// inside of a rule's { ... }). Not a general CSS-value parser -- just
// finds and trims the text between the property name and the next
// `;`/`}`/quote. `property` match is boundary-checked (not
// preceded/followed by identifier characters) so searching for "color"
// doesn't false-match inside "background-color", or "height" inside
// "line-height". Used by parseNumericDeclaration() below and by the
// white-space lookup.
inline std::optional<std::string_view> findDeclarationValue(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto matchStart = pos;
        auto after = pos + property.size();
        pos = after;
        if(matchStart > 0) {
            auto prev = block[matchStart - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue; // part of a longer property name
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue; // property name continues past what we matched, e.g. "line-height" vs "height"

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;
        auto end = block.find_first_of(";}\"'", after);
        auto value = block.substr(after, (end == std::string_view::npos ? block.size() : end) - after);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// Same idea as findDeclarationValue(), but scans to the next *top-level*
// `;`/`}` instead of stopping at the first quote -- needed for
// `@font-face`'s `src:` value, which legitimately contains both quotes
// and commas (`url(...) format("truetype"), url(...) format("woff2")`).
// Parens are tracked so a `;`/`}` inside a url()/format() call (neither
// occurs in valid CSS, but malformed input shouldn't cut the scan short)
// doesn't end the value early either.
inline std::optional<std::string_view> findRawDeclarationValue(std::string_view block, std::string_view property)
{
    size_t pos = 0;
    while((pos = block.find(property, pos)) != std::string_view::npos) {
        auto matchStart = pos;
        auto after = pos + property.size();
        pos = after;
        if(matchStart > 0) {
            auto prev = block[matchStart - 1];
            if(IS_ALPHA(prev) || IS_NUM(prev) || prev == '-' || prev == '_')
                continue;
        }
        if(after < block.size() && (IS_ALPHA(block[after]) || IS_NUM(block[after]) || block[after] == '-'))
            continue;

        while(after < block.size() && IS_WS(block[after]))
            ++after;
        if(after >= block.size() || block[after] != ':')
            continue;
        ++after;
        while(after < block.size() && IS_WS(block[after]))
            ++after;

        auto valueStart = after;
        auto end = after;
        int parenDepth = 0;
        while(end < block.size()) {
            auto ch = block[end];
            if(ch == '(') {
                // `url(data:...)` holds the whole embedded font. Nothing inside it ends the value, so when
                // the run up to the next `)` has no `(` in it, jump over it in one go.
                if(parenDepth == 0) {
                    const char* base = block.data() + end + 1;
                    const size_t remaining = block.size() - end - 1;
                    if(auto* close = static_cast<const char*>(std::memchr(base, ')', remaining))) {
                        if(std::memchr(base, '(', size_t(close - base)) == nullptr) {
                            end = size_t(close - block.data()) + 1;
                            continue;
                        }
                    }
                }
                ++parenDepth;
            } else if(ch == ')') {
                if(parenDepth > 0)
                    --parenDepth;
            } else if((ch == ';' || ch == '}') && parenDepth == 0) {
                break;
            }
            ++end;
        }

        auto value = block.substr(valueStart, end - valueStart);
        while(!value.empty() && IS_WS(value.back()))
            value.remove_suffix(1);
        if(!value.empty())
            return value;
    }

    return std::nullopt;
}

// A CSS colour value (`#f9f`, `rgb(..)`, `hsl(..)`, `rgba(..)`, a name) -> stb::color; nullopt if it isn't one.
inline std::optional<stb::color> parseCssColor(std::string_view value)
{
    color_t color;
    int length = color_parse(&color, value.data(), (int)value.length());
    if(length == 0)
        return std::nullopt;
    auto argb = color_to_argb32(&color);
    return stb::color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff);
}

// A parsed numeric CSS value: `raw` is the number itself, `isPixels`
// says whether it carried a "px" suffix (an absolute length) as opposed
// to a bare multiplier (e.g. line-height's "1.5"). Anything else
// (%, em, "normal", ...) isn't handled -- callers fall back to their own
// default for those, same as when the property is missing entirely.
struct NumericDeclaration {
    float raw;
    bool isPixels;
};

inline std::optional<NumericDeclaration> parseNumericDeclaration(std::string_view block, std::string_view property)
{
    auto value = findDeclarationValue(block, property);
    if(!value)
        return std::nullopt;

    char* end = nullptr;
    std::string valueStr(*value);
    auto number = std::strtof(valueStr.c_str(), &end);
    if(end == valueStr.c_str())
        return std::nullopt; // no leading number at all, e.g. "normal"

    auto unit = valueStr.c_str() + (end - valueStr.c_str());
    while(*unit == ' ')
        ++unit;
    return NumericDeclaration{number, std::string_view(unit) == "px"};
}

inline std::optional<std::string_view> htmlAttribute(std::string_view tag, std::string_view name)
{
    auto pos = tag.find(name);
    if(pos == std::string_view::npos)
        return std::nullopt;
    auto valueStart = pos + name.size();
    if(valueStart >= tag.size() || (tag[valueStart] != '"' && tag[valueStart] != '\''))
        return std::nullopt;
    auto quote = tag[valueStart];
    ++valueStart;
    auto valueEnd = tag.find(quote, valueStart);
    if(valueEnd == std::string_view::npos)
        return std::nullopt;
    return tag.substr(valueStart, valueEnd - valueStart);
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
struct ForeignObjectSegment {
    std::string text; ///< UTF-8; '\n' marks a forced line break (<br>, <p>, <div>, ...)
    unsigned char style = 0;
};

inline bool foreignObjectIsVoidTag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"area", "base", "br", "col", "embed", "hr", "img",
                                                    "input", "link", "meta", "source", "track", "wbr"};
    return tags.count(tag) > 0;
}

// Tags that sit INSIDE a line of text, so a boundary with them is not a boundary between words:
// "un<b>believ</b>able" is one word. Every tag NOT in this set, and not a block tag, still counts as
// a word boundary (a space), exactly as before styles existed -- see foreignObjectStyledText().
inline bool foreignObjectIsInlineTag(const std::string& tag)
{
    static const std::set<std::string_view> tags = {"a", "abbr", "b", "bdi", "bdo", "big", "cite", "code", "data", "del",
                                                    "dfn", "em", "font", "i", "ins", "kbd", "label", "mark", "q", "s",
                                                    "samp", "small", "span", "strike", "strong", "sub", "sup", "time",
                                                    "tt", "u", "var"};
    return tags.count(tag) > 0;
}

// The value of the style="..." attribute inside a tag's text (after its name), or empty.
inline std::string_view foreignObjectStyleAttribute(std::string_view attributes)
{
    size_t pos = 0;
    while(pos < attributes.size()) {
        while(pos < attributes.size() && (std::isspace(static_cast<unsigned char>(attributes[pos])) || attributes[pos] == '/'))
            ++pos;
        auto nameStart = pos;
        while(pos < attributes.size() && !std::isspace(static_cast<unsigned char>(attributes[pos])) && attributes[pos] != '=' && attributes[pos] != '/')
            ++pos;
        auto name = attributes.substr(nameStart, pos - nameStart);
        while(pos < attributes.size() && std::isspace(static_cast<unsigned char>(attributes[pos])))
            ++pos;
        std::string_view value;
        if(pos < attributes.size() && attributes[pos] == '=') {
            ++pos;
            while(pos < attributes.size() && std::isspace(static_cast<unsigned char>(attributes[pos])))
                ++pos;
            if(pos < attributes.size() && (attributes[pos] == '"' || attributes[pos] == '\'')) {
                auto quote = attributes[pos++];
                auto end = attributes.find(quote, pos);
                value = attributes.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
                pos = end == std::string_view::npos ? attributes.size() : end + 1;
            } else {
                auto valueStart = pos;
                while(pos < attributes.size() && !std::isspace(static_cast<unsigned char>(attributes[pos])))
                    ++pos;
                value = attributes.substr(valueStart, pos - valueStart);
            }
        }
        if(name.size() == 5 && (name[0] | 0x20) == 's' && (name[1] | 0x20) == 't' && (name[2] | 0x20) == 'y' && (name[3] | 0x20) == 'l' && (name[4] | 0x20) == 'e')
            return value;
    }
    return {};
}

// The lower-cased value of one declaration, without a trailing `!important`; empty if it is absent.
inline std::string foreignObjectDeclaration(std::string_view block, std::string_view property)
{
    auto value = findDeclarationValue(block, property);
    if(!value)
        return {};
    std::string result;
    for(char c : *value)
        result += char(std::tolower(static_cast<unsigned char>(c)));
    if(auto bang = result.find('!'); bang != std::string::npos)
        result.erase(bang);
    while(!result.empty() && std::isspace(static_cast<unsigned char>(result.back())))
        result.pop_back();
    return result;
}

// The style in effect inside an el that is being opened.
inline unsigned char foreignObjectStyleOfOpening(const std::string& tag, std::string_view attributes, unsigned char inherited)
{
    unsigned char style = inherited;
    if(tag == "b" || tag == "strong")
        style |= ForeignObjectFonts::Bold;
    else if(tag == "i" || tag == "em" || tag == "cite" || tag == "var" || tag == "dfn")
        style |= ForeignObjectFonts::Italic;

    auto inlineStyle = foreignObjectStyleAttribute(attributes);
    if(!inlineStyle.empty()) {
        if(auto weight = foreignObjectDeclaration(inlineStyle, "font-weight"); !weight.empty()) {
            style &= (unsigned char)~ForeignObjectFonts::Bold;
            if(parseFontWeight(weight) == FontWeight::Bold)
                style |= ForeignObjectFonts::Bold;
        }
        if(auto fontStyle = foreignObjectDeclaration(inlineStyle, "font-style"); !fontStyle.empty()) {
            style &= (unsigned char)~ForeignObjectFonts::Italic;
            if(parseFontStyle(fontStyle) == FontStyle::Italic)
                style |= ForeignObjectFonts::Italic;
        }
    }
    return style;
}

inline std::vector<ForeignObjectSegment> foreignObjectStyledText(std::string_view html, unsigned char baseStyle = 0)
{
    // Tag names that force a line break, matching their real-browser default of `display: block` (so a
    // `<br>`/`<p>`/`<div>` boundary becomes '\n', preserved through the whitespace-collapse pass below,
    // instead of collapsing into an ordinary space like other tags do). Mermaid's own labels are
    // `white-space: nowrap` by design (each is laid out to fit on one line at the font it expects) and
    // only ever span multiple lines where it inserted an explicit `<br>` itself at a deliberate break
    // point -- so honoring just these forced breaks reproduces real multi-line labels without needing
    // general width-based word-wrap.
    static const std::set<std::string_view> block_tags = {"br", "p", "div", "tr", "li"};

    std::vector<ForeignObjectSegment> raw;
    auto put = [&](char c, unsigned char style) {
        if(raw.empty() || raw.back().style != style)
            raw.push_back({std::string(), style});
        raw.back().text += c;
    };

    struct OpenElement {
        std::string tag;
        unsigned char style;
    };
    std::vector<OpenElement> open;
    unsigned char style = baseStyle;

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
            const bool selfClosing = !inside.empty() && inside.back() == '/';
            auto rest = closing ? inside.substr(1) : inside;
            size_t nameLength = 0;
            while(nameLength < rest.size() && std::isalnum(static_cast<unsigned char>(rest[nameLength])))
                ++nameLength;
            std::string name(rest.substr(0, nameLength));
            for(auto& ch : name)
                ch = char(std::tolower(static_cast<unsigned char>(ch)));

            // Every tag boundary is a word boundary (a space), so "Hello<br/>world" and "a</td><td>b"
            // never glue together; block tags force a line break instead; and the tags that sit inside
            // a line of text (<b>, <span>, ...) are no boundary at all.
            auto boundary = [&](unsigned char at) {
                if(block_tags.count(name))
                    put('\n', at);
                else if(!foreignObjectIsInlineTag(name))
                    put(' ', at);
            };

            if(closing) {
                for(size_t k = open.size(); k-- > 0;) {
                    if(open[k].tag == name) {
                        open.resize(k);
                        break;
                    }
                }
                style = open.empty() ? baseStyle : open.back().style;
                boundary(style);
            } else {
                boundary(style);
                if(!selfClosing && !foreignObjectIsVoidTag(name)) {
                    style = foreignObjectStyleOfOpening(name, rest.substr(nameLength), style);
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
    std::vector<ForeignObjectSegment> collapsed;
    auto append = [&](char c, unsigned char at) {
        if(collapsed.empty() || collapsed.back().style != at)
            collapsed.push_back({std::string(), at});
        collapsed.back().text += c;
    };
    auto dropLast = [&]() {
        collapsed.back().text.pop_back();
        if(collapsed.back().text.empty())
            collapsed.pop_back();
    };
    bool lastWasSpace = true; // trims leading whitespace too
    for(const auto& segment : raw) {
        for(char c : segment.text) {
            if(c == '\t' || c == '\r')
                c = ' ';
            if(c == '\n') {
                while(!collapsed.empty() && collapsed.back().text.back() == ' ')
                    dropLast();
                append('\n', segment.style);
                lastWasSpace = true;
                continue;
            }
            if(c == ' ') {
                if(lastWasSpace)
                    continue;
                lastWasSpace = true;
            } else {
                lastWasSpace = false;
            }
            append(c, segment.style);
        }
    }
    while(!collapsed.empty() && (collapsed.back().text.back() == ' ' || collapsed.back().text.back() == '\n'))
        dropLast();
    return collapsed;
}

// The same text with every style dropped: the plain "innerText" of the HTML ('\n' at forced breaks).
inline std::string foreignObjectPlainText(std::string_view html)
{
    std::string text;
    for(const auto& segment : foreignObjectStyledText(html))
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
inline std::optional<float> foreignObjectLineHeight(std::string_view rawHtml, float fontSize)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            if(auto declaration = parseNumericDeclaration(*style, "line-height"))
                return declaration->isPixels ? declaration->raw : declaration->raw * fontSize;
        }
        pos = tagEnd + 1;
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
inline bool foreignObjectAllowsWrap(std::string_view rawHtml)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            if(auto value = findDeclarationValue(*style, "white-space"))
                return !(*value == "nowrap" || *value == "pre");
        }
        pos = tagEnd + 1;
    }
    return true;
}

// The px width the HTML itself constrains its text to, when it says so:
// the outermost el declaring `width` and/or `max-width` in px (both ->
// the smaller, as in CSS). Only px is understood; anything else (%, em, auto)
// means "no declared width" and the caller falls back to the box it has.
inline std::optional<float> foreignObjectDeclaredWidth(std::string_view rawHtml)
{
    size_t pos = 0;
    while(pos < rawHtml.size()) {
        auto tagStart = rawHtml.find('<', pos);
        if(tagStart == std::string_view::npos)
            break;
        if(tagStart + 1 < rawHtml.size() && rawHtml[tagStart + 1] == '/') {
            pos = tagStart + 2;
            continue;
        }
        auto tagEnd = rawHtml.find('>', tagStart);
        if(tagEnd == std::string_view::npos)
            break;
        auto tag = rawHtml.substr(tagStart, tagEnd - tagStart);
        if(auto style = htmlAttribute(tag, "style=")) {
            std::optional<float> width;
            if(auto d = parseNumericDeclaration(*style, "width"); d && d->isPixels)
                width = d->raw;
            if(auto d = parseNumericDeclaration(*style, "max-width"); d && d->isPixels)
                width = width ? STB_MIN(*width, d->raw) : d->raw;
            if(width)
                return width;
        }
        pos = tagEnd + 1;
    }
    return std::nullopt;
}

// Greedy word-wrap of ONE paragraph to `maxWidth`: a line takes the next word
// while the whole candidate line ("a b c", measured as one string so kerning
// across the space counts, like a browser's shaping) still fits. A single
// word wider than maxWidth stays alone on an overflowing line -- CSS never
// breaks inside a word by default; render() condenses that one line.
inline void wrapForeignObjectParagraph(const std::u32string& paragraph, const Font& font, float maxWidth, std::vector<std::u32string>& lines)
{
    // Browsers lay out in 1/64px units, so "fits" tolerates that much slack.
    const float limit = maxWidth + 1.f / 128.f;
    std::u32string current;
    size_t i = 0;
    while(i < paragraph.size()) {
        while(i < paragraph.size() && (paragraph[i] == U' ' || paragraph[i] == U'\t'))
            ++i;
        auto wordEnd = i;
        while(wordEnd < paragraph.size() && paragraph[wordEnd] != U' ' && paragraph[wordEnd] != U'\t')
            ++wordEnd;
        if(wordEnd == i)
            break;
        auto word = paragraph.substr(i, wordEnd - i);
        i = wordEnd;

        if(current.empty()) {
            current = std::move(word);
            continue;
        }
        auto candidate = current;
        candidate += U' ';
        candidate += word;
        if(font.measureText(std::u32string_view(candidate)) <= limit) {
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
inline std::vector<std::u32string> wrapForeignObjectText(const std::string& text, const Font* font = nullptr, float maxWidth = 0.f)
{
    std::vector<std::u32string> lines;
    const bool softWrap = font != nullptr && !font->is_null() && maxWidth > 0.f;
    size_t pos = 0;
    while(pos <= text.size()) {
        auto nl = text.find('\n', pos);
        auto paragraph = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if(paragraph.empty())
            continue;
        if(softWrap)
            wrapForeignObjectParagraph(utf8ToU32(paragraph), *font, maxWidth, lines);
        else
            lines.push_back(utf8ToU32(paragraph));
    }

    if(lines.empty())
        lines.emplace_back();
    return lines;
}

// One stretch of a line in a single style; a line is a few of them in a row.
struct ForeignObjectRun {
    std::u32string text;
    unsigned char style = 0;
};
using ForeignObjectLine = std::vector<ForeignObjectRun>;

// The text of a segment list split at its forced breaks ('\n'), each paragraph as segments again.
inline std::vector<std::vector<ForeignObjectSegment>> splitForeignObjectParagraphs(const std::vector<ForeignObjectSegment>& segments)
{
    std::vector<std::vector<ForeignObjectSegment>> paragraphs(1);
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
inline float measureForeignObjectCells(const std::u32string& chars, const std::vector<unsigned char>& styles, const ForeignObjectFonts& fonts)
{
    float width = 0.f;
    size_t begin = 0;
    while(begin < chars.size()) {
        size_t end = begin + 1;
        while(end < chars.size() && styles[end] == styles[begin])
            ++end;
        width += fonts.get(styles[begin]).measureText(std::u32string_view(chars).substr(begin, end - begin));
        begin = end;
    }
    return width;
}

inline ForeignObjectLine makeForeignObjectLine(const std::u32string& chars, const std::vector<unsigned char>& styles)
{
    ForeignObjectLine line;
    for(size_t i = 0; i < chars.size(); ++i) {
        if(line.empty() || line.back().style != styles[i])
            line.push_back({std::u32string(), styles[i]});
        line.back().text += chars[i];
    }
    return line;
}

// Lays ONE paragraph out into lines (appended to `lines`). With maxWidth <= 0 it never soft-wraps.
inline void wrapForeignObjectStyledParagraph(const std::vector<ForeignObjectSegment>& paragraph, const ForeignObjectFonts& fonts, float maxWidth, std::vector<ForeignObjectLine>& lines)
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
        auto chars = utf8ToU32(text);
        const auto& font = fonts.get(style);
        if(maxWidth > 0.f && !font.is_null()) {
            std::vector<std::u32string> wrapped;
            wrapForeignObjectParagraph(chars, font, maxWidth, wrapped);
            for(auto& line : wrapped)
                lines.push_back(ForeignObjectLine{ForeignObjectRun{std::move(line), style}});
        } else {
            lines.push_back(ForeignObjectLine{ForeignObjectRun{std::move(chars), style}});
        }
        return;
    }

    std::u32string chars;
    std::vector<unsigned char> styles;
    for(const auto& segment : paragraph) {
        auto piece = utf8ToU32(segment.text);
        chars += piece;
        styles.insert(styles.end(), piece.size(), segment.style);
    }
    if(!(maxWidth > 0.f)) {
        lines.push_back(makeForeignObjectLine(chars, styles));
        return;
    }

    // Same greedy word-wrap as wrapForeignObjectParagraph(), a word at a time, but a candidate line is
    // measured style stretch by style stretch. A space takes the style it has in the source.
    const float limit = maxWidth + 1.f / 128.f;
    std::u32string current;
    std::vector<unsigned char> currentStyles;
    size_t i = 0;
    while(i < chars.size()) {
        unsigned char spaceStyle = 0;
        while(i < chars.size() && (chars[i] == U' ' || chars[i] == U'\t')) {
            spaceStyle = styles[i];
            ++i;
        }
        auto wordEnd = i;
        while(wordEnd < chars.size() && chars[wordEnd] != U' ' && chars[wordEnd] != U'\t')
            ++wordEnd;
        if(wordEnd == i)
            break;
        auto word = chars.substr(i, wordEnd - i);
        std::vector<unsigned char> wordStyles(styles.begin() + long(i), styles.begin() + long(wordEnd));
        i = wordEnd;

        if(current.empty()) {
            current = std::move(word);
            currentStyles = std::move(wordStyles);
            continue;
        }
        auto candidate = current;
        auto candidateStyles = currentStyles;
        candidate += U' ';
        candidateStyles.push_back(spaceStyle);
        candidate += word;
        candidateStyles.insert(candidateStyles.end(), wordStyles.begin(), wordStyles.end());
        if(measureForeignObjectCells(candidate, candidateStyles, fonts) <= limit) {
            current = std::move(candidate);
            currentStyles = std::move(candidateStyles);
        } else {
            lines.push_back(makeForeignObjectLine(current, currentStyles));
            current = std::move(word);
            currentStyles = std::move(wordStyles);
        }
    }
    if(!current.empty())
        lines.push_back(makeForeignObjectLine(current, currentStyles));
}

// Lines of a styled text, honouring its forced breaks; wrapWidth <= 0 = no soft wrapping. Never empty.
inline std::vector<ForeignObjectLine> layoutForeignObjectText(const std::vector<ForeignObjectSegment>& segments, const ForeignObjectFonts& fonts, float wrapWidth)
{
    std::vector<ForeignObjectLine> lines;
    for(const auto& paragraph : splitForeignObjectParagraphs(segments)) {
        if(!paragraph.empty())
            wrapForeignObjectStyledParagraph(paragraph, fonts, wrapWidth, lines);
    }
    if(lines.empty())
        lines.emplace_back();
    return lines;
}

inline float measureForeignObjectLine(const ForeignObjectLine& line, const ForeignObjectFonts& fonts)
{
    float width = 0.f;
    for(const auto& run : line)
        width += fonts.get(run.style).measureText(std::u32string_view(run.text));
    return width;
}

// Bit n set = style n appears somewhere in the text.
inline unsigned foreignObjectStylesUsed(const std::vector<ForeignObjectSegment>& segments)
{
    unsigned used = 0;
    for(const auto& segment : segments)
        used |= 1u << (segment.style & 3);
    return used;
}

} // namespace

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
struct ForeignObjectMetrics {
    float height = 0.f;   ///< Total height ForeignObjectSimple::render() will use for this content, in pixels.
    float width = 0.f;    ///< Widest line, per Font::measureText() -- see this struct's own field-level note above measureForeignObjectContent() for why this is NOT what render() itself uses to size anything.
    int lineCount = 0;    ///< Number of lines wrapForeignObjectText() splits this content into.
    float lineHeight = 0.f; ///< Per-line height (from the content's own inline `line-height:`, or `font.height() * 1.2`).
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
inline ForeignObjectMetrics measureForeignObjectSegments(std::string_view rawHtml, const std::vector<ForeignObjectSegment>& segments, const ForeignObjectFonts& fonts, unsigned char baseStyle, float maxWidth)
{
    ForeignObjectMetrics metrics;
    const auto& font = fonts.get(baseStyle);
    if(font.is_null())
        return metrics;
    // Same rule render() applies, so layout and paint agree: soft-wrap only if
    // the HTML's own `white-space` allows it, at the explicit maxWidth if one
    // was given, else at the width the HTML itself declares (px).
    float wrapWidth = 0.f;
    if(foreignObjectAllowsWrap(rawHtml)) {
        if(maxWidth > 0.f)
            wrapWidth = maxWidth;
        else if(auto declared = foreignObjectDeclaredWidth(rawHtml))
            wrapWidth = *declared;
    }
    auto lines = layoutForeignObjectText(segments, fonts, wrapWidth);
    metrics.lineCount = int(lines.size());
    metrics.lineHeight = foreignObjectLineHeight(rawHtml, font.size()).value_or(font.height() * 1.2f);
    metrics.height = metrics.lineHeight * float(metrics.lineCount);
    for(const auto& line : lines)
        // STB_MAX(), not std::max(): see novasvg.h's own
        // comment on why this codebase avoids std::max/std::min entirely
        // rather than special-casing each call site against the
        // Windows.h max/min macro collision.
        metrics.width = STB_MAX(metrics.width, measureForeignObjectLine(line, fonts));
    return metrics;
}

/**
 * @brief measureForeignObjectContent() for text that may carry bold and italic: `fonts` supplies the face
 * for each style (see ForeignObjectFonts; svg_foreign_object_element::fontsFor() builds it the way painting
 * does), `baseStyle` is the style the foreignObject itself has. This is exactly what painting does, so the
 * metrics and the painted box cannot disagree about a bold label being wider than a regular one.
 */
inline ForeignObjectMetrics measureForeignObjectContent(std::string_view rawHtml, const ForeignObjectFonts& fonts, float maxWidth = -1.f, unsigned char baseStyle = 0)
{
    return measureForeignObjectSegments(rawHtml, foreignObjectStyledText(rawHtml, baseStyle), fonts, baseStyle, maxWidth);
}

/**
 * @brief measureForeignObjectContent() with a single face: every style is measured with `font`, i.e. as
 * if the text had no bold or italic in it. Line breaks and heights are unaffected by bold or italic;
 * only the widths of styled stretches can differ from what painting produces, by however much the real
 * bold/italic faces differ from `font`. Use the ForeignObjectFonts overload when that matters.
 */
inline ForeignObjectMetrics measureForeignObjectContent(std::string_view rawHtml, const Font& font, float maxWidth = -1.f)
{
    auto segments = foreignObjectStyledText(rawHtml);
    for(auto& segment : segments)
        segment.style = 0; // one face for everything: measured exactly like plain text
    ForeignObjectFonts fonts;
    fonts.face[0] = font;
    return measureForeignObjectSegments(rawHtml, segments, fonts, 0, maxWidth);
}

// svg conditional processing for one switch child. `requiredExtensions` must be non-empty
// and name an extension we implement (the XHTML namespace -- <foreignObject> content);
// `systemLanguage` must list a tag matching the user language ("en"); no attribute -> true.
// (requiredFeatures was removed in svg 2 and browsers treat it as always true.)

} // namespace nova::svg
