#include "utils/text.h"
#include <cassert>
#include <exception>
#include <unordered_map>
#include <vector>

#include <stb/zlib.hpp>

#include "vg/render.h"  // base64_decode (data: URIs)

namespace nova::svg {

struct SimpleSelector;

using selector = std::vector<SimpleSelector>;
using selector_list = std::vector<selector>;

struct AttributeSelector {
    enum class MatchType {
        none,
        equals,
        contains,
        includes,
        starts_with,
        ends_with,
        dash_equals
    };

    MatchType matchType{MatchType::none};
    property_id id{property_id::unknown};
    std::string name; // the attribute's name, used when `id` is unknown (`data-*` & co)
    std::string value;
};

struct PseudoClassSelector {
    enum class type {
        unknown,
        empty,
        root,
        is,
        not_,
        FirstChild,
        LastChild,
        only_child,
        first_of_type,
        last_of_type,
        only_of_type,
        nth_child,
        nth_last_child,
        nth_of_type,
        nth_last_of_type
    };

    type type_{type::unknown};
    int nth_a{0}; // An+B formula of the :nth-*() pseudo-classes
    int nth_b{0};
    selector_list sub_selectors;
};

struct SimpleSelector {
    enum class combinator {
        none,
        descendant,
        child,
        direct_adjacent,
        in_direct_adjacent
    };

    explicit SimpleSelector(combinator combinator_) : combinator_(combinator_) {}

    combinator combinator_{combinator::descendant};
    element_id id{element_id::star};
    std::string tag; // lower-case tag name as written; empty for `*`. For matching HTML (no ElementID for it)
    std::vector<AttributeSelector> attribute_selectors;
    std::vector<PseudoClassSelector> pseudo_class_selectors;
};

struct declaration {
    int specificity;
    property_id id; // Unknown for a property no svg el has (only the HTML in a foreignObject reads those)
    std::string value;
    std::string name; // lower-case property name as written (a custom property's, `--x`, keeps its case)
};

using declaration_list = std::vector<declaration>;

struct rule {
    selector_list selectors;
    declaration_list declarations;
};

class rule_data {
public:
    rule_data(const selector& selector_, const declaration_list& declarations, size_t specificity, size_t position)
        : m_selector(selector_), m_declarations(declarations), m_specificity(specificity), m_position(position)
    {}

    bool is_less_than(const rule_data& rule_) const { return std::tie(m_specificity, m_position) < std::tie(rule_.m_specificity, rule_.m_position); }

    const selector& selector_() const { return m_selector; }
    const declaration_list& declarations() const { return m_declarations; }
    size_t specificity() const { return m_specificity; }
    size_t position() const { return m_position; }

    bool match(const svg_element* el) const;

private:
    selector m_selector;
    declaration_list m_declarations;
    size_t m_specificity;
    size_t m_position;
};

inline bool operator<(const rule_data& a, const rule_data& b) { return a.is_less_than(b); }

using rule_data_list = std::vector<rule_data>;

inline constexpr bool equals(std::string_view value, std::string_view subvalue)
{
    return value.compare(subvalue) == 0;
}

inline constexpr bool contains(std::string_view value, std::string_view subvalue)
{
    return value.find(subvalue) != std::string_view::npos;
}

inline constexpr bool includes(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    std::string_view input(value);
    while(!input.empty()) {
        skip_optional_spaces(input);
        std::string_view start(input);
        while(!input.empty() && !IS_WS(input.front()))
            input.remove_prefix(1);
        if(subvalue == start.substr(0, start.length() - input.length())) {
            return true;
        }
    }

    return false;
}

inline constexpr bool startswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(0, subvalue.size());
}

inline constexpr bool endswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(value.size() - subvalue.size(), subvalue.size());
}

inline constexpr bool dashequals(std::string_view value, std::string_view subvalue)
{
    if(startswith(value, subvalue))
        return (value.length() == subvalue.length() || value.at(subvalue.length()) == '-');
    return false;
}

static bool match_attribute_value(const AttributeSelector& selector, std::string_view value)
{
    if(selector.matchType == AttributeSelector::MatchType::none)
        return !value.empty();
    if(selector.matchType == AttributeSelector::MatchType::equals)
        return equals(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::contains)
        return contains(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::includes)
        return includes(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::starts_with)
        return startswith(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::ends_with)
        return endswith(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::dash_equals)
        return dashequals(value, selector.value);
    return false;
}

static bool match_attribute_selector(const AttributeSelector& selector, const svg_element* el)
{
    if(selector.id == property_id::unknown) {
        const auto* value = el->find_custom_attribute(selector.name);
        if(value == nullptr)
            return false;
        if(selector.matchType == AttributeSelector::MatchType::none)
            return true; // `[attr]` matches an attribute that is there, even an empty one
        return match_attribute_value(selector, *value);
    }

    return match_attribute_value(selector, el->attribute_(selector.id));
}

static bool match_simple_selector(const SimpleSelector& selector, const svg_element* el);

static bool match_pseudo_class_selector(const PseudoClassSelector& selector, const svg_element* el)
{
    if(selector.type_ == PseudoClassSelector::type::empty)
        return el->children().empty();
    if(selector.type_ == PseudoClassSelector::type::root)
        return el->is_root_element();
    if(selector.type_ == PseudoClassSelector::type::is) {
        for(const auto& sub_selector : selector.sub_selectors) {
            for(const auto& simpleSelector : sub_selector) {
                if(!match_simple_selector(simpleSelector, el)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type_ == PseudoClassSelector::type::not_) {
        for(const auto& sub_selector : selector.sub_selectors) {
            for(const auto& simpleSelector : sub_selector) {
                if(match_simple_selector(simpleSelector, el)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type_ == PseudoClassSelector::type::FirstChild)
        return !el->previous_element();
    if(selector.type_ == PseudoClassSelector::type::LastChild)
        return !el->next_element();
    if(selector.type_ == PseudoClassSelector::type::only_child)
        return !(el->previous_element() || el->next_element());
    if(selector.type_ == PseudoClassSelector::type::first_of_type) {
        auto sibling = el->previous_element();
        while(sibling) {
            if(sibling->id() == el->id())
                return false;
            sibling = sibling->previous_element();
        }

        return true;
    }

    if(selector.type_ == PseudoClassSelector::type::last_of_type) {
        auto sibling = el->next_element();
        while(sibling) {
            if(sibling->id() == el->id())
                return false;
            sibling = sibling->next_element();
        }

        return true;
    }

    if(selector.type_ == PseudoClassSelector::type::nth_child
        || selector.type_ == PseudoClassSelector::type::nth_last_child
        || selector.type_ == PseudoClassSelector::type::nth_of_type
        || selector.type_ == PseudoClassSelector::type::nth_last_of_type) {
        const bool from_end = selector.type_ == PseudoClassSelector::type::nth_last_child
            || selector.type_ == PseudoClassSelector::type::nth_last_of_type;
        const bool of_type = selector.type_ == PseudoClassSelector::type::nth_of_type
            || selector.type_ == PseudoClassSelector::type::nth_last_of_type;
        long position = 1;
        auto sibling = from_end ? el->next_element() : el->previous_element();
        while(sibling) {
            if(!of_type || sibling->id() == el->id())
                ++position;
            sibling = from_end ? sibling->next_element() : sibling->previous_element();
        }

        // is there an n >= 0 with a*n + b == position ?
        const long a = selector.nth_a;
        const long diff = position - selector.nth_b;
        if(a == 0)
            return diff == 0;
        return diff % a == 0 && diff / a >= 0;
    }

    return false;
}

static bool match_simple_selector(const SimpleSelector& selector, const svg_element* el)
{
    if(selector.id != element_id::star && selector.id != el->id())
        return false;
    for(const auto& sel : selector.attribute_selectors) {
        if(!match_attribute_selector(sel, el)) {
            return false;
        }
    }

    for(const auto& sel : selector.pseudo_class_selectors) {
        if(!match_pseudo_class_selector(sel, el)) {
            return false;
        }
    }

    return true;
}

static bool match_selector(const selector& selector, const svg_element* el)
{
    if(selector.empty())
        return false;
    auto it = selector.rbegin();
    auto end = selector.rend();
    if(!match_simple_selector(*it, el)) {
        return false;
    }

    auto combinator_ = it->combinator_;
    ++it;

    while(it != end) {
        switch(combinator_) {
        case SimpleSelector::combinator::child:
        case SimpleSelector::combinator::descendant:
            el = el->parent_element();
            break;
        case SimpleSelector::combinator::direct_adjacent:
        case SimpleSelector::combinator::in_direct_adjacent:
            el = el->previous_element();
            break;
        case SimpleSelector::combinator::none:
            assert(false);
        }

        if(el == nullptr)
            return false;
        if(match_simple_selector(*it, el)) {
            combinator_ = it->combinator_;
            ++it;
        } else if(combinator_ != SimpleSelector::combinator::descendant
            && combinator_ != SimpleSelector::combinator::in_direct_adjacent) {
            return false;
        }
    }

    return true;
}


inline bool read_css_identifier(std::string_view& input, std::string& output)
{
    if(input.empty() || !IS_CSS_STARTNAMECHAR(input.front()))
        return false;
    output.clear();
    do {
        output.push_back(input.front());
        input.remove_prefix(1);
    } while(!input.empty() && IS_CSS_NAMECHAR(input.front()));
    return true;
}

static bool parse_tag_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    if(skip_delimiter(input, '*'))
        simpleSelector.id = element_id::star;
    else if(read_css_identifier(input, name)) {
        simpleSelector.id = elementid(name);
        for(auto& ch : name)
            ch = utils::to_lower(ch);
        simpleSelector.tag = name;
    } else {
        return false;
    }
    return true;
}

static bool parse_id_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = property_id::id;
    a.matchType = AttributeSelector::MatchType::equals;
    if(!read_css_identifier(input, a.value))
        return false;
    simpleSelector.attribute_selectors.push_back(std::move(a));
    return true;
}

static bool parse_class_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = property_id::class_;
    a.matchType = AttributeSelector::MatchType::includes;
    if(!read_css_identifier(input, a.value))
        return false;
    simpleSelector.attribute_selectors.push_back(std::move(a));
    return true;
}

static bool parse_attribute_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    skip_optional_spaces(input);
    if(!read_css_identifier(input, name))
        return false;
    AttributeSelector a;
    a.id = propertyid(name);
    a.name = name;
    a.matchType = AttributeSelector::MatchType::none;
    if(skip_delimiter(input, '='))
        a.matchType = AttributeSelector::MatchType::equals;
    else if(skip_string(input, "*="))
        a.matchType = AttributeSelector::MatchType::contains;
    else if(skip_string(input, "~="))
        a.matchType = AttributeSelector::MatchType::includes;
    else if(skip_string(input, "^="))
        a.matchType = AttributeSelector::MatchType::starts_with;
    else if(skip_string(input, "$="))
        a.matchType = AttributeSelector::MatchType::ends_with;
    else if(skip_string(input, "|="))
        a.matchType = AttributeSelector::MatchType::dash_equals;
    if(a.matchType != AttributeSelector::MatchType::none) {
        skip_optional_spaces(input);
        if(!read_css_identifier(input, a.value)) {
            if(input.empty() || !(input.front() == '\"' || input.front() == '\''))
                return false;
            auto quote = input.front();
            input.remove_prefix(1);
            auto n = input.find(quote);
            if(n == std::string_view::npos)
                return false;
            a.value.assign(input.substr(0, n));
            input.remove_prefix(n + 1);
        }
    }

    skip_optional_spaces(input);
    if(!skip_delimiter(input, ']'))
        return false;
    simpleSelector.attribute_selectors.push_back(std::move(a));
    return true;
}

static bool parse_selectors(std::string_view& input, selector_list& selectors);

// Parses the `An+B` micro-syntax of :nth-child() & co: `odd`, `even`, `3`, `2n`, `-n+3`, `2n + 1`...
static bool parse_nth_formula(std::string_view text, int& a, int& b)
{
    std::string f;
    for(char ch : text) {
        if(!IS_WS(ch))
            f.push_back(utils::to_lower(ch));
    }

    if(f.empty())
        return false;
    if(f == "odd") {
        a = 2;
        b = 1;
        return true;
    }

    if(f == "even") {
        a = 2;
        b = 0;
        return true;
    }

    auto parse_int = [](const std::string& t, int& out) {
        size_t i = 0;
        bool negative = false;
        if(i < t.size() && (t[i] == '+' || t[i] == '-'))
            negative = t[i++] == '-';
        if(i >= t.size() || t.size() - i > 6)
            return false;
        int v = 0;
        for(; i < t.size(); ++i) {
            if(t[i] < '0' || t[i] > '9')
                return false;
            v = v * 10 + (t[i] - '0');
        }

        out = negative ? -v : v;
        return true;
    };

    auto n = f.find('n');
    if(n == std::string::npos) {
        a = 0;
        return parse_int(f, b);
    }

    auto a_part = f.substr(0, n);
    auto b_part = f.substr(n + 1);
    if(a_part.empty() || a_part == "+")
        a = 1;
    else if(a_part == "-")
        a = -1;
    else if(!parse_int(a_part, a))
        return false;
    if(b_part.empty()) {
        b = 0;
        return true;
    }

    if(b_part[0] != '+' && b_part[0] != '-')
        return false;
    return parse_int(b_part, b);
}

static bool parse_pseudo_class_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    if(!read_css_identifier(input, name))
        return false;
    PseudoClassSelector selector;
    if(name.compare("empty") == 0)
        selector.type_ = PseudoClassSelector::type::empty;
    else if(name.compare("root") == 0)
        selector.type_ = PseudoClassSelector::type::root;
    else if(name.compare("not") == 0)
        selector.type_ = PseudoClassSelector::type::not_;
    else if(name.compare("first-child") == 0)
        selector.type_ = PseudoClassSelector::type::FirstChild;
    else if(name.compare("last-child") == 0)
        selector.type_ = PseudoClassSelector::type::LastChild;
    else if(name.compare("only-child") == 0)
        selector.type_ = PseudoClassSelector::type::only_child;
    else if(name.compare("first-of-type") == 0)
        selector.type_ = PseudoClassSelector::type::first_of_type;
    else if(name.compare("last-of-type") == 0)
        selector.type_ = PseudoClassSelector::type::last_of_type;
    else if(name.compare("only-of-type") == 0)
        selector.type_ = PseudoClassSelector::type::only_of_type;
    else if(name.compare("nth-child") == 0)
        selector.type_ = PseudoClassSelector::type::nth_child;
    else if(name.compare("nth-last-child") == 0)
        selector.type_ = PseudoClassSelector::type::nth_last_child;
    else if(name.compare("nth-of-type") == 0)
        selector.type_ = PseudoClassSelector::type::nth_of_type;
    else if(name.compare("nth-last-of-type") == 0)
        selector.type_ = PseudoClassSelector::type::nth_last_of_type;
    if(selector.type_ == PseudoClassSelector::type::nth_child
        || selector.type_ == PseudoClassSelector::type::nth_last_child
        || selector.type_ == PseudoClassSelector::type::nth_of_type
        || selector.type_ == PseudoClassSelector::type::nth_last_of_type) {
        skip_optional_spaces(input);
        if(!skip_delimiter(input, '('))
            return false;
        auto close = input.find(')');
        if(close == std::string_view::npos)
            return false;
        if(!parse_nth_formula(input.substr(0, close), selector.nth_a, selector.nth_b))
            return false;
        input.remove_prefix(close + 1);
    }

    if(selector.type_ == PseudoClassSelector::type::is || selector.type_ == PseudoClassSelector::type::not_) {
        skip_optional_spaces(input);
        if(!skip_delimiter(input, '('))
            return false;
        skip_optional_spaces(input);
        if(!parse_selectors(input, selector.sub_selectors))
            return false;
        skip_optional_spaces(input);
        if(!skip_delimiter(input, ')')) {
            return false;
        }
    }

    simpleSelector.pseudo_class_selectors.push_back(std::move(selector));
    return true;
}

static bool parse_simple_selector(std::string_view& input, SimpleSelector& simpleSelector, bool& failed)
{
    auto consumed = parse_tag_selector(input, simpleSelector);
    do {
        if(skip_delimiter(input, '#'))
            failed = !parse_id_selector(input, simpleSelector);
        else if(skip_delimiter(input, '.'))
            failed = !parse_class_selector(input, simpleSelector);
        else if(skip_delimiter(input, '['))
            failed = !parse_attribute_selector(input, simpleSelector);
        else if(skip_delimiter(input, ':'))
            failed = !parse_pseudo_class_selector(input, simpleSelector);
        else
            break;
        consumed = true;
    } while(!failed);
    return consumed && !failed;
}

static bool parse_combinator(std::string_view& input, SimpleSelector::combinator& combinator_)
{
    combinator_ = SimpleSelector::combinator::none;
    while(!input.empty() && IS_WS(input.front())) {
        combinator_ = SimpleSelector::combinator::descendant;
        input.remove_prefix(1);
    }

    if(skip_delimiter_and_optional_spaces(input, '>'))
        combinator_ = SimpleSelector::combinator::child;
    else if(skip_delimiter_and_optional_spaces(input, '+'))
        combinator_ = SimpleSelector::combinator::direct_adjacent;
    else if(skip_delimiter_and_optional_spaces(input, '~'))
        combinator_ = SimpleSelector::combinator::in_direct_adjacent;
    return combinator_ != SimpleSelector::combinator::none;
}

static bool parse_selector(std::string_view& input, selector& selector)
{
    auto combinator_ = SimpleSelector::combinator::none;
    do {
        bool failed = false;
        SimpleSelector simpleSelector(combinator_);
        if(!parse_simple_selector(input, simpleSelector, failed))
            return !failed && (combinator_ == SimpleSelector::combinator::descendant);
        selector.push_back(std::move(simpleSelector));
    } while(parse_combinator(input, combinator_));
    return true;
}

static bool parse_selectors(std::string_view& input, selector_list& selectors)
{
    do {
        selector selector;
        if(!parse_selector(input, selector))
            return false;
        selectors.push_back(std::move(selector));
    } while(skip_delimiter_and_optional_spaces(input, ','));
    return true;
}

inline bool is_custom_property_name(std::string_view name)
{
    return name.size() > 2 && name[0] == '-' && name[1] == '-';
}

static bool parse_declarations(std::string_view& input, declaration_list& declarations)
{
    if(!skip_delimiter(input, '{'))
        return false;
    skip_optional_spaces(input);
    do {
        std::string name;
        if(!read_css_identifier(input, name))
            return false;
        skip_optional_spaces(input);
        if(!skip_delimiter(input, ':'))
            return false;
        skip_optional_spaces(input);
        std::string_view value(input);
        while(!input.empty() && !(input.front() == '!' || input.front() == ';' || input.front() == '}'))
            input.remove_prefix(1);
        value.remove_suffix(input.length());
        strip_trailing_spaces(value);

        declaration declaration_;
        declaration_.specificity = specificity::stylesheet;
        declaration_.id = propertyid(name);
        declaration_.value.assign(value);
        const bool custom = is_custom_property_name(name);
        if(!custom) {
            for(auto& ch : name)
                ch = utils::to_lower(ch);
        }
        declaration_.name = name;
        if(skip_delimiter(input, '!')) {
            skip_optional_spaces(input);
            if(!skip_string(input, "important"))
                return false;
            declaration_.specificity = specificity::stylesheet_important;
        }

        // `background-color` is no svg property, but the HTML inside a foreignObject paints it; a custom
        // property (`--x`) is none either, but `var(--x)` in the values of the elements below reads it
        if(declaration_.id != property_id::unknown || custom || declaration_.name == "background-color")
            declarations.push_back(std::move(declaration_));
        skip_optional_spaces_or_delimiter(input, ';');
    } while(!input.empty() && input.front() != '}');
    return skip_delimiter(input, '}');
}

static bool parse_rule(std::string_view& input, rule& rule_)
{
    if(!parse_selectors(input, rule_.selectors))
        return false;
    return parse_declarations(input, rule_.declarations);
}

// Splits a `src:` value on its *top-level* commas (each a
// `url(...)` optionally followed by `format(...)`) -- paren-depth-aware
// so a comma inside url()/format() (none occur in valid CSS, but
// malformed input shouldn't split incorrectly) doesn't split the list.
static std::vector<std::string_view> split_top_level_commas(std::string_view value)
{
    std::vector<std::string_view> parts;
    size_t start = 0;
    int depth = 0;
    for(size_t i = 0; i < value.size(); ++i) {
        auto ch = value[i];
        if(ch == '(')
            ++depth;
        else if(ch == ')') {
            if(depth > 0)
                --depth;
        } else if(ch == ',' && depth == 0) {
            parts.push_back(value.substr(start, i - start));
            start = i + 1;
        }
    }
    parts.push_back(value.substr(start));
    return parts;
}

static std::string_view strip_quotes(std::string_view value)
{
    strip_leading_and_trailing_spaces(value);
    if(!value.empty() && (value.front() == '\'' || value.front() == '"')) {
        auto quote = value.front();
        value.remove_prefix(1);
        if(!value.empty() && value.back() == quote)
            value.remove_suffix(1);
    }
    return value;
}

// Decodes a base64 payload holding raw TrueType/OpenType (SFNT) data into a
// FontFace; a null face when the payload is malformed or isn't raw SFNT
// (e.g. WOFF/WOFF2). Shared by `@font-face` data URIs and the embedded
// fallback font.
static font_face font_face_from_base64(std::string_view payload)
{
    size_t decoded_length = 0;
    auto* decoded = base64_decode(payload.data(), int(payload.size()), &decoded_length);
    if(decoded == nullptr)
        return font_face();
    return font_face(decoded, decoded_length, [](void* closure) { free(closure); }, decoded);
}

// Loads an `@font-face` rule's embedded font (a `src: url(data:...)`
// entry) and registers it, matching what the CSS itself declares:
// `font-family`, `font-weight`/`font-style` (bold/italic only -- numeric
// weights other than "bold" all collapse to regular, matching the rest
// of this codebase's Font handling elsewhere). Only *embedded* fonts
// (data: URIs) are in scope -- a `url("external.ttf")` reference isn't
// something we can resolve without filesystem/network context the svg
// doesn't provide, and is silently skipped, same as before this existed.
//
// Only raw TrueType/OpenType (SFNT) data is understood (via the same
// stb_truetype loader every other font already goes through) -- WOFF/
// WOFF2 aren't decoded. Rather than inspect each src alternative's
// `format()` hint to guess which might work, this just tries loading
// each in turn and keeps the first one that actually parses as valid
// SFNT, exactly matching how real `@font-face` `src` fallback lists are
// meant to be read (most-preferred format first, older formats after).
static void parse_font_face_rule(std::string_view block)
{
    auto family_value = find_raw_declaration_value(block, "font-family");
    if(!family_value)
        return;
    auto family = strip_quotes(*family_value);
    if(family.empty())
        return;

    auto src_value = find_raw_declaration_value(block, "src");
    if(!src_value)
        return;

    // The src value is where the megabytes are (the font itself, base64). font-weight/font-style are never
    // inside it, so look for them in what lies before and after it.
    const auto src_offset = size_t(src_value->data() - block.data());
    const auto before = block.substr(0, src_offset);
    const auto after = block.substr(src_offset + src_value->size());
    auto find_outside_src = [&](std::string_view property) {
        auto value = find_raw_declaration_value(before, property);
        return value ? value : find_raw_declaration_value(after, property);
    };

    auto bold = false;
    if(auto weight_value = find_outside_src("font-weight")) {
        std::string weight(*weight_value);
        for(auto& ch : weight)
            ch = utils::to_lower(ch);
        bold = weight.find("bold") != std::string::npos || std::strtof(weight.c_str(), nullptr) >= 600.f;
    }

    auto italic = false;
    if(auto style_value = find_outside_src("font-style")) {
        std::string style(*style_value);
        for(auto& ch : style)
            ch = utils::to_lower(ch);
        italic = style.find("italic") != std::string::npos || style.find("oblique") != std::string::npos;
    }

    for(auto alternative : split_top_level_commas(*src_value)) {
        auto url_pos = alternative.find("url(");
        if(url_pos == std::string_view::npos)
            continue;
        auto content_start = url_pos + 4;
        auto close_paren = alternative.find(')', content_start);
        if(close_paren == std::string_view::npos)
            continue;
        auto url = strip_quotes(alternative.substr(content_start, close_paren - content_start));

        if(url.compare(0, 5, "data:") != 0)
            continue; // not embedded -- out of scope, see comment above
        auto base64_pos = url.find("base64,");
        if(base64_pos == std::string_view::npos)
            continue;
        auto payload = url.substr(base64_pos + 7);

        auto face = font_face_from_base64(payload);
        if(face.is_null())
            continue; // malformed, or not raw SFNT (e.g. still WOFF/WOFF2) -- try the next alternative

        font_face_cache_()->add_font_face(std::string(family), bold, italic, face);
        return; // first successfully-loaded alternative wins
    }
}

static rule_data_list parse_style_sheet(std::string_view input)
{
    rule_data_list rules;
    while(!input.empty()) {
        skip_optional_spaces(input);
        if(skip_delimiter(input, '@')) {
            std::string at_keyword;
            read_css_identifier(input, at_keyword);
            for(auto& ch : at_keyword)
                ch = utils::to_lower(ch);

            if(at_keyword == "font-face") {
                skip_optional_spaces(input);
                if(skip_delimiter(input, '{')) {
                    int depth = 1;
                    size_t block_length = 0;
                    auto block = input;
                    // An embedded font makes this block hundreds of kilobytes, with no brace inside it but the
                    // last: look for that one with memchr, and only walk character by character if a `{`
                    // shows up before it.
                    if(auto* close = static_cast<const char*>(std::memchr(input.data(), '}', input.size()))) {
                        const size_t n = size_t(close - input.data());
                        if(std::memchr(input.data(), '{', n) == nullptr) {
                            block_length = n + 1;
                            depth = 0;
                        }
                    }

                    while(block_length < input.size() && depth > 0) {
                        auto ch = input[block_length];
                        if(ch == '{') ++depth;
                        else if(ch == '}') --depth;
                        ++block_length;
                    }
                    parse_font_face_rule(block.substr(0, depth == 0 ? block_length - 1 : block_length));
                    input.remove_prefix(block_length);
                }
                continue;
            }

            int depth = 0;
            while(!input.empty()) {
                auto ch = input.front();
                input.remove_prefix(1);
                if(ch == ';' && depth == 0)
                    break;
                if(ch == '{') ++depth;
                else if(ch == '}' && depth > 0) {
                    if(depth == 1)
                        break;
                    --depth;
                }
            }

            continue;
        }

        rule rule_;
        const auto rule_start = input;
        if(!parse_rule(input, rule_)) {
            // Invalid or unsupported rule: drop only this rule (CSS error recovery) and carry on with the
            // next one, instead of discarding the rest of the stylesheet.
            input = rule_start;
            int depth = 0;
            bool in_block = false;
            while(!input.empty()) {
                auto ch = input.front();
                input.remove_prefix(1);
                if(ch == '{') {
                    ++depth;
                    in_block = true;
                } else if(ch == '}' && depth > 0) {
                    if(--depth == 0)
                        break;
                } else if(ch == ';' && !in_block) {
                    break;
                }
            }

            continue;
        }

        for(const auto& selector : rule_.selectors) {
            size_t specificity = 0;
            for(const auto& simpleSelector : selector) {
                specificity += (simpleSelector.id == element_id::star) ? 0x0 : 0x1;
                for(const auto& attributeSelector : simpleSelector.attribute_selectors) {
                    specificity += (attributeSelector.id == property_id::id) ? 0x10000 : 0x100;
                }
                for(const auto& pseudoClassSelector : simpleSelector.pseudo_class_selectors) {
                    specificity += 0x100;
                }
            }

            rules.emplace_back(selector, rule_.declarations, specificity, rules.size());
        }
    }

    return rules;
}

static selector_list parse_query_selectors(std::string_view input)
{
    selector_list selectors;
    strip_leading_and_trailing_spaces(input);
    if(!parse_selectors(input, selectors)
        || !input.empty()) {
        return selector_list();
    }

    return selectors;
}

// Removes a trailing `!important` (any case, any spacing around the bang) from a declaration value and
// reports whether there was one. `fill:#f9f !important` is the colour #f9f, flagged important -- it is
// not a colour called "#f9f !important".

inline void parse_inline_style(std::string_view input, svg_element* el)
{
    std::string name;
    skip_optional_spaces(input);
    while(read_css_identifier(input, name)) {
        skip_optional_spaces(input);
        if(!skip_delimiter(input, ':'))
            return;
        std::string value;
        while(!input.empty() && input.front() != ';') {
            value.push_back(input.front());
            input.remove_prefix(1);
        }

        if(is_custom_property_name(name)) {
            const bool important = strip_important_flag(value);
            el->set_custom_property(important ? specificity::inline_important : specificity::InlineStyle, name, value);
            skip_optional_spaces_or_delimiter(input, ';');
            continue;
        }

        auto id = csspropertyid(name);
        if(id != property_id::unknown) {
            const bool important = strip_important_flag(value);
            el->set_attribute(important ? specificity::inline_important : specificity::InlineStyle, id, value);
        }
        skip_optional_spaces_or_delimiter(input, ';');
    }
}

inline void remove_style_comments(std::string& value)
{
    auto start = value.find("/*");
    while(start != std::string::npos) {
        auto end = value.find("*/", start + 2);
        value.erase(start, end - start + 2);
        start = value.find("/*");
    }
}

inline bool decode_text(std::string_view input, std::string& output)
{
    output.clear();
    while(!input.empty()) {
        // Copy the plain text up to the next `&` in one go; a style sheet with an embedded font is hundreds
        // of kilobytes with no entity in it at all.
        auto amp = input.find('&');
        if(amp == std::string_view::npos) {
            output.append(input.data(), input.size());
            return true;
        }

        output.append(input.data(), amp);
        input.remove_prefix(amp + 1);

        if(skip_delimiter(input, '#')) {
            int base = 10;
            if(skip_delimiter(input, 'x'))
                base = 16;
            unsigned int cp;
            if(!parse_integer(input, cp, base))
                return false;
            char c[5] = {0, 0, 0, 0, 0};
            if(cp < 0x80) {
                c[1] = 0;
                c[0] = char(cp);
            } else if(cp < 0x800) {
                c[2] = 0;
                c[1] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = char(cp | 0xC0);
            } else if(cp < 0x10000) {
                c[3] = 0;
                c[2] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[1] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = char(cp | 0xE0);
            } else if(cp < 0x200000) {
                c[4] = 0;
                c[3] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[2] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[1] = char((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = char(cp | 0xF0);
            }

            output.append(c);
        } else {
            if(skip_string(input, "amp")) {
                output.push_back('&');
            } else if(skip_string(input, "lt")) {
                output.push_back('<');
            } else if(skip_string(input, "gt")) {
                output.push_back('>');
            } else if(skip_string(input, "quot")) {
                output.push_back('\"');
            } else if(skip_string(input, "apos")) {
                output.push_back('\'');
            } else {
                return false;
            }
        }

        if(!skip_delimiter(input, ';')) {
            return false;
        }
    }

    return true;
}

constexpr bool IS_STARTNAMECHAR(int c) { return IS_ALPHA(c) ||  c == '_' || c == ':'; }
constexpr bool IS_NAMECHAR(int c) { return IS_STARTNAMECHAR(c) || IS_NUM(c) || c == '-' || c == '.'; }

inline bool read_identifier(std::string_view& input, std::string& output)
{
    if(input.empty() || !IS_STARTNAMECHAR(input.front()))
        return false;
    output.clear();
    do {
        output.push_back(input.front());
        input.remove_prefix(1);
    } while(!input.empty() && IS_NAMECHAR(input.front()));
    return true;
}


// ---------------------------------------------------------------------------------------------------
// The HTML inside a <foreignObject>, styled by the dom's CSS.
//
// ForeignObjectSimple paints HTML as plain text in one colour on one optional box, so all the CSS has to
// answer is: which `color` does the text end up with, and which `background-color` box sits behind it.
// A browser answers that with the cascade, and the part that matters in practice (mermaid) is a selector
// that reaches *through* the foreignObject into the svg around it: `.section-0 span{color:black}` styles a
// <span> because the <g class="section-0"> it lives in matches. So the HTML tags are modelled as nodes
// that have a parent chain -- first the HTML ancestors, then the foreignObject, then the svg elements
// above it -- and the very same selector matcher the svg elements use walks that chain.
// ---------------------------------------------------------------------------------------------------

// The markup's elements as the selector matcher sees them (converted from foreign_object's html_tag).
struct html_node {
    std::string tag;
    std::string id;
    std::string class_name;
    std::string style;
    int parent{-1};
};

// A step along the ancestor chain: an HTML nd, or -- once the HTML runs out -- an svg el.
struct style_node {
    const html_node* html{nullptr};
    const svg_element* svg{nullptr};
    explicit operator bool() const { return html || svg; }
};

static bool match_html_simple_selector(const SimpleSelector& selector, const html_node& nd)
{
    if(selector.id != element_id::star && (selector.tag.empty() || selector.tag != nd.tag))
        return false;
    if(!selector.pseudo_class_selectors.empty())
        return false; // :first-child, :not(...) & co are not modelled for HTML
    for(const auto& attribute : selector.attribute_selectors) {
        std::string_view value;
        if(attribute.id == property_id::class_)
            value = nd.class_name;
        else if(attribute.id == property_id::id)
            value = nd.id;
        else if(attribute.id == property_id::style)
            value = nd.style;
        if(!match_attribute_value(attribute, value))
            return false;
    }

    return true;
}

// matchSelector() for an HTML nd: the rightmost compound must match the nd, the ones before it its
// ancestors -- HTML elements first, then `owner` (the foreignObject) and the svg elements above it.
// Sibling combinators (+ ~) are not modelled for HTML, so a selector using one never matches.
static bool match_html_selector(const selector& selector, const std::vector<html_node>& nodes, int index, const svg_element* owner)
{
    if(selector.empty())
        return false;
    auto parent_of = [&](const style_node& nd) {
        if(nd.html) {
            if(nd.html->parent >= 0)
                return style_node{&nodes[nd.html->parent], nullptr};
            return style_node{nullptr, owner};
        }

        return style_node{nullptr, nd.svg->parent_element()};
    };
    auto matches = [](const SimpleSelector& simple, const style_node& nd) {
        return nd.html ? match_html_simple_selector(simple, *nd.html) : match_simple_selector(simple, nd.svg);
    };

    style_node nd{&nodes[index], nullptr};
    auto it = selector.rbegin();
    auto end = selector.rend();
    if(!matches(*it, nd))
        return false;
    auto combinator_ = it->combinator_;
    ++it;
    while(it != end) {
        if(combinator_ != SimpleSelector::combinator::child && combinator_ != SimpleSelector::combinator::descendant)
            return false;
        nd = parent_of(nd);
        if(!nd)
            return false;
        if(matches(*it, nd)) {
            combinator_ = it->combinator_;
            ++it;
        } else if(combinator_ != SimpleSelector::combinator::descendant) {
            return false;
        }
    }

    return true;
}

// One RuleDataList out of several stylesheets applied one after another, each parsed on its own (a rule
// the parser can't read ends only its own sheet) and numbered on from the sheets before it, so a later
// sheet wins a tie. Sorted the way apply_style_sheet() sorts.
static rule_data_list parse_style_sheets(const std::vector<std::string>& sheets)
{
    rule_data_list all;
    for(const auto& sheet : sheets) {
        for(auto& rule : parse_style_sheet(sheet))
            all.emplace_back(rule.selector_(), rule.declarations(), rule.specificity(), all.size());
    }

    std::sort(all.begin(), all.end());
    return all;
}

// Which rules can possibly match a given el, so that apply_style_sheet() does not have to try every rule on
// every el. A rule is filed under what its *rightmost* compound selector demands of the el itself:
// a tag name (`rect`), else a class (`.nd`), else nothing in particular. An el then only has to try the
// rules filed under its own tag, under each of its own classes, and the unfiled ones -- which are the only
// rules that could match it. The index only prunes; each candidate still goes through RuleData::match(), and
// candidates are visited in rule order, so the result is the same as trying every rule.
class rule_index {
public:
    explicit rule_index(const rule_data_list& rules) : m_by_tag(256)
    {
        for(size_t i = 0; i < rules.size(); ++i) {
            const auto& selector = rules[i].selector_();
            if(selector.empty())
                continue; // can never match
            const auto& last = selector.back();
            if(last.id != element_id::star) {
                m_by_tag[size_t(last.id)].push_back(uint32_t(i));
                continue;
            }

            const std::string* class_name = nullptr;
            for(const auto& attribute : last.attribute_selectors) {
                if(attribute.id == property_id::class_ && attribute.matchType == AttributeSelector::MatchType::includes
                    && !attribute.value.empty()) {
                    class_name = &attribute.value;
                    break;
                }
            }

            if(class_name)
                m_by_class[*class_name].push_back(uint32_t(i));
            else
                m_other.push_back(uint32_t(i));
        }
    }

    // Indices (ascending) of the rules that might match `el`.
    void candidates(const svg_element* el, std::vector<uint32_t>& out) const
    {
        out.clear();
        const auto& tagged = m_by_tag[size_t(el->id())];
        out.insert(out.end(), tagged.begin(), tagged.end());
        out.insert(out.end(), m_other.begin(), m_other.end());
        if(!m_by_class.empty()) {
            std::string_view classes(el->attribute_(property_id::class_));
            while(!classes.empty()) {
                skip_optional_spaces(classes);
                size_t n = 0;
                while(n < classes.size() && !IS_WS(classes[n]))
                    ++n;
                if(n > 0) {
                    auto it = m_by_class.find(std::string(classes.substr(0, n)));
                    if(it != m_by_class.end())
                        out.insert(out.end(), it->second.begin(), it->second.end());
                }
                classes.remove_prefix(n);
            }
        }

        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

private:
    std::vector<std::vector<uint32_t>> m_by_tag;
    std::unordered_map<std::string, std::vector<uint32_t>> m_by_class;
    std::vector<uint32_t> m_other;
};



} // namespace nova::svg
