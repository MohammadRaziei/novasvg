#include <cassert>
#include <exception>
#include <unordered_map>
#include <vector>

#include <stb/zlib.hpp>

#include "vg/render.h"  // base64_decode (data: URIs)

namespace nova::svg {

struct SimpleSelector;

using Selector = std::vector<SimpleSelector>;
using selector_list = std::vector<Selector>;

struct AttributeSelector {
    enum class MatchType {
        None,
        Equals,
        Contains,
        Includes,
        starts_with,
        ends_with,
        dash_equals
    };

    MatchType matchType{MatchType::None};
    property_id id{property_id::unknown};
    std::string name; // the attribute's name, used when `id` is unknown (`data-*` & co)
    std::string value;
};

struct PseudoClassSelector {
    enum class Type {
        unknown,
        Empty,
        Root,
        is,
        Not,
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

    Type type{Type::unknown};
    int nth_a{0}; // An+B formula of the :nth-*() pseudo-classes
    int nth_b{0};
    selector_list sub_selectors;
};

struct SimpleSelector {
    enum class Combinator {
        None,
        descendant,
        Child,
        direct_adjacent,
        in_direct_adjacent
    };

    explicit SimpleSelector(Combinator combinator) : combinator(combinator) {}

    Combinator combinator{Combinator::descendant};
    element_id id{element_id::star};
    std::string tag; // lower-case tag name as written; empty for `*`. For matching HTML (no ElementID for it)
    std::vector<AttributeSelector> attribute_selectors;
    std::vector<PseudoClassSelector> pseudo_class_selectors;
};

struct Declaration {
    int specificity;
    property_id id; // Unknown for a property no svg el has (only the HTML in a foreignObject reads those)
    std::string value;
    std::string name; // lower-case property name as written (a custom property's, `--x`, keeps its case)
};

using declaration_list = std::vector<Declaration>;

struct Rule {
    selector_list selectors;
    declaration_list declarations;
};

class rule_data {
public:
    rule_data(const Selector& selector, const declaration_list& declarations, size_t specificity, size_t position)
        : m_selector(selector), m_declarations(declarations), m_specificity(specificity), m_position(position)
    {}

    bool is_less_than(const rule_data& rule) const { return std::tie(m_specificity, m_position) < std::tie(rule.m_specificity, rule.m_position); }

    const Selector& selector() const { return m_selector; }
    const declaration_list& declarations() const { return m_declarations; }
    size_t specificity() const { return m_specificity; }
    size_t position() const { return m_position; }

    bool match(const svg_element* el) const;

private:
    Selector m_selector;
    declaration_list m_declarations;
    size_t m_specificity;
    size_t m_position;
};

inline bool operator<(const rule_data& a, const rule_data& b) { return a.is_less_than(b); }

using rule_data_list = std::vector<rule_data>;

NOVA_INLINE constexpr bool equals(std::string_view value, std::string_view subvalue)
{
    return value.compare(subvalue) == 0;
}

NOVA_INLINE constexpr bool contains(std::string_view value, std::string_view subvalue)
{
    return value.find(subvalue) != std::string_view::npos;
}

NOVA_INLINE constexpr bool includes(std::string_view value, std::string_view subvalue)
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

NOVA_INLINE constexpr bool startswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(0, subvalue.size());
}

NOVA_INLINE constexpr bool endswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(value.size() - subvalue.size(), subvalue.size());
}

NOVA_INLINE constexpr bool dashequals(std::string_view value, std::string_view subvalue)
{
    if(startswith(value, subvalue))
        return (value.length() == subvalue.length() || value.at(subvalue.length()) == '-');
    return false;
}

static bool match_attribute_value(const AttributeSelector& selector, std::string_view value)
{
    if(selector.matchType == AttributeSelector::MatchType::None)
        return !value.empty();
    if(selector.matchType == AttributeSelector::MatchType::Equals)
        return equals(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::Contains)
        return contains(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::Includes)
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
        if(selector.matchType == AttributeSelector::MatchType::None)
            return true; // `[attr]` matches an attribute that is there, even an empty one
        return match_attribute_value(selector, *value);
    }

    return match_attribute_value(selector, el->attribute(selector.id));
}

static bool match_simple_selector(const SimpleSelector& selector, const svg_element* el);

static bool match_pseudo_class_selector(const PseudoClassSelector& selector, const svg_element* el)
{
    if(selector.type == PseudoClassSelector::Type::Empty)
        return el->children().empty();
    if(selector.type == PseudoClassSelector::Type::Root)
        return el->is_root_element();
    if(selector.type == PseudoClassSelector::Type::is) {
        for(const auto& sub_selector : selector.sub_selectors) {
            for(const auto& simpleSelector : sub_selector) {
                if(!match_simple_selector(simpleSelector, el)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::Not) {
        for(const auto& sub_selector : selector.sub_selectors) {
            for(const auto& simpleSelector : sub_selector) {
                if(match_simple_selector(simpleSelector, el)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::FirstChild)
        return !el->previous_element();
    if(selector.type == PseudoClassSelector::Type::LastChild)
        return !el->next_element();
    if(selector.type == PseudoClassSelector::Type::only_child)
        return !(el->previous_element() || el->next_element());
    if(selector.type == PseudoClassSelector::Type::first_of_type) {
        auto sibling = el->previous_element();
        while(sibling) {
            if(sibling->id() == el->id())
                return false;
            sibling = sibling->previous_element();
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::last_of_type) {
        auto sibling = el->next_element();
        while(sibling) {
            if(sibling->id() == el->id())
                return false;
            sibling = sibling->next_element();
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::nth_child
        || selector.type == PseudoClassSelector::Type::nth_last_child
        || selector.type == PseudoClassSelector::Type::nth_of_type
        || selector.type == PseudoClassSelector::Type::nth_last_of_type) {
        const bool from_end = selector.type == PseudoClassSelector::Type::nth_last_child
            || selector.type == PseudoClassSelector::Type::nth_last_of_type;
        const bool of_type = selector.type == PseudoClassSelector::Type::nth_of_type
            || selector.type == PseudoClassSelector::Type::nth_last_of_type;
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

static bool match_selector(const Selector& selector, const svg_element* el)
{
    if(selector.empty())
        return false;
    auto it = selector.rbegin();
    auto end = selector.rend();
    if(!match_simple_selector(*it, el)) {
        return false;
    }

    auto combinator = it->combinator;
    ++it;

    while(it != end) {
        switch(combinator) {
        case SimpleSelector::Combinator::Child:
        case SimpleSelector::Combinator::descendant:
            el = el->parent_element();
            break;
        case SimpleSelector::Combinator::direct_adjacent:
        case SimpleSelector::Combinator::in_direct_adjacent:
            el = el->previous_element();
            break;
        case SimpleSelector::Combinator::None:
            assert(false);
        }

        if(el == nullptr)
            return false;
        if(match_simple_selector(*it, el)) {
            combinator = it->combinator;
            ++it;
        } else if(combinator != SimpleSelector::Combinator::descendant
            && combinator != SimpleSelector::Combinator::in_direct_adjacent) {
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
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        simpleSelector.tag = name;
    } else {
        return false;
    }
    return true;
}

static bool parse_id_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = property_id::Id;
    a.matchType = AttributeSelector::MatchType::Equals;
    if(!read_css_identifier(input, a.value))
        return false;
    simpleSelector.attribute_selectors.push_back(std::move(a));
    return true;
}

static bool parse_class_selector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = property_id::Class;
    a.matchType = AttributeSelector::MatchType::Includes;
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
    a.matchType = AttributeSelector::MatchType::None;
    if(skip_delimiter(input, '='))
        a.matchType = AttributeSelector::MatchType::Equals;
    else if(skipString(input, "*="))
        a.matchType = AttributeSelector::MatchType::Contains;
    else if(skipString(input, "~="))
        a.matchType = AttributeSelector::MatchType::Includes;
    else if(skipString(input, "^="))
        a.matchType = AttributeSelector::MatchType::starts_with;
    else if(skipString(input, "$="))
        a.matchType = AttributeSelector::MatchType::ends_with;
    else if(skipString(input, "|="))
        a.matchType = AttributeSelector::MatchType::dash_equals;
    if(a.matchType != AttributeSelector::MatchType::None) {
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
            f.push_back(char(std::tolower(static_cast<unsigned char>(ch))));
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
        selector.type = PseudoClassSelector::Type::Empty;
    else if(name.compare("root") == 0)
        selector.type = PseudoClassSelector::Type::Root;
    else if(name.compare("not") == 0)
        selector.type = PseudoClassSelector::Type::Not;
    else if(name.compare("first-child") == 0)
        selector.type = PseudoClassSelector::Type::FirstChild;
    else if(name.compare("last-child") == 0)
        selector.type = PseudoClassSelector::Type::LastChild;
    else if(name.compare("only-child") == 0)
        selector.type = PseudoClassSelector::Type::only_child;
    else if(name.compare("first-of-type") == 0)
        selector.type = PseudoClassSelector::Type::first_of_type;
    else if(name.compare("last-of-type") == 0)
        selector.type = PseudoClassSelector::Type::last_of_type;
    else if(name.compare("only-of-type") == 0)
        selector.type = PseudoClassSelector::Type::only_of_type;
    else if(name.compare("nth-child") == 0)
        selector.type = PseudoClassSelector::Type::nth_child;
    else if(name.compare("nth-last-child") == 0)
        selector.type = PseudoClassSelector::Type::nth_last_child;
    else if(name.compare("nth-of-type") == 0)
        selector.type = PseudoClassSelector::Type::nth_of_type;
    else if(name.compare("nth-last-of-type") == 0)
        selector.type = PseudoClassSelector::Type::nth_last_of_type;
    if(selector.type == PseudoClassSelector::Type::nth_child
        || selector.type == PseudoClassSelector::Type::nth_last_child
        || selector.type == PseudoClassSelector::Type::nth_of_type
        || selector.type == PseudoClassSelector::Type::nth_last_of_type) {
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

    if(selector.type == PseudoClassSelector::Type::is || selector.type == PseudoClassSelector::Type::Not) {
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

static bool parse_combinator(std::string_view& input, SimpleSelector::Combinator& combinator)
{
    combinator = SimpleSelector::Combinator::None;
    while(!input.empty() && IS_WS(input.front())) {
        combinator = SimpleSelector::Combinator::descendant;
        input.remove_prefix(1);
    }

    if(skip_delimiter_and_optional_spaces(input, '>'))
        combinator = SimpleSelector::Combinator::Child;
    else if(skip_delimiter_and_optional_spaces(input, '+'))
        combinator = SimpleSelector::Combinator::direct_adjacent;
    else if(skip_delimiter_and_optional_spaces(input, '~'))
        combinator = SimpleSelector::Combinator::in_direct_adjacent;
    return combinator != SimpleSelector::Combinator::None;
}

static bool parse_selector(std::string_view& input, Selector& selector)
{
    auto combinator = SimpleSelector::Combinator::None;
    do {
        bool failed = false;
        SimpleSelector simpleSelector(combinator);
        if(!parse_simple_selector(input, simpleSelector, failed))
            return !failed && (combinator == SimpleSelector::Combinator::descendant);
        selector.push_back(std::move(simpleSelector));
    } while(parse_combinator(input, combinator));
    return true;
}

static bool parse_selectors(std::string_view& input, selector_list& selectors)
{
    do {
        Selector selector;
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

        Declaration declaration;
        declaration.specificity = Specificity::stylesheet;
        declaration.id = propertyid(name);
        declaration.value.assign(value);
        const bool custom = is_custom_property_name(name);
        if(!custom) {
            for(auto& ch : name)
                ch = char(std::tolower(static_cast<unsigned char>(ch)));
        }
        declaration.name = name;
        if(skip_delimiter(input, '!')) {
            skip_optional_spaces(input);
            if(!skipString(input, "important"))
                return false;
            declaration.specificity = Specificity::stylesheet_important;
        }

        // `background-color` is no svg property, but the HTML inside a foreignObject paints it; a custom
        // property (`--x`) is none either, but `var(--x)` in the values of the elements below reads it
        if(declaration.id != property_id::unknown || custom || declaration.name == "background-color")
            declarations.push_back(std::move(declaration));
        skip_optional_spaces_or_delimiter(input, ';');
    } while(!input.empty() && input.front() != '}');
    return skip_delimiter(input, '}');
}

static bool parse_rule(std::string_view& input, Rule& rule)
{
    if(!parse_selectors(input, rule.selectors))
        return false;
    return parse_declarations(input, rule.declarations);
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
static FontFace font_face_from_base64(std::string_view payload)
{
    size_t decoded_length = 0;
    auto* decoded = base64_decode(payload.data(), int(payload.size()), &decoded_length);
    if(decoded == nullptr)
        return FontFace();
    return FontFace(decoded, decoded_length, [](void* closure) { free(closure); }, decoded);
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
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        bold = weight.find("bold") != std::string::npos || std::strtof(weight.c_str(), nullptr) >= 600.f;
    }

    auto italic = false;
    if(auto style_value = find_outside_src("font-style")) {
        std::string style(*style_value);
        for(auto& ch : style)
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
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

        fontFaceCache()->add_font_face(std::string(family), bold, italic, face);
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
                ch = char(std::tolower(static_cast<unsigned char>(ch)));

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

        Rule rule;
        const auto rule_start = input;
        if(!parse_rule(input, rule)) {
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

        for(const auto& selector : rule.selectors) {
            size_t specificity = 0;
            for(const auto& simpleSelector : selector) {
                specificity += (simpleSelector.id == element_id::star) ? 0x0 : 0x1;
                for(const auto& attributeSelector : simpleSelector.attribute_selectors) {
                    specificity += (attributeSelector.id == property_id::Id) ? 0x10000 : 0x100;
                }
                for(const auto& pseudoClassSelector : simpleSelector.pseudo_class_selectors) {
                    specificity += 0x100;
                }
            }

            rules.emplace_back(selector, rule.declarations, specificity, rules.size());
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
inline bool strip_important_flag(std::string& value)
{
    auto bang = value.rfind('!');
    if(bang == std::string::npos)
        return false;
    std::string_view flag(value);
    flag.remove_prefix(bang + 1);
    skip_optional_spaces(flag);
    strip_trailing_spaces(flag);
    if(flag.size() != 9)
        return false;
    for(size_t i = 0; i < flag.size(); ++i) {
        if(std::tolower(static_cast<unsigned char>(flag[i])) != "important"[i])
            return false;
    }

    value.erase(bang);
    while(!value.empty() && IS_WS(value.back()))
        value.pop_back();
    return true;
}

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
            el->set_custom_property(important ? Specificity::inline_important : Specificity::InlineStyle, name, value);
            skip_optional_spaces_or_delimiter(input, ';');
            continue;
        }

        auto id = csspropertyid(name);
        if(id != property_id::unknown) {
            const bool important = strip_important_flag(value);
            el->set_attribute(important ? Specificity::inline_important : Specificity::InlineStyle, id, value);
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
            if(skipString(input, "amp")) {
                output.push_back('&');
            } else if(skipString(input, "lt")) {
                output.push_back('<');
            } else if(skipString(input, "gt")) {
                output.push_back('>');
            } else if(skipString(input, "quot")) {
                output.push_back('\"');
            } else if(skipString(input, "apos")) {
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

struct html_node {
    std::string tag; // lower case
    std::string id;
    std::string class_name;
    std::string style; // the style="" attribute, verbatim
    int parent{-1};    // index in the nd list; -1 = a direct child of the foreignObject
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
        ch = char(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

// The opening/closing tags of a piece of HTML as a flat list in dom order, each with the index of
// the el it sits in. Only what the cascade needs is read: tag name, id, class, style.
static std::vector<html_node> parse_html_nodes(std::string_view html)
{
    std::vector<html_node> nodes;
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

        html_node nd;
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
        if(attribute.id == property_id::Class)
            value = nd.class_name;
        else if(attribute.id == property_id::Id)
            value = nd.id;
        else if(attribute.id == property_id::Style)
            value = nd.style;
        if(!match_attribute_value(attribute, value))
            return false;
    }

    return true;
}

// matchSelector() for an HTML nd: the rightmost compound must match the nd, the ones before it its
// ancestors -- HTML elements first, then `owner` (the foreignObject) and the svg elements above it.
// Sibling combinators (+ ~) are not modelled for HTML, so a selector using one never matches.
static bool match_html_selector(const Selector& selector, const std::vector<html_node>& nodes, int index, const svg_element* owner)
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
    auto combinator = it->combinator;
    ++it;
    while(it != end) {
        if(combinator != SimpleSelector::Combinator::Child && combinator != SimpleSelector::Combinator::descendant)
            return false;
        nd = parent_of(nd);
        if(!nd)
            return false;
        if(matches(*it, nd)) {
            combinator = it->combinator;
            ++it;
        } else if(combinator != SimpleSelector::Combinator::descendant) {
            return false;
        }
    }

    return true;
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
static void offer_inline_style(std::string_view style, cascaded_value& color, cascaded_value& background)
{
    while(!style.empty()) {
        auto end = style.find(';');
        auto declaration = style.substr(0, end);
        style.remove_prefix(end == std::string_view::npos ? style.size() : end + 1);
        auto colon = declaration.find(':');
        if(colon == std::string_view::npos)
            continue;
        auto name_text = declaration.substr(0, colon);
        strip_leading_and_trailing_spaces(name_text);
        auto name = lower_case(name_text);
        std::string value(declaration.substr(colon + 1));
        const int priority = strip_important_flag(value) ? Specificity::inline_important : Specificity::InlineStyle;
        std::string_view trimmed(value);
        strip_leading_and_trailing_spaces(trimmed);
        if(name == "color")
            color.offer(priority, std::string(trimmed));
        else if(name == "background-color")
            background.offer(priority, std::string(trimmed));
    }
}

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
static void resolve_foreign_object_style(svg_foreign_object_element* foreignObject, const rule_data_list& rules)
{
    const auto nodes = parse_html_nodes(foreignObject->raw_content());
    std::vector<const rule_data*> relevant;
    for(const auto& rule : rules) {
        for(const auto& declaration : rule.declarations()) {
            if(declaration.name == "color" || declaration.name == "background-color") {
                relevant.push_back(&rule);
                break;
            }
        }
    }

    std::vector<std::optional<stb::color>> color_of(nodes.size());
    std::optional<stb::color> text_color, text_color_of_last_node, background;
    bool any_text = false;
    for(size_t i = 0; i < nodes.size(); ++i) {
        cascaded_value color, box;
        for(const auto* rule : relevant) {
            if(!match_html_selector(rule->selector(), nodes, int(i), foreignObject))
                continue;
            for(const auto& declaration : rule->declarations()) {
                if(declaration.name == "color")
                    color.offer(declaration.specificity, declaration.value);
                else if(declaration.name == "background-color")
                    box.offer(declaration.specificity, declaration.value);
            }
        }

        offer_inline_style(nodes[i].style, color, box);

        std::optional<stb::color> declared;
        if(color.specificity >= 0)
            declared = parse_css_color(color.value);
        color_of[i] = declared ? declared : (nodes[i].parent >= 0 ? color_of[nodes[i].parent] : std::nullopt);
        text_color_of_last_node = color_of[i];
        if(nodes[i].has_text) {
            text_color = color_of[i];
            any_text = true;
        }

        if(box.specificity >= 0) {
            if(auto fill = parse_css_color(box.value); fill && is_visible(*fill))
                background = fill;
        }
    }

    foreignObject->set_html_style(any_text ? text_color : text_color_of_last_node, background);
}

// One RuleDataList out of several stylesheets applied one after another, each parsed on its own (a rule
// the parser can't read ends only its own sheet) and numbered on from the sheets before it, so a later
// sheet wins a tie. Sorted the way apply_style_sheet() sorts.
static rule_data_list parse_style_sheets(const std::vector<std::string>& sheets)
{
    rule_data_list all;
    for(const auto& sheet : sheets) {
        for(auto& rule : parse_style_sheet(sheet))
            all.emplace_back(rule.selector(), rule.declarations(), rule.specificity(), all.size());
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
            const auto& selector = rules[i].selector();
            if(selector.empty())
                continue; // can never match
            const auto& last = selector.back();
            if(last.id != element_id::star) {
                m_by_tag[size_t(last.id)].push_back(uint32_t(i));
                continue;
            }

            const std::string* class_name = nullptr;
            for(const auto& attribute : last.attribute_selectors) {
                if(attribute.id == property_id::Class && attribute.matchType == AttributeSelector::MatchType::Includes
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
            std::string_view classes(el->attribute(property_id::Class));
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
