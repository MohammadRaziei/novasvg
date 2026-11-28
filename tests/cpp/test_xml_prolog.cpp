#include "doctest.h"

#include <string>

#include <nova/svg/svg.h>

using namespace nova::svg;

// What the one-pass tokenizer (document::parse) does with the XML that is not SVG itself: the prolog, a DOCTYPE with
// an internal subset, comments, CDATA and character references (ADR-0013).

namespace {
const char* k_svg =
    "<?xml version=\"1.0\"?>\n"
    "<!DOCTYPE svg [<!ENTITY c \"red\">]>\n"
    "<!-- a comment -->\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
    "<rect id=\"a&amp;b\" fill=\"&c;\" width=\"5\" height=\"5\"/>"
    "<text id=\"t\">x &lt; y <![CDATA[<raw>]]> &#65;&#x42;</text>"
    "</svg>";

std::string text_of(const element& e)
{
    std::string out;
    for(auto child : e.children())
        if(child.is_text_node())
            out += child.to_text_node().data();
    return out;
}
}  // namespace

TEST_CASE("xml prolog: a DOCTYPE with an internal subset, a comment and a prolog do not stop the document loading")
{
    CHECK(document::load_data(k_svg));
}

TEST_CASE("xml: the predefined entities, character references and CDATA are decoded")
{
    auto doc = document::load_data(k_svg);
    REQUIRE(doc);
    element rect, text;
    for(auto child : doc->document_element().children()) {
        if(!child.is_element())
            continue;
        auto e = child.to_element();
        if(e.attribute("id") == "t")
            text = e;
        else
            rect = e;
    }
    CHECK(rect.attribute("id") == "a&b");
    CHECK(text_of(text) == "x < y <raw> AB");
}

TEST_CASE("xml: an entity declared in the DTD is NOT expanded (a known limit, see ADR-0013)")
{
    auto doc = document::load_data(k_svg);
    REQUIRE(doc);
    for(auto child : doc->document_element().children()) {
        if(child.is_element() && child.to_element().attribute("id") == "a&b") {
            // When DTD entities are supported this becomes "red" and this check is the one to flip.
            CHECK(child.to_element().attribute("fill") != "red");
            return;
        }
    }
    FAIL("the rect is missing");
}
