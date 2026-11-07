// The CSS side of the simple HTML renderer: which `color` the text ends up with and which `background-color`
// box sits behind it. The markup is parsed here; the document's style sheets are asked through css_query.

#include "utils/text.h"
#include "html_style.h"

#include "../svg_css_utils.h"

namespace nova::svg {
using namespace vg;

namespace {

struct parsed_html_node {
    std::string tag; // lower case
    std::string id;
    std::string class_name;
    std::string style; // the style="" attribute, verbatim
    int parent{-1};    // index in the list; -1 = a direct child of the foreignObject
    bool has_text{false}; // has non-blank text of its own
};

static bool is_void_html_tag(const std::string& tag)
{
    static constexpr std::string_view names[] = {"area", "base", "br", "col", "embed", "hr", "img",
                                                 "input", "link", "meta", "source", "track", "wbr"};
    for(auto name : names) {
        if(tag == name)
            return true;
    }

    return false;
}

static std::string lower_case(std::string_view text)
{
    std::string out(text);
    for(auto& ch : out)
        ch = utils::to_lower(ch);
    return out;
}

// The opening/closing tags of a piece of HTML as a flat list in dom order, each with the index of
// the el it sits in. Only what the cascade needs is read: tag name, id, class, style.
static std::vector<parsed_html_node> parse_html_nodes(std::string_view html)
{
    std::vector<parsed_html_node> nodes;
    std::vector<int> open;
    size_t pos = 0;
    while(pos < html.size()) {
        auto lt = html.find('<', pos);
        auto text = html.substr(pos, lt == std::string_view::npos ? std::string_view::npos : lt - pos);
        if(!open.empty()) {
            for(char ch : text) {
                if(!IS_WS(ch)) {
                    nodes[open.back()].has_text = true;
                    break;
                }
            }
        }

        if(lt == std::string_view::npos)
            break;
        if(html.compare(lt, 4, "<!--") == 0) {
            auto end = html.find("-->", lt + 4);
            pos = end == std::string_view::npos ? html.size() : end + 3;
            continue;
        }

        // the end of the tag: the first '>' that is not inside a quoted attribute value
        size_t gt = lt + 1;
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

        auto inside = html.substr(lt + 1, gt - lt - 1);
        pos = STB_MIN(gt + 1, html.size());
        if(inside.empty() || inside.front() == '!' || inside.front() == '?')
            continue;

        if(inside.front() == '/') {
            inside.remove_prefix(1);
            size_t n = 0;
            while(n < inside.size() && !IS_WS(inside[n]))
                ++n;
            auto name = lower_case(inside.substr(0, n));
            for(size_t i = open.size(); i-- > 0;) {
                if(nodes[open[i]].tag == name) {
                    open.resize(i);
                    break;
                }
            }

            continue;
        }

        parsed_html_node nd;
        size_t n = 0;
        while(n < inside.size() && !IS_WS(inside[n]) && inside[n] != '/')
            ++n;
        nd.tag = lower_case(inside.substr(0, n));
        inside.remove_prefix(n);
        bool self_closing = false;
        while(!inside.empty()) {
            skip_optional_spaces(inside);
            if(inside.empty())
                break;
            if(inside.front() == '/') {
                self_closing = true;
                inside.remove_prefix(1);
                continue;
            }

            self_closing = false;
            size_t m = 0;
            while(m < inside.size() && !IS_WS(inside[m]) && inside[m] != '=' && inside[m] != '/')
                ++m;
            auto name = lower_case(inside.substr(0, m));
            inside.remove_prefix(m);
            skip_optional_spaces(inside);
            std::string value;
            if(!inside.empty() && inside.front() == '=') {
                inside.remove_prefix(1);
                skip_optional_spaces(inside);
                if(!inside.empty() && (inside.front() == '"' || inside.front() == '\'')) {
                    auto q = inside.front();
                    inside.remove_prefix(1);
                    auto end = inside.find(q);
                    value.assign(inside.substr(0, end));
                    inside.remove_prefix(end == std::string_view::npos ? inside.size() : end + 1);
                } else {
                    size_t v = 0;
                    while(v < inside.size() && !IS_WS(inside[v]))
                        ++v;
                    value.assign(inside.substr(0, v));
                    inside.remove_prefix(v);
                }
            }

            if(name == "class")
                nd.class_name = value;
            else if(name == "id")
                nd.id = value;
            else if(name == "style")
                nd.style = value;
        }

        nd.parent = open.empty() ? -1 : open.back();
        nodes.push_back(std::move(nd));
        if(!self_closing && !is_void_html_tag(nodes.back().tag))
            open.push_back(int(nodes.size()) - 1);
    }

    return nodes;
}

// The declaration of one property that currently wins, by cascade priority (a later one wins a tie).
struct cascaded_value {
    int specificity{-1};
    std::string value;
    void offer(int priority, const std::string& candidate)
    {
        if(priority >= specificity) {
            specificity = priority;
            value = candidate;
        }
    }
};

// Offers every `color` / `background-color` declaration of a style="" attribute.
static void offer_inline_style(std::string_view style, cascaded_value& color_, cascaded_value& background)
{
    while(!style.empty()) {
        auto end = style.find(';');
        auto declaration_ = style.substr(0, end);
        style.remove_prefix(end == std::string_view::npos ? style.size() : end + 1);
        auto colon = declaration_.find(':');
        if(colon == std::string_view::npos)
            continue;
        auto name_text = declaration_.substr(0, colon);
        strip_leading_and_trailing_spaces(name_text);
        auto name = lower_case(name_text);
        std::string value(declaration_.substr(colon + 1));
        const int priority = strip_important_flag(value) ? specificity::inline_important : specificity::InlineStyle;
        std::string_view trimmed(value);
        strip_leading_and_trailing_spaces(trimmed);
        if(name == "color")
            color_.offer(priority, std::string(trimmed));
        else if(name == "background-color")
            background.offer(priority, std::string(trimmed));
    }
}

} // namespace

// Works out the text colour and the box colour the CSS gives this foreignObject's HTML and stores them on
// it. `rules` must be sorted (least to most specific, then source order), as apply_style_sheet() leaves them.
//  * text colour: `color` inherits, so every nd has the colour of its own rule or else its parent's; the
//    text is painted in the colour of the last el that holds text itself (black if nothing sets one).
//    This is the HTML's CSS `color` -- deliberately not the svg `fill` that happens to be in effect around
//    the foreignObject: a classDef like `.green>*{fill:#9f6}` also matches the label's <g>, and must not
//    turn the label text the same shade as its box.
//  * box colour: `background-color` does not inherit. Checked against a real WebKit render: mermaid's edge
//    labels have an opaque box on an inner <span> stacked over a translucent one on the outer <div>, and
//    the opaque one is what shows -- so the last el in dom order that has a visible box wins,
//    like paint order (later/nested elements paint over earlier ones). `transparent` is no box.
html_style resolve_html_style(std::string_view markup, const css_query& css)
{
    const auto nodes = parse_html_nodes(markup);
    std::vector<html_tag> tags(nodes.size());
    for(size_t i = 0; i < nodes.size(); ++i) {
        tags[i].tag = nodes[i].tag;
        tags[i].id = nodes[i].id;
        tags[i].class_names = nodes[i].class_name;
        tags[i].style = nodes[i].style;
        tags[i].parent = nodes[i].parent;
    }

    std::vector<std::optional<stb::color>> color_of(nodes.size());
    std::optional<stb::color> text_color, text_color_of_last_node, background;
    bool any_text = false;
    for(size_t i = 0; i < nodes.size(); ++i) {
        cascaded_value color_, box_;
        if(auto declared = css.declared(tags, int(i), "color"))
            color_.offer(declared->specificity, declared->value);
        if(auto declared = css.declared(tags, int(i), "background-color"))
            box_.offer(declared->specificity, declared->value);

        offer_inline_style(nodes[i].style, color_, box_);

        std::optional<stb::color> declared;
        if(color_.specificity >= 0)
            declared = parse_css_color(color_.value);
        color_of[i] = declared ? declared : (nodes[i].parent >= 0 ? color_of[nodes[i].parent] : std::nullopt);
        text_color_of_last_node = color_of[i];
        if(nodes[i].has_text) {
            text_color = color_of[i];
            any_text = true;
        }

        if(box_.specificity >= 0) {
            if(auto fill = parse_css_color(box_.value); fill && is_visible(*fill))
                background = fill;
        }
    }

    html_style style;
    style.text_color = any_text ? text_color : text_color_of_last_node;
    style.background_color = background;
    return style;
}

} // namespace nova::svg
