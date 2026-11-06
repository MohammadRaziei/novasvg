#include "vg/pixmap.h"
#include "doctest.h"

#include <filesystem>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>

#include <nova/svg/svg.h>

namespace {
std::filesystem::path project_root()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path data_path(const std::string& filename)
{
    return project_root() / "data" / filename;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}
} // namespace

TEST_CASE("Bitmap advanced operations") {
    // Test pixmap constructor with data
    const int width = 100;
    const int height = 100;
    const int stride = width * 4;
    std::vector<uint8_t> data(width * height * 4, 0xFF); // White ARGB
    
    nova::vg::pixmap bitmap(data.data(), width, height, stride);
    CHECK_FALSE(bitmap.is_null());
    CHECK(bitmap.width() == width);
    CHECK(bitmap.height() == height);
    CHECK(bitmap.stride() == stride);
    
    // Test convertToRGBA
    bitmap.convert_to_rgba();
    
    // The straight-alpha image made from a bitmap encodes without trouble
    {
        nova::vg::pixmap copy(data.data(), width, height, stride);
        auto image = nova::vg::bitmap_to_rgba_image(copy);
        CHECK_FALSE(image.empty());
        CHECK_FALSE(image.to_png().empty());
    }

    // Test move assignment operator
    nova::vg::pixmap bitmap2;
    bitmap2 = std::move(bitmap);
    CHECK_FALSE(bitmap2.is_null());
    CHECK(bitmap.is_null()); // Should be null after move
}

TEST_CASE("Matrix advanced operations") {
    // Shear and rotation (radians) just have to produce something that is not the identity.
    auto sheared = stb::matrix::shearing(0.1f, 0.05f);
    CHECK_FALSE(sheared.is_identity());
    auto rotated = stb::matrix::rotation(0.785398f);
    CHECK_FALSE(rotated.is_identity());

    // then(): the left matrix is applied first, so (0,0) -> (10,20) -> (20,60).
    stb::matrix m1 = stb::matrix::translation(10.0f, 20.0f);
    stb::matrix m2 = stb::matrix::scaling(2.0f, 3.0f);
    auto m3 = m1.then(m2);
    CHECK(m3.e == doctest::Approx(20.0f));
    CHECK(m3.f == doctest::Approx(60.0f));

    // Identity.then(m1) is m1
    auto m4 = stb::matrix().then(m1);
    CHECK(m4.e == doctest::Approx(10.0f));
    CHECK(m4.f == doctest::Approx(20.0f));

    stb::matrix m5;
    m5.shear(0.1f, 0.05f);
    CHECK_FALSE(m5.is_identity());

    stb::matrix m6;
    m6.rotate(0.5f);
    CHECK_FALSE(m6.is_identity());
}

TEST_CASE("Box operations") {
    // Test Box constructor with parameters
    nova::vg::Box box(10.0f, 20.0f, 30.0f, 40.0f);
    CHECK(box.x == doctest::Approx(10.0f));
    CHECK(box.y == doctest::Approx(20.0f));
    CHECK(box.w == doctest::Approx(30.0f));
    CHECK(box.h == doctest::Approx(40.0f));
    
    // Test transform method (in-place)
    stb::matrix m = stb::matrix::translation(5.0f, 10.0f);
    box.transform(m);
    CHECK(box.x == doctest::Approx(15.0f));
    CHECK(box.y == doctest::Approx(30.0f));
}

TEST_CASE("TextNode operations") {
    // Create a simple svg with text
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <text id="text1" x="10" y="20">Hello World</text>
    </svg>)";
    
    auto dom = nova::svg::document::load_data(svg_data);
    REQUIRE(dom != nullptr);
    
    // Get text el
    auto text_element = dom->element_by_id("text1");
    REQUIRE_FALSE(text_element.is_null());
    
    // Get children (should contain text nd)
    auto children = text_element.children();
    CHECK(children.size() >= 1);
    
    // Find text nd
    for (const auto& child : children) {
        if (child.is_text_node()) {
            auto textNode = child.to_text_node();
            CHECK_FALSE(textNode.is_null());
            CHECK(textNode.data() == "Hello World");
            
            // Test set_data
            textNode.set_data("Modified Text");
            CHECK(textNode.data() == "Modified Text");
            break;
        }
    }
}

TEST_CASE("Document render with matrix") {
    auto svg_file = data_path("rect.svg");
    auto dom = nova::svg::document::load_file(svg_file.string());
    REQUIRE(dom != nullptr);
    
    // Create bitmap
    nova::vg::pixmap bitmap(100, 100);
    
    // Test render with identity matrix
    dom->render(bitmap, stb::matrix());
    
    // Test render with translation matrix
    auto matrix = stb::matrix::translation(10.0f, 20.0f);
    dom->render(bitmap, matrix);
    
    // Test render with scaling matrix
    auto matrix2 = stb::matrix::scaling(0.5f, 0.5f);
    dom->render(bitmap, matrix2);
}

TEST_CASE("Element render with matrix") {
    auto svg_file = data_path("rect.svg");
    auto dom = nova::svg::document::load_file(svg_file.string());
    REQUIRE(dom != nullptr);
    
    auto docElement = dom->document_element();
    auto children = docElement.children();
    
    if (!children.empty()) {
        auto rectElement = children[0].to_element();
        CHECK_FALSE(rectElement.is_null());
        
        // Create bitmap
        nova::vg::pixmap bitmap(50, 50);
        
        // Test render with identity matrix
        rectElement.render(bitmap, stb::matrix());
        
        // Test render with translation matrix
        auto matrix = stb::matrix::translation(5.0f, 5.0f);
        rectElement.render(bitmap, matrix);
    }
}

TEST_CASE("Document methods with edge cases") {
    // Test with empty svg
    std::string empty_svg = R"(<svg xmlns="http://www.w3.org/2000/svg"></svg>)";
    auto dom = nova::svg::document::load_data(empty_svg);
    REQUIRE(dom != nullptr);
    
    // Test width/height with empty svg
    CHECK(dom->width() == doctest::Approx(0.0f));
    CHECK(dom->height() == doctest::Approx(0.0f));
    
    // Test bounding_box with empty svg
    auto bbox = dom->bounding_box();
    CHECK(bbox.w == doctest::Approx(0.0f));
    CHECK(bbox.h == doctest::Approx(0.0f));
    
    // Test render_to_pixmap with empty dimensions
    auto bitmap = dom->render_to_pixmap(0, 0);
    CHECK(bitmap.is_null());
    
    // Test render_to_pixmap with auto-scaling from empty svg
    auto bitmap2 = dom->render_to_pixmap(-1, 100);
    CHECK(bitmap2.is_null());
    
    auto bitmap3 = dom->render_to_pixmap(100, -1);
    CHECK(bitmap3.is_null());
}

TEST_CASE("Query selector edge cases") {
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <g id="group1">
            <rect class="shape" x="10" y="10" width="20" height="20"/>
            <circle class="shape" cx="50" cy="50" r="10"/>
        </g>
    </svg>)";
    
    auto dom = nova::svg::document::load_data(svg_data);
    REQUIRE(dom != nullptr);
    
    // Test query_selector_all with class selector (if supported)
    // Note: This may not work depending on CSS selector support
    
    // Test element_by_id with nested el
    auto group = dom->element_by_id("group1");
    CHECK_FALSE(group.is_null());
    
    // Test children of group
    auto groupChildren = group.children();
    CHECK(groupChildren.size() >= 2);
}

TEST_CASE("Apply stylesheet edge cases") {
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <rect id="rect1" x="10" y="10" width="80" height="80"/>
    </svg>)";
    
    auto dom = nova::svg::document::load_data(svg_data);
    REQUIRE(dom != nullptr);
    
    // Apply empty stylesheet
    dom->apply_style_sheet("");
    
    // Apply stylesheet with invalid CSS (should not crash)
    dom->apply_style_sheet("invalid { css: syntax; }");
    
    // Apply valid CSS
    dom->apply_style_sheet("#rect1 { fill: #FF0000; }");
}

TEST_CASE("Node equality operators") {
    auto svg_file = data_path("rect.svg");
    auto dom = nova::svg::document::load_file(svg_file.string());
    REQUIRE(dom != nullptr);
    
    auto docElement = dom->document_element();
    auto children = docElement.children();
    
    if (children.size() >= 1) {
        auto node1 = children[0];
        auto node2 = children[0];
        
        // Test equality
        CHECK(node1 == node2);
        CHECK_FALSE(node1 != node2);
        
        // Test with null nd
        nova::svg::node nullNode;
        CHECK_FALSE(node1 == nullNode);
        CHECK(node1 != nullNode);
        
        // Test self-equality
        CHECK(node1 == node1);
        CHECK_FALSE(node1 != node1);
    }
}