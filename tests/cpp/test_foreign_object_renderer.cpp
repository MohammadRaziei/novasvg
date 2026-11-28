// The foreignObject extension point (ADR-0010): a renderer written against nova/svg/foreign_object.h only.
#include "doctest.h"
#include <nova/svg/svg.h>

#include <memory>
#include <string>

using namespace nova::svg;

namespace {

const char* page =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\">"
    "<style>.k span { color: #123456 }</style>"
    "<g class=\"k\"><foreignObject x=\"0\" y=\"0\" width=\"40\" height=\"40\"><div><span>hi</span></div></foreignObject></g>"
    "</svg>";

// Fills the whole box red; remembers what it was given.
class recording_renderer final : public foreign_object_renderer {
public:
    std::shared_ptr<void> prepare(std::string_view markup, const css_query& css) const override
    {
        ++prepare_calls;
        prepared_markup = std::string(markup);
        // <div><span>: the span is index 1
        const std::vector<html_tag> tags = {{"div", "", "", "", -1}, {"span", "", "", "", 0}};
        if(auto declared = css.declared(tags, 1, "color"))
            color_for_span = declared->value;
        return std::make_shared<int>(42);
    }

    void render(const foreign_object_context& context, foreign_object_painter& painter) const override
    {
        ++render_calls;
        if(context.prepared != nullptr)
            prepared_value = *static_cast<const int*>(context.prepared);
        box_width = context.box.w;
        painter.fill_rect(context.box, stb::color(255, 0, 0));
    }

    mutable int prepare_calls = 0;
    mutable int render_calls = 0;
    mutable std::string prepared_markup;
    mutable std::string color_for_span;
    mutable int prepared_value = 0;
    mutable float box_width = 0.f;
};

} // namespace

TEST_CASE("a custom foreignObject renderer draws through the painter")
{
    auto dom = document::load_data(page);
    REQUIRE(dom != nullptr);
    auto renderer = std::make_shared<recording_renderer>();
    dom->set_foreign_object_renderer(renderer);

    const auto image = dom->render_to_image();
    CHECK(renderer->render_calls == 1);
    CHECK(renderer->box_width == doctest::Approx(40.f));
    CHECK(image(20, 20, 0) == 255);
    CHECK(image(20, 20, 1) == 0);
    CHECK(image(20, 20, 2) == 0);
}

TEST_CASE("prepare() sees the markup and the document's CSS, through the svg ancestors")
{
    auto dom = document::load_data(page);
    REQUIRE(dom != nullptr);
    auto renderer = std::make_shared<recording_renderer>();
    dom->set_foreign_object_renderer(renderer); // set after loading: prepared again from the sheets applied so far

    CHECK(renderer->prepare_calls == 1);
    CHECK(renderer->prepared_markup == "<div><span>hi</span></div>");
    CHECK(renderer->color_for_span == "#123456");

    (void)dom->render_to_image();
    CHECK(renderer->prepared_value == 42);
}

TEST_CASE("a null foreignObject renderer skips the element; the default draws the text")
{
    auto dom = document::load_data(page);
    REQUIRE(dom != nullptr);
    CHECK(dom->foreign_object_renderer_ptr() == default_foreign_object_renderer());

    dom->set_foreign_object_renderer(nullptr);
    const auto blank = dom->render_to_image();
    for(int y = 0; y < 40; ++y)
        for(int x = 0; x < 40; ++x)
            REQUIRE(blank(x, y, 3) == 0);

#ifndef NOVA_NO_SIMPLE_FOREIGN_OBJECT
    dom->set_foreign_object_renderer(default_foreign_object_renderer());
    const auto text = dom->render_to_image();
    int inked = 0;
    for(int y = 0; y < 40; ++y)
        for(int x = 0; x < 40; ++x)
            inked += text(x, y, 3) != 0;
    CHECK(inked > 0); // "hi" in #123456
#endif
}
