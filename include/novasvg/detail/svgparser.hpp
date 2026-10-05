#include <cassert>

namespace novasvg {

struct SimpleSelector;

using Selector = std::vector<SimpleSelector>;
using SelectorList = std::vector<Selector>;

struct AttributeSelector {
    enum class MatchType {
        None,
        Equals,
        Contains,
        Includes,
        StartsWith,
        EndsWith,
        DashEquals
    };

    MatchType matchType{MatchType::None};
    PropertyID id{PropertyID::Unknown};
    std::string value;
};

struct PseudoClassSelector {
    enum class Type {
        Unknown,
        Empty,
        Root,
        Is,
        Not,
        FirstChild,
        LastChild,
        OnlyChild,
        FirstOfType,
        LastOfType,
        OnlyOfType
    };

    Type type{Type::Unknown};
    SelectorList subSelectors;
};

struct SimpleSelector {
    enum class Combinator {
        None,
        Descendant,
        Child,
        DirectAdjacent,
        InDirectAdjacent
    };

    explicit SimpleSelector(Combinator combinator) : combinator(combinator) {}

    Combinator combinator{Combinator::Descendant};
    ElementID id{ElementID::Star};
    std::string tag; // lower-case tag name as written; empty for `*`. For matching HTML (no ElementID for it)
    std::vector<AttributeSelector> attributeSelectors;
    std::vector<PseudoClassSelector> pseudoClassSelectors;
};

struct Declaration {
    int specificity;
    PropertyID id; // Unknown for a property no SVG element has (only the HTML in a foreignObject reads those)
    std::string value;
    std::string name; // lower-case property name as written
};

using DeclarationList = std::vector<Declaration>;

struct Rule {
    SelectorList selectors;
    DeclarationList declarations;
};

class RuleData {
public:
    RuleData(const Selector& selector, const DeclarationList& declarations, size_t specificity, size_t position)
        : m_selector(selector), m_declarations(declarations), m_specificity(specificity), m_position(position)
    {}

    bool isLessThan(const RuleData& rule) const { return std::tie(m_specificity, m_position) < std::tie(rule.m_specificity, rule.m_position); }

    const Selector& selector() const { return m_selector; }
    const DeclarationList& declarations() const { return m_declarations; }
    size_t specificity() const { return m_specificity; }
    size_t position() const { return m_position; }

    bool match(const SVGElement* element) const;

private:
    Selector m_selector;
    DeclarationList m_declarations;
    size_t m_specificity;
    size_t m_position;
};

inline bool operator<(const RuleData& a, const RuleData& b) { return a.isLessThan(b); }

using RuleDataList = std::vector<RuleData>;

NOVASVG_INLINE constexpr bool equals(std::string_view value, std::string_view subvalue)
{
    return value.compare(subvalue) == 0;
}

NOVASVG_INLINE constexpr bool contains(std::string_view value, std::string_view subvalue)
{
    return value.find(subvalue) != std::string_view::npos;
}

NOVASVG_INLINE constexpr bool includes(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    std::string_view input(value);
    while(!input.empty()) {
        skipOptionalSpaces(input);
        std::string_view start(input);
        while(!input.empty() && !IS_WS(input.front()))
            input.remove_prefix(1);
        if(subvalue == start.substr(0, start.length() - input.length())) {
            return true;
        }
    }

    return false;
}

NOVASVG_INLINE constexpr bool startswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(0, subvalue.size());
}

NOVASVG_INLINE constexpr bool endswith(std::string_view value, std::string_view subvalue)
{
    if(subvalue.empty() || subvalue.length() > value.length())
        return false;
    return subvalue == value.substr(value.size() - subvalue.size(), subvalue.size());
}

NOVASVG_INLINE constexpr bool dashequals(std::string_view value, std::string_view subvalue)
{
    if(startswith(value, subvalue))
        return (value.length() == subvalue.length() || value.at(subvalue.length()) == '-');
    return false;
}

static bool matchAttributeValue(const AttributeSelector& selector, std::string_view value)
{
    if(selector.matchType == AttributeSelector::MatchType::None)
        return !value.empty();
    if(selector.matchType == AttributeSelector::MatchType::Equals)
        return equals(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::Contains)
        return contains(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::Includes)
        return includes(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::StartsWith)
        return startswith(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::EndsWith)
        return endswith(value, selector.value);
    if(selector.matchType == AttributeSelector::MatchType::DashEquals)
        return dashequals(value, selector.value);
    return false;
}

static bool matchAttributeSelector(const AttributeSelector& selector, const SVGElement* element)
{
    return matchAttributeValue(selector, element->getAttribute(selector.id));
}

static bool matchSimpleSelector(const SimpleSelector& selector, const SVGElement* element);

static bool matchPseudoClassSelector(const PseudoClassSelector& selector, const SVGElement* element)
{
    if(selector.type == PseudoClassSelector::Type::Empty)
        return element->children().empty();
    if(selector.type == PseudoClassSelector::Type::Root)
        return element->isRootElement();
    if(selector.type == PseudoClassSelector::Type::Is) {
        for(const auto& subSelector : selector.subSelectors) {
            for(const auto& simpleSelector : subSelector) {
                if(!matchSimpleSelector(simpleSelector, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::Not) {
        for(const auto& subSelector : selector.subSelectors) {
            for(const auto& simpleSelector : subSelector) {
                if(matchSimpleSelector(simpleSelector, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::FirstChild)
        return !element->previousElement();
    if(selector.type == PseudoClassSelector::Type::LastChild)
        return !element->nextElement();
    if(selector.type == PseudoClassSelector::Type::OnlyChild)
        return !(element->previousElement() || element->nextElement());
    if(selector.type == PseudoClassSelector::Type::FirstOfType) {
        auto sibling = element->previousElement();
        while(sibling) {
            if(sibling->id() == element->id())
                return false;
            sibling = sibling->previousElement();
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::LastOfType) {
        auto sibling = element->nextElement();
        while(sibling) {
            if(sibling->id() == element->id())
                return false;
            sibling = sibling->nextElement();
        }

        return true;
    }

    return false;
}

static bool matchSimpleSelector(const SimpleSelector& selector, const SVGElement* element)
{
    if(selector.id != ElementID::Star && selector.id != element->id())
        return false;
    for(const auto& sel : selector.attributeSelectors) {
        if(!matchAttributeSelector(sel, element)) {
            return false;
        }
    }

    for(const auto& sel : selector.pseudoClassSelectors) {
        if(!matchPseudoClassSelector(sel, element)) {
            return false;
        }
    }

    return true;
}

static bool matchSelector(const Selector& selector, const SVGElement* element)
{
    if(selector.empty())
        return false;
    auto it = selector.rbegin();
    auto end = selector.rend();
    if(!matchSimpleSelector(*it, element)) {
        return false;
    }

    auto combinator = it->combinator;
    ++it;

    while(it != end) {
        switch(combinator) {
        case SimpleSelector::Combinator::Child:
        case SimpleSelector::Combinator::Descendant:
            element = element->parentElement();
            break;
        case SimpleSelector::Combinator::DirectAdjacent:
        case SimpleSelector::Combinator::InDirectAdjacent:
            element = element->previousElement();
            break;
        case SimpleSelector::Combinator::None:
            assert(false);
        }

        if(element == nullptr)
            return false;
        if(matchSimpleSelector(*it, element)) {
            combinator = it->combinator;
            ++it;
        } else if(combinator != SimpleSelector::Combinator::Descendant
            && combinator != SimpleSelector::Combinator::InDirectAdjacent) {
            return false;
        }
    }

    return true;
}

NOVASVG_INLINE bool RuleData::match(const SVGElement* element) const
{
    return matchSelector(m_selector, element);
}

constexpr bool IS_CSS_STARTNAMECHAR(int c) { return IS_ALPHA(c) || c == '_' || c == '-'; }
constexpr bool IS_CSS_NAMECHAR(int c) { return IS_CSS_STARTNAMECHAR(c) || IS_NUM(c); }

inline bool readCSSIdentifier(std::string_view& input, std::string& output)
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

static bool parseTagSelector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    if(skipDelimiter(input, '*'))
        simpleSelector.id = ElementID::Star;
    else if(readCSSIdentifier(input, name)) {
        simpleSelector.id = elementid(name);
        for(auto& ch : name)
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        simpleSelector.tag = name;
    } else {
        return false;
    }
    return true;
}

static bool parseIdSelector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = PropertyID::Id;
    a.matchType = AttributeSelector::MatchType::Equals;
    if(!readCSSIdentifier(input, a.value))
        return false;
    simpleSelector.attributeSelectors.push_back(std::move(a));
    return true;
}

static bool parseClassSelector(std::string_view& input, SimpleSelector& simpleSelector)
{
    AttributeSelector a;
    a.id = PropertyID::Class;
    a.matchType = AttributeSelector::MatchType::Includes;
    if(!readCSSIdentifier(input, a.value))
        return false;
    simpleSelector.attributeSelectors.push_back(std::move(a));
    return true;
}

static bool parseAttributeSelector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    skipOptionalSpaces(input);
    if(!readCSSIdentifier(input, name))
        return false;
    AttributeSelector a;
    a.id = propertyid(name);
    a.matchType = AttributeSelector::MatchType::None;
    if(skipDelimiter(input, '='))
        a.matchType = AttributeSelector::MatchType::Equals;
    else if(skipString(input, "*="))
        a.matchType = AttributeSelector::MatchType::Contains;
    else if(skipString(input, "~="))
        a.matchType = AttributeSelector::MatchType::Includes;
    else if(skipString(input, "^="))
        a.matchType = AttributeSelector::MatchType::StartsWith;
    else if(skipString(input, "$="))
        a.matchType = AttributeSelector::MatchType::EndsWith;
    else if(skipString(input, "|="))
        a.matchType = AttributeSelector::MatchType::DashEquals;
    if(a.matchType != AttributeSelector::MatchType::None) {
        skipOptionalSpaces(input);
        if(!readCSSIdentifier(input, a.value)) {
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

    skipOptionalSpaces(input);
    if(!skipDelimiter(input, ']'))
        return false;
    simpleSelector.attributeSelectors.push_back(std::move(a));
    return true;
}

static bool parseSelectors(std::string_view& input, SelectorList& selectors);

static bool parsePseudoClassSelector(std::string_view& input, SimpleSelector& simpleSelector)
{
    std::string name;
    if(!readCSSIdentifier(input, name))
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
        selector.type = PseudoClassSelector::Type::OnlyChild;
    else if(name.compare("first-of-type") == 0)
        selector.type = PseudoClassSelector::Type::FirstOfType;
    else if(name.compare("last-of-type") == 0)
        selector.type = PseudoClassSelector::Type::LastOfType;
    else if(name.compare("only-of-type") == 0)
        selector.type = PseudoClassSelector::Type::OnlyOfType;
    if(selector.type == PseudoClassSelector::Type::Is || selector.type == PseudoClassSelector::Type::Not) {
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, '('))
            return false;
        skipOptionalSpaces(input);
        if(!parseSelectors(input, selector.subSelectors))
            return false;
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, ')')) {
            return false;
        }
    }

    simpleSelector.pseudoClassSelectors.push_back(std::move(selector));
    return true;
}

static bool parseSimpleSelector(std::string_view& input, SimpleSelector& simpleSelector, bool& failed)
{
    auto consumed = parseTagSelector(input, simpleSelector);
    do {
        if(skipDelimiter(input, '#'))
            failed = !parseIdSelector(input, simpleSelector);
        else if(skipDelimiter(input, '.'))
            failed = !parseClassSelector(input, simpleSelector);
        else if(skipDelimiter(input, '['))
            failed = !parseAttributeSelector(input, simpleSelector);
        else if(skipDelimiter(input, ':'))
            failed = !parsePseudoClassSelector(input, simpleSelector);
        else
            break;
        consumed = true;
    } while(!failed);
    return consumed && !failed;
}

static bool parseCombinator(std::string_view& input, SimpleSelector::Combinator& combinator)
{
    combinator = SimpleSelector::Combinator::None;
    while(!input.empty() && IS_WS(input.front())) {
        combinator = SimpleSelector::Combinator::Descendant;
        input.remove_prefix(1);
    }

    if(skipDelimiterAndOptionalSpaces(input, '>'))
        combinator = SimpleSelector::Combinator::Child;
    else if(skipDelimiterAndOptionalSpaces(input, '+'))
        combinator = SimpleSelector::Combinator::DirectAdjacent;
    else if(skipDelimiterAndOptionalSpaces(input, '~'))
        combinator = SimpleSelector::Combinator::InDirectAdjacent;
    return combinator != SimpleSelector::Combinator::None;
}

static bool parseSelector(std::string_view& input, Selector& selector)
{
    auto combinator = SimpleSelector::Combinator::None;
    do {
        bool failed = false;
        SimpleSelector simpleSelector(combinator);
        if(!parseSimpleSelector(input, simpleSelector, failed))
            return !failed && (combinator == SimpleSelector::Combinator::Descendant);
        selector.push_back(std::move(simpleSelector));
    } while(parseCombinator(input, combinator));
    return true;
}

static bool parseSelectors(std::string_view& input, SelectorList& selectors)
{
    do {
        Selector selector;
        if(!parseSelector(input, selector))
            return false;
        selectors.push_back(std::move(selector));
    } while(skipDelimiterAndOptionalSpaces(input, ','));
    return true;
}

static bool parseDeclarations(std::string_view& input, DeclarationList& declarations)
{
    if(!skipDelimiter(input, '{'))
        return false;
    skipOptionalSpaces(input);
    do {
        std::string name;
        if(!readCSSIdentifier(input, name))
            return false;
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, ':'))
            return false;
        skipOptionalSpaces(input);
        std::string_view value(input);
        while(!input.empty() && !(input.front() == '!' || input.front() == ';' || input.front() == '}'))
            input.remove_prefix(1);
        value.remove_suffix(input.length());
        stripTrailingSpaces(value);

        Declaration declaration;
        declaration.specificity = Specificity::Stylesheet;
        declaration.id = propertyid(name);
        declaration.value.assign(value);
        for(auto& ch : name)
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        declaration.name = name;
        if(skipDelimiter(input, '!')) {
            skipOptionalSpaces(input);
            if(!skipString(input, "important"))
                return false;
            declaration.specificity = Specificity::StylesheetImportant;
        }

        // `background-color` is no SVG property, but the HTML inside a foreignObject paints it
        if(declaration.id != PropertyID::Unknown || declaration.name == "background-color")
            declarations.push_back(std::move(declaration));
        skipOptionalSpacesOrDelimiter(input, ';');
    } while(!input.empty() && input.front() != '}');
    return skipDelimiter(input, '}');
}

static bool parseRule(std::string_view& input, Rule& rule)
{
    if(!parseSelectors(input, rule.selectors))
        return false;
    return parseDeclarations(input, rule.declarations);
}

// Splits a `src:` value on its *top-level* commas (each a
// `url(...)` optionally followed by `format(...)`) -- paren-depth-aware
// so a comma inside url()/format() (none occur in valid CSS, but
// malformed input shouldn't split incorrectly) doesn't split the list.
static std::vector<std::string_view> splitTopLevelCommas(std::string_view value)
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

static std::string_view stripQuotes(std::string_view value)
{
    stripLeadingAndTrailingSpaces(value);
    if(!value.empty() && (value.front() == '\'' || value.front() == '"')) {
        auto quote = value.front();
        value.remove_prefix(1);
        if(!value.empty() && value.back() == quote)
            value.remove_suffix(1);
    }
    return value;
}

// Loads an `@font-face` rule's embedded font (a `src: url(data:...)`
// entry) and registers it, matching what the CSS itself declares:
// `font-family`, `font-weight`/`font-style` (bold/italic only -- numeric
// weights other than "bold" all collapse to regular, matching the rest
// of this codebase's Font handling elsewhere). Only *embedded* fonts
// (data: URIs) are in scope -- a `url("external.ttf")` reference isn't
// something we can resolve without filesystem/network context the SVG
// doesn't provide, and is silently skipped, same as before this existed.
//
// Only raw TrueType/OpenType (SFNT) data is understood (via the same
// stb_truetype loader every other font already goes through) -- WOFF/
// WOFF2 aren't decoded. Rather than inspect each src alternative's
// `format()` hint to guess which might work, this just tries loading
// each in turn and keeps the first one that actually parses as valid
// SFNT, exactly matching how real `@font-face` `src` fallback lists are
// meant to be read (most-preferred format first, older formats after).
static void parseFontFaceRule(std::string_view block)
{
    auto familyValue = findRawDeclarationValue(block, "font-family");
    if(!familyValue)
        return;
    auto family = stripQuotes(*familyValue);
    if(family.empty())
        return;

    auto srcValue = findRawDeclarationValue(block, "src");
    if(!srcValue)
        return;

    auto bold = false;
    if(auto weightValue = findRawDeclarationValue(block, "font-weight")) {
        std::string weight(*weightValue);
        for(auto& ch : weight)
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        bold = weight.find("bold") != std::string::npos || std::strtof(weight.c_str(), nullptr) >= 600.f;
    }

    auto italic = false;
    if(auto styleValue = findRawDeclarationValue(block, "font-style")) {
        std::string style(*styleValue);
        for(auto& ch : style)
            ch = char(std::tolower(static_cast<unsigned char>(ch)));
        italic = style.find("italic") != std::string::npos || style.find("oblique") != std::string::npos;
    }

    for(auto alternative : splitTopLevelCommas(*srcValue)) {
        auto urlPos = alternative.find("url(");
        if(urlPos == std::string_view::npos)
            continue;
        auto contentStart = urlPos + 4;
        auto closeParen = alternative.find(')', contentStart);
        if(closeParen == std::string_view::npos)
            continue;
        auto url = stripQuotes(alternative.substr(contentStart, closeParen - contentStart));

        if(url.compare(0, 5, "data:") != 0)
            continue; // not embedded -- out of scope, see comment above
        auto base64Pos = url.find("base64,");
        if(base64Pos == std::string_view::npos)
            continue;
        auto payload = url.substr(base64Pos + 7);

        size_t decodedLength = 0;
        auto* decoded = base64_decode(payload.data(), int(payload.size()), &decodedLength);
        if(decoded == nullptr)
            continue;

        FontFace face(decoded, decodedLength, [](void* closure) { free(closure); }, decoded);
        if(face.isNull())
            continue; // not raw SFNT (e.g. still WOFF/WOFF2) -- try the next alternative

        fontFaceCache()->addFontFace(std::string(family), bold, italic, face);
        return; // first successfully-loaded alternative wins
    }
}

static RuleDataList parseStyleSheet(std::string_view input)
{
    RuleDataList rules;
    while(!input.empty()) {
        skipOptionalSpaces(input);
        if(skipDelimiter(input, '@')) {
            std::string atKeyword;
            readCSSIdentifier(input, atKeyword);
            for(auto& ch : atKeyword)
                ch = char(std::tolower(static_cast<unsigned char>(ch)));

            if(atKeyword == "font-face") {
                skipOptionalSpaces(input);
                if(skipDelimiter(input, '{')) {
                    int depth = 1;
                    size_t blockLength = 0;
                    auto block = input;
                    while(blockLength < input.size() && depth > 0) {
                        auto ch = input[blockLength];
                        if(ch == '{') ++depth;
                        else if(ch == '}') --depth;
                        ++blockLength;
                    }
                    parseFontFaceRule(block.substr(0, depth == 0 ? blockLength - 1 : blockLength));
                    input.remove_prefix(blockLength);
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
        if(!parseRule(input, rule))
            break;
        for(const auto& selector : rule.selectors) {
            size_t specificity = 0;
            for(const auto& simpleSelector : selector) {
                specificity += (simpleSelector.id == ElementID::Star) ? 0x0 : 0x1;
                for(const auto& attributeSelector : simpleSelector.attributeSelectors) {
                    specificity += (attributeSelector.id == PropertyID::Id) ? 0x10000 : 0x100;
                }
                for(const auto& pseudoClassSelector : simpleSelector.pseudoClassSelectors) {
                    specificity += 0x100;
                }
            }

            rules.emplace_back(selector, rule.declarations, specificity, rules.size());
        }
    }

    return rules;
}

static SelectorList parseQuerySelectors(std::string_view input)
{
    SelectorList selectors;
    stripLeadingAndTrailingSpaces(input);
    if(!parseSelectors(input, selectors)
        || !input.empty()) {
        return SelectorList();
    }

    return selectors;
}

// Removes a trailing `!important` (any case, any spacing around the bang) from a declaration value and
// reports whether there was one. `fill:#f9f !important` is the colour #f9f, flagged important -- it is
// not a colour called "#f9f !important".
inline bool stripImportantFlag(std::string& value)
{
    auto bang = value.rfind('!');
    if(bang == std::string::npos)
        return false;
    std::string_view flag(value);
    flag.remove_prefix(bang + 1);
    skipOptionalSpaces(flag);
    stripTrailingSpaces(flag);
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

inline void parseInlineStyle(std::string_view input, SVGElement* element)
{
    std::string name;
    skipOptionalSpaces(input);
    while(readCSSIdentifier(input, name)) {
        skipOptionalSpaces(input);
        if(!skipDelimiter(input, ':'))
            return;
        std::string value;
        while(!input.empty() && input.front() != ';') {
            value.push_back(input.front());
            input.remove_prefix(1);
        }

        auto id = csspropertyid(name);
        if(id != PropertyID::Unknown) {
            const bool important = stripImportantFlag(value);
            element->setAttribute(important ? Specificity::InlineImportant : Specificity::InlineStyle, id, value);
        }
        skipOptionalSpacesOrDelimiter(input, ';');
    }
}

inline void removeStyleComments(std::string& value)
{
    auto start = value.find("/*");
    while(start != std::string::npos) {
        auto end = value.find("*/", start + 2);
        value.erase(start, end - start + 2);
        start = value.find("/*");
    }
}

inline bool decodeText(std::string_view input, std::string& output)
{
    output.clear();
    while(!input.empty()) {
        auto ch = input.front();
        input.remove_prefix(1);
        if(ch != '&') {
            output.push_back(ch);
            continue;
        }

        if(skipDelimiter(input, '#')) {
            int base = 10;
            if(skipDelimiter(input, 'x'))
                base = 16;
            unsigned int cp;
            if(!parseInteger(input, cp, base))
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

        if(!skipDelimiter(input, ';')) {
            return false;
        }
    }

    return true;
}

constexpr bool IS_STARTNAMECHAR(int c) { return IS_ALPHA(c) ||  c == '_' || c == ':'; }
constexpr bool IS_NAMECHAR(int c) { return IS_STARTNAMECHAR(c) || IS_NUM(c) || c == '-' || c == '.'; }

inline bool readIdentifier(std::string_view& input, std::string& output)
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

NOVASVG_INLINE bool Document::parse(const char* data, size_t length)
{
    std::string buffer;
    std::string styleSheet;
    SVGElement* currentElement = nullptr;
    int ignoring = 0;
    auto handleText = [&](std::string_view text, bool in_cdata) {
        if(text.empty() || currentElement == nullptr || ignoring > 0)
            return;
        if(currentElement->id() != ElementID::Text && currentElement->id() != ElementID::Tspan && currentElement->id() != ElementID::Style) {
            return;
        }

        if(in_cdata) {
            buffer.assign(text);
        } else {
            decodeText(text, buffer);
        }

        if(currentElement->id() == ElementID::Style) {
            removeStyleComments(buffer);
            styleSheet.append(buffer);
        } else {
            auto node = std::make_unique<SVGTextNode>(this);
            node->setData(buffer);
            currentElement->addChild(std::move(node));
        }
    };

    std::string_view input(data, length);
    if(length >= 3) {
        auto buffer = (const uint8_t*)(data);

        const auto c1 = buffer[0];
        const auto c2 = buffer[1];
        const auto c3 = buffer[2];
        if(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF) {
            input.remove_prefix(3);
        }
    }

    while(!input.empty()) {
        if(currentElement) {
            auto text = input.substr(0, input.find('<'));
            handleText(text, false);
            input.remove_prefix(text.length());
        } else {
            if(!skipOptionalSpaces(input)) {
                break;
            }
        }

        if(!skipDelimiter(input, '<'))
            return false;
        if(skipDelimiter(input, '?')) {
            if(!readIdentifier(input, buffer))
                return false;
            auto n = input.find("?>");
            if(n == std::string_view::npos)
                return false;
            input.remove_prefix(n + 2);
            continue;
        }

        if(skipDelimiter(input, '!')) {
            if(skipString(input, "--")) {
                auto n = input.find("-->");
                if(n == std::string_view::npos)
                    return false;
                handleText(input.substr(0, n), false);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skipString(input, "[CDATA[")) {
                auto n = input.find("]]>");
                if(n == std::string_view::npos)
                    return false;
                handleText(input.substr(0, n), true);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skipString(input, "DOCTYPE")) {
                while(!input.empty() && input.front() != '>') {
                    if(input.front() == '[') {
                        int depth = 1;
                        input.remove_prefix(1);
                        while(!input.empty() && depth > 0) {
                            if(input.front() == '[') ++depth;
                            else if(input.front() == ']') --depth;
                            input.remove_prefix(1);
                        }
                    } else {
                        input.remove_prefix(1);
                    }
                }

                if(!skipDelimiter(input, '>'))
                    return false;
                continue;
            }

            return false;
        }

        if(skipDelimiter(input, '/')) {
            if(currentElement == nullptr && ignoring == 0)
                return false;
            if(!readIdentifier(input, buffer))
                return false;
            if(ignoring == 0) {
                auto id = elementid(buffer);
                if(id != currentElement->id())
                    return false;
                currentElement = currentElement->parentElement();
            } else {
                --ignoring;
            }

            skipOptionalSpaces(input);
            if(!skipDelimiter(input, '>'))
                return false;
            continue;
        }

        if(!readIdentifier(input, buffer))
            return false;
        SVGElement* element = nullptr;
        if(ignoring > 0) {
            ++ignoring;
        } else {
            auto id = elementid(buffer);
            if(id == ElementID::Unknown) {
                ignoring = 1;
            } else {
                if(m_rootElement && currentElement == nullptr)
                    return false;
                if(m_rootElement == nullptr) {
                    if(id != ElementID::Svg)
                        return false;
                    m_rootElement = std::make_unique<SVGRootElement>(this);
                    element = m_rootElement.get();
                } else {
                    auto child = SVGElement::create(this, id);
                    element = child.get();
                    currentElement->addChild(std::move(child));
                }
            }
        }

        skipOptionalSpaces(input);
        while(readIdentifier(input, buffer)) {
            skipOptionalSpaces(input);
            if(!skipDelimiter(input, '='))
                return false;
            skipOptionalSpaces(input);
            if(input.empty() || !(input.front() == '\"' || input.front() == '\''))
                return false;
            auto quote = input.front();
            input.remove_prefix(1);
            auto n = input.find(quote);
            if(n == std::string_view::npos)
                return false;
            auto id = PropertyID::Unknown;
            if(element != nullptr)
                id = propertyid(buffer);
            if(id != PropertyID::Unknown) {
                decodeText(input.substr(0, n), buffer);
                if(id == PropertyID::Style) {
                    removeStyleComments(buffer);
                    parseInlineStyle(buffer, element);
                } else {
                    if(id == PropertyID::Id)
                        m_rootElement->addElementById(buffer, element);
                    element->setAttribute(Specificity::PresentationAttribute, id, buffer);
                }
            }

            input.remove_prefix(n + 1);
            skipOptionalSpaces(input);
        }

        if(skipDelimiter(input, '>')) {
            if(element != nullptr) {
                if(element->id() == ElementID::ForeignObject) {
                    // foreignObject content is arbitrary HTML/XML, not SVG --
                    // capture it verbatim instead of trying (and failing) to
                    // parse div/span/p/etc. as SVG elements. Mirrors the
                    // CDATA/comment handling above: find the matching close
                    // tag and skip straight past it.
                    static constexpr std::string_view closeTag = "</foreignObject>";
                    auto n = input.find(closeTag);
                    if(n == std::string_view::npos)
                        return false;
                    static_cast<SVGForeignObjectElement*>(element)->setRawContent(std::string(input.substr(0, n)));
                    input.remove_prefix(n + closeTag.length());
                } else {
                    currentElement = element;
                }
            }
            continue;
        }

        if(skipDelimiter(input, '/')) {
            if(!skipDelimiter(input, '>'))
                return false;
            if(ignoring > 0)
                --ignoring;
            continue;
        }

        return false;
    }

    if(m_rootElement == nullptr || ignoring > 0 || !input.empty())
        return false;
    applyStyleSheet(styleSheet);
    m_rootElement->build();
    return true;
}

// ---------------------------------------------------------------------------------------------------
// The HTML inside a <foreignObject>, styled by the document's CSS.
//
// ForeignObjectSimple paints HTML as plain text in one colour on one optional box, so all the CSS has to
// answer is: which `color` does the text end up with, and which `background-color` box sits behind it.
// A browser answers that with the cascade, and the part that matters in practice (mermaid) is a selector
// that reaches *through* the foreignObject into the SVG around it: `.section-0 span{color:black}` styles a
// <span> because the <g class="section-0"> it lives in matches. So the HTML tags are modelled as nodes
// that have a parent chain -- first the HTML ancestors, then the foreignObject, then the SVG elements
// above it -- and the very same selector matcher the SVG elements use walks that chain.
// ---------------------------------------------------------------------------------------------------

struct HtmlNode {
    std::string tag; // lower case
    std::string id;
    std::string className;
    std::string style; // the style="" attribute, verbatim
    int parent{-1};    // index in the node list; -1 = a direct child of the foreignObject
    bool hasText{false}; // has non-blank text of its own
};

static bool isVoidHtmlTag(const std::string& tag)
{
    static constexpr std::string_view names[] = {"area", "base", "br", "col", "embed", "hr", "img",
                                                 "input", "link", "meta", "source", "track", "wbr"};
    for(auto name : names) {
        if(tag == name)
            return true;
    }

    return false;
}

static std::string lowerCase(std::string_view text)
{
    std::string out(text);
    for(auto& ch : out)
        ch = char(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

// The opening/closing tags of a piece of HTML as a flat list in document order, each with the index of
// the element it sits in. Only what the cascade needs is read: tag name, id, class, style.
static std::vector<HtmlNode> parseHtmlNodes(std::string_view html)
{
    std::vector<HtmlNode> nodes;
    std::vector<int> open;
    size_t pos = 0;
    while(pos < html.size()) {
        auto lt = html.find('<', pos);
        auto text = html.substr(pos, lt == std::string_view::npos ? std::string_view::npos : lt - pos);
        if(!open.empty()) {
            for(char ch : text) {
                if(!IS_WS(ch)) {
                    nodes[open.back()].hasText = true;
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
        pos = std::min(gt + 1, html.size());
        if(inside.empty() || inside.front() == '!' || inside.front() == '?')
            continue;

        if(inside.front() == '/') {
            inside.remove_prefix(1);
            size_t n = 0;
            while(n < inside.size() && !IS_WS(inside[n]))
                ++n;
            auto name = lowerCase(inside.substr(0, n));
            for(size_t i = open.size(); i-- > 0;) {
                if(nodes[open[i]].tag == name) {
                    open.resize(i);
                    break;
                }
            }

            continue;
        }

        HtmlNode node;
        size_t n = 0;
        while(n < inside.size() && !IS_WS(inside[n]) && inside[n] != '/')
            ++n;
        node.tag = lowerCase(inside.substr(0, n));
        inside.remove_prefix(n);
        bool selfClosing = false;
        while(!inside.empty()) {
            skipOptionalSpaces(inside);
            if(inside.empty())
                break;
            if(inside.front() == '/') {
                selfClosing = true;
                inside.remove_prefix(1);
                continue;
            }

            selfClosing = false;
            size_t m = 0;
            while(m < inside.size() && !IS_WS(inside[m]) && inside[m] != '=' && inside[m] != '/')
                ++m;
            auto name = lowerCase(inside.substr(0, m));
            inside.remove_prefix(m);
            skipOptionalSpaces(inside);
            std::string value;
            if(!inside.empty() && inside.front() == '=') {
                inside.remove_prefix(1);
                skipOptionalSpaces(inside);
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
                node.className = value;
            else if(name == "id")
                node.id = value;
            else if(name == "style")
                node.style = value;
        }

        node.parent = open.empty() ? -1 : open.back();
        nodes.push_back(std::move(node));
        if(!selfClosing && !isVoidHtmlTag(nodes.back().tag))
            open.push_back(int(nodes.size()) - 1);
    }

    return nodes;
}

// A step along the ancestor chain: an HTML node, or -- once the HTML runs out -- an SVG element.
struct StyleNode {
    const HtmlNode* html{nullptr};
    const SVGElement* svg{nullptr};
    explicit operator bool() const { return html || svg; }
};

static bool matchHtmlSimpleSelector(const SimpleSelector& selector, const HtmlNode& node)
{
    if(selector.id != ElementID::Star && (selector.tag.empty() || selector.tag != node.tag))
        return false;
    if(!selector.pseudoClassSelectors.empty())
        return false; // :first-child, :not(...) & co are not modelled for HTML
    for(const auto& attribute : selector.attributeSelectors) {
        std::string_view value;
        if(attribute.id == PropertyID::Class)
            value = node.className;
        else if(attribute.id == PropertyID::Id)
            value = node.id;
        else if(attribute.id == PropertyID::Style)
            value = node.style;
        if(!matchAttributeValue(attribute, value))
            return false;
    }

    return true;
}

// matchSelector() for an HTML node: the rightmost compound must match the node, the ones before it its
// ancestors -- HTML elements first, then `owner` (the foreignObject) and the SVG elements above it.
// Sibling combinators (+ ~) are not modelled for HTML, so a selector using one never matches.
static bool matchHtmlSelector(const Selector& selector, const std::vector<HtmlNode>& nodes, int index, const SVGElement* owner)
{
    if(selector.empty())
        return false;
    auto parentOf = [&](const StyleNode& node) {
        if(node.html) {
            if(node.html->parent >= 0)
                return StyleNode{&nodes[node.html->parent], nullptr};
            return StyleNode{nullptr, owner};
        }

        return StyleNode{nullptr, node.svg->parentElement()};
    };
    auto matches = [](const SimpleSelector& simple, const StyleNode& node) {
        return node.html ? matchHtmlSimpleSelector(simple, *node.html) : matchSimpleSelector(simple, node.svg);
    };

    StyleNode node{&nodes[index], nullptr};
    auto it = selector.rbegin();
    auto end = selector.rend();
    if(!matches(*it, node))
        return false;
    auto combinator = it->combinator;
    ++it;
    while(it != end) {
        if(combinator != SimpleSelector::Combinator::Child && combinator != SimpleSelector::Combinator::Descendant)
            return false;
        node = parentOf(node);
        if(!node)
            return false;
        if(matches(*it, node)) {
            combinator = it->combinator;
            ++it;
        } else if(combinator != SimpleSelector::Combinator::Descendant) {
            return false;
        }
    }

    return true;
}

// The declaration of one property that currently wins, by cascade priority (a later one wins a tie).
struct CascadedValue {
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
static void offerInlineStyle(std::string_view style, CascadedValue& color, CascadedValue& background)
{
    while(!style.empty()) {
        auto end = style.find(';');
        auto declaration = style.substr(0, end);
        style.remove_prefix(end == std::string_view::npos ? style.size() : end + 1);
        auto colon = declaration.find(':');
        if(colon == std::string_view::npos)
            continue;
        auto nameText = declaration.substr(0, colon);
        stripLeadingAndTrailingSpaces(nameText);
        auto name = lowerCase(nameText);
        std::string value(declaration.substr(colon + 1));
        const int priority = stripImportantFlag(value) ? Specificity::InlineImportant : Specificity::InlineStyle;
        std::string_view trimmed(value);
        stripLeadingAndTrailingSpaces(trimmed);
        if(name == "color")
            color.offer(priority, std::string(trimmed));
        else if(name == "background-color")
            background.offer(priority, std::string(trimmed));
    }
}

// Works out the text colour and the box colour the CSS gives this foreignObject's HTML and stores them on
// it. `rules` must be sorted (least to most specific, then source order), as applyStyleSheet() leaves them.
//  * text colour: `color` inherits, so every node has the colour of its own rule or else its parent's; the
//    text is painted in the colour of the last element that holds text itself (black if nothing sets one).
//    This is the HTML's CSS `color` -- deliberately not the SVG `fill` that happens to be in effect around
//    the foreignObject: a classDef like `.green>*{fill:#9f6}` also matches the label's <g>, and must not
//    turn the label text the same shade as its box.
//  * box colour: `background-color` does not inherit. Checked against a real WebKit render: mermaid's edge
//    labels have an opaque box on an inner <span> stacked over a translucent one on the outer <div>, and
//    the opaque one is what shows -- so the last element in document order that has a visible box wins,
//    like paint order (later/nested elements paint over earlier ones). `transparent` is no box.
static void resolveForeignObjectStyle(SVGForeignObjectElement* foreignObject, const RuleDataList& rules)
{
    const auto nodes = parseHtmlNodes(foreignObject->rawContent());
    std::vector<const RuleData*> relevant;
    for(const auto& rule : rules) {
        for(const auto& declaration : rule.declarations()) {
            if(declaration.name == "color" || declaration.name == "background-color") {
                relevant.push_back(&rule);
                break;
            }
        }
    }

    std::vector<std::optional<Color>> colorOf(nodes.size());
    std::optional<Color> textColor, textColorOfLastNode, background;
    bool anyText = false;
    for(size_t i = 0; i < nodes.size(); ++i) {
        CascadedValue color, box;
        for(const auto* rule : relevant) {
            if(!matchHtmlSelector(rule->selector(), nodes, int(i), foreignObject))
                continue;
            for(const auto& declaration : rule->declarations()) {
                if(declaration.name == "color")
                    color.offer(declaration.specificity, declaration.value);
                else if(declaration.name == "background-color")
                    box.offer(declaration.specificity, declaration.value);
            }
        }

        offerInlineStyle(nodes[i].style, color, box);

        std::optional<Color> declared;
        if(color.specificity >= 0)
            declared = parseCssColor(color.value);
        colorOf[i] = declared ? declared : (nodes[i].parent >= 0 ? colorOf[nodes[i].parent] : std::nullopt);
        textColorOfLastNode = colorOf[i];
        if(nodes[i].hasText) {
            textColor = colorOf[i];
            anyText = true;
        }

        if(box.specificity >= 0) {
            if(auto fill = parseCssColor(box.value); fill && fill->isVisible())
                background = fill;
        }
    }

    foreignObject->setHtmlStyle(anyText ? textColor : textColorOfLastNode, background);
}

// One RuleDataList out of several stylesheets applied one after another, each parsed on its own (a rule
// the parser can't read ends only its own sheet) and numbered on from the sheets before it, so a later
// sheet wins a tie. Sorted the way applyStyleSheet() sorts.
static RuleDataList parseStyleSheets(const std::vector<std::string>& sheets)
{
    RuleDataList all;
    for(const auto& sheet : sheets) {
        for(auto& rule : parseStyleSheet(sheet))
            all.emplace_back(rule.selector(), rule.declarations(), rule.specificity(), all.size());
    }

    std::sort(all.begin(), all.end());
    return all;
}

NOVASVG_INLINE void Document::applyStyleSheet(const std::string& content)
{
    auto rules = parseStyleSheet(content);
    if(!rules.empty()) {
        std::sort(rules.begin(), rules.end());
        m_rootElement->transverse([&rules](SVGElement* element) {
            for(const auto& rule : rules) {
                if(rule.match(element)) {
                    for(const auto& declaration : rule.declarations()) {
                        if(declaration.id != PropertyID::Unknown)
                            element->setAttribute(declaration.specificity, declaration.id, declaration.value);
                    }
                }
            }
        });
    }

    if(!content.empty())
        m_rootElement->addStyleSheet(content);

    // The HTML in a foreignObject is not part of the element tree above, so it is styled here, from every
    // sheet applied so far (a stylesheet given later than the one in the file sits after it in the cascade).
    const auto htmlRules = parseStyleSheets(m_rootElement->styleSheets());
    m_rootElement->transverse([&htmlRules](SVGElement* element) {
        if(element->id() == ElementID::ForeignObject)
            resolveForeignObjectStyle(static_cast<SVGForeignObjectElement*>(element), htmlRules);
    });
}

NOVASVG_INLINE ElementList Document::querySelectorAll(const std::string& content) const
{
    auto selectors = parseQuerySelectors(content);
    if(selectors.empty())
        return ElementList();
    ElementList elements;
    m_rootElement->transverse([&](SVGElement* element) {
        for(const auto& selector : selectors) {
            if(matchSelector(selector, element)) {
                elements.push_back(element);
                break;
            }
        }
    });

    return elements;
}

} // namespace novasvg
