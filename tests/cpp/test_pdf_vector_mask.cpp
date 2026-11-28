#include "doctest.h"

#include <string>

#include <nova/svg/svg.h>

// A mask or a clip mask on a group is a soft mask of the PDF page (the content stays vector); only what a soft mask
// cannot say falls back to a bitmap placed as an image.
using nova::svg::document;

namespace {

const std::string head = "<svg xmlns='http://www.w3.org/2000/svg' width='100' height='100' viewBox='0 0 100 100'>";
const std::string gradient = "<linearGradient id='g'><stop offset='0' stop-color='#fff'/><stop offset='1' stop-color='#000'/></linearGradient>";
const std::string two_circles = "<clipPath id='c'><circle cx='35' cy='50' r='25'/><circle cx='65' cy='50' r='25'/></clipPath>";

std::string pdf_of(const std::string& body)
{
    auto doc = document::load_data(head + body + "</svg>");
    REQUIRE(doc != nullptr);
    const auto bytes = doc->render_to_pdf(document::pdf_mode::vector, 192.f);
    REQUIRE(bytes.size() > 8);
    return std::string(bytes.begin(), bytes.end());
}

bool has(const std::string& pdf, const char* what) { return pdf.find(what) != std::string::npos; }

}

TEST_CASE("pdf: a luminance mask is a luminosity soft mask, no bitmap")
{
    const auto pdf = pdf_of("<defs>" + gradient + "<mask id='m'><rect width='100' height='100' fill='url(#g)'/></mask></defs>"
                            "<rect width='100' height='100' fill='#06c' mask='url(#m)'/>");
    CHECK(has(pdf, "/SMask << /Type /Mask /S /Luminosity /G "));
    CHECK(!has(pdf, "/S /Alpha"));
    CHECK(!has(pdf, "/Subtype /Image"));
}

TEST_CASE("pdf: mask-type: alpha is an alpha soft mask")
{
    const auto pdf = pdf_of("<defs><mask id='m' style='mask-type:alpha'><circle cx='50' cy='50' r='30'/></mask></defs>"
                            "<rect width='100' height='100' fill='#c30' mask='url(#m)'/>");
    CHECK(has(pdf, "/SMask << /Type /Mask /S /Alpha /G "));
    CHECK(!has(pdf, "/Subtype /Image"));
}

TEST_CASE("pdf: a clip path of several shapes is an alpha soft mask (their union), no bitmap")
{
    const auto pdf = pdf_of("<defs>" + two_circles + "</defs><rect width='100' height='100' fill='#093' clip-path='url(#c)'/>");
    CHECK(has(pdf, "/SMask << /Type /Mask /S /Alpha /G "));
    CHECK(!has(pdf, "/Subtype /Image"));
}

TEST_CASE("pdf: a plain clip path stays a vector clip, with no soft mask at all")
{
    const auto pdf = pdf_of("<defs><clipPath id='c'><rect x='10' y='10' width='50' height='50'/></clipPath></defs>"
                            "<rect width='100' height='100' fill='#093' clip-path='url(#c)'/>");
    CHECK(!has(pdf, "/S /Alpha"));
    CHECK(!has(pdf, "/S /Luminosity"));
    CHECK(!has(pdf, "/Subtype /Image"));
}

TEST_CASE("pdf: a mask together with opacity is still one soft mask")
{
    const auto pdf = pdf_of("<defs><mask id='m'><circle cx='50' cy='50' r='35' fill='#fff'/></mask></defs>"
                            "<g opacity='.5' mask='url(#m)'><rect width='100' height='100' fill='#06c'/></g>");
    CHECK(has(pdf, "/S /Luminosity"));
    CHECK(has(pdf, "/CA 0.5"));
    CHECK(!has(pdf, "/Subtype /Image"));
}

TEST_CASE("pdf: what a soft mask cannot say stays a bitmap")
{
    // a mask with a mask of its own
    CHECK(has(pdf_of("<defs><mask id='inner'><circle cx='50' cy='50' r='30' fill='#fff'/></mask>"
                     "<mask id='outer' mask='url(#inner)'><rect width='100' height='100' fill='#fff'/></mask></defs>"
                     "<rect width='100' height='100' fill='#06c' mask='url(#outer)'/>"),
              "/Subtype /Image"));
    // a mask and a filter
    CHECK(has(pdf_of("<defs><mask id='m'><rect width='100' height='100' fill='#fff'/></mask>"
                     "<filter id='f'><feGaussianBlur stdDeviation='2'/></filter></defs>"
                     "<rect x='20' y='20' width='60' height='60' fill='#06c' mask='url(#m)' filter='url(#f)'/>"),
              "/Subtype /Image"));
    // a clip mask and a mask on the same element
    CHECK(has(pdf_of("<defs>" + two_circles + "<mask id='m'><rect width='100' height='100' fill='#fff'/></mask></defs>"
                     "<rect width='100' height='100' fill='#093' clip-path='url(#c)' mask='url(#m)'/>"),
              "/Subtype /Image"));
    // a clip path with a clip path of its own
    CHECK(has(pdf_of("<defs><clipPath id='inner'><circle cx='50' cy='50' r='30'/></clipPath>"
                     "<clipPath id='outer' clip-path='url(#inner)'><circle cx='35' cy='50' r='25'/><circle cx='65' cy='50' r='25'/></clipPath></defs>"
                     "<rect width='100' height='100' fill='#093' clip-path='url(#outer)'/>"),
              "/Subtype /Image"));
}

TEST_CASE("pdf: a gradient in a group is placed in the group's own (pixel) space")
{
    // The group's form works in pixels (BBox 0 0 100 100); a pattern matrix that carried the page's px -> pt scale
    // and flip as well ([75 0 0 -75 0 75]) shrank every gradient inside a group to three quarters of its box.
    const auto pdf = pdf_of("<defs>" + gradient + "</defs><g opacity='.999'><rect width='100' height='100' fill='url(#g)'/></g>");
    CHECK(has(pdf, "/PatternType 2"));
    CHECK(has(pdf, "/Matrix [100 0 0 100 0 0]"));
    CHECK(!has(pdf, "/Matrix [75 0 0 -75 0 75]"));
    // ... and on the page itself it keeps the page's scale and flip
    const auto page = pdf_of("<defs>" + gradient + "</defs><rect width='100' height='100' fill='url(#g)'/>");
    CHECK(has(page, "/Matrix [75 0 0 -75 0 75]"));
}
