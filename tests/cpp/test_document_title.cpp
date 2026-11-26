#include "doctest.h"

#include <string>

#include <nova/svg/svg.h>

using nova::svg::document;

namespace {

std::unique_ptr<document> load(const std::string& body)
{
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg' width='40' height='20'>" + body + "</svg>");
    REQUIRE(doc != nullptr);
    return doc;
}

std::string pdf_of(const document& doc)
{
    const auto bytes = doc.render_to_pdf(document::pdf_mode::vector, 192.f);
    return std::string(bytes.begin(), bytes.end());
}

}

TEST_CASE("document title: the text of the root's first <title> child")
{
    CHECK(load("<title>Release plan</title><rect width='40' height='20'/>")->title() == "Release plan");
    CHECK(load("<rect width='40' height='20'/><title>After the shapes</title>")->title() == "After the shapes");
    CHECK(load("<rect width='40' height='20'/>")->title().empty());
}

TEST_CASE("document title: entities decoded, white space trimmed, the first one wins")
{
    CHECK(load("<title>\n   Q&amp;A &lt;draft&gt;  \n</title>")->title() == "Q&A <draft>");
    CHECK(load("<title>first</title><title>second</title>")->title() == "first");
    CHECK(load("<title>  </title>")->title().empty());
    CHECK(load("<title/>")->title().empty());
}

TEST_CASE("document title: only a <title> directly under the root counts")
{
    CHECK(load("<g><title>a tooltip</title><rect width='40' height='20'/></g>")->title().empty());
    CHECK(load("<g><title>a tooltip</title></g><title>the title</title>")->title() == "the title");
}

TEST_CASE("document title: it is the PDF's /Title, ASCII or not")
{
    const auto plain = pdf_of(*load("<title>Release (v2) plan</title><rect width='40' height='20'/>"));
    CHECK(plain.find("/Title (Release \\(v2\\) plan)") != std::string::npos);

    const auto unicode = pdf_of(*load("<title>Caf\xC3\xA9 \xE2\x9C\x93</title><rect width='40' height='20'/>"));
    // UTF-16BE with a byte order mark: C a f \u00E9 space \u2713
    CHECK(unicode.find("/Title <FEFF00430061006600E900202713>") != std::string::npos);

    const auto none = pdf_of(*load("<rect width='40' height='20'/>"));
    CHECK(none.find("/Title") == std::string::npos);
    CHECK(none.find("/Producer") != std::string::npos);
}
