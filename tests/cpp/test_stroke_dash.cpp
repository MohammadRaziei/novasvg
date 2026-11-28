#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

#include <nova/svg/svg.h>
#include <stb/logging.hpp>

using namespace nova::svg;

// SVG 1.1 section 11.4: a stroke-dasharray that sums to zero is `none`. mermaid writes `stroke-dasharray: 0`, and
// handed to libstb as it was, every stroke logged "the dash pattern is not usable".

namespace {
stb::image render(const std::string& dasharray)
{
    const std::string svg = "<svg xmlns='http://www.w3.org/2000/svg' width='60' height='20'>"
                            "<path d='M5 10H55' stroke='black' stroke-width='6' fill='none' stroke-dasharray='" + dasharray + "'/></svg>";
    auto doc = document::load_data(svg);
    REQUIRE(doc);
    return doc->render_to_image();
}

bool same(const stb::image& a, const stb::image& b)
{
    return a.size_bytes() == b.size_bytes() && std::memcmp(a.data(), b.data(), a.size_bytes()) == 0;
}
}  // namespace

TEST_CASE("stroke-dasharray: a zero sum is none, and nothing is logged for it")
{
    std::vector<std::string> warnings;
    auto& lg = stb::logging::get_logger("libstb");
    auto& h = lg.add_handler(stb::logging::callback_handler([&](const stb::logging::record& r) {
        warnings.push_back(std::string(r.name) + ": " + std::string(r.message));
    }));

    const auto none = render("none");
    CHECK(same(render("0"), none));
    CHECK(same(render("0 0"), none));
    CHECK(same(render("0,0,0"), none));

    lg.remove_handler(h);
    CHECK(warnings.empty());
}

TEST_CASE("stroke-dasharray: a real dash array still dashes")
{
    CHECK_FALSE(same(render("5 5"), render("none")));
    CHECK_FALSE(same(render("0 8"), render("none")));  // a zero dash with a gap is a valid pattern (dots with round caps)
}
