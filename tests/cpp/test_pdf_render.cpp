#include "doctest.h"
#include <novasvg/novasvg.h>

#include <stb/logging.hpp>

#include <cstdlib>
#include <vector>
#include <string>

// document::render_to_pdf(): the same SVG as a vector page and as one bitmap. NOVA_PDF_DIR=dir also writes
// vector.pdf and raster.pdf there, to open in a viewer.
using namespace nova::svg;
using namespace nova::vg;

namespace {

const char* kSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="240" height="160" viewBox="0 0 240 160">
  <defs>
    <linearGradient id="g" x1="0" x2="1"><stop offset="0" stop-color="#ffd400"/><stop offset="1" stop-color="#0057ff"/></linearGradient>
    <pattern id="p" width="10" height="10" patternUnits="userSpaceOnUse"><rect width="5" height="5" fill="#c00"/><rect x="5" y="5" width="5" height="5" fill="#c00"/></pattern>
    <mask id="m"><rect x="120" y="10" width="50" height="50" fill="white"/><circle cx="145" cy="35" r="14" fill="black"/></mask>
    <filter id="f"><feGaussianBlur stdDeviation="3"/></filter>
  </defs>
  <rect x="10" y="10" width="100" height="60" fill="url(#g)" stroke="#222" stroke-width="3" stroke-dasharray="8 4"/>
  <rect x="120" y="10" width="50" height="50" fill="#e0301e" mask="url(#m)"/>
  <rect x="180" y="10" width="50" height="50" fill="url(#p)"/>
  <g opacity="0.5"><circle cx="40" cy="110" r="25" fill="#00a050"/><circle cx="60" cy="110" r="25" fill="#0050a0"/></g>
  <circle cx="130" cy="110" r="20" fill="#7a2bd6" filter="url(#f)"/>
  <circle cx="190" cy="110" r="22" fill="#e07000" style="mix-blend-mode:multiply"/>
  <circle cx="205" cy="110" r="22" fill="#00a0c0" style="mix-blend-mode:multiply"/>
  <text x="10" y="152" font-size="16" fill="#111">Vector text ffi</text>
</svg>)svg";

std::string text_of(const std::vector<uint8_t>& bytes) { return std::string(bytes.begin(), bytes.end()); }

}

TEST_CASE("render_to_pdf: vector and raster pages")
{
    auto doc = document::load_data(kSvg);
    REQUIRE(doc);

    auto vector = doc->render_to_pdf(document::pdf_mode::vector);
    auto raster = doc->render_to_pdf(document::pdf_mode::raster);
    REQUIRE(!vector.empty());
    REQUIRE(!raster.empty());

    std::string v = text_of(vector), r = text_of(raster);
    for(const std::string* s : {&v, &r}) {
        CHECK(s->rfind("%PDF-1.5", 0) == 0);
        CHECK(s->find("/MediaBox [0 0 180 120]") != std::string::npos);
        CHECK(s->substr(s->size() - 6) == "%%EOF\n");
    }

    // the vector page keeps the gradient as a shading and the faded group as a transparency group;
    // the raster page has neither, only an image
    CHECK(v.find("/ShadingType 2") != std::string::npos);
    CHECK(v.find("/Subtype /Form") != std::string::npos);
    CHECK(v.find("/BM /Multiply") != std::string::npos);
    CHECK(r.find("/ShadingType") == std::string::npos);
    CHECK(r.find("/Subtype /Image") != std::string::npos);

    if(const char* dir = std::getenv("NOVA_PDF_DIR")) {
        CHECK(doc->save_pdf(std::string(dir) + "/vector.pdf", document::pdf_mode::vector));
        CHECK(doc->save_pdf(std::string(dir) + "/raster.pdf", document::pdf_mode::raster));
    }
}

TEST_CASE("render_to_pdf: no size, no page")
{
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg'/>");
    REQUIRE(doc);
    CHECK(doc->render_to_pdf().empty());
}

// novasvg logs through stb::logging ("novasvg", "novasvg.svg", "novasvg.pdf"): a handler on "novasvg" sees them.
TEST_CASE("logging: failures and PDF limits reach a stb::logging handler")
{
    std::vector<std::string> seen;
    auto& lg = stb::logging::get_logger("novasvg");
    const auto old_level = lg.get_level();
    lg.set_level(stb::logging::level::debug);
    auto& h = lg.add_handler(stb::logging::callback_handler([&](const stb::logging::record& r) {
        seen.push_back(std::string(r.name) + ": " + std::string(r.message));
    }));

    CHECK_FALSE(document::load_file("/nonexistent/none.svg"));
    CHECK_FALSE(document::load_data("<svg"));
    auto doc = document::load_data("<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'><defs>"
        "<linearGradient id='g' spreadMethod='reflect'><stop offset='0' stop-color='red'/><stop offset='1' stop-color='blue'/></linearGradient>"
        "</defs><rect width='10' height='10' fill='url(#g)'/></svg>");
    REQUIRE(doc);
    CHECK_FALSE(doc->render_to_pdf().empty());

    lg.remove_handler(h);
    lg.set_level(old_level);

    auto has = [&](const char* part) {
        for(const auto& line : seen)
            if(line.find(part) != std::string::npos)
                return true;
        return false;
    };
    CHECK(has("novasvg.svg: cannot open"));
    CHECK(has("novasvg.svg: cannot parse"));
    CHECK(has("novasvg.pdf: gradient spreadMethod"));
}
