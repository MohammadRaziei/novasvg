#pragma once

// The CSS side of the svg reader: selectors, rules and the style-sheet parser. Bodies live in svg_style.cpp.
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "svg_property.h"

namespace nova::svg {

class svg_element;

struct css_simple_selector;

using css_selector = std::vector<css_simple_selector>;
using selector_list = std::vector<css_selector>;

struct attribute_selector {
    enum class match_type {
        none,
        equals,
        contains,
        includes,
        starts_with,
        ends_with,
        dash_equals
    };

    match_type match_type_{match_type::none};
    property_id id{property_id::unknown};
    std::string name; // the attribute's name, used when `id` is unknown (`data-*` & co)
    std::string value;
};

struct pseudo_class_selector {
    enum class type {
        unknown,
        empty,
        root,
        is,
        not_,
        first_child,
        last_child,
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

struct css_simple_selector {
    enum class combinator {
        none,
        descendant,
        child,
        direct_adjacent,
        in_direct_adjacent
    };

    explicit css_simple_selector(combinator combinator_) : combinator_(combinator_) {}

    combinator combinator_{combinator::descendant};
    element_id id{element_id::star};
    std::string tag; // lower-case tag name as written; empty for `*`. For matching HTML (no ElementID for it)
    std::vector<attribute_selector> attribute_selectors;
    std::vector<pseudo_class_selector> pseudo_class_selectors;
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
    rule_data(const css_selector& selector_, const declaration_list& declarations, size_t specificity, size_t position)
        : m_selector(selector_), m_declarations(declarations), m_specificity(specificity), m_position(position)
    {}

    bool is_less_than(const rule_data& rule_) const { return std::tie(m_specificity, m_position) < std::tie(rule_.m_specificity, rule_.m_position); }

    const css_selector& selector() const { return m_selector; }
    const declaration_list& declarations() const { return m_declarations; }
    size_t specificity() const { return m_specificity; }
    size_t position() const { return m_position; }

    bool match(const svg_element* el) const;

private:
    css_selector m_selector;
    declaration_list m_declarations;
    size_t m_specificity;
    size_t m_position;
};

inline bool operator<(const rule_data& a, const rule_data& b) { return a.is_less_than(b); }

using rule_data_list = std::vector<rule_data>;

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

// What the rest of the svg reader calls.
bool is_custom_property_name(std::string_view name);
rule_data_list parse_style_sheet(std::string_view input);
rule_data_list parse_style_sheets(const std::vector<std::string>& sheets);
selector_list parse_query_selectors(std::string_view input);
void parse_inline_style(std::string_view input, svg_element* el);
void remove_style_comments(std::string& value);
bool decode_text(std::string_view input, std::string& output);
bool read_identifier(std::string_view& input, std::string& output);
bool match_selector(const css_selector& selector, const svg_element* el);
bool match_html_selector(const css_selector& selector, const std::vector<html_node>& nodes, int index, const svg_element* owner);

// Which rules can possibly match a given el, so that apply_style_sheet() does not have to try every rule on
// every el. A rule is filed under what its *rightmost* compound selector demands of the el itself: a tag name
// (`rect`), else a class (`.nd`), else nothing in particular. The index only prunes; each candidate still goes
// through rule_data::match(), and candidates are visited in rule order.
class rule_index {
public:
    explicit rule_index(const rule_data_list& rules);

    // Indices (ascending) of the rules that might match `el`.
    void candidates(const svg_element* el, std::vector<uint32_t>& out) const;

private:
    std::vector<std::vector<uint32_t>> m_by_tag;
    std::unordered_map<std::string, std::vector<uint32_t>> m_by_class;
    std::vector<uint32_t> m_other;
};

} // namespace nova::svg
